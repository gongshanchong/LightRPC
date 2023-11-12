#include "tcp_server.h"
#include "../eventloop.h"
#include "tcp_connection.h"
#include "../../common/log.h"
#include "../../common/config.h"
#include <cstdio>

namespace lightrpc {

TcpServer::TcpServer(NetAddr::s_ptr local_addr, NetAddr::s_ptr zookeeper_addr, ProtocalType protocol, int timeout) : m_local_addr_(local_addr), m_zookeeper_addr_(zookeeper_addr), protocol_(protocol), timeout_(timeout){
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
}

void TcpServer::Init() {
  m_acceptor_ = std::make_shared<TcpAcceptor>(m_local_addr_);

  m_main_event_loop_ = EventLoop::GetCurrentEventLoop();
  m_io_thread_pool_ = new IOThreadPool(Config::GetGlobalConfig()->m_io_threads_);
  // 主loop添加服务器端的事件监听
  m_listen_fd_event_ = std::make_shared<FdEvent>(m_acceptor_->GetListenFd());
  m_listen_fd_event_->Listen(FdEvent::IN_EVENT, std::bind(&TcpServer::OnAccept, this));
  m_main_event_loop_->AddEpollEvent(m_listen_fd_event_.get());
}

void TcpServer::OnAccept() {
  // 获取连接客户端的文件描述符和地址
  auto re = m_acceptor_->Accept();
  int client_fd = re.first;
  NetAddr::s_ptr peer_addr = re.second;

  // 把 cleintfd 添加到任意 IO 线程里面，对线程池的线程进行轮流添加
  IOThread* io_thread = m_io_thread_pool_->GetIOThread();
  // 将客服端放入到线程中的loop中进行监听
  // 间隔为timeout_
  TcpConnection::s_ptr connetion = std::make_shared<TcpConnection>(io_thread->GetEventLoop(), client_fd, 128, peer_addr, m_local_addr_, protocol_);
  TimerEvent::s_ptr timer_event = std::make_shared<TimerEvent>(timeout_, false, std::bind(&TcpServer::ClearClientTimerFunc, this, std::placeholders::_1), client_fd);
	connetion->AddTimerEvent(timer_event);
  connetion->SetState(Connected);
  // 添加到链接组中
  m_client_[client_fd] = std::move(connetion);

  LOG_INFO("TcpServer succ get client, fd=%d", client_fd);
}

void TcpServer::Start() {
  // 把当前rpc节点上要发布的服务全部注册到zk上面
  RpcDispatcher::GetRpcDispatcher()->RegisterToZookeeper(m_local_addr_, m_zookeeper_addr_);
  m_io_thread_pool_->Start();
  m_main_event_loop_->Loop();
}

void TcpServer::ClearClientTimerFunc(int fd) {
  if((fd == -1) || (m_client_.find(fd) == m_client_.end())) return;

  // 智能指针conn，当conn引用计数为0时，conn会销毁，conn中的智能指针sock、channel也会销毁
  auto connetion = m_client_[fd];
  connetion->SetState(TcpState::Closed);
  LOG_DEBUG("TcpConection [fd:%d] will delete becasue of timeout, state=%d", connetion->GetFd(), connetion->GetState());
  m_client_.erase(fd);
}
}