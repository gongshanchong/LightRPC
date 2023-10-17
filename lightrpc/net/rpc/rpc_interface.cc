#include "../../common/log.h"
#include "rpc_closure.h"
#include "rpc_closure.h"
#include "rpc_controller.h"
#include "rpc_interface.h"

namespace lightrpc {

RpcInterface::RpcInterface(const google::protobuf::Message* req, google::protobuf::Message* rsp, RpcClosure* done, RpcController* controller)
  : m_req_base_(req), m_rsp_base_(rsp), m_done_(done) , m_controller_(controller) {
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

std::shared_ptr<RpcClosure> RpcInterface::NewRpcClosure(std::function<void()>& cb) {
  return std::make_shared<RpcClosure>(shared_from_this(), cb);
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