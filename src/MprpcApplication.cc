#include "MprpcApplication.h"
#include <iostream>
#include <unistd.h>
#include <string>
/**
 * class Foo {
 *      static int foo;
 * };
 * 
 * int Foo::foo = 0;
 */

//配置类，读取配置文件


MprpcApplication::MprpcConfig MprpcApplication::m_config;       //前面是类型，后面是名称

void MprpcApplication::MprpcConfig::LoadConfigFile(const char *config_file)
{
    std::cout << "start to Load Config File" << std::endl;

    // 打开文件
    FILE *pf = fopen(config_file, "r");
    if (nullptr == pf)
    {
        std::cout << config_file << " is not exist!" << std::endl;
        exit(EXIT_FAILURE);
    }

    /**
     * 1.注释
     * 2.正确的配置项 =
     * 3.去掉开头的多余的空格
     */
    while (!feof(pf))
    {
        // 读取一行，"xxxx\n"
        char buf[512] = {0};
        fgets(buf, 512, pf);

        // 去掉字符串前面多余空格
        std::string read_buf(buf);
        Trim(read_buf);

        // 判断#的注释
        if (read_buf[0] == '#' || read_buf.empty())
        {
            continue;
        }

        // 解析配置项
        int idx = read_buf.find('=');
        if (idx == -1)
        {
            continue;
        }

        std::string key;
        std::string value;
        // [0,idx)
        key = read_buf.substr(0, idx);
        // 防止这种情况出现：rpc_server_ip =127.0.0.1\n
        Trim(key);

        // rpcserverip=127.0.0.1\n
        int end_idx = read_buf.find('\n', idx);
        /**
         *            idx          endidx
         *             |             |         
         * rpcserverip = 127.0.0.1  \n
         * 截断了 \n 
         */
        value = read_buf.substr(idx+1, end_idx-idx-1);
        Trim(value);
        m_configMap.insert({key, value});
    }
}

// 查询配置项信息
std::string MprpcApplication::MprpcConfig::Load(const std::string &key)
{
    auto it = m_configMap.find(key);
    if (it == m_configMap.end())
    {
        return "";
    }
    return it->second;
}


void MprpcApplication::MprpcConfig::Trim(std::string &src_buf)
{
    // 找到第一个不是空格的字符
    int idx = src_buf.find_first_not_of(' ');
    if (idx != -1)
    {
        // 字符串前面有空格的情况
        src_buf = src_buf.substr(idx, src_buf.size()-idx);
    }

    // 去掉字符串后面多余的空格
    idx = src_buf.find_last_not_of(' ');
    if (idx != -1)
    {
        src_buf = src_buf.substr(0, idx+1);
    }
}

// -----------------------------------------------------------------------+
// Class MprpcApplication                                                 +
// -----------------------------------------------------------------------+

// 参数提示信息
void ShowArgsHelp()
{
    std::cout << "format: command -i <configfile>" << std::endl;
}

void MprpcApplication::Init(int argc, char **argv)
{
    if (argc < 2) 
    {
        //未传入任何参数,错误
        ShowArgsHelp();
        exit(EXIT_FAILURE);
    }

    int opt = 0;
    std::string config_file;
    //getopt() 方法是用来分析命令行参数的，该方法由 Unix 标准库提供，包含在 <unistd.h> 头文件中
    //argc表示参数数量，argv表示参数的字符串变量数组，由 main 函数直接传入
    while ((opt = getopt(argc, argv, "i:")) != -1)
    {
        switch (opt)
        {
            case 'i':
                //optarg：如果某个选项有参数，这包含当前选项的参数字符串
                config_file = optarg;   //传入配置文件，用config_file存储
                break;
            case '?':
                std::cout << "invalid args!" << std::endl;
                exit(EXIT_FAILURE);
            case ':':
                std::cout << "need <configfile>" << std::endl;
                exit(EXIT_FAILURE);
            default:
                break;
        }
    }

    // 开始加载配置文件
    // 用一个哈希表存储，rpc_server_ip，rpc_server_port，zookeeper_ip，zookeeper_port等信息
    m_config.LoadConfigFile(config_file.c_str());

    printf("=======================Loading Configure Information======================\n");
    printf("config_file:      %-10s\n", config_file.c_str());
    printf("rpc_server_ip:    %-10s\n", m_config.Load("rpc_server_ip").c_str());
    printf("rpc_server_port:  %-10s\n", m_config.Load("rpc_server_port").c_str());
    printf("zookeeper_ip:     %-10s\n", m_config.Load("zookeeper_ip").c_str());
    printf("zookeeper_port:   %-10s\n", m_config.Load("zookeeper_port").c_str());
    printf("==========================================================================\n");  
}

MprpcApplication& MprpcApplication::GetInstance()
{
    static MprpcApplication app;
    return app;
}

MprpcApplication::MprpcConfig& MprpcApplication::GetConfig()
{
    return m_config;
}

