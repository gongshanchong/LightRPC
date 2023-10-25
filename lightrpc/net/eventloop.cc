#include <memory>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <string.h>
#include "eventloop.h"
#include "../common/log.h"
#include "../common/util.h"

// 向epoll添加监听事件
#define ADD_TO_EPOLL() \
    auto it = m_listen_fds_.find(event->GetFd()); \
    int op = EPOLL_CTL_ADD; \
    std::string op_str = "add";\
    if (it != m_listen_fds_.end()) { \
      op = EPOLL_CTL_MOD; \
      op_str = "update";\
    } \
    epoll_event tmp = event->GetEpollEvent(); \
    LOG_INFO("epoll_event.events = %d", (int)tmp.events); \
    int rt = epoll_ctl(m_epoll_fd_, op, event->GetFd(), &tmp); \
    if (rt == -1) { \
      LOG_ERROR("failed epoll_ctl when add fd, errno=%d, error=%s", errno, strerror(errno)); \
    } \
    m_listen_fds_.insert(event->GetFd()); \
    LOG_DEBUG("%s event success, fd[%d]", op_str.c_str(), event->GetFd()) \

// 向epoll添删除监听事件
#define DELETE_TO_EPOLL() \
    auto it = m_listen_fds_.find(event->GetFd()); \
    if (it == m_listen_fds_.end()) { \
      return; \
    } \
    int op = EPOLL_CTL_DEL; \
    epoll_event tmp = event->GetEpollEvent(); \
    int rt = epoll_ctl(m_epoll_fd_, op, event->GetFd(), NULL); \
    if (rt == -1) { \
      LOG_ERROR("failed epoll_ctl when add fd, errno=%d, error=%s", errno, strerror(errno)); \
    } \
    m_listen_fds_.erase(event->GetFd()); \
    LOG_DEBUG("delete event success, fd[%d]", event->GetFd()); \

