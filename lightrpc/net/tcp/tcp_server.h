#ifndef LIGHTRPC_NET_TCP_SERVER_H
#define LIGHTRPC_NET_TCP_SERVER_H

#include <set>
#include "tcp_acceptor.h"
#include "tcp_connection.h"
#include "net_addr.h"
#include "../eventloop.h"
#include "../io_thread_pool.h"

namespace lightrpc {

class TcpServer {
 public:
  TcpServer(NetAddr::s_ptr local_addr, ProtocalType protocol, int timeout);

  ~TcpServer();

  // 启动tcp
  void Start();

 private:
  // 初始化
  void Init();

  // 当有新客户端连接之后需要执行
  void OnAccept();

  // 清除 closed 的连接
  void ClearClientTimerFunc(int fd);

 private:
  TcpAcceptor::s_ptr m_acceptor_;  // 链接器

  NetAddr::s_ptr m_local_addr_;    // 本地监听地址

  EventLoop* m_main_event_loop_ {NULL};     // mainReactor
  
  IOThreadPool* m_io_thread_pool_ {NULL};   // subReactor 组

  FdEvent* m_listen_fd_event_;              // 当前监听事件

  ProtocalType protocol_;                    // 通信协议

  // TCP连接
  std::unordered_map<int, TcpConnection::s_ptr> m_client_;

  int timeout_;
};
}
#endif