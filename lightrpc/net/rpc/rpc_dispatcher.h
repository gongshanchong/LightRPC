#ifndef LIGHTRPC_NET_RPC_RPC_DISPATCHER_H
#define LIGHTRPC_NET_RPC_RPC_DISPATCHER_H

#include <map>
#include <memory>
#include <google/protobuf/service.h>

#include "../tinypb/abstract_protocol.h"
#include "../tinypb/tinypb_protocol.h"

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
  // 将这个类注册到了服务器中
  void RegisterService(service_s_ptr service);
  // 对响应信息设置错误码
  void SetTinyPBError(std::shared_ptr<TinyPBProtocol> msg, int32_t err_code, const std::string err_info);

 private:
  // 解析 service_full_name，得到 service_name 和 method_name
  bool ParseServiceFullName(const std::string& full_name, std::string& service_name, std::string& method_name);

 private:
  // 服务名-服务
  std::map<std::string, service_s_ptr> m_service_map_;
};
}
#endif