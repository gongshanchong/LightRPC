#ifndef LIGHTRPC_NET_RPC_ABSTRACT_CODER_H
#define LIGHTRPC_NET_RPC_ABSTRACT_CODER_H

#include <vector>
#include "../tcp/tcp_buffer.h"
#include "abstract_protocol.h"

namespace lightrpc {

class AbstractCoder {
 public:
  // 将 message 对象转化为字节流，写入到 buffer
  virtual void Encode(std::vector<AbstractProtocol::s_ptr>& messages, TcpBuffer::s_ptr out_buffer) = 0;

  // 将 buffer 里面的字节流转换为 message 对象
  virtual void Decode(std::vector<AbstractProtocol::s_ptr>& out_messages, TcpBuffer::s_ptr buffer) = 0;

  virtual ~AbstractCoder() {}
};
}
#endif