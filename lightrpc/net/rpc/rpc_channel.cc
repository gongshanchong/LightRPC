#include "rpc_channel.h"
#include "abstract_protocol.h"
#include <string>

namespace lightrpc {

RpcChannel::RpcChannel(NetAddr::s_ptr peer_addr) : m_peer_addr_(peer_addr){
  LOG_INFO("RpcChannel");
}

RpcChannel::~RpcChannel() {
  LOG_INFO("~RpcChannel");
}

void RpcChannel::Init(google::protobuf::RpcController* controller, const google::protobuf::Message* request,
                          google::protobuf::Message* response, google::protobuf::Closure* done){
    m_request_ = request;
    m_response_ = response;
    m_closure_ = done;
    m_controller_ = controller;

    RpcController* method_controller = dynamic_cast<RpcController*>(m_controller_);
    // 判断参数是否为空
    if (method_controller == NULL || request == NULL || response == NULL) {
      LOG_ERROR("failed callmethod, RpcController convert error");
      if(method_controller == NULL){ return; }
      method_controller->SetError(ERROR_RPC_CHANNEL_INIT, "controller or request or response NULL");
      CallBack();
      return;
    }
    // 获取客户端地址
    if (m_peer_addr_ == nullptr) {
      LOG_ERROR("failed get peer addr");
      method_controller->SetError(ERROR_RPC_PEER_ADDR, "peer addr nullptr");
      CallBack();
      return;
    }
    m_client_ = std::make_shared<TcpClient>(m_peer_addr_, method_controller->GetProtocol());
}

void RpcChannel::CallBack() {
  RpcController* method_controller = dynamic_cast<RpcController*>(m_controller_);
  if (method_controller->GetErrorCode() != 0) {
    LOG_ERROR("call rpc failed, request[%s], error code[%d], error info[%s]", 
        m_request_->ShortDebugString().c_str(), 
        method_controller->GetErrorCode(), 
        method_controller->GetInfo().c_str());
    method_controller->StartCancel();
    return;
  }
  if (method_controller->Finished()) {
    LOG_INFO("call rpc repeat, request[%s] of this controller finished", m_request_->ShortDebugString().c_str());
    return;
  }
  // 执行闭包函数
  if (m_closure_) {
    m_closure_->Run();
    if (method_controller) {
      method_controller->SetFinished(true);
    }
  }
}

void RpcChannel::CallTinyPBService(const google::protobuf::MethodDescriptor* method,
                        google::protobuf::RpcController* controller, const google::protobuf::Message* request,
                        google::protobuf::Message* response, google::protobuf::Closure* done) {
  RpcController* method_controller = dynamic_cast<RpcController*>(m_controller_);
  std::shared_ptr<lightrpc::TinyPBProtocol> req_protocol = std::make_shared<lightrpc::TinyPBProtocol>();
  // 获取msg_id
  if (method_controller->GetMsgId().empty()) {
    // 这样的目的是为了实现 msg_id 的透传，假设服务 A 调用了 B，那么同一个 msgid 可以在服务 A 和 B 之间串起来，方便日志追踪
    req_protocol->m_msg_id_ = MsgIDUtil::GenMsgID();
    method_controller->SetMsgId(req_protocol->m_msg_id_);
  } else {
    // 如果 controller 指定了 msgid, 直接使用
    req_protocol->m_msg_id_ = method_controller->GetMsgId();
  }
  // 获取请求的方法
  req_protocol->m_method_name_ = method->full_name();
  LOG_INFO("%s | call method name [%s]", req_protocol->m_msg_id_.c_str(), req_protocol->m_method_name_.c_str());
  // requeset 的序列化
  if (!request->SerializeToString(&(req_protocol->m_pb_data_))) {
    std::string err_info = "failde to serialize";
    method_controller->SetError(ERROR_FAILED_SERIALIZE, err_info);
    LOG_ERROR("%s | %s, origin requeset [%s] ", req_protocol->m_msg_id_.c_str(), err_info.c_str(), request->ShortDebugString().c_str());
    CallBack();
    return;
  }

  s_ptr channel = shared_from_this(); 
  // 设置超时事件
  TimerEvent::s_ptr timer_event = std::make_shared<TimerEvent>(method_controller->GetTimeout(), false, [channel, method_controller, done](int) mutable {
    LOG_INFO("%s | call rpc timeout arrive", method_controller->GetMsgId().c_str());
    if (method_controller->Finished()) {
      return;
    }

    method_controller->StartCancel();
    method_controller->SetError(ERROR_RPC_CALL_TIMEOUT, "rpc call timeout " + std::to_string(method_controller->GetTimeout()));

    channel->CallBack();
  }, m_client_->GetFd());
  m_client_->AddTimerEvent(timer_event);
  // 设置connnect的回调函数，连接成功后发送请求并接受响应
  m_client_->Connect([method_controller, req_protocol, this]() mutable {
    // 连接错误
    if (GetTcpClient()->GetConnectErrorCode() != 0) {
      method_controller->SetError(GetTcpClient()->GetConnectErrorCode(), GetTcpClient()->GetConnectErrorInfo());
      LOG_ERROR("%s | connect error, error coode[%d], error info[%s], peer addr[%s]", 
        req_protocol->m_msg_id_.c_str(), method_controller->GetErrorCode(), 
        method_controller->GetInfo().c_str(), GetTcpClient()->GetPeerAddr()->ToString().c_str());
      
      CallBack();
      return;
    }
    // 连接成功，输出日志信息
    LOG_INFO("%s | connect success, peer addr[%s], local addr[%s]",
      req_protocol->m_msg_id_.c_str(), 
      GetTcpClient()->GetPeerAddr()->ToString().c_str(), 
      GetTcpClient()->GetPeerAddr()->ToString().c_str()); 
    // 先调用 writeMessage 发送 req_protocol, 然后调用 readMessage 等待回包
    // 发送请求并设置回调函数
    GetTcpClient()->WriteMessage(req_protocol, [method_controller, req_protocol, this](AbstractProtocol::s_ptr) mutable {
      // 发送请求成功，输出日志
      LOG_INFO("%s | send rpc request success. call method name[%s], peer addr[%s], local addr[%s]", 
        req_protocol->m_msg_id_.c_str(), req_protocol->m_method_name_.c_str(),
        GetTcpClient()->GetPeerAddr()->ToString().c_str(), GetTcpClient()->GetLocalAddr()->ToString().c_str());

      // 读取响应并设置回调函数
      GetTcpClient()->ReadMessage(req_protocol->m_msg_id_, [method_controller, this](AbstractProtocol::s_ptr msg) mutable {
        std::shared_ptr<lightrpc::TinyPBProtocol> rsp_protocol = std::dynamic_pointer_cast<lightrpc::TinyPBProtocol>(msg);
        // 读取响应成功，输出日志
        LOG_INFO("%s | success get rpc response, call method name[%s], peer addr[%s], local addr[%s]", 
          rsp_protocol->m_msg_id_.c_str(), rsp_protocol->m_method_name_.c_str(),
          GetTcpClient()->GetPeerAddr()->ToString().c_str(), GetTcpClient()->GetLocalAddr()->ToString().c_str());
        // 连接错误
        if (rsp_protocol->m_err_code_ != 0) {
          LOG_ERROR("%s | call rpc methood[%s] failed, error code[%d], error info[%s]", 
            rsp_protocol->m_msg_id_.c_str(), rsp_protocol->m_method_name_.c_str(),
            rsp_protocol->m_err_code_, rsp_protocol->m_err_info_.c_str());

          method_controller->SetError(rsp_protocol->m_err_code_, rsp_protocol->m_err_info_);
          
          CallBack();
          return;
        }
        // 反序列化响应
        if (!(m_response_->ParseFromString(rsp_protocol->m_pb_data_))){
          LOG_ERROR("%s | serialize error", rsp_protocol->m_msg_id_.c_str());
          method_controller->SetError(ERROR_FAILED_SERIALIZE, "serialize error");
          
          CallBack();
          return;
        }
        // 调用rpc成功
        LOG_INFO("%s | call rpc success, call method name[%s], peer addr[%s], local addr[%s]",
          rsp_protocol->m_msg_id_.c_str(), rsp_protocol->m_method_name_.c_str(),
          GetTcpClient()->GetPeerAddr()->ToString().c_str(), GetTcpClient()->GetLocalAddr()->ToString().c_str())

        CallBack();
      });
    });
  });
}

void RpcChannel::CallHttpService(const google::protobuf::MethodDescriptor* method,
                        google::protobuf::RpcController* controller, const google::protobuf::Message* request,
                        google::protobuf::Message* response, google::protobuf::Closure* done){
  RpcController* method_controller = dynamic_cast<RpcController*>(m_controller_);
  std::shared_ptr<lightrpc::HttpRequest> req_protocol = std::make_shared<lightrpc::HttpRequest>();
  // 获取msg_id
  if (method_controller->GetMsgId().empty()) {
    // 这样的目的是为了实现 msg_id 的透传，假设服务 A 调用了 B，那么同一个 msgid 可以在服务 A 和 B 之间串起来，方便日志追踪
    req_protocol->m_msg_id_ = MsgIDUtil::GenMsgID();
    method_controller->SetMsgId(req_protocol->m_msg_id_);
  } else {
    // 如果 controller 指定了 msgid, 直接使用
    req_protocol->m_msg_id_ = method_controller->GetMsgId();
  }
  // req_protocol的初始设置
  req_protocol->http_type_ = HttpType::REQUEST;
  req_protocol->m_request_version_ = method_controller->GetHttpVersion();
  req_protocol->m_header_ = method_controller->GetHttpHeader();
  req_protocol->m_header_.SetKeyValue("Msg-Id", req_protocol->m_msg_id_);
  // 获取请求的方法
  std::string method_full_name = method->full_name();
  std::string service_name;
  std::string method_name;
  size_t i = method_full_name.find_first_of(".");
  service_name = method_full_name.substr(0, i);
  method_name = method_full_name.substr(i + 1, method_full_name.length() - i - 1);
  req_protocol->m_request_path_ = "/" + service_name + "/" + method_name;
  LOG_INFO("%s | call method name [%s]", req_protocol->m_msg_id_.c_str(), method_full_name.c_str());
  // requeset 的序列化
  if(method_controller->GetCallMethod() == HttpMethod::POST){
    req_protocol->m_request_method_ = HttpMethod::POST;
    if (!request->SerializeToString(&(req_protocol->m_body_))) {
      std::string err_info = "failde to serialize";
      method_controller->SetError(ERROR_FAILED_SERIALIZE, err_info);
      LOG_ERROR("%s | %s, origin requeset [%s] ", req_protocol->m_msg_id_.c_str(), err_info.c_str(), request->ShortDebugString().c_str());
      CallBack();
      return;
    }
    req_protocol->m_header_.SetKeyValue("Content-Length", std::to_string(req_protocol->m_body_.size()));
  }else{
    req_protocol->m_request_method_ = HttpMethod::GET;
    std::string tmp;
    if (!request->SerializeToString(&tmp)) {
      std::string err_info = "failde to serialize";
      method_controller->SetError(ERROR_FAILED_SERIALIZE, err_info);
      LOG_ERROR("%s | %s, origin requeset [%s] ", req_protocol->m_msg_id_.c_str(), err_info.c_str(), request->ShortDebugString().c_str());
      CallBack();
      return;
    }
    req_protocol->m_request_path_ += ("?" + tmp);
  }
  s_ptr channel = shared_from_this(); 
  // 设置超时事件
  TimerEvent::s_ptr timer_event = std::make_shared<TimerEvent>(method_controller->GetTimeout(), false, [channel, method_controller, done](int) mutable {
    LOG_INFO("%s | call rpc timeout arrive", method_controller->GetMsgId().c_str());
    if (method_controller->Finished()) {
      return;
    }

    method_controller->StartCancel();
    method_controller->SetError(ERROR_RPC_CALL_TIMEOUT, "rpc call timeout " + std::to_string(method_controller->GetTimeout()));

    channel->CallBack();
  });
  m_client_->AddTimerEvent(timer_event);
  // 设置connnect的回调函数，连接成功后发送请求并接受响应
  m_client_->Connect([method_controller, req_protocol, this]() mutable {
    // 连接错误
    if (GetTcpClient()->GetConnectErrorCode() != 0) {
      method_controller->SetError(GetTcpClient()->GetConnectErrorCode(), GetTcpClient()->GetConnectErrorInfo());
      LOG_ERROR("%s | connect error, error coode[%d], error info[%s], peer addr[%s]", 
        req_protocol->m_msg_id_.c_str(), method_controller->GetErrorCode(), 
        method_controller->GetInfo().c_str(), GetTcpClient()->GetPeerAddr()->ToString().c_str());
      
      CallBack();
      return;
    }
    // 连接成功，输出日志信息
    LOG_INFO("%s | connect success, peer addr[%s], local addr[%s]",
      req_protocol->m_msg_id_.c_str(), 
      GetTcpClient()->GetPeerAddr()->ToString().c_str(), 
      GetTcpClient()->GetPeerAddr()->ToString().c_str());
    // 先调用 writeMessage 发送 req_protocol, 然后调用 readMessage 等待回包
    // 发送请求并设置回调函数
    GetTcpClient()->WriteMessage(req_protocol, [method_controller, req_protocol, this](AbstractProtocol::s_ptr) mutable {
      // 发送请求成功，输出日志
      LOG_INFO("%s | send rpc request success. call method name[%s], peer addr[%s], local addr[%s]", 
        req_protocol->m_msg_id_.c_str(), req_protocol->m_request_path_.c_str(),
        GetTcpClient()->GetPeerAddr()->ToString().c_str(), GetTcpClient()->GetLocalAddr()->ToString().c_str());

      // 读取响应并设置回调函数
      GetTcpClient()->ReadMessage(req_protocol->m_msg_id_, [method_controller, req_protocol, this](AbstractProtocol::s_ptr msg) mutable {
        std::shared_ptr<lightrpc::HttpResponse> rsp_protocol = std::dynamic_pointer_cast<lightrpc::HttpResponse>(msg);
        rsp_protocol->m_msg_id_ = req_protocol->m_msg_id_;
        // 读取响应成功，输出日志
        LOG_INFO("%s | success get rpc response, call method name[%s], peer addr[%s], local addr[%s]", 
          rsp_protocol->m_msg_id_.c_str(), req_protocol->m_request_path_.c_str(),
          GetTcpClient()->GetPeerAddr()->ToString().c_str(), GetTcpClient()->GetLocalAddr()->ToString().c_str());
        // 连接错误
        if (rsp_protocol->m_response_code_ != HttpCode::HTTP_OK) {
          LOG_ERROR("%s | call rpc methood[%s] failed, error code[%d], error info[%s]", 
            rsp_protocol->m_msg_id_.c_str(), req_protocol->m_request_path_.c_str(),
            rsp_protocol->m_response_code_, HttpCodeToString(rsp_protocol->m_response_code_));

          method_controller->SetError(rsp_protocol->m_response_code_, HttpCodeToString(rsp_protocol->m_response_code_));
          
          CallBack();
          return;
        }
        // 反序列化响应
        if (!(m_response_->ParseFromString(rsp_protocol->m_body_))){
          LOG_ERROR("%s | serialize error", rsp_protocol->m_msg_id_.c_str());
          method_controller->SetError(ERROR_FAILED_SERIALIZE, "serialize error");
          
          CallBack();
          return;
        }
        // 调用rpc成功
        LOG_INFO("%s | call rpc success, call method name[%s], peer addr[%s], local addr[%s]",
          rsp_protocol->m_msg_id_.c_str(), req_protocol->m_request_path_.c_str(),
          GetTcpClient()->GetPeerAddr()->ToString().c_str(), GetTcpClient()->GetLocalAddr()->ToString().c_str())

        CallBack();
      });
    });
  });
}

void RpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                        google::protobuf::RpcController* controller, const google::protobuf::Message* request,
                        google::protobuf::Message* response, google::protobuf::Closure* done){
  // 初始化
  Init(controller, request, response, done);
  // 依据协议进行不同的服务调用
  if(dynamic_cast<RpcController*>(controller)->GetProtocol() == ProtocalType::TINYPB){
    CallTinyPBService(method, controller, request, response, done);
  }else{
    CallHttpService(method, controller, request, response, done);
  }  
}

TcpClient* RpcChannel::GetTcpClient() {
  return m_client_.get();
}

NetAddr::s_ptr RpcChannel::FindAddr(const std::string& str) {
  if (IPNetAddr::CheckValid(str)) {
    return std::make_shared<IPNetAddr>(str);
  } else {
    auto it = Config::GetGlobalConfig()->m_rpc_stubs_.find(str);
    if (it != Config::GetGlobalConfig()->m_rpc_stubs_.end()) {
      LOG_INFO("find addr [%s] in global config of str[%s]", (*it).second.addr_->ToString().c_str(), str.c_str());
      return (*it).second.addr_;
    } else {
      LOG_INFO("can not find addr in global config of str[%s]", str.c_str());
      return nullptr;
    }
  }
}
}