#ifndef LIGHTRPC_NET_CODER_TINYPB_CODER_H
#define LIGHTRPC_NET_CODER_TINYPB_CODER_H

#include "../rpc/abstract_coder.h"
#include "tinypb_protocol.h"

namespace lightrpc {
// 编解码模块，一些字符串的解析操作
class TinyPBCoder : public AbstractCoder {
 public:

  TinyPBCoder() {}
  ~TinyPBCoder() {}

  // 将 message 对象转化为字节流，写入到 buffer
  void Encode(std::vector<AbstractProtocol::s_ptr>& messages, TcpBuffer::s_ptr out_buffer);

  // 将 buffer 里面的字节流转换为 message 对象
  void Decode(std::vector<AbstractProtocol::s_ptr>& out_messages, TcpBuffer::s_ptr buffer);

  // 获取协议类型
  ProtocalType GetProtocalType();

 private:
  const char* EncodeTinyPB(std::shared_ptr<TinyPBProtocol> message, int& len);

};
}
#endif