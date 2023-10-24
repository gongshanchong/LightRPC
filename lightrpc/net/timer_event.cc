#include "timer_event.h"
#include "../common/log.h"
#include "../common/util.h"

namespace lightrpc {

TimerEvent::TimerEvent(int interval, bool is_repeated, std::function<void(int fd)> cb, int fd)
   : m_interval_(interval), m_is_repeated_(is_repeated), m_task_(cb), m_fd_(fd) {
  ResetArriveTime();
}

void TimerEvent::ResetArriveTime() {
  m_arrive_time_ = GetNowMs() + m_interval_;
}

}