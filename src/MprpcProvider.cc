#include "MprpcProvider.h"
#include "rpc_header.pb.h"
#include "google/protobuf/descriptor.h"

#include <functional>

// 框架，需要设计成抽象基类

//注册服务，将服务用哈希表存储
void MprpcProvider::NotifyService(google::protobuf::Service *service) 
{
    ServiceInfo service_info;

    // 获取服务对象的描述信息
    const google::protobuf::ServiceDescriptor *pserviceDesc = service->GetDescriptor();
    
    // 获取服务的名字
    // 需包含 google/protobuf/descriptor.h 
    std::string service_name = pserviceDesc->name();

    std::cout << "=========================================" << std::endl;
    std::cout << "service_name:" << service_name << std::endl;
    
    // 遍历方法 ,进行存储
    for (int i = 0; i < pserviceDesc->method_count(); ++i)
    {
        // 获取了服务对象指定下标的服务方法的描述「方法名，方法」
        const google::protobuf::MethodDescriptor* pmethodDesc = pserviceDesc->method(i);    //方法
        std::string method_name = pmethodDesc->name();                                      //方法名
        service_info.m_methodMap.insert({method_name, pmethodDesc});                        //方法名-方法，哈希表
        std::cout << "method_name:" << method_name << std::endl;
    }
    std::cout << "=========================================" << std::endl;
    service_info.m_service = service;                         //服务对象  
    m_serviceMap.insert({service_name, service_info});        //服务名-服务信息（包含服务对象以及方法哈希表）   
}

// 将服务注册到zookeeper上，通过ZkClient
void MprpcProvider::RegisterZookeeper(const muduo::net::InetAddress& address, ZkClient* zkCli)
{
    // 把当前rpc节点上要发布的服务全部注册到zk上面，让rpc client可以从zk上发现服务
    zkCli->Start();     //callee端和zookeeper服务端建立连接
    // service_name为永久性节点，method_name为临时性节点
    for (auto &sp : m_serviceMap) 
    {
        // /UserServiceRpc
        std::string service_path = "/" + sp.first;          //服务路径
        zkCli->Create(service_path.c_str(), nullptr, 0);    //默认state=0是永久性节点，最后一个参数为默认值，在声明中已设定
        for (auto &mp : sp.second.m_methodMap)
        {
            // path = /UserServiceRpc/Login 
            // value = 127.0.0.1:2181
            std::string method_path = service_path + "/" + mp.first;
            char method_path_data[128] = {0};
            sprintf(method_path_data, "%s:%d", address.toIp().c_str(), 2181);
            // ZOO_EPHEMERAL表示znode是一个临时性节点
            // method_path_data服务的地址
            zkCli->Create(method_path.c_str(), method_path_data, strlen(method_path_data), ZOO_EPHEMERAL);
        }
    }
}


// 启动rpc服务节点，开始提供rpc远程网络调用服务
void MprpcProvider::Run() 
{
    // TcpServer绑定此地址
    std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpc_server_ip");
    uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpc_server_port").c_str());

    muduo::net::InetAddress address(ip, port);

    // 创建 TcpServer 对象
    muduo::net::TcpServer server(&m_eventLoop, address, "RpcProvider");

    // 设置muduo库线程数量
    server.setThreadNum(4);

    // 绑定连接回调和消息读写回调方法  分离了网络代码和业务代码
    // 连接回调
    server.setConnectionCallback(std::bind(&MprpcProvider::OnConnection, this, std::placeholders::_1));
    // 消息读写回调
    server.setMessageCallback(std::bind(&MprpcProvider::OnMessage, this, std::placeholders::_1, 
            std::placeholders::_2, std::placeholders::_3));

    // 将服务注册到zookeeper上
    //没有包含头文件，为啥可以定义，因为MprpcApplication.h包含了ZookeeperUtil.h，而文件包含了MprpcApplication.h
    ZkClient zkCli;                             //zookeeper客户端
    RegisterZookeeper(address, &zkCli);         //将ip和端口保存在zookeeper上
    
    LOG_INFO << "RpcProvider start service at ip:" << ip << " port:" << port;

    // 启动网络服务
    server.start();
    m_eventLoop.loop();
}


// socket连接回调(新连接到来或者连接关闭事件)
void MprpcProvider::OnConnection(const muduo::net::TcpConnectionPtr &conn)
{
    if (!conn->connected())
    {
        // 和rpc client的连接断开了
        conn->shutdown();
    }
}


// 解析数据包
// 将buffer前四个字节(一个字符就是一个字节)读出来，方便知道接下来要读取多少数据
void MprpcProvider::ParseRequest(muduo::net::Buffer* buffer, RequestInfo* req_info)
{
    // 网络上接受的远程rpc调用请求的字符流
    std::string recv_buf = buffer->retrieveAllAsString();
    
    uint32_t header_size = 0;
    char* address = (char*)&header_size;
    //copy函数的作用是从string对象中取出若干字符存放到数组s中。其中，s是字符数组，n表示要取出字符的个数，pos表示要取出字符的开始位置。
    //获取rpc请求头的大小
    recv_buf.copy(address, 4, 0);

    // copy[4, 4 + header_size) -> rpc_header_str
    std::string rpc_header_str = recv_buf.substr(4, header_size);

    // 数据反序列化
    mprpc::RpcHeader rpcHeader;
    if (!rpcHeader.ParseFromString(rpc_header_str))
    {
        LOG_WARN << "rpc_header_str:" << rpc_header_str << " parse error!";
        return;
    }

    //请求信息
    req_info->service_name = rpcHeader.service_name();
    req_info->method_name = rpcHeader.methon_name();
    //参数大小
    req_info->args_size = rpcHeader.args_size();
    //参数
    req_info->args_str = recv_buf.substr(4 + header_size, req_info->args_size);
    
    return;
}

