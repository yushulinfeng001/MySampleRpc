#ifndef __MPRPC_PROVIDER_H__
#define __MPRPC_PROVIDER_H__

#include "google/protobuf/service.h"
#include "MprpcApplication.h"
#include <memory>
#include <muduo/net/TcpServer.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>

//请求信息
struct RequestInfo
{
    std::string service_name;       //服务对象名
    std::string method_name;        //方法名
    uint32_t args_size;             //参数大小
    std::string args_str;           //参数字符串
};

//服务提供端，服务器

class MprpcProvider
{
public:
    // 框架，需要设计成抽象基类
    // RpcProvider 以哈希的方式 存储Service 及其下的method , 从而可以根据服务名与服务快速调用该函数
    void NotifyService(google::protobuf::Service *service);

    // 启动rpc服务节点，开始提供rpc远程网络调用服务
    void Run();

private:
    // 新的socket连接回调
    void OnConnection(const muduo::net::TcpConnectionPtr&);

    // 已建立连接用户的读写事件回调
    void OnMessage(const muduo::net::TcpConnectionPtr&, muduo::net::Buffer*, muduo::Timestamp);

    // Closure的回调操作，用于序列化rpc的响应和网络发送
    void SendRpcResponse(const muduo::net::TcpConnectionPtr&, google::protobuf::Message*);

    // 将服务注册到zookeeper上
    void RegisterZookeeper(const muduo::net::InetAddress&, ZkClient*);

    // 解析数据包
    void ParseRequest(muduo::net::Buffer* buffer, RequestInfo*);

    // 服务类型信息
    struct ServiceInfo
    {
        // 保存服务对象
        google::protobuf::Service *m_service;
        // 保存服务方法       方法名-方法
        std::unordered_map<std::string, const google::protobuf::MethodDescriptor*> m_methodMap;
    };
    //服务名-服务信息
    std::unordered_map<std::string, ServiceInfo> m_serviceMap; // 存储注册成功的服务对象和服务方法的所有信息
    
    //TCP服务器
    std::unique_ptr<muduo::net::TcpServer> m_tcpserverPtr; // 使用智能指针管理
    //事件循环
    muduo::net::EventLoop m_eventLoop;
};

#endif // __MPRPC_PROVIDER_H__