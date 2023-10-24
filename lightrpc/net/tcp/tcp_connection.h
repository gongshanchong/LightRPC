#ifndef LIGHTRPC_NET_TCP_TCP_CONNECTION_H
#define LIGHTRPC_NET_TCP_TCP_CONNECTION_H

#include <memory>
#include <map>
#include <queue>
#include "net_addr.h"
#include "tcp_buffer.h"
#include "../io_thread.h"
#include "../rpc/abstract_codec.h"
#include "../rpc/rpc_dispatcher.h"

namespace lightrpc {

// TCP连接状态
enum TcpState {
  NotConnected = 1,
  Connected = 2,
  HalfClosing = 3,
  Closed = 4,
};

enum TcpConnectionType {
  TcpConnectionByServer = 1,  // 作为服务端使用，代表跟对端客户端的连接
  TcpConnectionByClient = 2,  // 作为客户端使用，代表跟对赌服务端的连接
};

class TcpConnection {
 public:
  typedef std::shared_ptr<TcpConnection> s_ptr;

 public:
  TcpConnection(EventLoop* event_loop, int fd, int buffer_size, NetAddr::s_ptr peer_addr, NetAddr::s_ptr local_addr, ProtocalType protocol, TcpConnectionType type = TcpConnectionByServer);

  ~TcpConnection();

  // 调用 read 系统函数从 socket 的内核缓冲区里面读取数据，然后放入到我们自己的应用层读缓冲区（m_read_buffer）
  void OnRead();

  // 进行处理
  void Excute();

  // 写意味着调用 write 函数将写缓冲区的数据写入到 socket 上，然后由内核负责发送给对端。
  void OnWrite();

  void SetState(const TcpState state);

  TcpState GetState();

  NetAddr::s_ptr GetLocalAddr();

  NetAddr::s_ptr GetPeerAddr();

  void Clear();

  int GetFd();

  // 服务器主动关闭连接
  void Shutdown();

  void SetConnectionType(TcpConnectionType type);

  // 启动监听可写事件
  void ListenWrite();

  // 启动监听可读事件
  void ListenRead();

  // 添加TinyPB协议的写函数
  void PushSendMessage(AbstractProtocol::s_ptr message, std::function<void(AbstractProtocol::s_ptr)> done);

  // 添加TinyPB协议的读函数
  void PushReadMessage(const std::string& msg_id, std::function<void(AbstractProtocol::s_ptr)> done);

  // 任务分发时，回复TinyPB协议
  void Reply(std::vector<AbstractProtocol::s_ptr>& replay_messages);

private:
  EventLoop* m_event_loop_ {NULL};    // 代表持有该连接的 IO 线程

  NetAddr::s_ptr m_local_addr_;       // 监听的服务器地址
  NetAddr::s_ptr m_peer_addr_;        // 链接的客户端的地址

  TcpBuffer::s_ptr m_in_buffer_;      // 接收缓冲区
  TcpBuffer::s_ptr m_out_buffer_;     // 发送缓冲区

  int m_fd_ {0};
  FdEvent* m_fd_event_ {NULL};        // 当前 fd_event 对象

  // 服务器状态
  TcpState m_state_;
  TcpConnectionType m_connection_type_ {TcpConnectionByServer};

  // 协议编解码
  AbstractCodec* m_coder_ {NULL};

  // TinyPB协议的读写处理函数，回调函数
  // 如果发送 message 成功，会调用 done 函数， 函数的入参就是 message 对象 
  std::vector<std::pair<AbstractProtocol::s_ptr, std::function<void(AbstractProtocol::s_ptr)>>> m_write_dones_;
  // key 为 msg_id
  // 如果读取 message 成功，会调用 done 函数， 函数的入参就是 message 对象 
  std::map<std::string, std::function<void(AbstractProtocol::s_ptr)>> m_read_dones_;
};
}
#endif