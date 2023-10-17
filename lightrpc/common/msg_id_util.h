#ifndef LIGHTRPC_COMMON_MSGID_UTIL_H
#define LIGHTRPC_COMMON_MSGID_UTIL_H

#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "log.h"

namespace lightrpc {

class MsgIDUtil {
 public:
  static std::string GenMsgID();

};
}
#endif