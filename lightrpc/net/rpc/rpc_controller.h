#ifndef LIGHTRPC_NET_RPC_RPC_CONTROLLER_H
#define LIGHTRPC_NET_RPC_RPC_CONTROLLER_H

#include <google/protobuf/service.h>
#include <google/protobuf/stubs/callback.h>
#include <string>

#include "../tcp/net_addr.h"
#include "../../common/log.h"
#include "abstract_protocol.h"
#include "../http/http_protocol.h"

namespace lightrpc {

// 主要目的是提供一种特定于 RPC 实现的设置并找出 RPC 级错误的方法。
class RpcController : public google::protobuf::RpcController {
 public:
  RpcController() { LOG_INFO("RpcController"); } 
  ~RpcController() { LOG_INFO("~RpcController"); } 

  // Client-side methods ---------------------------------------------
  void Reset();

  bool Failed() const;

  std::string ErrorText() const;

  void StartCancel();
  // -----------------------------------------------------------------

  // Server-side methods ---------------------------------------------
  void SetFailed(const std::string& reason);

  bool IsCanceled() const;

  void NotifyOnCancel(google::protobuf::Closure* callback);
  // -----------------------------------------------------------------

  void SetError(int32_t error_code, const std::string error_info);

  int32_t GetErrorCode();

  std::string GetErrorInfo();

  void SetMsgId(const std::string& msg_id);

  std::string GetMsgId();

  void SetLocalAddr(NetAddr::s_ptr addr);

  void SetPeerAddr(NetAddr::s_ptr addr);

  NetAddr::s_ptr GetLocalAddr();

  NetAddr::s_ptr GetPeerAddr();

  void SetTimeout(int timeout);

  int GetTimeout();

  ProtocalType GetProtocol();

  void SetProtocol(ProtocalType protocol);

  HttpMethod GetCallMethod();

  void SetCallMethod(HttpMethod call_method);

  bool Finished();

  void SetFinished(bool value);
 
 private:
  int32_t m_error_code_ {0};   // RPC 框架级错误码
  std::string m_error_info_;
  std::string m_msg_id_;

  bool m_is_failed_ {false};
  bool m_is_cancled_ {false};
  bool m_is_finished_ {false};

  NetAddr::s_ptr m_local_addr_;
  NetAddr::s_ptr m_peer_addr_;

  ProtocalType protocol_;    // 协议类型
  HttpMethod call_method_;   // http请求类型类型
  int m_timeout_ {1000};     // ms
};
}
#endif