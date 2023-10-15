#ifndef LIGHTRPC_COMMON_UTIL_H
#define LIGHTRPC_COMMON_UTIL_H

#include <sys/types.h>
#include <unistd.h>

namespace lightrpc {

// 获取进程id、线程id
pid_t GetPid();
pid_t GetThreadId();
// 获取当前时间
int64_t GetNowMs();
int32_t GetInt32FromNetByte(const char* buf);

}

#endif