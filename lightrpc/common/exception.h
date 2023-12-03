#ifndef LIGHTRPC_COMMON_EXCEPTION_H
#define LIGHTRPC_COMMON_EXCEPTION_H

#include <exception>
#include <string>

namespace lightrpc {

class LightrpcException : public std::exception {
 public:

  LightrpcException(int error_code, const std::string& error_info) : m_error_code_(error_code), m_error_info_(error_info) {}

  // 异常处理
  // 当捕获到 LightrpcException 及其子类对象的异常时，会执行该函数
  virtual void handle() = 0;

  virtual ~LightrpcException() {};

  int errorCode() {
    return m_error_code_;
  }

  std::string errorInfo() {
    return m_error_info_;
  }

 protected:
  int m_error_code_ {0};
  std::string m_error_info_;
};
}
#endif