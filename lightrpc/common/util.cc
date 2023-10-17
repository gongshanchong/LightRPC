#include "util.h"

namespace lightrpc {

// thread_local 关键字修饰的变量具有线程（thread）周期，这些变量在线程开始的时候被生成，
// 在线程结束的时候被销毁，并且每一个线程都拥有一个独立的变量实例。
static int g_pid = 0;
static thread_local int t_thread_id = 0;

pid_t GetPid() {
  if (g_pid != 0) {
    return g_pid;
  }
  return getpid();
}

pid_t GetThreadId() {
  if (t_thread_id != 0) {
    return t_thread_id;
  }
  return syscall(SYS_gettid);
}


int64_t GetNowMs() {
  timeval val;
  gettimeofday(&val, NULL);

  return val.tv_sec * 1000 + val.tv_usec / 1000;
}


int32_t GetInt32FromNetByte(const char* buf) {
  int32_t re;
  memcpy(&re, buf, sizeof(re));
  return ntohl(re);
}

}