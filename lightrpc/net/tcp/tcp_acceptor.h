#ifndef LIGHTRPC_NET_TCP_TCP_ACCEPTOR_H
#define LIGHTRPC_NET_TCP_TCP_ACCEPTOR_H

#include <memory>
#include "net_addr.h"

namespace lightrpc {

class TcpAcceptor {
 public:
  typedef std::shared_ptr<TcpAcceptor> s_ptr;

  TcpAcceptor(NetAddr::s_ptr local_addr);

  std::pair<int, NetAddr::s_ptr> Accept();

  int GetListenFd();

 private:
  NetAddr::s_ptr m_local_addr_; // 服务端监听的地址，addr -> ip:port 

  int m_family_ {-1};   // 套接字的协议家族

  int m_listenfd_ {-1}; // 监听套接字

  bool init_flag_{true}; // 初始化是否成功
};
}
#endif