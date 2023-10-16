#ifndef LIGHTRPC_NET_FD_EVENT_POOL_H
#define LIGHTRPC_NET_FD_EVENT_POOL_H

#include <vector>
#include <mutex>
#include "../net/fd_event.h"
#include "../common/log.h"

namespace lightrpc {
// 事件池
class FdEventPool {
 public:
  FdEventPool(int size);

  ~FdEventPool();
  FdEvent* GetFdEvent(int fd);

 public:
  static FdEventPool* GetFdEventPool();

 private:
  int m_size_ {0};
  std::vector<FdEvent*> m_fd_pool_;
  std::mutex handle_mtx_;
};
}

#endif