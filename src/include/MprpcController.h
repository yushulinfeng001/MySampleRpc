#ifndef __MPRPC_CONTROLLER_H__
#define __MPRPC_CONTROLLER_H__

#include <google/protobuf/service.h>
#include <string>
//MprpcController继承自RpcController，RpcController存储了一些控制信息，让我们清楚地知道当前RPC调用的状态。
//用于控制RPC调用的状态和错误信息

class MprpcController : public google::protobuf::RpcController
{
public:
    MprpcController();
    void Reset();
    bool Failed() const;
    std::string ErrorText() const;
    void SetFailed(const std::string& reason);

    // TODO:目前未实现具体的功能
    void StartCancel();
    bool IsCanceled() const;
    void NotifyOnCancel(google::protobuf::Closure* callback);
private:
    bool m_failed;              // RPC方法执行过程中的状态
    std::string m_errText;      // RPC方法执行过程中的错误信息
};

#endif // __MPRPC_CONTROLLER_H__