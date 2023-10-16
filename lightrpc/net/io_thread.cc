#include <pthread.h>
#include <assert.h>
#include "io_thread.h"
#include "../common/log.h"
#include "../common/util.h"

namespace lightrpc {

IOThread::IOThread() {  
  // 初始化信号量
  int rt = sem_init(&m_init_semaphore_, 0, 0);
  assert(rt == 0);
  rt = sem_init(&m_start_semaphore_, 0, 0);
  assert(rt == 0);

  // 创建线程并绑定函数
  pthread_create(&m_thread_, NULL, &IOThread::Main, this);

  // wait, 直到新线程执行完 Main 函数的前置
  // 防止线程创建时函数已结束导致参数捕获失败
  sem_wait(&m_init_semaphore_);

  LOG_DEBUG("IOThread [%d] create success", m_thread_id_);
}
  
IOThread::~IOThread() {
  m_event_loop_->Stop();
  sem_destroy(&m_init_semaphore_);
  sem_destroy(&m_start_semaphore_);

  pthread_join(m_thread_, NULL);

  if (m_event_loop_) {
    delete m_event_loop_;
    m_event_loop_ = NULL;
  }
}

void* IOThread::Main(void* arg) {
  IOThread* thread = static_cast<IOThread*> (arg);

  thread->m_event_loop_ = new EventLoop();
  thread->m_thread_id_ = GetThreadId();

  // 唤醒等待的主线程
  sem_post(&thread->m_init_semaphore_);

  // 让 IO 线程等待，直到我们主动的启动
  LOG_DEBUG("IOThread %d created, wait start semaphore", thread->m_thread_id_);
  sem_wait(&thread->m_start_semaphore_);
 
  // 启动 IO 线程
  LOG_DEBUG("IOThread %d start loop ", thread->m_thread_id_);
  thread->m_event_loop_->Loop();
  LOG_DEBUG("IOThread %d end loop ", thread->m_thread_id_);

  return NULL;
}

EventLoop* IOThread::GetEventLoop() {
  return m_event_loop_;
}

void IOThread::Start() {
  LOG_DEBUG("Now invoke IOThread %d", m_thread_id_);
  sem_post(&m_start_semaphore_);
}

void IOThread::Join() {
  pthread_join(m_thread_, NULL);
}
}