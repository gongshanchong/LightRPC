#ifndef LIGHTRPC_COMMON_RUN_TIME_H
#define LIGHTRPC_COMMON_RUN_TIME_H

#include <string>

namespace lightrpc {
class RunTime {
 public:
  static RunTime* GetRunTime();

 public:
  // 当前线程处理的请求的 msgid
  std::string m_msgid_;
  std::string m_method_name_;
};
}
#endif