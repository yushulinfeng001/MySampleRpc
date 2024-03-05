#include "user.pb.h"
#include "MprpcApplication.h"
#include "MprpcProvider.h"
#include "MprpcController.h"

#include <iostream>
#include <string>

// 一个本地服务，现在要将它变成rpc服务
class UserService : public fixbug::UserServiceRpc 
{
public:
    bool Login(const std::string& name, const std::string& pwd)
    {
        std::cout << "doing local service: Login" << std::endl;
        std::cout << "name:" << name << " pwd:" << pwd << std::endl;
        return true;
    }

    
    bool Register(uint32_t id, std::string name, std::string pwd)
    {
        std::cout << "doing local service: Register" << std::endl;
        std::cout << "id" << id << "name:" << name << " pwd:" << pwd << std::endl;
        return true;
    }
    
    //函数重载
    void Login(::google::protobuf::RpcController* controller,
                    const ::fixbug::LoginRequest* request,
                    ::fixbug::LoginResponse* response,
                    ::google::protobuf::Closure* done)
    {
        // 框架给业务上报了请求参数 LoginRequest，应用获取数据进行业务
        // protobuf直接把网络上的字节流反序列化成我们能直接识别的LoginRequest对象

        std::string name = request->name();
        std::string pwd = request->pwd();

        // 此处是本地业务
        bool login_result = Login(name, pwd);

        // 把响应写入
        fixbug::ResultCode *code = response->mutable_result();
        code->set_errcode(0);
        code->set_errmsg("success");
        response->set_success(login_result);

        // 执行回调操作
        // 完成响应对象数据的序列化和网络发送
        // 调用了之前闭包中封装的成员函数RpcProvider::SendRpcResponse(conn, response);
        //将reponse消息体作为Login处理结果发送回caller。
        done->Run();
    }

    //函数重载
    void Register(::google::protobuf::RpcController* controller,
                const ::fixbug::RegisterRequest* request,
                ::fixbug::RegisterResponse* response,
                ::google::protobuf::Closure* done)
    {
        uint32_t id = request->id();
        std::string name = request->name();
        std::string pwd = request->pwd();

        bool ret = Register(id, name, pwd);     //本地业务

        //设置response对象
        response->mutable_result()->set_errcode(0);
        response->mutable_result()->set_errmsg("");
        response->set_success(ret);

        //序列化response对象并通过网络发送
        done->Run();
    }

};

int main(int argc, char **argv)
{
    // 框架的初始化操作，读取配置文件
    MprpcApplication::Init(argc, argv);
    
    // provider是一个rpc网络服务对象，把userService对象发布到rpc节点上
    MprpcProvider provider;

    // UserService类继承fixbug::UserServiceRpc，并且重新定义了Login和Resister函数
    
    //注册服务
    provider.NotifyService(new UserService());

    // 启动一个rpc发布节点，进入阻塞状态等待远处rpc请求
    // 启动muduo的事件循环 
    // 将服务注册到zookeeper上
    provider.Run();

    return 0;
}