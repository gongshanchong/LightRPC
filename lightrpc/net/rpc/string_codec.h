#ifndef LIGHTRPC_NET_STRING_CODER_H
#define LIGHTRPC_NET_STRING_CODER_H

#include "abstract_codec.h"
#include "abstract_protocol.h"

namespace lightrpc {

class StringProtocol : public AbstractProtocol {
 public:
  std::string info_;
};

class StringCodec : public AbstractCodec {
  // 将 message 对象转化为字节流，写入到 buffer
  void Encode(std::vector<AbstractProtocol::s_ptr>& messages, TcpBuffer::s_ptr out_buffer) {
    for (size_t i = 0; i < messages.size(); ++i) {
      std::shared_ptr<StringProtocol> msg = std::dynamic_pointer_cast<StringProtocol>(messages[i]);
      out_buffer->WriteToBuffer(msg->info_.c_str(), msg->info_.length());
    }
  }

  // 将 buffer 里面的字节流转换为 message 对象
  void Decode(std::vector<AbstractProtocol::s_ptr>& out_messages, TcpBuffer::s_ptr buffer) {
    std::vector<char> re;
    buffer->ReadFromBuffer(re, buffer->ReadAble());
    std::string info;
    for (size_t i = 0; i < re.size(); ++i) {
      info += re[i];
    }

    std::shared_ptr<StringProtocol> msg = std::make_shared<StringProtocol>();
    msg->info_ = info;
    msg->m_msg_id_ = "123456";
    out_messages.push_back(msg);
  }
};
}
#endif