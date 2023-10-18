#include <string.h>
#include <fcntl.h>
#include "fd_event.h"
#include "../common/log.h"

namespace lightrpc {

FdEvent::FdEvent(int fd) : m_fd_(fd) {
  memset(&m_listen_events_, 0, sizeof(m_listen_events_));
}

FdEvent::FdEvent() {
  memset(&m_listen_events_, 0, sizeof(m_listen_events_));
}

std::function<void()> FdEvent::Handler(TriggerEvent event) {
  if (event == TriggerEvent::IN_EVENT) {
    return m_read_callback_;
  } else if (event == TriggerEvent::OUT_EVENT) {
    return m_write_callback_;
  } else if (event == TriggerEvent::ERROR_EVENT) {
    return m_error_callback_;
  }
  return nullptr;
}

void FdEvent::Listen(TriggerEvent event_type, std::function<void()> callback, std::function<void()> error_callback /*= nullptr*/) {
  if (event_type == TriggerEvent::IN_EVENT) {
    m_listen_events_.events |= EPOLLIN;
    m_read_callback_ = callback;
  } else {
    m_listen_events_.events |= EPOLLOUT;
    m_write_callback_ = callback;
  }
  m_error_callback_ = error_callback;
  // 利用event结构体中的void指针指向FdEvent来进行事件的处理
  m_listen_events_.data.ptr = this;
}

void FdEvent::Cancle(TriggerEvent event_type) {
  if (event_type == TriggerEvent::IN_EVENT) {
    m_listen_events_.events &= (~EPOLLIN);
    LOG_DEBUG("cancle in_event success, fd[%d]", m_fd_);
  } else {
    m_listen_events_.events &= (~EPOLLOUT);
    LOG_DEBUG("cancle out_event success, fd[%d]", m_fd_);
  }
}

void FdEvent::SetNonBlock() {
  int flag = fcntl(m_fd_, F_GETFL, 0);
  if (flag & O_NONBLOCK) {
    return;
  }

  fcntl(m_fd_, F_SETFL, flag | O_NONBLOCK);
}
}