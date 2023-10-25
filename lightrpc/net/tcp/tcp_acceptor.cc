#include <assert.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <string.h>
#include "../../common/log.h"
#include "net_addr.h"
#include "tcp_acceptor.h"

namespace lightrpc {

TcpAcceptor::TcpAcceptor(NetAddr::s_ptr local_addr) : m_local_addr_(local_addr) {
  if (!local_addr->IsValid()) {
    init_flag_ = false;
    LOG_ERROR("invalid local addr %s", local_addr->ToString().c_str());
  }
  m_family_ = m_local_addr_->GetFamily();
  // SOCK_STREAM（TCP流）
  m_listenfd_ = socket(m_family_, SOCK_STREAM, 0);

  if (m_listenfd_ < 0) {
    init_flag_ = false;
    LOG_ERROR("invalid listenfd %d", m_listenfd_);
  }
  // 绑定地址快速重用
  int val = 1;
  if (setsockopt(m_listenfd_, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) != 0) {
    init_flag_ = false;
    LOG_ERROR("setsockopt REUSEADDR error, local addr %s, errno=%d, error=%s", local_addr->ToString().c_str(), errno, strerror(errno));
  }
  // 绑定地址
  socklen_t len = m_local_addr_->GetSockLen();
  if(bind(m_listenfd_, m_local_addr_->GetSockAddr(), len) != 0) {
    init_flag_ = false;
    LOG_ERROR("bind error, local addr %s, errno=%d, error=%s", local_addr->ToString().c_str(), errno, strerror(errno));
  }
  // 进行监听
  if(listen(m_listenfd_, 1000) != 0) {
    init_flag_ = false;
    LOG_ERROR("listen error, local addr %s, errno=%d, error=%s", local_addr->ToString().c_str(), errno, strerror(errno));
  }
}

int TcpAcceptor::GetListenFd() {
  return m_listenfd_;
}

std::pair<int, NetAddr::s_ptr> TcpAcceptor::Accept() {
  if (m_family_ == AF_INET && init_flag_ == true) {
    // 设置新的连接客户端的地址
    sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    socklen_t clien_addr_len = sizeof(clien_addr_len);
    // 调用了 accept 函数获取一个新连接的文件描述符
    int client_fd = ::accept(m_listenfd_, reinterpret_cast<sockaddr*>(&client_addr), &clien_addr_len);
    if (client_fd < 0) {
      LOG_ERROR("accept error, errno=%d, error=%s", errno, strerror(errno));
    }
    IPNetAddr::s_ptr peer_addr = std::make_shared<IPNetAddr>(client_addr);
    LOG_INFO("A client have accpeted succ, peer addr [%s]", peer_addr->ToString().c_str());
    // 返回新连接的文件描述符和地址
    return std::make_pair(client_fd, peer_addr);
  } else {
    return std::make_pair(-1, nullptr);
  }
}

}