#ifndef LIGHTRPC_NET_TIMER_H
#define LIGHTRPC_NET_TIMER_H

#include <map>
#include <mutex>
#include "fd_event.h"
#include "timer_event.h"

namespace lightrpc {
// 时间事件管理器
class Timer : public FdEvent {
 public:

  Timer();

  void AddTimerEvent(TimerEvent::s_ptr event);

  void DeleteTimerEvent(TimerEvent::s_ptr event);

  // timerfd的handler
  void OnTimer(); // 当发送了 IO 事件后，eventloop 会执行这个回调函数

 private:
  void ResetArriveTime();   // 重置timerfd触发时间

 private:
  // 定时器事件，以时间作为下标
  // multimap是关联式容器，它按特定的次序（按照key来比较）存储由键key和值value组合而成的元素,多个键值对之间的key可以重复
  std::multimap<int64_t, TimerEvent::s_ptr> m_pending_events_;
  std::mutex handle_mtx_;
};

}

#endif