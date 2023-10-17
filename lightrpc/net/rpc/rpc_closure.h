#ifndef LIGHTRPC_NET_RPC_RPC_CLOSURE_H
#define LIGHTRPC_NET_RPC_RPC_CLOSURE_H

#include <google/protobuf/stubs/callback.h>
#include <functional>
#include <memory>
#include "../../common/run_time.h"
#include "../../common/log.h"
#include "../../common/exception.h"
#include "rpc_interface.h"

namespace lightrpc {
class RpcClosure : public google::protobuf::Closure {
 public:
  typedef std::shared_ptr<RpcInterface> it_s_ptr;

  RpcClosure(it_s_ptr interface, std::function<void()> cb) : m_rpc_interface_(interface), m_cb_(cb) {
    LOG_INFO("RpcClosure");
  }

  ~RpcClosure() {
    LOG_INFO("~RpcClosure");
  }

  void Run() override {
    // 更新 runtime 的 RpcInterFace, 这里在执行 cb 的时候，都会以 RpcInterface 找到对应的接口，实现打印 app 日志等
    if (!m_rpc_interface_) {
      RunTime::GetRunTime()->m_rpc_interface_ = m_rpc_interface_.get();
    }

    try {
      if (m_cb_ != nullptr) {
        m_cb_();
      }
      if (m_rpc_interface_) {
        m_rpc_interface_.reset();
      }
    } catch (RocketException& e) {
      LOG_ERROR("RocketException exception[%s], deal handle", e.what());
      e.handle();
      if (m_rpc_interface_) {
        m_rpc_interface_->SetError(e.errorCode(), e.errorInfo());
        m_rpc_interface_.reset();
      }
    } catch (std::exception& e) {
      LOG_ERROR("std::exception[%s]", e.what());
      if (m_rpc_interface_) {
        m_rpc_interface_->SetError(-1, "unkonwn std::exception");
        m_rpc_interface_.reset();
      }
    } catch (...) {
      LOG_ERROR("Unkonwn exception");
      if (m_rpc_interface_) {
        m_rpc_interface_->SetError(-1, "unkonwn exception");
        m_rpc_interface_.reset();
      }
    }
    
  }

 private:
  it_s_ptr m_rpc_interface_ {nullptr};
  std::function<void()> m_cb_ {nullptr};

};

}
#endif