namespace lightrpc {
// 单例模式的饿汉模式，线程变量
static thread_local EventLoop* t_current_eventloop = NULL;
static int g_epoll_max_timeout = 10000;
static int g_epoll_max_events = 10;

// 创建epoll
EventLoop::EventLoop() {
  if (t_current_eventloop != NULL) {
    LOG_ERROR("failed to create event loop, this thread has created event loop");
    exit(0);
  }
  m_thread_id_ = GetThreadId();
  m_epoll_fd_ = epoll_create(10);

  if (m_epoll_fd_ == -1) {
    LOG_ERROR("failed to create event loop, epoll_create error, error info[%d]", errno);
    exit(0);
  }

  InitWakeUpFdEevent();     // 初始化唤醒事件
  InitTimer();              // 初始化时间事件管理器

  LOG_INFO("succ create event loop in thread %d", m_thread_id_);
  t_current_eventloop = this;
}

EventLoop::~EventLoop() {
  close(m_epoll_fd_);
}

// 初始化时间事件管理器
void EventLoop::InitTimer() {
  m_timer_ = std::make_shared<Timer>();
  AddEpollEvent(m_timer_.get());
}

// 向时间事件管理器添加时间事件
void EventLoop::AddTimerEvent(TimerEvent::s_ptr event) {
  m_timer_->AddTimerEvent(event);
}

void EventLoop::InitWakeUpFdEevent() {
  // 事件 fd 类型。顾名思义，就是专门用于事件通知的文件描述符（ fd ）
  m_wakeup_fd_ = eventfd(0, EFD_NONBLOCK);
  if (m_wakeup_fd_ < 0) {
    LOG_ERROR("failed to create event loop, eventfd create error, error info[%d]", errno);
    exit(0);
  }
  LOG_INFO("wakeup fd = %d", m_wakeup_fd_);
  // 为事件fd设置event类
  m_wakeup_fd_event_ = std::make_shared<WakeUpFdEvent>(m_wakeup_fd_);
  m_wakeup_fd_event_->Listen(FdEvent::IN_EVENT, [this]() {
    char buf[8];
    while(read(m_wakeup_fd_, buf, 8) != -1 && errno != EAGAIN) {
    }
    LOG_DEBUG("read full bytes from wakeup fd[%d]", m_wakeup_fd_);
  });

  AddEpollEvent(m_wakeup_fd_event_.get());
}

void EventLoop::Loop() {
  m_is_looping_ = true;
  while(!m_stop_flag_) {
    std::unique_lock<std::mutex> lock(handle_mtx_); 
    // 获取监听返回的事件
    std::queue<std::function<void()>> tmp_tasks; 
    m_pending_tasks_.swap(tmp_tasks); 
    lock.unlock();
    // 执行监听返回的事件
    while (!tmp_tasks.empty()) {
      std::function<void()> cb = tmp_tasks.front();
      tmp_tasks.pop();
      if (cb) {
        cb();
      }
    }
    // 判断一个定时任务需要执行？ （now() > TimerEvent.arrtive_time）
    // 对epoll_fd进行监听
    int timeout = g_epoll_max_timeout; 
    epoll_event result_events[g_epoll_max_events];
    // epoll监听返回的事件并进行处理
    // 此处会被阻塞
    int rt = epoll_wait(m_epoll_fd_, result_events, g_epoll_max_events, timeout);
    if (rt < 0) {
      // 如果进程在一个慢系统调用(slow system call)中阻塞时，当捕获到某个信号且相应信号处理函数返回时，这个系统调用不再阻塞而是被中断，就会调用返回错误（一般为-1）&&设置errno为EINTR
      if(errno == EINTR){ continue; }
      LOG_ERROR("epoll_wait error, errno=%d, error=%s", errno, strerror(errno));
    } else {
      // 便利返回的事件并进行处理
      for (int i = 0; i < rt; ++i) {
        epoll_event trigger_event = result_events[i];
        FdEvent* fd_event = static_cast<FdEvent*>(trigger_event.data.ptr);
        if (fd_event == NULL) {
          LOG_ERROR("fd_event = NULL, continue");
          continue;
        }
        // 依据trigger_event中的响应事件进行相应的处理
        if (trigger_event.events & EPOLLIN) { 
          AddTask(fd_event->Handler(FdEvent::IN_EVENT));
        }
        if (trigger_event.events & EPOLLOUT) { 
          AddTask(fd_event->Handler(FdEvent::OUT_EVENT));
        }
        // EPOLLHUP EPOLLERR
        if (trigger_event.events & EPOLLERR) {
          LOG_DEBUG("fd %d trigger EPOLLERROR event", fd_event->GetFd());
          // 删除出错的套接字
          DeleteEpollEvent(fd_event);
          if (fd_event->Handler(FdEvent::ERROR_EVENT) != nullptr) {
            LOG_DEBUG("fd %d add error callback", fd_event->GetFd())
            AddTask(fd_event->Handler(FdEvent::ERROR_EVENT));
          }
        }
      }
    }
  }
}

void EventLoop::Wakeup() {
  LOG_INFO("WAKE UP");
  // 进行唤醒，执行任务队列中的任务
  m_wakeup_fd_event_->Wakeup();
}

void EventLoop::Stop() {
  m_stop_flag_ = true;
  Wakeup();
}

void EventLoop::AddEpollEvent(FdEvent* event) {
  // 保证线程安全
  if (IsInLoopThread()) {
    ADD_TO_EPOLL();
  } else {
    auto cb = [this, event]() {
      ADD_TO_EPOLL();
    };
    AddTask(cb, true);
  }
}

void EventLoop::DeleteEpollEvent(FdEvent* event) {
  // 保证线程安全
  // 即事件循环不属于当前线程，就需要尽量将对这个对象的操作移到它所属的那个线程执行，避免回调函数被析构产生错误
  if (IsInLoopThread()) {
    DELETE_TO_EPOLL();
  } else {
    auto cb = [this, event]() {
      DELETE_TO_EPOLL();
    };
    AddTask(cb, true);
  }
}

void EventLoop::AddTask(std::function<void()> cb, bool is_wake_up /*=false*/) {
  std::unique_lock<std::mutex> lock(handle_mtx_); 
  m_pending_tasks_.push(cb);
  lock.unlock();

  // 假设 mainReactor 向某个 subReactor 添加 fd 的时候，subReactor正在 epoll_wait 沉睡，就会造成处理这个clientfd 延迟。 
  // 为了解决这个延迟，mainReactor 在添加clientfd给 subReactor 时，需要某种机制，把 subReactor 从 epoll_wait 唤醒，这个唤醒动作就叫 wakeup
  if (is_wake_up) {
    Wakeup();
  }
}

bool EventLoop::IsInLoopThread() {
  return GetThreadId() == m_thread_id_;
}

EventLoop* EventLoop::GetCurrentEventLoop() {
  if (t_current_eventloop) {
    return t_current_eventloop;
  }
  t_current_eventloop = new EventLoop();
  return t_current_eventloop;
}

bool EventLoop::IsLooping() {
  return m_is_looping_;
}
}