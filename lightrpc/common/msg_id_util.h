#ifndef LIGHTRPC_COMMON_MSGID_UTIL_H
#define LIGHTRPC_COMMON_MSGID_UTIL_H

#include <string>


namespace lightrpc {

class MsgIDUtil {
 public:
  static std::string GenMsgID();

};
}
#endif