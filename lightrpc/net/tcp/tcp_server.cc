#include "tcp_server.h"
#include "../eventloop.h"
#include "tcp_connection.h"
#include "../../common/log.h"
#include "../../common/config.h"

namespace lightrpc {

TcpServer::TcpServer(NetAddr::s_ptr local_addr, ProtocalType protocol, int timeout) : m_local_addr_(local_addr), protocol_(protocol), timeout_(timeout){
  Init(); 
  LOG_INFO("lightrpc TcpServer listen sucess on [%s]", m_local_addr_->ToString().c_str());
}

TcpServer::~TcpServer() {
  if (m_main_event_loop_) {
    delete m_main_event_loop_;
    m_main_event_loop_ = NULL;
  }
  if (m_io_thread_pool_) {
    delete m_io_thread_pool_;
    m_io_thread_pool_ = NULL; 
  }
  if (m_listen_fd_event_) {
    delete m_listen_fd_event_;
    m_listen_fd_event_ = NULL;
  }
}

void TcpServer::Init() {
  m_acceptor_ = std::make_shared<TcpAcceptor>(m_local_addr_);

  m_main_event_loop_ = EventLoop::GetCurrentEventLoop();
  m_io_thread_pool_ = new IOThreadPool(Config::GetGlobalConfig()->m_io_threads_);
  // 主loop添加服务器端的事件监听
  m_listen_fd_event_ = new FdEvent(m_acceptor_->GetListenFd());
  m_listen_fd_event_->Listen(FdEvent::IN_EVENT, std::bind(&TcpServer::OnAccept, this));
  m_main_event_loop_->AddEpollEvent(m_listen_fd_event_);

  // 主loop添加服务器端的时间事件监听，定时去除已经关闭的连接
  // 重复，间隔为5秒
  m_clear_client_timer_event_ = std::make_shared<TimerEvent>(timeout_, true, std::bind(&TcpServer::ClearClientTimerFunc, this));
	m_main_event_loop_->AddTimerEvent(m_clear_client_timer_event_);
}

void TcpServer::OnAccept() {
  // 获取连接客户端的文件描述符和地址
  auto re = m_acceptor_->Accept();
  int client_fd = re.first;
  NetAddr::s_ptr peer_addr = re.second;

  m_client_counts_++;
  // 把 cleintfd 添加到任意 IO 线程里面，对线程池的线程进行轮流添加
  IOThread* io_thread = m_io_thread_pool_->GetIOThread();
  // 将客服端放入到线程中的loop中进行监听
  TcpConnection::s_ptr connetion = std::make_shared<TcpConnection>(io_thread->GetEventLoop(), client_fd, 128, peer_addr, m_local_addr_, protocol_);
  connetion->SetState(Connected);
  m_client_.insert(connetion);

  LOG_INFO("TcpServer succ get client, fd=%d", client_fd);
}

void TcpServer::Start() {
  m_io_thread_pool_->Start();
  m_main_event_loop_->Loop();
}

void TcpServer::ClearClientTimerFunc() {
  auto it = m_client_.begin();
  for (it = m_client_.begin(); it != m_client_.end(); ) {
    if ((*it) != nullptr && (*it).use_count() > 0 && (*it)->GetState() == Closed) {
      // need to delete TcpConnection
      LOG_DEBUG("TcpConection [fd:%d] will delete, state=%d", (*it)->GetFd(), (*it)->GetState());
      it = m_client_.erase(it);
    } else {
      it++;
    }
  }
}
}