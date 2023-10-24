#include <sys/timerfd.h>
#include <string.h>
#include "timer.h"
#include "../common/log.h"
#include "../common/util.h"

namespace lightrpc {

Timer::Timer() : FdEvent() {
  // timerfd使用内核提供的异步I/O机制，可以将定时器超时事件通知给应用程序，而不需要应用程序不断地轮询
  // timerfd 常用来做定时器的使用，设置超时时间之后，每隔一段时间 timerfd 就是可读的。
  m_fd_ = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  LOG_DEBUG("timer fd=%d", m_fd_);

  // 当定时器超时，read读事件发生即可读
  // 为时间的 fd 可读事件设置反应函数
  // 返回超时次数（从上次调用timerfd_settime()启动开始或上次read成功读取开始），它是一个8字节的unit64_t类型整数，如果定时器没有发生超时事件，则read将阻塞；
  // 若timerfd为阻塞模式，否则返回EAGAIN 错误（O_NONBLOCK模式）；如果read时提供的缓冲区小于8字节将以EINVAL错误返回。
  Listen(FdEvent::IN_EVENT, std::bind(&Timer::OnTimer, this));
}

void Timer::OnTimer() {
  // 处理缓冲区数据，防止下一次继续触发可读事件
  // 读取触发timerfd返回的数据
  char buf[8];
  while(1) {
    if ((read(m_fd_, buf, 8) == -1) && errno == EAGAIN) {
      break;
    }
  }

  // 执行时间事件任务
  // 获取当前时间
  int64_t now = GetNowMs();
  // 获取超时时间事件
  std::vector<TimerEvent::s_ptr> tmps;
  std::vector<std::pair<int64_t, std::function<void(int fd)>>> tasks;
  // 加锁保证安全访问公共资源
  std::unique_lock<std::mutex> lock(handle_mtx_);
  // 获取超时时间事件及时间事件的方法
  auto it = m_pending_events_.begin();
  for (it = m_pending_events_.begin(); it != m_pending_events_.end(); ++it) {
    if ((*it).first <= now) {
      if (!(*it).second->IsCancled()) {
        tmps.push_back((*it).second);
        tasks.push_back(std::make_pair((*it).second->GetFd(), (*it).second->GetCallBack()));
      }
    } else {
      break;
    }
  }
  // 在当前队列中去除超时事件
  m_pending_events_.erase(m_pending_events_.begin(), it);
  lock.unlock();

  // 需要把重复的TimerEvent再次添加进去
  for (auto i = tmps.begin(); i != tmps.end(); ++i) {
    if ((*i)->IsRepeated()) {
      // 调整 arriveTime
      (*i)->ResetArriveTime();
      AddTimerEvent(*i);
    }
  }

  // 重置timerfd触发时间
  ResetArriveTime();

  // 执行时间事件的回调函数
  for (auto i: tasks) {
    if (i.second) {
      i.second(i.first);
    }
  }

}

void Timer::ResetArriveTime() {
  std::unique_lock<std::mutex> lock(handle_mtx_);
  auto tmp = m_pending_events_;
  lock.unlock();

  if (tmp.size() == 0) {
    return;
  }

  int64_t now = GetNowMs();
  auto it = tmp.begin();
  int64_t inteval = 0;
  if (it->second->GetArriveTime() > now) {
    inteval = it->second->GetArriveTime() - now;
  } else {
    inteval = 100;
  }
  timespec ts;
  memset(&ts, 0, sizeof(ts));
  ts.tv_sec = inteval / 1000;
  ts.tv_nsec = (inteval % 1000) * 1000000;
  // 在结构体 itimerspec 的 it_value 字段标识定时器超时时间，it_interval 标识之后的超时间隔。
  itimerspec value;
  memset(&value, 0, sizeof(value));
  value.it_value = ts;
  
  // 启动或关闭 timerfd 对应的定时器
  // 是启停 timerfd 超时的，用来设置超时的时间，间隔的。
  int rt = timerfd_settime(m_fd_, 0, &value, NULL);
  if (rt != 0) {
    LOG_ERROR("timerfd_settime error, errno=%d, error=%s", errno, strerror(errno));
  }
}

void Timer::AddTimerEvent(TimerEvent::s_ptr event) {
  // 是否重置触发时间
  bool is_reset_timerfd = false;

  std::unique_lock<std::mutex> lock(handle_mtx_);
  if (m_pending_events_.empty()) {
    is_reset_timerfd = true;
  } else {
    auto it = m_pending_events_.begin();
    // 如果新加入的时间事件的超时时间小于当前最小的，则需要重置m_fd的触发时间
    if ((*it).second->GetArriveTime() > event->GetArriveTime()) {
      is_reset_timerfd = true;
    }
  }
  m_pending_events_.emplace(event->GetArriveTime(), event);
  lock.unlock();

  // 重置m_fd的触发时间
  if (is_reset_timerfd) {
    ResetArriveTime();
  }
}

void Timer::DeleteTimerEvent(TimerEvent::s_ptr event) {
  event->SetCancled(true);

  // 二分查找进行时间事件的删除
  std::unique_lock<std::mutex> lock(handle_mtx_);
  auto begin = m_pending_events_.lower_bound(event->GetArriveTime());
  auto end = m_pending_events_.upper_bound(event->GetArriveTime());
  auto it = begin;
  for (it = begin; it != end; ++it) {
    if (it->second == event) {
      break;
    }
  }
  if (it != end) {
    m_pending_events_.erase(it);
  }
  lock.unlock();

  LOG_DEBUG("success delete TimerEvent at arrive time %lld", event->GetArriveTime());
}
}