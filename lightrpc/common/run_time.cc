#include "run_time.h"

namespace lightrpc {
// 单例模式的饿汉模式，线程变量
thread_local RunTime* t_run_time = NULL; 

RunTime* RunTime::GetRunTime() {
  if (t_run_time) {
    return t_run_time;
  }
  t_run_time = new RunTime();
  return t_run_time;
}

RpcInterface* RunTime::GetRpcInterface() {
  return m_rpc_interface_;
}
}