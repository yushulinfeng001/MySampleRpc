#ifndef __MPRPC_APPLICATION_H__
#define __MPRPC_APPLICATION_H__

#include "MprpcChannel.h"
#include "MprpcController.h"
#include "ZookeeperUtil.h"

#include <memory.h>

// mrpc 框架的初始化类
// 框架只需要一个 因此用单例模式进行设计


//可执行文件的参数要求, 在运行时必须指明配置文件 即必须要有 - i选项 且提供配置文件
//使用单例模式 饿汉模式 线程安全 在局部函数中申请静态变量, 延长其生命周期.同时C++11 编译器能保证初始化静态局部变量的线程安全性
class MprpcApplication
{
public:
    // MprpcConfig用于读取配置文件
    class MprpcConfig
    {
    public:

        void LoadConfigFile(const char *config_file);

        std::string Load(const std::string &key);

        // 去掉字符串前后的空格
        void Trim(std::string &src_buf);

        // 键值对储存信息
        std::unordered_map<std::string, std::string> m_configMap;        
    };

    static void Init(int argc, char **argv);
    static MprpcApplication& GetInstance();
    static MprpcConfig& GetConfig();
    
private:   
    static MprpcConfig m_config;

    //采用单例模式 , 把构造相关的函数都给delete掉, 默认构造函数只能设置成private
    MprpcApplication() = default;
    MprpcApplication(const MprpcApplication&) = delete;
    MprpcApplication(MprpcApplication&&) = delete;
    void operator=(const MprpcApplication&) = delete;
};

#endif // __MPRPC_APPLICATION_H__