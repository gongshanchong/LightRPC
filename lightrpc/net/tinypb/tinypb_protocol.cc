#include "tinypb_protocol.h"

namespace lightrpc {

char TinyPBProtocol::PB_START_ = 0x02;
char TinyPBProtocol::PB_END_ = 0x03;

void TinyPBProtocol::SetTinyPBError(int32_t err_code, const std::string err_info) {
  m_err_code_ = err_code;
  m_err_info_ = err_info;
  m_err_info_len_ = err_info.length();
}

}