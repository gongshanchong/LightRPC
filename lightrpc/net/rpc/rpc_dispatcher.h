#ifndef LIGHTRPC_NET_RPC_RPC_DISPATCHER_H
#define LIGHTRPC_NET_RPC_RPC_DISPATCHER_H

#include <map>
#include <memory>
#include <google/protobuf/service.h>
#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

#include "../tinypb/tinypb_protocol.h"
#include "../../common/log.h"
#include "../../common/error_code.h"
#include "rpc_controller.h"
#include "rpc_closure.h"
#include "../tcp/net_addr.h"
#include "../../common/run_time.h"
#include "abstract_protocol.h"
#include "../tinypb/tinypb_protocol.h"
#include "../http/http_servlet.h"

namespace lightrpc {

class TcpConnection;
// 事件分发模块，把数据解码完成的数据包交给对应协议的事件分发器来处理
class RpcDispatcher {
 public:
  static RpcDispatcher* GetRpcDispatcher();

 public:
  typedef std::shared_ptr<google::protobuf::Service> service_s_ptr;
  // 事件分发
  void Dispatch(AbstractProtocol::s_ptr request, AbstractProtocol::s_ptr response, TcpConnection* connection);
  // 将这个服务注册到了服务器中
  void RegisterService(service_s_ptr service);
  void RegisterServlet(const std::string& path, HttpServlet::ptr servlet);

 private:
  // 调用TINYPB协议的服务
  void CallTinyPBService(AbstractProtocol::s_ptr request, AbstractProtocol::s_ptr response, TcpConnection* connection);
  // 调用HTTP协议的服务
  void CallHttpServlet(AbstractProtocol::s_ptr request, AbstractProtocol::s_ptr response, TcpConnection* connection);
  // 解析 service_full_name，得到 service_name 和 method_name
  bool ParseServiceFullName(const std::string& full_name, std::string& service_name, std::string& method_name);

  // 回复客户端的回调函数
  void Reply(AbstractProtocol::s_ptr response, TcpConnection* connection);

 private:
  // 服务名-服务
  std::map<std::string, service_s_ptr> m_service_map_;
  std::map<std::string, HttpServlet::ptr> m_servlets_map_;
};
}
#endif