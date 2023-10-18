#include <string.h>
#include <regex>
#include "../../common/log.h"
#include "net_addr.h"


namespace lightrpc {

bool IPNetAddr::CheckValid(const std::string& addr) {
    // 找到ip和端口号之间的符号
    size_t i = addr.find_first_of(":");
    if (i == addr.npos) {
        return false;
    }
    // 切分出ip和端口号
    std::string ip = addr.substr(0, i);
    std::string port = addr.substr(i + 1, addr.size() - i - 1);
    if (ip.empty() || port.empty()) {
        return false;
    }
    // 正则表达式判断ip地址是否正确
    std::regex pattern("((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)");
    std::smatch res;
    if(!regex_match(ip, res, pattern)){
        return false;
    }
    // 判断端口号是否在正确的范围内
    // TCP与UDP段结构中端口地址都是16位，可以有在0---65535范围内的端口号
    // 1、公认端口（Well Known Ports）：从0到1023，它们紧密绑定（binding）于一些服务。其中0不使用，通常这些端口的通讯明确表明了某种服务的协议。
    // 2、注册端口（Registered Ports）：从1024到49151。它们松散地绑定于一些服务。也就是说有许多服务绑定于这些端口，这些端口同样用于许多其它目的。
    // 3、动态和/或私有端口（Dynamic and/or Private Ports）：从49152到65535。理论上，不应为服务分配这些端口。实际上，机器通常从1024起分配动态端口。
    int iport = std::atoi(port.c_str());
    if (iport <= 0 || iport >= 65536) {
    return false;
    }

    return true;
}

IPNetAddr::IPNetAddr(const std::string& ip, uint16_t port) : m_ip_(ip), m_port_(port) {
    memset(&m_addr_, 0, sizeof(m_addr_));
    // 表示套接字要使用的协议簇
    // AF_UNIX（本机通信）、AF_INET（TCP/IP – IPv4）、AF_INET6（TCP/IP – IPv6）
    m_addr_.sin_family = AF_INET;
    // 指定指定 ip 地址和端口号
    m_addr_.sin_addr.s_addr = inet_addr(m_ip_.c_str());
    m_addr_.sin_port = htons(m_port_);
}
  
IPNetAddr::IPNetAddr(const std::string& addr) {
  // 从字符串格式的地址中提取出ip地址和端口号
  size_t i = addr.find_first_of(":");
  if (i == addr.npos) {
    LOG_ERROR("invalid ipv4 addr %s", addr.c_str());
    return;
  }
  m_ip_ = addr.substr(0, i);
  m_port_ = std::atoi(addr.substr(i + 1, addr.size() - i - 1).c_str());

  memset(&m_addr_, 0, sizeof(m_addr_));
  // 表示套接字要使用的协议簇
  // AF_UNIX（本机通信）、AF_INET（TCP/IP – IPv4）、AF_INET6（TCP/IP – IPv6）
  m_addr_.sin_family = AF_INET;
  // 指定指定 ip 地址和端口号
  m_addr_.sin_addr.s_addr = inet_addr(m_ip_.c_str());
  m_addr_.sin_port = htons(m_port_);
}

IPNetAddr::IPNetAddr(sockaddr_in addr) : m_addr_(addr) {
  // 将一个网络字节序的IP地址（也就是结构体in_addr类型变量）转化为点分十进制的IP地址（字符串）
  // 使用特定的IP地址则需要使用inet_addr 和inet_ntoa完成字符串和in_addr结构体的互换
  m_ip_ = std::string(inet_ntoa(m_addr_.sin_addr));
  // 将一个16位数由网络字节顺序转换为主机字节顺序
  m_port_ = ntohs(m_addr_.sin_port);
}

sockaddr* IPNetAddr::GetSockAddr() {
  // reinterpret_cast 运算符并不会改变括号中运算对象的值，而是对该对象从位模式上进行重新解释
  return reinterpret_cast<sockaddr*>(&m_addr_);
}

socklen_t IPNetAddr::GetSockLen() {
  return sizeof(m_addr_);
}

int IPNetAddr::GetFamily() {
  return m_addr_.sin_family;
}

std::string IPNetAddr::ToString() {
  std::string add;
  add = m_ip_ + ":" + std::to_string(m_port_);
  return add;
}

bool IPNetAddr::IsValid() {
  if (m_ip_.empty()) {
    return false;  
  }

  if (m_port_ <= 0 || m_port_ > 65535) {
    return false;
  }
  // INADDR_NONE 是个宏定义，代表IpAddress无效的IP地址
  if (inet_addr(m_ip_.c_str()) == INADDR_NONE) {
    return false;
  }
  return true;
}

}