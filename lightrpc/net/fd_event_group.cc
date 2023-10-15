#include "fd_event_group.h"

namespace lightrpc {
// 单例模式的饿汉模式
static FdEventGroup* g_fd_event_group = NULL;

FdEventGroup* FdEventGroup::GetFdEventGroup() {
  if (g_fd_event_group != NULL) {
    return g_fd_event_group;
  }

  g_fd_event_group = new FdEventGroup(128);
  return g_fd_event_group;
}

FdEventGroup::FdEventGroup(int size) :m_size_(size) {
  for (int i = 0; i < m_size_; i++) {
    m_fd_group_.push_back(new FdEvent(i));
  }
}

FdEventGroup::~FdEventGroup() {
  for (int i = 0; i < m_size_; ++i) {
    if (m_fd_group_[i] != NULL) {
      delete m_fd_group_[i];
      m_fd_group_[i] = NULL;
    }
  }
}

FdEvent* FdEventGroup::GetFdEvent(int fd) {
  std::unique_lock<std::mutex> lock(handle_mtx_);
  if ((size_t) fd < m_fd_group_.size()) {
    return m_fd_group_[fd];
  }

  int new_size = int(fd * 1.5);
  for (int i = m_fd_group_.size(); i < new_size; ++i) {
    m_fd_group_.push_back(new FdEvent(i));
  }
  return m_fd_group_[fd];
}
}