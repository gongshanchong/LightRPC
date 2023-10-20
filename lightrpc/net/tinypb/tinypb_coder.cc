#include "tinypb_coder.h"
#include "../../common/log.h"
#include "../../common/util.h"
#include "tinypb_protocol.h"
#include <arpa/inet.h>
#include <string.h>
#include <vector>

namespace lightrpc {

// 将 message 对象转化为字节流，写入到 buffer
void TinyPBCoder::Encode(std::vector<AbstractProtocol::s_ptr> &messages,
                         TcpBuffer::s_ptr out_buffer) {
  for (auto &i : messages) {
    std::shared_ptr<TinyPBProtocol> msg =
        std::dynamic_pointer_cast<TinyPBProtocol>(i);
    int len = 0;
    const char *buf = EncodeTinyPB(msg, len);
    if (buf != NULL && len != 0) {
      out_buffer->WriteToBuffer(buf, len);
    }
    if (buf) {
      free((void *)buf);
      buf = NULL;
    }
  }
}

// 将 buffer 里面的字节流转换为 message 对象
void TinyPBCoder::Decode(std::vector<AbstractProtocol::s_ptr> &out_messages,
                         TcpBuffer::s_ptr buffer) {
  while (1) {
    // 遍历 buffer，找到
    // PB_START，找到之后，解析出整包的长度。然后得到结束符的位置，判断是否为
    // PB_END
    std::vector<char> tmp = buffer->m_buffer;
    int start_index = buffer->ReadIndex();
    int end_index = -1;

    int pk_len = 0;
    bool parse_success_ = false;
    int i = 0;
    for (i = start_index; i < buffer->WriteIndex(); ++i) {
      if (tmp[i] == TinyPBProtocol::PB_START_) {
        // 读下去四个字节。由于是网络字节序，需要转为主机字节序
        if (i + 1 < buffer->WriteIndex()) {
          pk_len = GetInt32FromNetByte(&tmp[i + 1]);
          LOG_DEBUG("get pk_len = %d", pk_len);

          // 结束符的索引
          int j = i + pk_len - 1;
          if (j >= buffer->WriteIndex()) {
            continue;
          }
          if (tmp[j] == TinyPBProtocol::PB_END_) {
            start_index = i;
            end_index = j;
            parse_success_ = true;
            break;
          }
        }
      }
    }

    if (i >= buffer->WriteIndex()) {
      LOG_DEBUG("decode end, read all buffer data");
      return;
    }
    // 开始对buffer里面的内容进行解析
    if (parse_success_) {
      buffer->MoveReadIndex(end_index - start_index + 1);
      std::shared_ptr<TinyPBProtocol> message =
          std::make_shared<TinyPBProtocol>();
      message->m_pk_len_ = pk_len;
      // 获取msg_id_len在buffer中的索引，并获取msg_id_len
      int msg_id_len_index =
          start_index + sizeof(char) + sizeof(message->m_pk_len_);
      if (msg_id_len_index >= end_index) {
        message->parse_success_ = false;
        LOG_ERROR("parse error, msg_id_len_index[%d] >= end_index[%d]",
                  msg_id_len_index, end_index);
        continue;
      }
      message->m_msg_id_len_ = GetInt32FromNetByte(&tmp[msg_id_len_index]);
      LOG_DEBUG("parse msg_id_len=%d", message->m_msg_id_len_);
      // 获取msg_id在buffer中的索引，并获取msg_id
      int msg_id_index = msg_id_len_index + sizeof(message->m_msg_id_len_);
      char msg_id[100] = {0};
      memcpy(&msg_id[0], &tmp[msg_id_index], message->m_msg_id_len_);
      message->m_msg_id_ = std::string(msg_id);
      LOG_DEBUG("parse msg_id=%s", message->m_msg_id_.c_str());
      // 获取method_name_len在buffer中的索引，并获取method_name_len
      int method_name_len_index = msg_id_index + message->m_msg_id_len_;
      if (method_name_len_index >= end_index) {
        message->parse_success_ = false;
        LOG_ERROR("parse error, method_name_len_index[%d] >= end_index[%d]",
                  method_name_len_index, end_index);
        continue;
      }
      message->m_method_name_len_ =
          GetInt32FromNetByte(&tmp[method_name_len_index]);
      // 获取method_name在buffer中的索引，并获取method_name
      int method_name_index =
          method_name_len_index + sizeof(message->m_method_name_len_);
      char method_name[512] = {0};
      memcpy(&method_name[0], &tmp[method_name_index],
             message->m_method_name_len_);
      message->m_method_name_ = std::string(method_name);
      LOG_DEBUG("parse method_name=%s", message->m_method_name_.c_str());
      // 获取err_code在buffer中的索引，并获取err_code
      int err_code_index = method_name_index + message->m_method_name_len_;
      if (err_code_index >= end_index) {
        message->parse_success_ = false;
        LOG_ERROR("parse error, err_code_index[%d] >= end_index[%d]",
                  err_code_index, end_index);
        continue;
      }
      message->m_err_code_ = GetInt32FromNetByte(&tmp[err_code_index]);
      // 获取error_info_len在buffer中的索引，并获取error_info_len
      int error_info_len_index = err_code_index + sizeof(message->m_err_code_);
      if (error_info_len_index >= end_index) {
        message->parse_success_ = false;
        LOG_ERROR("parse error, error_info_len_index[%d] >= end_index[%d]",
                  error_info_len_index, end_index);
        continue;
      }
      message->m_err_info_len_ =
          GetInt32FromNetByte(&tmp[error_info_len_index]);
      // 获取err_info在buffer中的索引，并获取err_info
      int err_info_index =
          error_info_len_index + sizeof(message->m_err_info_len_);
      char error_info[512] = {0};
      memcpy(&error_info[0], &tmp[err_info_index], message->m_err_info_len_);
      message->m_err_info_ = std::string(error_info);
      LOG_DEBUG("parse error_info=%s", message->m_err_info_.c_str());
      // 获取pb_data_len，以及pd_data在buffer中的索引，并获取pd_data
      int pb_data_len = message->m_pk_len_ - message->m_method_name_len_ -
                        message->m_msg_id_len_ - message->m_err_info_len_ - 2 -
                        24;
      int pd_data_index = err_info_index + message->m_err_info_len_;
      message->m_pb_data_ = std::string(&tmp[pd_data_index], pb_data_len);

      // 这里校验和去解析
      message->parse_success_ = true;

      out_messages.push_back(message);
    }
  }
}

// 获取协议类型
ProtocalType TinyPBCoder::GetProtocalType() { return TinyPb_Protocal; }

const char *TinyPBCoder::EncodeTinyPB(std::shared_ptr<TinyPBProtocol> message,
                                      int &len) {
  if (message->m_msg_id_.empty()) {
    message->m_msg_id_ = "123456789";
  }
  LOG_DEBUG("msg_id = %s", message->m_msg_id_.c_str());
  int pk_len = 2 + 24 + message->m_msg_id_.length() +
               message->m_method_name_.length() +
               message->m_err_info_.length() + message->m_pb_data_.length();
  LOG_DEBUG("pk_len = %d", pk_len);

  char *buf = reinterpret_cast<char *>(malloc(pk_len));
  char *tmp = buf;
  // 报文开始符
  *tmp = TinyPBProtocol::PB_START_;
  tmp++;
  // 整个报文长度：单位为byte，且包括 [strat] 字符 和 [end] 字符。
  int32_t pk_len_net = htonl(pk_len);
  memcpy(tmp, &pk_len_net, sizeof(pk_len_net));
  tmp += sizeof(pk_len_net);
  // 报文id长度
  int msg_id_len = message->m_msg_id_.length();
  int32_t msg_id_len_net = htonl(msg_id_len);
  memcpy(tmp, &msg_id_len_net, sizeof(msg_id_len_net));
  tmp += sizeof(msg_id_len_net);
  // 报文id
  if (!message->m_msg_id_.empty()) {
    memcpy(tmp, &(message->m_msg_id_[0]), msg_id_len);
    tmp += msg_id_len;
  }
  // 方法名长度
  int method_name_len = message->m_method_name_.length();
  int32_t method_name_len_net = htonl(method_name_len);
  memcpy(tmp, &method_name_len_net, sizeof(method_name_len_net));
  tmp += sizeof(method_name_len_net);
  // 方法名
  if (!message->m_method_name_.empty()) {
    memcpy(tmp, &(message->m_method_name_[0]), method_name_len);
    tmp += method_name_len;
  }
  // 框架级错误代码
  int32_t err_code_net = htonl(message->m_err_code_);
  memcpy(tmp, &err_code_net, sizeof(err_code_net));
  tmp += sizeof(err_code_net);
  // err_info 长度
  int err_info_len = message->m_err_info_.length();
  int32_t err_info_len_net = htonl(err_info_len);
  memcpy(tmp, &err_info_len_net, sizeof(err_info_len_net));
  tmp += sizeof(err_info_len_net);
  // err_info 内容
  if (!message->m_err_info_.empty()) {
    memcpy(tmp, &(message->m_err_info_[0]), err_info_len);
    tmp += err_info_len;
  }
  // 业务 protobuf 数据，由 google 的 protobuf 序列化后得到
  if (!message->m_pb_data_.empty()) {
    memcpy(tmp, &(message->m_pb_data_[0]), message->m_pb_data_.length());
    tmp += message->m_pb_data_.length();
  }
  // 包检验和，用于检验包数据是否有损坏
  int32_t check_sum_net = htonl(1);
  memcpy(tmp, &check_sum_net, sizeof(check_sum_net));
  tmp += sizeof(check_sum_net);
  // 报文结束符
  *tmp = TinyPBProtocol::PB_END_;
  // 设置message结构体中的长度信息
  // TinyPb 协议里面所有的 int 类型的字段在编码时都会先转为网络字节序！
  message->m_pk_len_ = pk_len;
  message->m_msg_id_len_ = msg_id_len;
  message->m_method_name_len_ = method_name_len;
  message->m_err_info_len_ = err_info_len;
  message->parse_success_ = true;
  len = pk_len;

  LOG_DEBUG("encode message[%s] success", message->m_msg_id_.c_str());

  return buf;
}
} // namespace lightrpc