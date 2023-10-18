#include "rpc_controller.h"

namespace lightrpc {

// 将RpcController重置为初始状态，以便可以在新的调用中重用它。RPC正在进行时不能被调用。
void RpcController::Reset() {
  m_error_code_ = 0;
  m_error_info_ = "";
  m_msg_id_ = "";
  m_is_failed_ = false;
  m_is_cancled_ = false;
  m_is_finished_ = false;
  m_local_addr_ = nullptr;
  m_peer_addr_ = nullptr;
  m_timeout_ = 1000;   // ms
}

// 调用结束后，如果调用失败，则返回true。失败的原因取决于RPC实现，在调用完成之前，不能调用Failed，
bool RpcController::Failed() const {
  return m_is_failed_;
}

// 如果Failed为true，则返回可读的错误描述。
std::string RpcController::ErrorText() const {
  return m_error_info_;
}

// 建议RPC系统调用者希望RPC调用被取消。RPC系统可能会立即取消它，可能会等待一段时间然后取消它，甚至可能根本不会取消该调用。  
// 如果该调用被取消，则“done”回调将仍然被调用，并且RpcController将指示此时调用失败。
void RpcController::StartCancel() {
  m_is_cancled_ = true;
  m_is_failed_ = true;
  SetFinished(true);
}

// 使得Failed在客户端返回true。
void RpcController::SetFailed(const std::string& reason) {
  m_error_info_ = reason;
  m_is_failed_ = true;
}

// 如果为true，则表示客户端取消了RPC，因此服务器可能放弃回复它。服务器仍然应该调用最终的“done”回调。
bool RpcController::IsCanceled() const {
  return m_is_cancled_;
}

// 要求在取消RPC时调用给定的回调。回调将始终被调用一次。如果RPC完成而未被取消，则完成后将调用回调。
// 如果在调用NotifyOnCancel时RPC已被取消，则立即调用回调。NotifyOnCancel（）每个请求只能调用一次。
void RpcController::NotifyOnCancel(google::protobuf::Closure* callback) {

}

void RpcController::SetError(int32_t error_code, const std::string error_info) {
  m_error_code_ = error_code;
  m_error_info_ = error_info;
  m_is_failed_ = true;
}

int32_t RpcController::GetErrorCode() {
  return m_error_code_;
}

std::string RpcController::GetErrorInfo() {
  return m_error_info_;
}

void RpcController::SetMsgId(const std::string& msg_id) {
  m_msg_id_ = msg_id;
}

std::string RpcController::GetMsgId() {
  return m_msg_id_;
}

void RpcController::SetLocalAddr(NetAddr::s_ptr addr) {
  m_local_addr_ = addr;
}

void RpcController::SetPeerAddr(NetAddr::s_ptr addr) {
  m_peer_addr_ = addr;
}

NetAddr::s_ptr RpcController::GetLocalAddr() {
  return m_local_addr_;
}

NetAddr::s_ptr RpcController::GetPeerAddr() {
  return m_peer_addr_;
}

void RpcController::SetTimeout(int timeout) {
  m_timeout_ = timeout;
}

int RpcController::GetTimeout() {
  return m_timeout_;
}

bool RpcController::Finished() {
  return m_is_finished_;
}

void RpcController::SetFinished(bool value) {
  m_is_finished_ = value;
}
}