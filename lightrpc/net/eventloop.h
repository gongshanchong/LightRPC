#ifndef LIGHTRPC_NET_EVENTLOOP_H
#define LIGHTRPC_NET_EVENTLOOP_H

#include <pthread.h>
#include <set>
#include <functional>
#include <queue>
#include <mutex>
#include "fd_event.h"
#include "wakeup_fd_event.h"
#include "timer.h"

namespace lightrpc {
class EventLoop {
 public:
  EventLoop();

  ~EventLoop();

  void Loop();

  void Wakeup();

  void Stop();

  void AddEpollEvent(FdEvent* event);

  void DeleteEpollEvent(FdEvent* event);

  bool IsInLoopThread();

  void AddTask(std::function<void()> cb, bool is_wake_up = false);

  void AddTimerEvent(TimerEvent::s_ptr event);

  bool IsLooping();

 public:
  static EventLoop* GetCurrentEventLoop();

 private:
  void InitWakeUpFdEevent();

  void InitTimer();

 private:
  // 线程id
  pid_t m_thread_id_ {0};

  // epoll_fd
  int m_epoll_fd_ {0};

  // 唤醒事件的fd
  int m_wakeup_fd_ {0};
  // 这里wakeup的作用仅仅是触发可读事件唤醒epoll来停止loop循环
  WakeUpFdEvent::s_ptr m_wakeup_fd_event_ {NULL};

  // 是否停止
  bool m_stop_flag_ {false};

  // 监听事件描述符集合
  std::set<int> m_listen_fds_;

  // 任务队列，回调函数集合
  std::queue<std::function<void()>> m_pending_tasks_;
  
  std::mutex handle_mtx_;

  // 时间事件管理器
  Timer::s_ptr m_timer_ {NULL};

  // 是否在loop中
  bool m_is_looping_ {false};
};

}
#endif