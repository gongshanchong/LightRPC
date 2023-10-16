#include "fd_event_pool.h"

namespace lightrpc {
// 单例模式的饿汉模式
static FdEventPool* g_fd_event_pool = NULL;

FdEventPool* FdEventPool::GetFdEventPool() {
  if (g_fd_event_pool != NULL) {
    return g_fd_event_pool;
  }

  g_fd_event_pool = new FdEventPool(128);
  return g_fd_event_pool;
}

FdEventPool::FdEventPool(int size) :m_size_(size) {
  for (int i = 0; i < m_size_; i++) {
    m_fd_pool_.push_back(new FdEvent(i));
  }
}

FdEventPool::~FdEventPool() {
  for (int i = 0; i < m_size_; ++i) {
    if (m_fd_pool_[i] != NULL) {
      delete m_fd_pool_[i];
      m_fd_pool_[i] = NULL;
    }
  }
}

FdEvent* FdEventPool::GetFdEvent(int fd) {
  std::unique_lock<std::mutex> lock(handle_mtx_);
  if ((size_t) fd < m_fd_pool_.size()) {
    return m_fd_pool_[fd];
  }

  int new_size = int(fd * 1.5);
  for (int i = m_fd_pool_.size(); i < new_size; ++i) {
    m_fd_pool_.push_back(new FdEvent(i));
  }
  return m_fd_pool_[fd];
}
}