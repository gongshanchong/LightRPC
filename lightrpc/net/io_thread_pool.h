#ifndef LIGHTRPC_NET_IO_THREAD_POOL_H
#define LIGHTRPC_NET_IO_THREAD_POOL_H

#include <vector>
#include "../common/log.h"
#include "io_thread.h"

namespace lightrpc {

class IOThreadPool {

public:
  IOThreadPool(int size);

  ~IOThreadPool();

  void Start();

  void Join();

  IOThread* GetIOThread();

private:
  int m_size_ {0};
  std::vector<IOThread*> m_io_thread_pool_;

  int m_index_ {0};
};
}
#endif