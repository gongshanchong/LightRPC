#ifndef LIGHTRPC_NET_WAKEUP_FDEVENT_H
#define LIGHTRPC_NET_WAKEUP_FDEVENT_H

#include "fd_event.h"

namespace lightrpc {

class WakeUpFdEvent : public FdEvent {
 public:
  // 智能指针
  typedef std::shared_ptr<WakeUpFdEvent> s_ptr;
  WakeUpFdEvent(int fd);
  void Wakeup();
};
}

#endif