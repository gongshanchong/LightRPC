#include "io_thread_pool.h"
#include "../common/log.h"

namespace lightrpc {
IOThreadPool::IOThreadPool(int size) : m_size_(size) {
  m_io_thread_pool_.resize(size);
  for (size_t i = 0; (int)i < size; ++i) {
    m_io_thread_pool_[i] = new IOThread();
  }
}

IOThreadPool::~IOThreadPool(){
  for (size_t i = 0; (int)i < m_size_; ++i) {
    if(m_io_thread_pool_[i]){
      delete m_io_thread_pool_[i];
      m_io_thread_pool_[i] = NULL;
    }
  }
}

void IOThreadPool::Start() {
  for (size_t i = 0; i < m_io_thread_pool_.size(); ++i) {
    m_io_thread_pool_[i]->Start();
  }
}

void IOThreadPool::Join() {
  for (size_t i = 0; i < m_io_thread_pool_.size(); ++i) {
    m_io_thread_pool_[i]->Join();
  }
} 

IOThread* IOThreadPool::GetIOThread() {
  if (m_index_ == (int)m_io_thread_pool_.size() || m_index_ == -1)  {
    m_index_ = 0;
  }
  return m_io_thread_pool_[m_index_++];
}

}