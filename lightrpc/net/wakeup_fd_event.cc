#include <unistd.h>
#include "wakeup_fd_event.h"
#include "../common/log.h"


namespace lightrpc {

WakeUpFdEvent::WakeUpFdEvent(int fd) : FdEvent(fd) {
}

void WakeUpFdEvent::Wakeup() {
  char buf[8] = {'a'};
  int rt = write(m_fd_, buf, 8);
  if (rt != 8) {
    LOG_ERROR("write to wakeup fd less than 8 bytes, fd[%d]", m_fd_);
  }
  LOG_DEBUG("success read 8 bytes");
}
}