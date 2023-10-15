#ifndef LIGHTRPC_COMMON_RUN_TIME_H
#define LIGHTRPC_COMMON_RUN_TIME_H

#include <string>

namespace lightrpc {
class RpcInterface;
class RunTime {
 public:
  RpcInterface* getRpcInterface();

 public:
  static RunTime* GetRunTime();

 public:
  // 当前线程处理的请求的 msgid
  std::string m_msgid;
  std::string m_method_name;
  RpcInterface* m_rpc_interface {NULL};
};
}
#endif