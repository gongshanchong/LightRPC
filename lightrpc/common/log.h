#ifndef LIGHTRPC_COMMON_LOG_H
#define LIGHTRPC_COMMON_LOG_H

#include <sys/time.h>
#include <sstream>
#include <stdio.h>
#include <assert.h>
#include <signal.h>
#include <unistd.h>
#include <string>
#include <queue>
#include <memory>
#include <mutex>
#include <vector>
#include <semaphore.h>
#include <condition_variable>

#include "util.h"
#include "run_time.h"
#include "config.h"
#include "../net/eventloop.h"
#include "../net/timer_event.h"

namespace lightrpc {

// 模板，可变参数，字符串格式化
template<typename... Args>
std::string FormatString(const char* str, Args&&... args) {
    // 格式化字符串
    int size = snprintf(nullptr, 0, str, args...);

    std::string result;
    if (size > 0) {
        result.resize(size);
        snprintf(&result[0], size + 1, str, args...);
    }

    return result;
}

// 定义宏
// 将日志消息添加到队列
#define LOG_DEBUG(str, ...) \
    if (lightrpc::Logger::GetGlobalLogger()->GetLogLevel() && lightrpc::Logger::GetGlobalLogger()->GetLogLevel() <= lightrpc::Debug) \
    { \
        lightrpc::Logger::GetGlobalLogger()->PushLog(lightrpc::LogEvent(lightrpc::LogLevel::Debug).ToString() \
        + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" + lightrpc::FormatString(str, ##__VA_ARGS__) + "\n");\
    } \

#define LOG_INFO(str, ...) \
    if (lightrpc::Logger::GetGlobalLogger()->GetLogLevel() <= lightrpc::Info) \
    { \
        lightrpc::Logger::GetGlobalLogger()->PushLog(lightrpc::LogEvent(lightrpc::LogLevel::Info).ToString() \
        + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" + lightrpc::FormatString(str, ##__VA_ARGS__) + "\n");\
    } \

#define LOG_ERROR(str, ...) \
    if (lightrpc::Logger::GetGlobalLogger()->GetLogLevel() <= lightrpc::Error) \
    { \
        lightrpc::Logger::GetGlobalLogger()->PushLog(lightrpc::LogEvent(lightrpc::LogLevel::Error).ToString() \
        + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" + lightrpc::FormatString(str, ##__VA_ARGS__) + "\n");\
    } \

#define LOG_APPDEBUG(str, ...) \
    if (lightrpc::Logger::GetGlobalLogger()->GetLogLevel() <= lightrpc::Debug) \
    { \
        lightrpc::Logger::GetGlobalLogger()->PushAppLog(lightrpc::LogEvent(lightrpc::LogLevel::Debug).ToString() \
        + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" + lightrpc::FormatString(str, ##__VA_ARGS__) + "\n");\
    } \

#define LOG_APPINFO(str, ...) \
    if (lightrpc::Logger::GetGlobalLogger()->GetLogLevel() <= lightrpc::Info) \
    { \
        lightrpc::Logger::GetGlobalLogger()->PushAppLog(lightrpc::LogEvent(lightrpc::LogLevel::Info).ToString() \
        + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" + lightrpc::FormatString(str, ##__VA_ARGS__) + "\n");\
    } \

#define LOG_APPERROR(str, ...) \
    if (lightrpc::Logger::GetGlobalLogger()->GetLogLevel() <= lightrpc::Error) \
    { \
        lightrpc::Logger::GetGlobalLogger()->PushAppLog(lightrpc::LogEvent(lightrpc::LogLevel::Error).ToString() \
        + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" + lightrpc::FormatString(str, ##__VA_ARGS__) + "\n");\
    } \

// 日志类型
enum LogLevel {
    Unknown = 0,
    Debug = 1,
    Info = 2,
    Error = 3
};

// 日志类型与字符串的转换
std::string LogLevelToString(LogLevel level);
LogLevel StringToLogLevel(const std::string& log_level);

// 日志异步写
class AsyncLogger {
public:
    typedef std::shared_ptr<AsyncLogger> s_ptr;
    AsyncLogger(const std::string& file_name, const std::string& file_path, int max_size);

    void Stop();

    // 刷新到磁盘
    void Flush();
    
    // 添加到消息队列并唤醒异步日志线程
    void PushLogBuffer(std::vector<std::string>& vec);

public:
    // 生产者-消费者（条件变量实现）
    static void* Loop(void*);

public:
    pthread_t m_thread_;

private:
    std::queue<std::vector<std::string>> m_buffer_;    // 消息队列

    std::string m_file_name_;    // 日志输出文件名字
    std::string m_file_path_;    // 日志输出路径
    int m_max_file_size_ {0};    // 日志单个文件最大大小, 单位为字节

    sem_t m_sempahore_;           // 信号量
    std::mutex handle_mtx_;       // 互斥锁
    std::condition_variable condition_variable_;    // 条件变量
 
    std::string m_date_;             // 当前打印日志的文件日期
    FILE* m_file_hanlder_ {NULL};    // 当前打开的日志文件句柄

    bool m_reopen_flag_ {false};     // 是否重新生成一个日志文件

    int m_no_ {0};                   // 日志文件序号

    bool m_stop_flag_ {false};       // 是否停止
};

// 日志管理器
class Logger {
public:
    typedef std::shared_ptr<Logger> s_ptr;

    Logger(LogLevel level, int type = 1);

    void PushLog(const std::string& msg);

    void PushAppLog(const std::string& msg);

    void Init();

    LogLevel GetLogLevel() const {
        return m_set_level_;
    }

    AsyncLogger::s_ptr GetAsyncAppLopger() {
        return m_asnyc_app_logger_;
    }

    AsyncLogger::s_ptr GetAsyncLopger() {
        return m_asnyc_logger_;
    }

    void SyncLoop();

    void Flush();

public:
    static Logger* GetGlobalLogger();

    static void InitGlobalLogger(int type = 1);

private:
    LogLevel m_set_level_;
    // 日志信息
    std::vector<std::string> m_buffer_;
    std::vector<std::string> m_app_buffer_;
    // 互斥锁,也成互斥量,可以保护关键代码段,以确保独占式访问
    std::mutex m_mutex_;                       
    std::mutex m_app_mutex_;

    // m_file_path/m_file_name_yyyymmdd.1
    std::string m_file_name_;    // 日志输出文件名字
    std::string m_file_path_;    // 日志输出路径
    int m_max_file_size_ {0};    // 日志单个文件最大大小

    AsyncLogger::s_ptr m_asnyc_logger_;      // rpc框架日志
    AsyncLogger::s_ptr m_asnyc_app_logger_;  // 业务逻辑日志
    TimerEvent::s_ptr m_timer_event_;        // 日志产生的时间事件

    int m_type_ {0};
};

// 日志事件（进程号、线程号、时间、文件名、行号）
class LogEvent {
public:
    LogEvent(LogLevel level) : m_level_(level) {}

    std::string GetFileName() const {
        return m_file_name_;  
    }

    LogLevel GetLogLevel() const {
        return m_level_;
    }

    std::string ToString();

private:
    std::string m_file_name_;       // 文件名
    int32_t m_file_line_;           // 行号
    int32_t m_pid_;                 // 进程号
    int32_t m_thread_id_;           // 线程号

    LogLevel m_level_;              //日志级别
};
}
#endif