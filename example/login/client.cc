#include "user.pb.h"
#include "MprpcApplication.h"
#include "MprpcChannel.h"
#include "MprpcController.h"

#include <iostream>


// 本地方法，利用代理调用远程方法
void loginService(fixbug::UserServiceRpc_Stub& stub, MprpcController& controller)
{
    // rpc方法的请求
    fixbug::LoginRequest request;
    request.set_name("zhang san");
    request.set_pwd("123456");

    // rpc方法的响应
    fixbug::LoginResponse response;

    // 发起rpc方法的调用
    stub.Login(&controller, &request, &response, nullptr);

    // 如果rpc远程调用失败，打印错误信息
    if (controller.Failed())
    {
        std::cout << controller.ErrorText() << std::endl;
    }
    // 调用rpc成功
    else
    {
        // 业务成功响应码为0
        if (0 == response.result().errcode())
        {
            std::cout << "rpc Login response success:" << response.success() << std::endl;
        }
        // 业务失败打印错误信息
        else
        {
            std::cout << "rpc response error : " << response.result().errmsg() << std::endl;
        }
    }
}

// 本地方法，利用代理调用远程方法
void registerService(fixbug::UserServiceRpc_Stub& stub, MprpcController& controller)
{
    // 演示调用Register

    // fixbug::RegisterResponse和fixbug::RegisterRequest均继承google::protobuf::Message
    fixbug::RegisterRequest req;

    // TODO:整数形式是否可行
    req.set_id(100);
    req.set_name("mprpc");
    req.set_pwd("123456");
    fixbug::RegisterResponse rsp;

    // 以同步的方式发起rpc调用请求，等待返回结果
    // Register底层会调用channel_->CallMethod(descriptor()->method(1),controller, request, response, done);
    stub.Register(&controller, &req, &rsp, nullptr);

    if (controller.Failed())
    {
        std::cout << controller.ErrorText() << std::endl;
    }
    else
    {
        if (0 == rsp.result().errcode())
        {
            std::cout << "rpc Register response success:" << rsp.success() << std::endl;
        }
        else
        {
            std::cout << "rpc response error : " << rsp.result().errmsg() << std::endl;
        }
    }
}

int main(int argc, char **argv)
{
    //初始化配置文件
    MprpcApplication::Init(argc, argv);

    // ?为什么可以用MprpcChannel来初始化fixbug::UserServiceRpc_Stub
    // 这是UserServiceRpc_Stub的构造函数，由protobuf生成的，位于user.pb.cc中
    // UserServiceRpc_Stub中定义了Login和Register函数，该函数会调用::PROTOBUF_NAMESPACE_ID::RpcChannel中的CallMethod方法
    // RpcChannel是一个抽象类！其包含一个CallMethod()虚函数。
    // MprpcChannel继承google::protobuf::RpcChannel类，并重写了CallMethod方法

    /*由于派生类是可以用基类指针的(多态)，因此fixbug::UserServiceRpc_Stub桩类不管调用哪个方法，最终都调用到我们的MprpcChannel的
    CallMethod方法，我们在这里就可以进行rpc方法的序列化和反序列化，然后发起远程的rpc调用请求*/
    
    //在命名空间前加::是什么意思？在命名空间前加 :: 表示全局命名空间的前缀。这种用法被称为全局作用域运算符
    //::用于明确全局作用域，确保从全局作用域开始查找，这种用法在处理有多层命名空间的大型项目中非常常见，尤其是在涉及到多个库或框架时。
    //这有助于确保代码的清晰性和正确性，防止由于命名空间冲突导致的类型解析错误。

    //UserServiceRpc_Stub继承UserServiceRpc，UserServiceRpc中有一个descriptor()函数可以返回
    //服务对象的描述信息google::protobuf::ServiceDescriptor*，其中的method方法可以返回google::protobuf::MethodDescriptor*
    
    //客户端是怎么获得服务对象描述信息的？并由此得到服务对象名以及服务对象方法信息的？
    //与服务端使用同一份.proto文件, 生成依赖代码文件
    //UserServiceRpc_Stub使用descriptor()->method(0)可以获得MethodDescriptor*
    //descriptor()定义在UserServiceRpc中，UserServiceRpc_Stub继承UserServiceRpc，通过descriptor()函数返回ServiceDescriptor*，
    //但该ServiceDescriptor*是基类的，并没有真正的业务函数，因此不能通过该ServiceDescriptor*来调用返回结果，
    //但是可以通过该ServiceDescriptor*来获取MethodDescriptor*，同理获得Method和Service的name等信息


    // method不能通过MethodDescriptor调用, 只能通过service进行method调用
    // 创建代理
    fixbug::UserServiceRpc_Stub stub(new MprpcChannel());   
    MprpcController controller;

    // 调用远程发布的rpc方法Login
    
    loginService(stub, controller);
    registerService(stub, controller);

    return 0;
}