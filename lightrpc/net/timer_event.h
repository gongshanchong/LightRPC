#ifndef LIGHTRPC_NET_TIMEREVENT
#define LIGHTRPC_NET_TIMEREVENT

#include <functional>
#include <memory>

namespace lightrpc {

class TimerEvent {
 public:
  typedef std::shared_ptr<TimerEvent> s_ptr;
  // 时间事件初始化
  TimerEvent(int interval, bool is_repeated, int fd, std::function<void(int fd)> cb);

  int64_t GetArriveTime() const {
    return m_arrive_time_;
  }

  int GetFd() {
    return m_fd_;
  }

  void SetCancled(bool value) {
    m_is_cancled_ = value;
  }

  bool IsCancled() {
    return m_is_cancled_;
  }

  bool IsRepeated() {
    return m_is_repeated_;
  }

  std::function<void(int fd)> GetCallBack() {
    return m_task_;
  }

  // 更新时间事件截止时间
  void ResetArriveTime();

 private:
  int64_t m_arrive_time_;    // ms，截止时间
  int64_t m_interval_;       // ms，时间间隔
  bool m_is_repeated_ {false};   // 是否重复通知
  bool m_is_cancled_ {false};    // 是否关闭

  int m_fd_ {0};                 // 对应链接的fd
  std::function<void(int fd)> m_task_; // 时间事件的处理函数
};
}
#endif