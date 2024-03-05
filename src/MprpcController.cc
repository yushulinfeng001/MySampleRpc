#include "MprpcController.h"

//初始化，一开始认为是RPC调用的状态是正确的
MprpcController::MprpcController()
{
    m_failed = false;
    m_errText = "";
}
//重置RPC调用的状态
void MprpcController::Reset()
{
    m_failed = false;
    m_errText = "";
}
//判断当前RPC调用的成功与否
bool MprpcController::Failed() const
{
    return m_failed;
}
//获取当前RPC调用的错误信息
std::string MprpcController::ErrorText() const
{
    return m_errText;
}
//设置当前RPC调用的错误信息
void MprpcController::SetFailed(const std::string& reason)
{
    m_failed = true;
    m_errText = reason;
}

// 目前未实现具体的功能
void MprpcController::StartCancel(){}
bool MprpcController::IsCanceled() const {return false;}
void MprpcController::NotifyOnCancel(google::protobuf::Closure* callback) {}