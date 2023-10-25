#ifndef LIGHTRPC_NET_FDEVENT_H
#define LIGHTRPC_NET_FDEVENT_H

#include <functional>
#include <memory>
#include <sys/epoll.h>

namespace lightrpc {
class FdEvent {
 public:
  // 智能指针
  typedef std::shared_ptr<FdEvent> s_ptr;
  // 触发事件
  enum TriggerEvent {
    IN_EVENT = EPOLLIN,
    OUT_EVENT = EPOLLOUT,
    ERROR_EVENT = EPOLLERR,
  };
  // 初始化
  FdEvent(int fd);
  FdEvent();
  ~FdEvent();

  // 设置非阻塞
  void SetNonBlock();

  // 获取事件的响应函数
  std::function<void()> Handler(TriggerEvent event_type);

  // 为事件设置反应函数
  void Listen(TriggerEvent event_type, std::function<void()> callback, std::function<void()> error_callback = nullptr);

  // 取消监听
  void Cancle(TriggerEvent event_type);

  // 获取文件描述符
  int GetFd() const {
    return m_fd_;
  }

  // 获取监听的事件
  epoll_event GetEpollEvent() {
    return m_listen_events_;
  }

 protected:
  int m_fd_ {-1};
  // 监听事件
  epoll_event m_listen_events_;

  // 事件响应函数
  std::function<void()> m_read_callback_ {nullptr};
  std::function<void()> m_write_callback_ {nullptr};
  std::function<void()> m_error_callback_ {nullptr};
};
}

#endif