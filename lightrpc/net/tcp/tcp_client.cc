#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include "../../common/log.h"
#include "tcp_client.h"
#include "../eventloop.h"
#include "../../common/error_code.h"
#include "net_addr.h"

namespace lightrpc {

TcpClient::TcpClient(NetAddr::s_ptr peer_addr, ProtocalType protocol) : m_peer_addr_(peer_addr), protocol_(protocol){
  m_event_loop_ = EventLoop::GetCurrentEventLoop();
  m_fd_ = socket(peer_addr->GetFamily(), SOCK_STREAM, 0);

  if (m_fd_ < 0) {
    LOG_ERROR("TcpClient::TcpClient() error, failed to create fd");
    return;
  }
  // 创建客户端的事件
  m_fd_event_ = std::make_shared<FdEvent>(m_fd_);
  m_fd_event_->SetNonBlock();
  // 创建客户端的链接
  m_connection_ = std::make_shared<TcpConnection>(m_event_loop_, m_fd_, 128, peer_addr, nullptr, protocol_, TcpConnectionByClient);
  m_connection_->SetConnectionType(TcpConnectionByClient);
  m_connection_->SetFdEvent(m_fd_event_);
}

TcpClient::~TcpClient() {
  LOG_DEBUG("TcpClient::~TcpClient()");
}

// 异步的进行 conenct
// 如果connect 成功，done 会被执行
void TcpClient::Connect(std::function<void()> done) {
  int rt = ::connect(m_fd_, m_peer_addr_->GetSockAddr(), m_peer_addr_->GetSockLen());
  if (rt == 0) {
    LOG_DEBUG("connect [%s] sussess", m_peer_addr_->ToString().c_str());
    m_connection_->SetState(Connected);
    InitLocalAddr();
    if (done) {
      done();
    }
  } else if (rt == -1) {
    // 套接字是非阻塞的，且一个连接尝试将被阻塞
    if (errno == EINPROGRESS) {
      // epoll 监听可写事件，然后判断错误码
      m_fd_event_->Listen(FdEvent::OUT_EVENT, 
        [this, done]() {
          int rt = ::connect(m_fd_, m_peer_addr_->GetSockAddr(), m_peer_addr_->GetSockLen());
          if ((rt < 0 && errno == EISCONN) || (rt == 0)) {
            LOG_DEBUG("connect [%s] sussess", m_peer_addr_->ToString().c_str());
            InitLocalAddr();
            m_connection_->SetState(Connected);
          } else {
            if (errno == ECONNREFUSED) {
              m_connect_error_code_ = ERROR_PEER_CLOSED;
              m_connect_error_info_ = "connect refused, sys error = " + std::string(strerror(errno));
            } else {
              m_connect_error_code_ = ERROR_FAILED_CONNECT;
              m_connect_error_info_ = "connect unkonwn error, sys error = " + std::string(strerror(errno));
            }
            LOG_DEBUG("connect errror, errno=%d, error=%s", errno, strerror(errno));
            close(m_fd_);
            m_fd_ = socket(m_peer_addr_->GetFamily(), SOCK_STREAM, 0);
          }

          // 连接完后需要去掉可写事件的监听，不然会一直触发
          m_event_loop_->DeleteEpollEvent(m_fd_event_.get());
          LOG_DEBUG("now begin to done");
          // 如果连接完成，才会执行回调函数
          if (done) {
            done();
          }
        }
      );
      // 重新添加可写事件的监听，便于下次重新连接
      m_event_loop_->AddEpollEvent(m_fd_event_.get());
      // 启动loop监听
      if (!m_event_loop_->IsLooping()) {
        m_event_loop_->Loop();
      }
    } else {
      LOG_DEBUG("connect errror, errno=%d, error=%s", errno, strerror(errno));
      m_connect_error_code_ = ERROR_FAILED_CONNECT;
      m_connect_error_info_ = "connect error, sys error = " + std::string(strerror(errno));
      if (done) {
        done();
      }
    }
  }

}

void TcpClient::Stop() {
  if (m_event_loop_->IsLooping()) {
    m_event_loop_->Stop();
  }
}

int TcpClient::GetFd() {
  return m_fd_;
}

// 异步的发送 message
// 如果发送 message 成功，会调用 done 函数， 函数的入参就是 message 对象 
void TcpClient::WriteMessage(AbstractProtocol::s_ptr message, std::function<void(AbstractProtocol::s_ptr)> done) {
  // 1. 把 message 对象写入到 Connection 的 buffer, done 也写入
  // 2. 启动 connection 可写事件
  m_connection_->PushSendMessage(message, done);
  m_connection_->ListenWrite();
}

// 异步的读取 message
// 如果读取 message 成功，会调用 done 函数， 函数的入参就是 message 对象 
void TcpClient::ReadMessage(const std::string& msg_id, std::function<void(AbstractProtocol::s_ptr)> done) {
  // 1. 监听可读事件
  // 2. 从 buffer 里 decode 得到 message 对象, 判断是否 msg_id 相等，相等则读成功，执行其回调
  m_connection_->PushReadMessage(msg_id, done);
  m_connection_->ListenRead();
}

int TcpClient::GetConnectErrorCode() {
  return m_connect_error_code_;
}

std::string TcpClient::GetConnectErrorInfo() {
  return m_connect_error_info_;
}

NetAddr::s_ptr TcpClient::GetPeerAddr() {
  return m_peer_addr_;
}

NetAddr::s_ptr TcpClient::GetLocalAddr() {
  return m_local_addr_;
}

void TcpClient::InitLocalAddr() {
  sockaddr_in local_addr;
  socklen_t len = sizeof(local_addr);

  // 可以获得一个与socket相关的地址
  // 服务器端可以通过它得到相关客户端地址。而客户端也可以得到当前已连接成功的socket的ip和端口。
  int ret = getsockname(m_fd_, reinterpret_cast<sockaddr*>(&local_addr), &len);
  if (ret != 0) {
    LOG_ERROR("initLocalAddr error, getsockname error. errno=%d, error=%s", errno, strerror(errno));
    return;
  }

  m_local_addr_ = std::make_shared<IPNetAddr>(local_addr);
}

void TcpClient::AddTimerEvent(TimerEvent::s_ptr timer_event) {
  m_connection_->AddTimerEvent(timer_event);
}
}