#include "rpc_dispatcher.h"
#include "../tcp/tcp_connection.h"

namespace lightrpc {

#define DELETE_RESOURCE(XX) \
  if (XX != NULL) { \
    delete XX;      \
    XX = NULL;      \
  }                 \

// 单例模式的饿汉模式
static RpcDispatcher* g_rpc_dispatcher = NULL;
RpcDispatcher* RpcDispatcher::GetRpcDispatcher() {
  if (g_rpc_dispatcher != NULL) {
    return g_rpc_dispatcher;
  }
  g_rpc_dispatcher = new RpcDispatcher;
  return g_rpc_dispatcher;
}


void RpcDispatcher::Dispatch(AbstractProtocol::s_ptr request, AbstractProtocol::s_ptr response, TcpConnection* connection) {
  // 构造请求、响应信息
  std::shared_ptr<TinyPBProtocol> req_protocol = std::dynamic_pointer_cast<TinyPBProtocol>(request);
  std::shared_ptr<TinyPBProtocol> rsp_protocol = std::dynamic_pointer_cast<TinyPBProtocol>(response);

  std::string method_full_name = req_protocol->m_method_name_;
  std::string service_name;
  std::string method_name;

  rsp_protocol->m_msg_id_ = req_protocol->m_msg_id_;
  rsp_protocol->m_method_name_ = req_protocol->m_method_name_;
  // 解析完整的 rpc 方法名
  if (!ParseServiceFullName(method_full_name, service_name, method_name)) {
    // 解析失败，返回对应错误码, 不进行其他的处理
    SetTinyPBError(rsp_protocol, ERROR_PARSE_SERVICE_NAME, "parse service name error");
    Reply(rsp_protocol, connection);
    return;
  }
  // 找到对应的 service
  auto it = m_service_map_.find(service_name);
  if (it == m_service_map_.end()) {
    LOG_ERROR("%s | sericve neame[%s] not found", req_protocol->m_msg_id_.c_str(), service_name.c_str());
    SetTinyPBError(rsp_protocol, ERROR_SERVICE_NOT_FOUND, "service not found");
    Reply(rsp_protocol, connection);
    return;
  }
  // 找到对应的 method
  service_s_ptr service = (*it).second;
  const google::protobuf::MethodDescriptor* method = service->GetDescriptor()->FindMethodByName(method_name);
  if (method == NULL) {
    LOG_ERROR("%s | method neame[%s] not found in service[%s]", req_protocol->m_msg_id_.c_str(), method_name.c_str(), service_name.c_str());
    SetTinyPBError(rsp_protocol, ERROR_SERVICE_NOT_FOUND, "method not found");
    Reply(rsp_protocol, connection);
    return;
  }
  // 根据 method 对象反射出 request 和 response 对象
  // 获取请求信息
  // 反序列化，将 pb_data 反序列化为 req_msg
  google::protobuf::Message* req_msg = service->GetRequestPrototype(method).New();
  if (!req_msg->ParseFromString(req_protocol->m_pb_data_)) {
    LOG_ERROR("%s | deserilize error", req_protocol->m_msg_id_.c_str(), method_name.c_str(), service_name.c_str());
    SetTinyPBError(rsp_protocol, ERROR_FAILED_DESERIALIZE, "deserilize error");
    Reply(rsp_protocol, connection);
    DELETE_RESOURCE(req_msg);
    return;
  }
  LOG_INFO("%s | get rpc request[%s]", req_protocol->m_msg_id_.c_str(), req_msg->ShortDebugString().c_str());
  // 获取响应信息
  // 设置相关信息，辅助对象
  google::protobuf::Message* rsp_msg = service->GetResponsePrototype(method).New();
  RpcController* rpc_controller = new RpcController();
  rpc_controller->SetLocalAddr(connection->GetLocalAddr());
  rpc_controller->SetPeerAddr(connection->GetPeerAddr());
  rpc_controller->SetMsgId(req_protocol->m_msg_id_);
  RunTime::GetRunTime()->m_msgid_ = req_protocol->m_msg_id_;
  RunTime::GetRunTime()->m_method_name_ = method_name;
  // closure 就会把 response 对象再序列化，最终生成一个 TinyPBProtocol 的结构体，最后通过 TcpConnection::reply 函数，将数据再发送给客户端
  RpcClosure* closure = new RpcClosure([req_msg, rsp_msg, req_protocol, rsp_protocol, connection, rpc_controller, this]() mutable {
    if (!rsp_msg->SerializeToString(&(rsp_protocol->m_pb_data_))) {
      LOG_ERROR("%s | serilize error, origin message [%s]", req_protocol->m_msg_id_.c_str(), rsp_msg->ShortDebugString().c_str());
      SetTinyPBError(rsp_protocol, ERROR_FAILED_SERIALIZE, "serilize error");
    } 
    else if (rpc_controller->GetErrorCode() != 0){
      LOG_ERROR("%s | run error [%s]", req_protocol->m_msg_id_.c_str(), rpc_controller->GetErrorInfo());
      SetTinyPBError(rsp_protocol, rpc_controller->GetErrorCode(), rpc_controller->GetErrorInfo());
    }
    else {
      rsp_protocol->m_err_code_ = 0;
      rsp_protocol->m_err_info_ = "OK";
      LOG_INFO("%s | dispatch success, requesut[%s], response[%s]", req_protocol->m_msg_id_.c_str(), req_msg->ShortDebugString().c_str(), rsp_msg->ShortDebugString().c_str());
    }

    this->Reply(rsp_protocol, connection);
  });
  // 调用业务处理方法，本质上就是输入一个 request 对象，然后获得一个 response 对象
  service->CallMethod(method, rpc_controller, req_msg, rsp_msg, closure);
}

bool RpcDispatcher::ParseServiceFullName(const std::string& full_name, std::string& service_name, std::string& method_name) {
  if (full_name.empty()) {
    LOG_ERROR("full name empty"); 
    return false;
  }
  size_t i = full_name.find_first_of(".");
  if (i == full_name.npos) {
    LOG_ERROR("not find . in full name [%s]", full_name.c_str());
    return false;
  }
  service_name = full_name.substr(0, i);
  method_name = full_name.substr(i + 1, full_name.length() - i - 1);

  LOG_INFO("parse sericve_name[%s] and method_name[%s] from full name [%s]", service_name.c_str(), method_name.c_str(),full_name.c_str());

  return true;
}

void RpcDispatcher::Reply(AbstractProtocol::s_ptr response, TcpConnection* connection){
  std::vector<AbstractProtocol::s_ptr> replay_messages;
  replay_messages.emplace_back(response);
  connection->Reply(replay_messages);
}

void RpcDispatcher::RegisterService(service_s_ptr service) {
  std::string service_name = service->GetDescriptor()->full_name();
  m_service_map_[service_name] = service;

}

void RpcDispatcher::SetTinyPBError(std::shared_ptr<TinyPBProtocol> msg, int32_t err_code, const std::string err_info) {
  msg->m_err_code_ = err_code;
  msg->m_err_info_ = err_info;
  msg->m_err_info_len_ = err_info.length();
}
}