/**
 * 如果远程有一个rpc服务的调用请求，那么OnMessage方法就会响应
 * 这个发送的信息都是经过了协议商定的
 * service_name method_name args 定义proto的message类型，进行数据头的序列化和反序列化
 * 既然是针对于数据头，那么我们需要获取指定的一些长度(需要考虑TCP黏包问题)
 * header_size header_str args_str
 */
void MprpcProvider::OnMessage(const muduo::net::TcpConnectionPtr& conn, 
                                muduo::net::Buffer* buffer,
                                muduo::Timestamp timestamp)
{
    // 解析自定义的请求信息
    RequestInfo request_info;
    ParseRequest(buffer, &request_info);        //得到请求信息

#if 0
    // 打印调试信息
    LOG_INFO << "============================================";
    LOG_INFO << "header_size: " << header_size <<; 
    LOG_INFO << "rpc_header_str: " << rpc_header_str <<;
    LOG_INFO << "service_name: " << service_name <<;
    LOG_INFO << "method_name: " << method_name <<;
    LOG_INFO << "args_str: " << args_str <<;
    LOG_INFO << "============================================";
#endif

    // 在注册表上查找服务
    //根据服务名查找服务对象
    auto it = m_serviceMap.find(request_info.service_name);
    if (it == m_serviceMap.end())
    {
        LOG_WARN << request_info.service_name << " is not exist!";
        return;
    }
    //
    auto mit = it->second.m_methodMap.find(request_info.method_name);
    if (mit == it->second.m_methodMap.end())
    {
        LOG_WARN << request_info.service_name << ":" << request_info.method_name << " is not exist!";
        return;
    }
    
    // 负责Response
    google::protobuf::Service *service = it->second.m_service;          //服务对象
    const google::protobuf::MethodDescriptor *method = mit->second;     //服务方法

    // 生成rpc方法调用的请求request和响应response参数
    // request参数需要单独解析，之前只是知道了request的序列化字符串长度
    // 创建request对象，并从字符串中解析出请求信息
    google::protobuf::Message *request = service->GetRequestPrototype(method).New();

    //将args_str解码至request对象中
    //之后request对象就可以这样使用了：request.name() = "zhangsan" | request.pwd() = "123456"
    if (!request->ParseFromString(request_info.args_str))       //由string解析请求request
    {
        LOG_WARN << "request parse error, content:" << request_info.args_str;
        return;
    }

    // 定义一个空的response，用于下面的函数service->CallMethod()将caller请求的函数的结果填写进里面。
    google::protobuf::Message *response = service->GetResponsePrototype(method).New();

    // 给下面的method方法的调用，绑定一个Closure的回调函数
    
    //google::protobuf::NewCallback<...>(...)函数会返回一个google::protobuf::Closure类的对象，该Closure类其实相当于一个闭包。
    //这个闭包捕获了一个成员对象的成员函数，以及这个成员函数需要的参数。然后闭包类提供了一个方法Run()，当执行这个闭包对象的Run()函数时，
    //他就会执行捕获到的成员对象的成员函数，也就是相当于执行void RpcProvider::SendRpcResponse(conn, response);，
    //这个函数可以将reponse消息体发送给Tcp连接的另一端，即caller。
    
    //闭包就是在函数中定义lamda表达式或者function对象
    google::protobuf::Closure *done = google::protobuf::NewCallback<MprpcProvider, 
                                                                    const muduo::net::TcpConnectionPtr&, 
                                                                    google::protobuf::Message*>
                                                                    (this, 
                                                                    &MprpcProvider::SendRpcResponse, 
                                                                    conn, response);

    // 在框架上根据远端rpc请求，调用当前rpc节点上发布的方法
    // new UserService().Login(controller, request, response, done)
    // service是我们的UserService， CallMethod是Service基类中的函数
    // 与客户端类似，UserService继承fixbug::UserServiceRpc，fixbug::UserServiceRpc继承google::protobuf::Service类，
    // google::protobuf::Service类中有一个虚函数CallMethod，CallMethod函数在fixbug::UserServiceRpc中实现，定义在user.pb.cc中
    // UserServiceRpc::CallMethod函数可以根据传递进来的方法描述符method来选择调用注册在user.proto上的哪一个函数（Login，Register）
    // 由于我们用派生类UserService继承了UserServiceRpc并且重写了（Login，Register）函数的实现。
    // 所以当我们调用service->CallMethod()的时候，调用的其实是UserService中的（Login，Register）函数。

    //UserServiceRpc::CallMethod函数会进行下行转换，将基类指针或引用转换为派生类指针或引用。
    //将google::protobuf::Message*转换为::fixbug::RegisterRequest*和::fixbug::RegisterResponse*
    service->CallMethod(method, nullptr, request, response, done); 
}

// Closure的回调操作，用于序列化rpc的响应和网络发送
void MprpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr& conn, 
                                    google::protobuf::Message* response)
{
    std::string response_str;       //定义空字符串对象，将其解析后，通过网络发送
    // 序列化response
    if (response->SerializeToString(&response_str))
    {
        // 序列化成功后，通过网络将rpc方法执行结果发送给rpc的调用方
        conn->send(response_str);
    }
    else
    {
        std::cout << "serialize response_str error!" << std::endl;
    }
    // 半关闭
    conn->shutdown();
}