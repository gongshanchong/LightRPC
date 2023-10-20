#ifndef LIGHTRPC_NET_CODER_TINYPB_PROTOCOL_H
#define LIGHTRPC_NET_CODER_TINYPB_PROTOCOL_H 

#include <string>
#include "../rpc/abstract_protocol.h"

namespace lightrpc {
// TinyPB 协议是基于 protobuf 的一种自定义协议，主要是加了一些必要的字段如 错误码、RPC 方法名、起始结束标志位等。
// TinyPb 协议里面所有的 int 类型的字段在编码时都会先转为网络字节序！
struct TinyPBProtocol : public AbstractProtocol {
 public:
  TinyPBProtocol(){}
  ~TinyPBProtocol() {}

 public:
  static char PB_START_;             // 报文的开始
  static char PB_END_;               // 报文的结束

 public:
  int32_t m_pk_len_ {0};             // 整个报文长度，单位为byte，且包括 [strat] 字符 和 [end] 字符。
  int32_t m_msg_id_len_ {0};         // 请求号的长度
  // msg_id 继承父类

  int32_t m_method_name_len_ {0};    // 方法名字长度
  std::string m_method_name_;        // 方法名
  int32_t m_err_code_ {0};           // 框架级错误代码. 0 代表调用正常，非 0 代表调用失败，代表调用 RPC 过程中发生的错误，如对端关闭、调用超时等。
  int32_t m_err_info_len_ {0};       // err_info 长度
  std::string m_err_info_;           // 详细错误信息， err_code 非0时会设置该字段值
  std::string m_pb_data_;            // 业务 protobuf 数据，由 google 的 protobuf 序列化后得到
  int32_t m_check_sum_ {0};          // 包检验和，用于检验包数据是否有损坏

  bool parse_success_ {false};       // 解析是否成功
};
}
#endif