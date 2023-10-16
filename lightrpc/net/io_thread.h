#ifndef LIGHTRPC_NET_IO_THREAD_H
#define LIGHTRPC_NET_IO_THREAD_H

#include <pthread.h>
#include <semaphore.h>
#include "eventloop.h"

namespace lightrpc {

class IOThread {
 public:
  IOThread();
  
  ~IOThread();

  EventLoop* GetEventLoop();

  void Start();

  void Join();

 public:
  static void* Main(void* arg);

 private:
  pid_t m_thread_id_ {-1};    // 线程号
  pthread_t m_thread_ {0};    // 线程句柄

  EventLoop* m_event_loop_ {NULL}; // 当前 io 线程的 loop 对象

  // 信号量
  sem_t m_init_semaphore_;
  sem_t m_start_semaphore_;
};

}

#endif