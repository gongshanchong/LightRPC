#ifndef LIGHTRPC_NET_RPC_RPC_CHANNEL_H
#define LIGHTRPC_NET_RPC_RPC_CHANNEL_H

#include <google/protobuf/service.h>
#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <memory>

#include "rpc_controller.h"
#include "../tinypb/tinypb_protocol.h"
#include "../tcp/tcp_client.h"
#include "../../common/log.h"
#include "../../common/msg_id_util.h"
#include "../../common/error_code.h"
#include "../../common/run_time.h"
#include "../../net/timer_event.h"
#include "../../net/tcp/net_addr.h"
#include "../../net/tcp/tcp_client.h"
#include "../../net/timer_event.h"

namespace lightrpc {
#define NEWMESSAGE(type, var_name) \
  std::shared_ptr<type> var_name = std::make_shared<type>(); \

#define NEWRPCCONTROLLER(var_name) \
  std::shared_ptr<lightrpc::RpcController> var_name = std::make_shared<lightrpc::RpcController>(); \

#define NEWRPCCHANNEL(addr, var_name) \
  std::shared_ptr<lightrpc::RpcChannel> var_name = std::make_shared<lightrpc::RpcChannel>(lightrpc::RpcChannel::FindAddr(addr)); \

// RpcChannel 代表了 RPC 的通信。通常情况下，不应该直接调用 RpcChannel，而是构造一个包装它的 stub Service。
class RpcChannel : public google::protobuf::RpcChannel, public std::enable_shared_from_this<RpcChannel> {
 public:
  typedef std::shared_ptr<RpcChannel> s_ptr;
  typedef google::protobuf::RpcController* controller_ptr;
  typedef google::protobuf::Message* message_ptr;
  typedef google::protobuf::Closure* closure_ptr;

 public:
  // 获取 addr
  // 若 str 是 ip:port, 直接返回
  // 否则认为是 rpc 服务名，尝试从配置文件里面获取对应的 ip:port（后期会加上服务发现）
  static NetAddr::s_ptr FindAddr(const std::string& str);

 public:
  RpcChannel(NetAddr::s_ptr peer_addr);

  ~RpcChannel();

  void CallMethod(const google::protobuf::MethodDescriptor* method,
                          google::protobuf::RpcController* controller, const google::protobuf::Message* request,
                          google::protobuf::Message* response, google::protobuf::Closure* done);

  TcpClient* GetTcpClient();

 private:
  void Init(google::protobuf::RpcController* controller, const google::protobuf::Message* request,
                          google::protobuf::Message* response, google::protobuf::Closure* done);
  void CallBack();

 private:
  NetAddr::s_ptr m_peer_addr_ {nullptr};
  TcpClient::s_ptr m_client_ {nullptr};

  controller_ptr m_controller_ {nullptr};
  const google::protobuf::Message* m_request_ {nullptr};
  message_ptr m_response_ {nullptr};
  closure_ptr m_closure_ {nullptr};
};
}
#endif