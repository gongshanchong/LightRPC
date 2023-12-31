#include "rpc_closure.h"
#include "rpc_interface.h"

namespace lightrpc {

RpcInterface::RpcInterface(const google::protobuf::Message* req, google::protobuf::Message* rsp, google::protobuf::RpcController* controller, ::google::protobuf::Closure* done)
  : m_req_base_(req), m_rsp_base_(rsp), m_done_(done){
    m_controller_ = dynamic_cast<RpcController*>(controller);
    LOG_INFO("RpcInterface");
}

RpcInterface::~RpcInterface() {
  LOG_INFO("~RpcInterface");

  Reply();

  Destroy();
}

void RpcInterface::Reply() {
  // reply to client
  // you should call is when you wan to set response back
  // it means this rpc method done 
  if (m_done_) {
    m_done_->Run();
  }
}

void RpcInterface::Destroy() {
  if (m_req_base_) {
    delete m_req_base_;
    m_req_base_ = NULL;
  }

  if (m_rsp_base_) {
    delete m_rsp_base_;
    m_rsp_base_ = NULL;
  }

  if (m_done_) {
    delete m_done_;
    m_done_ = NULL;
  }

  if (m_controller_) {
    delete m_controller_;
    m_controller_ = NULL;
  }
}
}