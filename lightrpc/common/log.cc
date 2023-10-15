#include <sys/time.h>
#include <sstream>
#include <stdio.h>

#include <assert.h>
#include <signal.h>
#include <unistd.h>
#include "log.h"
#include "util.h"
#include "config.h"
#include "../net/eventloop.h"
#include "run_time.h"


namespace lightrpc {

//  单例模式的饿汉模式
static Logger* g_logger = NULL;

// 段错误的处理，异步写线程写入日志
void CoredumpHandler(int signal_no) {
    LOG_ERROR("progress received invalid signal, will exit");
    g_logger->Flush();
    pthread_join(g_logger->GetAsyncLopger()->m_thread_, NULL);
    pthread_join(g_logger->GetAsyncAppLopger()->m_thread_, NULL);
    // SIG_DF默认信号处理 ，即恢复信号本来处理的过程
    signal(signal_no, SIG_DFL);
    raise(signal_no);
}

Logger* Logger::GetGlobalLogger() {
    return g_logger;
}

// 日志初始化
Logger::Logger(LogLevel level, int type /*=1*/) : m_set_level_(level), m_type_(type) {
    if (m_type_ == 0) {
        return;
    }
    // 分别初始化框架和业务的日志异步写
    m_asnyc_logger_ = std::make_shared<AsyncLogger>(
        Config::GetGlobalConfig()->m_log_file_name_ + "_rpc",
        Config::GetGlobalConfig()->m_log_file_path_,
        Config::GetGlobalConfig()->m_log_max_file_size_);
    
    m_asnyc_app_logger_ = std::make_shared<AsyncLogger>(
        Config::GetGlobalConfig()->m_log_file_name_ + "_app",
        Config::GetGlobalConfig()->m_log_file_path_,
        Config::GetGlobalConfig()->m_log_max_file_size_);
}

// 刷新
void Logger::Flush() {
    // 同步消息到异步写缓冲区
    SyncLoop();
    // 刷新缓冲区
    m_asnyc_logger_->Stop();
    m_asnyc_logger_->Flush();
    m_asnyc_app_logger_->Stop();
    m_asnyc_app_logger_->Flush();
}

// 日志系统开始运行
// 定时写
void Logger::Init() {
    if (m_type_ == 0) {
        return;
    }
    // 为日志生成时间事件并监听
    m_timer_event_ = std::make_shared<TimerEvent>(Config::GetGlobalConfig()->m_log_sync_inteval_, true, std::bind(&Logger::SyncLoop, this));
    EventLoop::GetCurrentEventLoop()->AddTimerEvent(m_timer_event_);
    // 为日志绑定信号的处理函数，便于可以将终止时的错误写入日志
    signal(SIGSEGV, CoredumpHandler);
    signal(SIGABRT, CoredumpHandler);
    signal(SIGTERM, CoredumpHandler);
    signal(SIGKILL, CoredumpHandler);
    signal(SIGINT, CoredumpHandler);
    signal(SIGSTKFLT, CoredumpHandler);
}

// 同步消息到异步写缓冲区
void Logger::SyncLoop() {
    // 同步 m_buffer 到 async_logger 的buffer队尾
    std::vector<std::string> tmp_vec;
    std::unique_lock<std::mutex> lock(m_mutex_);
    tmp_vec.swap(m_buffer_);
    lock.unlock();

    if (!tmp_vec.empty()) {
        m_asnyc_logger_->PushLogBuffer(tmp_vec);
    }
    tmp_vec.clear();

    // 同步 m_app_buffer 到 app_async_logger 的buffer队尾
    std::vector<std::string> tmp_vec2;
    std::unique_lock<std::mutex> lock_app(m_app_mutex_);
    tmp_vec2.swap(m_app_buffer_);
    lock_app.unlock();

    if (!tmp_vec2.empty()) {
        m_asnyc_app_logger_->PushLogBuffer(tmp_vec2);
    }
}

// 初始化静态全局日志变量
void Logger::InitGlobalLogger(int type /*=1*/) {
    LogLevel global_log_level = StringToLogLevel(Config::GetGlobalConfig()->m_log_level_);
    printf("Init log level [%s]\n", LogLevelToString(global_log_level).c_str());
    g_logger = new Logger(global_log_level, type);
    g_logger->Init();
}


std::string LogLevelToString(LogLevel level) {
    switch (level) {
    case Debug:
        return "DEBUG";
    case Info:
        return "INFO";
    case Error:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

LogLevel StringToLogLevel(const std::string& log_level) {
    if (log_level == "DEBUG") {
        return Debug;
    } else if (log_level == "INFO") {
        return Info;
    } else if (log_level == "ERROR") {
        return Error;
    } else {
        return Unknown;
    }
}

std::string LogEvent::ToString() {
    // 获取时间
    struct timeval now_time;
    gettimeofday(&now_time, nullptr);
    struct tm now_time_t;
    localtime_r(&(now_time.tv_sec), &now_time_t);
    // 将时间转换为字符串格式
    char buf[128];
    strftime(&buf[0], 128, "%y-%m-%d %H:%M:%S", &now_time_t);
    std::string time_str(buf);
    int ms = now_time.tv_usec / 1000;
    time_str = time_str + "." + std::to_string(ms);
    
    // 获取进程id和线程id
    m_pid_ = GetPid();
    m_thread_id_ = GetThreadId();

    std::stringstream ss;

    ss << "[" << LogLevelToString(m_level_) << "]\t"
      << "[" << time_str << "]\t"
      << "[" << m_pid_ << ":" << m_thread_id_ << "]\t";

    // 获取当前线程处理的请求的 msgid
    std::string msgid = RunTime::GetRunTime()->m_msgid;
    std::string method_name = RunTime::GetRunTime()->m_method_name;
    if (!msgid.empty()) {
      ss << "[" << msgid << "]\t";
    }

    if (!method_name.empty()) {
      ss << "[" << method_name << "]\t";
    }
    return ss.str();
}

void Logger::PushLog(const std::string& msg) {
    if (m_type_ == 0) {
      printf("%s", (msg + "\n").c_str());
      return;
    }
    std::unique_lock<std::mutex> lock(m_mutex_);
    m_buffer_.push_back(msg);
    lock.unlock();
}

void Logger::PushAppLog(const std::string& msg) {
    std::unique_lock<std::mutex> lock(m_app_mutex_);
    m_app_buffer_.push_back(msg);
    lock.unlock();
}

// 异步写初始化
AsyncLogger::AsyncLogger(const std::string& file_name, const std::string& file_path, int max_size) 
  : m_file_name_(file_name), m_file_path_(file_path), m_max_file_size_(max_size) {
  
    // 创建线程
    assert(pthread_create(&m_thread_, NULL, &AsyncLogger::Loop, this) == 0);
}

// 生产者-消费者（条件变量实现）
void* AsyncLogger::Loop(void* arg) {
  // 将 buffer 里面的全部数据打印到文件中，然后线程睡眠，直到有新的数据再重复这个过程
  AsyncLogger* logger = reinterpret_cast<AsyncLogger*>(arg); 

  while(1) {
    // 加互斥锁保证读取消息队列安全
    std::unique_lock<std::mutex> lock(logger->handle_mtx_);
    // 等待条件变量，线程被唤醒后，先重新判断pred的值。如果pred为false，则会释放mutex并重新阻塞然后休眠在wait
    // 当pred为false时，wait才会阻塞当前线程，为true才会解除阻塞
    // 由于写入文件io比较慢，在写文件的时候也可能收到通知，所以从消息队列尽可能多的读取消息(!logQueue.empty())
    logger->condition_variable_.wait(lock, [&](){ return !logger->m_buffer_.empty();});
  
    std::vector<std::string> tmp;
    tmp.swap(logger->m_buffer_.front());
    logger->m_buffer_.pop();
    lock.unlock();
    // 日志文件判断
    timeval now;
    gettimeofday(&now, NULL);

    struct tm now_time;
    localtime_r(&(now.tv_sec), &now_time);
    
    const char* format = "%Y%m%d";
    char date[32];
    strftime(date, sizeof(date), format, &now_time);

    // 判断是否需要重新生成一个日志文件
    if (std::string(date) != logger->m_date_) {
      logger->m_no_ = 0;
      logger->m_reopen_flag_ = true;
      logger->m_date_ = std::string(date);
    }
    if (logger->m_file_hanlder_ == NULL) {
      logger->m_reopen_flag_ = true;
    }

    std::stringstream ss;
    ss << logger->m_file_path_ << logger->m_file_name_ << "_"
      << std::string(date) << "_log.";
    std::string log_file_name = ss.str() + std::to_string(logger->m_no_);

    if (logger->m_reopen_flag_) {
      if (logger->m_file_hanlder_) {
        fclose(logger->m_file_hanlder_);
      }
      logger->m_file_hanlder_ = fopen(log_file_name.c_str(), "a");
      logger->m_reopen_flag_ = false;
    }
    // 日志文件写满了，重新生成一个
    if (ftell(logger->m_file_hanlder_) > logger->m_max_file_size_) {
      fclose(logger->m_file_hanlder_);

      log_file_name = ss.str() + std::to_string(logger->m_no_++);
      logger->m_file_hanlder_ = fopen(log_file_name.c_str(), "a");
      logger->m_reopen_flag_ = false;

    }
    // 将数据写入日志文件
    for (auto& i : tmp) {
      if (!i.empty()) {
        fwrite(i.c_str(), 1, i.length(), logger->m_file_hanlder_);
      }
    }
    fflush(logger->m_file_hanlder_);

    if (logger->m_stop_flag_) {
      return NULL;
    }
  }

  return NULL;
}


void AsyncLogger::Stop() {
  m_stop_flag_ = true;
}

void AsyncLogger::Flush() {
  if (m_file_hanlder_) {
    fflush(m_file_hanlder_);
  }
}

void AsyncLogger::PushLogBuffer(std::vector<std::string>& vec) {
  // 这时候需要唤醒异步日志线程
  std::unique_lock<std::mutex> lock(handle_mtx_);
  m_buffer_.push(vec);
  // 通知消费者
  condition_variable_.notify_all();
}

}