#ifndef LIGHTRPC_NET_TCP_NET_ADDR_H
#define LIGHTRPC_NET_TCP_NET_ADDR_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <memory>

namespace lightrpc {
// 设置基类便与后扩展
class NetAddr {
public:
    // 智能指针
    typedef std::shared_ptr<NetAddr> s_ptr;
    // 获取套接字的地址
    virtual sockaddr* GetSockAddr() = 0;
    // 获取套接字的长度
    virtual socklen_t GetSockLen() = 0;
    // 获取套接字的协议家族
    virtual int GetFamily() = 0;
    // 将套接字转换为字符串表示
    virtual std::string ToString() = 0;
    // 验证套接字是否有效
    virtual bool IsValid() = 0;
};

class IPNetAddr : public NetAddr {
public:
    static bool CheckValid(const std::string& addr);

public:
    // 初始化
    IPNetAddr(const std::string& ip, uint16_t port);
    IPNetAddr(const std::string& addr);
    IPNetAddr(sockaddr_in addr);

    sockaddr* GetSockAddr();

    socklen_t GetSockLen();

    int GetFamily();

    std::string ToString();

    bool IsValid();
 
private:
    // 字符串表示的ip地址
    std::string m_ip_;
    // 端口号
    uint16_t m_port_ {0};
    // 套接字地址
    // sockaddr和sockaddr_in包含的数据都是一样的，但他们在使用上有区别
    // 把类型、ip地址、端口填充sockaddr_in结构体，然后强制转换成sockaddr，作为参数传递给系统调用函数
    sockaddr_in m_addr_;
};
}
#endif