#ifndef LIGHTRPC_NET_WAKEUP_FDEVENT_H
#define LIGHTRPC_NET_WAKEUP_FDEVENT_H

#include "fd_event.h"

namespace lightrpc {

class WakeUpFdEvent : public FdEvent {
 public:
  WakeUpFdEvent(int fd);
  void Wakeup();
};
}

#endif