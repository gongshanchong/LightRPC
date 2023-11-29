#include "rpc_dispatcher.h"
#include "../tcp/tcp_connection.h"
#include "abstract_protocol.h"
#include <memory>

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

void RpcDispatcher::CallTinyPBService(AbstractProtocol::s_ptr request, TcpConnection* connection){
  // 构造请求、响应信息
  std::shared_ptr<TinyPBProtocol> req_protocol = std::dynamic_pointer_cast<TinyPBProtocol>(request);
  std::shared_ptr<TinyPBProtocol> rsp_protocol = std::make_shared<TinyPBProtocol>();
  std::string method_full_name = req_protocol->m_method_name_;
  std::string service_name;
  std::string method_name;

  rsp_protocol->m_msg_id_ = req_protocol->m_msg_id_;
  rsp_protocol->m_method_name_ = req_protocol->m_method_name_;
  // 解析完整的 rpc 方法名
  if (!ParseServiceFullName(method_full_name, service_name, method_name)) {
    // 解析失败，返回对应错误码, 不进行其他的处理
    rsp_protocol->SetTinyPBError(ERROR_PARSE_SERVICE_NAME, "parse service name error");
    Reply(rsp_protocol, connection);
    return;
  }
  // 找到对应的 service
  auto it = m_service_map_.find(service_name);
  if (it == m_service_map_.end()) {
    LOG_ERROR("%s | sericve neame[%s] not found", req_protocol->m_msg_id_.c_str(), service_name.c_str());
    rsp_protocol->SetTinyPBError(ERROR_SERVICE_NOT_FOUND, "service not found");
    Reply(rsp_protocol, connection);
    return;
  }
  // 找到对应的 method
  service_s_ptr service = (*it).second;
  const google::protobuf::MethodDescriptor* method = service->GetDescriptor()->FindMethodByName(method_name);
  if (method == NULL) {
    LOG_ERROR("%s | method neame[%s] not found in service[%s]", req_protocol->m_msg_id_.c_str(), method_name.c_str(), service_name.c_str());
    rsp_protocol->SetTinyPBError(ERROR_SERVICE_NOT_FOUND, "method not found");
    Reply(rsp_protocol, connection);
    return;
  }
  // 根据 method 对象反射出 request 和 response 对象
  // 获取请求信息
  // 反序列化，将 pb_data 反序列化为 req_msg
  google::protobuf::Message* req_msg = service->GetRequestPrototype(method).New();
  if (!req_msg->ParseFromString(req_protocol->m_pb_data_)) {
    LOG_ERROR("%s | deserilize error", req_protocol->m_msg_id_.c_str(), method_name.c_str(), service_name.c_str());
    rsp_protocol->SetTinyPBError(ERROR_FAILED_DESERIALIZE, "deserilize error");
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
  // closure 就会把 response 对象再序列化，最终生成一个 TinyPBProtocol 的结构体，最后通过 TcpConnection::reply 函数，将数据再发送给客户端
  RpcClosure* closure = new RpcClosure([req_msg, rsp_msg, req_protocol, rsp_protocol, connection, rpc_controller, this]() mutable {
    if (!rsp_msg->SerializeToString(&(rsp_protocol->m_pb_data_))) {
      LOG_ERROR("%s | serilize error, origin message [%s]", req_protocol->m_msg_id_.c_str(), rsp_msg->ShortDebugString().c_str());
      rsp_protocol->SetTinyPBError(ERROR_FAILED_SERIALIZE, "serilize error");
    } 
    else if (rpc_controller->GetErrorCode() != 0){
      LOG_ERROR("%s | run error [%s]", req_protocol->m_msg_id_.c_str(), rpc_controller->GetInfo().c_str());
      rsp_protocol->SetTinyPBError(rpc_controller->GetErrorCode(), rpc_controller->GetInfo());
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

void RpcDispatcher::CallHttpService(AbstractProtocol::s_ptr request, TcpConnection* connection){
  // 构造请求、响应信息
  std::shared_ptr<HttpRequest> req_protocol = std::dynamic_pointer_cast<HttpRequest>(request);
  std::shared_ptr<HttpResponse> rsp_protocol = std::make_shared<HttpResponse>();
  rsp_protocol->http_type_ = HttpType::RESPONSE;
  LOG_INFO("begin to dispatch client http request, msgno = %s", request->m_msg_id_.c_str());
  std::string url_path = req_protocol->m_request_path_;
  std::string service_name;
  std::string method_name;

  rsp_protocol->m_msg_id_ = req_protocol->m_msg_id_;
  rsp_protocol->m_header_.SetKeyValue("Msg-Id", rsp_protocol->m_msg_id_);
  // 设置connection和http版本
  SetCommParam(req_protocol, rsp_protocol);
  // 解析完整的 rpc 方法名
  if (!ParseUrlPathToervice(url_path, service_name, method_name)) {
    // 解析失败，返回对应错误码, 不进行其他的处理
    LOG_ERROR("404, url path{%s}, msgno= %s", url_path.c_str(), request->m_msg_id_.c_str());
    // 设置未发现响应报文
    SetNotFoundHttp(rsp_protocol);
    // 回复客户端
    Reply(rsp_protocol, connection);
    return;
  }
  // 找到对应的 service
  auto it = m_service_map_.find(service_name);
  if (it == m_service_map_.end()) {
    LOG_ERROR("%s | sericve neame[%s] not found", req_protocol->m_msg_id_.c_str(), service_name.c_str());
    // 设置未发现响应报文
    SetNotFoundHttp(rsp_protocol);
    Reply(rsp_protocol, connection);
    return;
  }
  // 找到对应的 method
  if(service_name == "/" && method_name.empty()){ method_name = "Begin"; }
  service_s_ptr service = (*it).second;
  const google::protobuf::MethodDescriptor* method = service->GetDescriptor()->FindMethodByName(method_name);
  if (method == NULL) {
    LOG_ERROR("%s | method neame[%s] not found in service[%s]", req_protocol->m_msg_id_.c_str(), method_name.c_str(), service_name.c_str());
    // 设置未发现响应报文
    SetNotFoundHttp(rsp_protocol);
    Reply(rsp_protocol, connection);
    return;
  }
  // 根据 method 对象反射出 request 和 response 对象
  google::protobuf::Message* req_msg = service->GetRequestPrototype(method).New();
  // 判断请求是否为rpc类型，并反序列化获得请求的内容
  if(req_protocol->m_header_.GetValue("Content-Type") == lightrpc::content_type_lightrpc){
    if(req_protocol->m_request_method_ == HttpMethod::POST){
      if (!req_msg->ParseFromString(req_protocol->m_body_)) {
        LOG_ERROR("%s | sericve_name[%s] and method_name[%s], deserilize error", req_protocol->m_msg_id_.c_str(), service_name.c_str(), method_name.c_str());
        SetInternalErrorHttp(rsp_protocol, "deserilize error");
        Reply(rsp_protocol, connection);
        DELETE_RESOURCE(req_msg);
        return;
      }
    }else{
      if (!req_msg->ParseFromString(req_protocol->m_request_query_)) {
        LOG_ERROR("%s | sericve_name[%s] and method_name[%s], deserilize error", req_protocol->m_msg_id_.c_str(), service_name.c_str(), method_name.c_str());
        SetInternalErrorHttp(rsp_protocol, "deserilize error");
        Reply(rsp_protocol, connection);
        DELETE_RESOURCE(req_msg);
        return;
      }
    }
  }else{
    LOG_ERROR("%s | Content-Type is not application/lightrpc", req_protocol->m_msg_id_.c_str());
    SetInternalErrorHttp(rsp_protocol, "Content-Type is not application/lightrpc");
    Reply(rsp_protocol, connection);
    DELETE_RESOURCE(req_msg);
    return;
  }
  
  google::protobuf::Message* rsp_msg = service->GetResponsePrototype(method).New();
  // 设置相关信息，辅助对象
  RpcController* rpc_controller = new RpcController();
  rpc_controller->SetLocalAddr(connection->GetLocalAddr());
  rpc_controller->SetPeerAddr(connection->GetPeerAddr());
  rpc_controller->SetMsgId(req_protocol->m_msg_id_);
  rpc_controller->SetHttpRequest(req_protocol);
  // closure 就会把 response 对象再序列化，最终生成一个 HttpProtocol 的结构体，最后通过 TcpConnection::reply 函数，将数据再发送给客户端
  RpcClosure* closure = new RpcClosure([req_msg, rsp_msg, req_protocol, rsp_protocol, connection, rpc_controller, this]() mutable {
    // 产生错误
    if (rpc_controller->GetErrorCode() != 0){
      LOG_ERROR("%s | run error [%s]", req_protocol->m_msg_id_.c_str(), rpc_controller->GetInfo().c_str());
      SetInternalErrorHttp(rsp_protocol, rpc_controller->GetInfo());
    }
    else {
      SetHttpCode(rsp_protocol, lightrpc::HTTP_OK);
      if(req_protocol->m_header_.GetValue("Content-Type") == lightrpc::content_type_lightrpc){
        SetHttpContentType(rsp_protocol, lightrpc::content_type_lightrpc);
        // 将序列化内容存储到响应体中
        if (!rsp_msg->SerializeToString(&(rsp_protocol->m_body_))) {
          LOG_ERROR("%s | serilize error, origin message [%s]", req_protocol->m_msg_id_.c_str(), rsp_msg->ShortDebugString().c_str());
          SetInternalErrorHttp(rsp_protocol, rpc_controller->GetInfo());
        }
        rsp_protocol->m_header_.SetKeyValue("Content-Length", std::to_string(rsp_protocol->m_body_.size()));
        LOG_INFO("%s | http dispatch success, requesut[%s], response[%s]", req_protocol->m_msg_id_.c_str(), req_msg->ShortDebugString().c_str(), rsp_msg->ShortDebugString().c_str());
      }
      else{
        // 由外部设置的响应头来扩展响应头
        std::map<std::string, std::string> controller_http_header = rpc_controller->GetHttpHeader().m_maps_;
        rsp_protocol->m_header_.m_maps_.insert(controller_http_header.begin(), controller_http_header.end());
        // 设置响应体
        rsp_protocol->m_body_ = rpc_controller->GetInfo();
        rsp_protocol->m_header_.SetKeyValue("Content-Length", std::to_string(rsp_protocol->m_body_.size()));
        LOG_INFO("%s | http dispatch success",req_protocol->m_msg_id_.c_str());
      }
    }

    this->Reply(rsp_protocol, connection);
  });
  // 调用业务处理方法，本质上就是输入一个 request 对象，然后获得一个 response 对象
  service->CallMethod(method, rpc_controller, req_msg, rsp_msg, closure);
}

void RpcDispatcher::Dispatch(AbstractProtocol::s_ptr request, TcpConnection* connection) {
  // 依据协议进行不同的服务分发
  if(request->protocol_ == ProtocalType::TINYPB){
    CallTinyPBService(request, connection);
  }else{
    CallHttpService(request, connection);
  }
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

bool RpcDispatcher::ParseUrlPathToervice(const std::string& url, std::string& service_name, std::string& method_name){
  if (url.empty()) {
    LOG_ERROR("url empty"); 
    return false;
  }
  size_t i = url.find_last_of("/");
  if (i == url.npos) {
    LOG_ERROR("not find / in full name [%s]", url.c_str());
    return false;
  }
  else if(i == 0){
    if(url.size() == 1){
      service_name = url;
    }else{
      service_name = url.substr(1, url.size()-1);
    }
  }
  else{
    service_name = url.substr(1, i-1);
    method_name = url.substr(i + 1, url.length() - i - 1);
  }
  
  LOG_INFO("parse sericve_name[%s] and method_name[%s] from url [%s]", service_name.c_str(), method_name.c_str(), url.c_str());

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

void RpcDispatcher::RegisterServlet(const std::string& path, service_s_ptr service){
  auto it = m_service_map_.find(path);
  if (it == m_service_map_.end()) {
    LOG_DEBUG("register servlet success to path {%s}", path);
    m_service_map_[path] = service;
  } else {
    LOG_DEBUG("failed to register, beacuse path {%s} has already register sertlet", path);
  }
}

void RpcDispatcher::RegisterToZookeeper(NetAddr::s_ptr local_addr, NetAddr::s_ptr zookeeper_addr){
  //把当前rpc节点上要发布的服务全部注册到zk上面，让rpc client可以从zk上发现服务
  ZookeeperClient zk_client;
  zk_client.start(zookeeper_addr);

  // 在配置中心中创建节点
  // 格式：/服务名
  for (auto &sp : m_service_map_){
    std::string service_path = (sp.first.find("/") != std::string::npos) ? sp.first : ("/" + sp.first);
    char method_path_data[128] = {0};
    sprintf(method_path_data, "%s", local_addr->ToString().c_str());
    //ZOO_EPHEMERAL 表示znode时候临时性节点
    zk_client.create(service_path.c_str(), method_path_data, strlen(method_path_data), 0);
  }
  
}
}