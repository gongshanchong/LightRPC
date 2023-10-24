#include <unistd.h>
#include "../../common/log.h"
#include "../../net/fd_event_pool.h"
#include "tcp_connection.h"
#include "../rpc/string_codec.h"
#include "../tinypb/tinypb_codec.h"
#include "../http/http_codec.h"

namespace lightrpc {

TcpConnection::TcpConnection(EventLoop* event_loop, int fd, int buffer_size, NetAddr::s_ptr peer_addr, NetAddr::s_ptr local_addr, ProtocalType protocol, TcpConnectionType type /*= TcpConnectionByServer*/)
    : m_event_loop_(event_loop), m_fd_(fd), m_peer_addr_(peer_addr), m_local_addr_(local_addr), m_connection_type_(type) , m_state_(NotConnected){
  // 缓冲区初始化
  m_in_buffer_ = std::make_shared<TcpBuffer>(buffer_size);
  m_out_buffer_ = std::make_shared<TcpBuffer>(buffer_size);
  // 从事件池中获取事件，并设置为非阻塞
  m_fd_event_ = FdEventPool::GetFdEventPool()->GetFdEvent(fd);
  m_fd_event_->SetNonBlock();
  // 协议的编解码
  if(protocol == ProtocalType::HTTP){
    m_coder_ = new HttpCodec();
  }else{
    m_coder_ = new TinyPBCodec();
  }
  // 判断连接的类型
  if (m_connection_type_ == TcpConnectionByServer) {
    ListenRead();
  }
}

TcpConnection::~TcpConnection() {
  LOG_DEBUG("~TcpConnection");
  Clear();
  if (m_coder_) {
    delete m_coder_;
    m_coder_ = NULL;
  }
}

void TcpConnection::OnRead() {
  // 1. 从 socket 缓冲区，调用 系统的 read 函数读取字节 in_buffer 里面
  if (m_state_ != Connected) {
    LOG_ERROR("onRead error, client has already disconneced, addr[%s], clientfd[%d]", m_peer_addr_->ToString().c_str(), m_fd_);
    return;
  }

  bool is_read_all = false;
  bool is_close = false;
  while(!is_read_all) {
    if (m_in_buffer_->WriteAble() == 0) {
      m_in_buffer_->ResizeBuffer(2 * m_in_buffer_->m_buffer_.size());
    }
    int read_count = m_in_buffer_->WriteAble();
    int write_index = m_in_buffer_->WriteIndex(); 
    // 从socket中读取数据
    int rt = read(m_fd_, &(m_in_buffer_->m_buffer_[write_index]), read_count);
    LOG_DEBUG("success read %d bytes from addr[%s], client fd[%d]", rt, m_peer_addr_->ToString().c_str(), m_fd_);
    // 依据读取的数据大小进行判断
    if (rt > 0) {
      m_in_buffer_->MoveWriteIndex(rt);
      if (rt == read_count) {
        continue;
      } else if (rt < read_count) {
        is_read_all = true;
        break;
      }
    } else if (rt == 0) {
      is_close = true;
      break;
    } else if (rt == -1 && errno == EAGAIN) {
      is_read_all = true;
      break;
    }
  }

  if (is_close) {
    LOG_INFO("peer closed, peer addr [%s], clientfd [%d]", m_peer_addr_->ToString().c_str(), m_fd_);
    Clear();
    return;
  }

  if (!is_read_all) {
    LOG_ERROR("not read all data");
  }

  // 2.补充 RPC 协议解析 
  Excute();
}

void TcpConnection::Excute() {
  // 从 buffer 里 decode 得到响应 message 对象, 执行其回调
  std::vector<AbstractProtocol::s_ptr> result;
  m_coder_->Decode(result, m_in_buffer_);

  if (m_connection_type_ == TcpConnectionByServer) {
    for (size_t i = 0;  i < result.size(); ++i) {
      // 1. 针对每一个请求，调用 rpc 方法，获取响应 message
      // 2. 将响应 message 放入到发送缓冲区，监听可写事件回包
      LOG_INFO("success get request[%s] from client[%s]", result[i]->m_msg_id_.c_str(), m_peer_addr_->ToString().c_str());
      // 3. 进行事件的分发并进行处理，最后将响应发送出去
      RpcDispatcher::GetRpcDispatcher()->Dispatch(result[i], this);
    }
  } else {
    // 如果读取响应 message 成功，会调用 done 函数
    for (size_t i = 0; i < result.size(); ++i) {
      std::string msg_id = result[i]->m_msg_id_;
      auto it = m_read_dones_.find(msg_id);
      if (it != m_read_dones_.end()) {
        it->second(result[i]);
        m_read_dones_.erase(it);
      }
    }
  }
}

void TcpConnection::Reply(std::vector<AbstractProtocol::s_ptr>& replay_messages) {
  m_coder_->Encode(replay_messages, m_out_buffer_);
  ListenWrite();
}

void TcpConnection::OnWrite() {
  // 将当前 out_buffer 里面的数据全部发送给 client
  if (m_state_ != Connected) {
    LOG_ERROR("onWrite error, client has already disconneced, addr[%s], clientfd[%d]", m_peer_addr_->ToString().c_str(), m_fd_);
    return;
  }

  if (m_connection_type_ == TcpConnectionByClient) {
    // 1. 将 message encode 得到字节流
    // 2. 将字节流入到 buffer 里面，然后全部发送
    std::vector<AbstractProtocol::s_ptr> messages;
    for (size_t i = 0; i< m_write_dones_.size(); ++i) {
      messages.push_back(m_write_dones_[i].first);
    } 
    m_coder_->Encode(messages, m_out_buffer_);
  }

  bool is_write_all = false;
  while(true) {
    if (m_out_buffer_->ReadAble() == 0) {
      LOG_DEBUG("no data need to send to client [%s]", m_peer_addr_->ToString().c_str());
      is_write_all = true;
      break;
    }
    int write_size = m_out_buffer_->ReadAble();
    int read_index = m_out_buffer_->ReadIndex();
    // 向socket中写数据
    int rt = write(m_fd_, &(m_out_buffer_->m_buffer_[read_index]), write_size);
    // 依据写的数据大小进行判断
    if (rt >= write_size) {
      LOG_DEBUG("no data need to send to client [%s]", m_peer_addr_->ToString().c_str());
      is_write_all = true;
      break;
    } if (rt == -1 && errno == EAGAIN) {
      // 发送缓冲区已满，不能再发送了。
      // 这种情况我们等下次 fd 可写的时候再次发送数据即可
      LOG_ERROR("write data error, errno==EAGIN and rt == -1");
      break;
    }
  }
  if (is_write_all) {
    // 移除写事件的监听
    m_fd_event_->Cancle(FdEvent::OUT_EVENT);
    m_event_loop_->AddEpollEvent(m_fd_event_);
  }

  // 请求发送成功，执行请求 done 函数（设置读的回调函数）
  if (m_connection_type_ == TcpConnectionByClient) {
    for (size_t i = 0; i < m_write_dones_.size(); ++i) {
      m_write_dones_[i].second(m_write_dones_[i].first);
    }
    m_write_dones_.clear();
  }
}

void TcpConnection::SetState(const TcpState state) {
  m_state_ = state;
}

TcpState TcpConnection::GetState() {
  return m_state_;
}

void TcpConnection::Clear() {
  // 处理一些关闭连接后的清理动作
  if (m_state_ == Closed) {
    return;
  }
  m_fd_event_->Cancle(FdEvent::IN_EVENT);
  m_fd_event_->Cancle(FdEvent::OUT_EVENT);
  m_event_loop_->DeleteEpollEvent(m_fd_event_);

  m_state_ = Closed;
}

void TcpConnection::Shutdown() {
  if (m_state_ == Closed || m_state_ == NotConnected) {
    return;
  }

  // 处于半关闭
  m_state_ = HalfClosing;

  // 调用 shutdown 关闭读写，意味着服务器不会再对这个 fd 进行读写操作了
  // 发送 FIN 报文， 触发了四次挥手的第一个阶段
  // 当 fd 发生可读事件，但是可读的数据为0，即 对端发送了 FIN
  ::shutdown(m_fd_, SHUT_RDWR);
}

void TcpConnection::SetConnectionType(TcpConnectionType type) {
  m_connection_type_ = type;
}

void TcpConnection::ListenWrite() {
  // 当前链接对应的loop开始监听写事件
  m_fd_event_->Listen(FdEvent::OUT_EVENT, std::bind(&TcpConnection::OnWrite, this));
  m_event_loop_->AddEpollEvent(m_fd_event_);
}

void TcpConnection::ListenRead() {
  // 当前链接对应的loop开始监听读事件
  m_fd_event_->Listen(FdEvent::IN_EVENT, std::bind(&TcpConnection::OnRead, this));
  m_event_loop_->AddEpollEvent(m_fd_event_);
}

void TcpConnection::PushSendMessage(AbstractProtocol::s_ptr message, std::function<void(AbstractProtocol::s_ptr)> done) {
  m_write_dones_.push_back(std::make_pair(message, done));
}

void TcpConnection::PushReadMessage(const std::string& msg_id, std::function<void(AbstractProtocol::s_ptr)> done) {
  m_read_dones_.insert(std::make_pair(msg_id, done));
}

NetAddr::s_ptr TcpConnection::GetLocalAddr() {
  return m_local_addr_;
}

NetAddr::s_ptr TcpConnection::GetPeerAddr() {
  return m_peer_addr_;
}

int TcpConnection::GetFd() {
  return m_fd_;
}
}