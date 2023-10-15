#ifndef LIGHTRPC_NET_FD_EVENT_GROUP_H
#define LIGHTRPC_NET_FD_EVENT_GROUP_H

#include <vector>
#include <mutex>
#include "../net/fd_event.h"
#include "../common/log.h"

namespace lightrpc {
// 事件池
class FdEventGroup {
 public:
  FdEventGroup(int size);

  ~FdEventGroup();
  FdEvent* GetFdEvent(int fd);

 public:
  static FdEventGroup* GetFdEventGroup();

 private:
  int m_size_ {0};
  std::vector<FdEvent*> m_fd_group_;
  std::mutex handle_mtx_;
};
}

#endif