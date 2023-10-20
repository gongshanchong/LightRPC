#ifndef LIGHTRPC_NET_TCP_TCP_CLIENT_H
#define LIGHTRPC_NET_TCP_TCP_CLIENT_H

#include <memory>
#include "net_addr.h"
#include "../eventloop.h"
#include "tcp_connection.h"
#include "../rpc/abstract_protocol.h"
#include "../timer_event.h"

namespace lightrpc {

class TcpClient {
 public:
  typedef std::shared_ptr<TcpClient> s_ptr;

  TcpClient(NetAddr::s_ptr peer_addr);

  ~TcpClient();

  // 异步的进行 conenct
  // 如果 connect 完成，done 会被执行
  void Connect(std::function<void()> done);

  // 异步的发送 message
  // 如果发送 message 成功，会调用 done 函数， 函数的入参就是 message 对象 
  void WriteMessage(AbstractProtocol::s_ptr message, std::function<void(AbstractProtocol::s_ptr)> done);

  // 异步的读取 message
  // 如果读取 message 成功，会调用 done 函数， 函数的入参就是 message 对象 
  void ReadMessage(const std::string& msg_id, std::function<void(AbstractProtocol::s_ptr)> done);

  void Stop();

  int GetConnectErrorCode();

  std::string GetConnectErrorInfo();

  NetAddr::s_ptr GetPeerAddr();

  NetAddr::s_ptr GetLocalAddr();

  void InitLocalAddr();

  void AddTimerEvent(TimerEvent::s_ptr timer_event);

 private:
  NetAddr::s_ptr m_peer_addr_;    // 连接的另一端地址
  NetAddr::s_ptr m_local_addr_;   // 本地监听地址

  EventLoop* m_event_loop_ {NULL};

  int m_fd_ {-1};
  FdEvent* m_fd_event_ {NULL};

  TcpConnection::s_ptr m_connection_; // 连接

  // 连接的错误信息
  int m_connect_error_code_ {0};
  std::string m_connect_error_info_;
};  
}
#endif