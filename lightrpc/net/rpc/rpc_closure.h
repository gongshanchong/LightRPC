#ifndef LIGHTRPC_NET_RPC_RPC_CLOSURE_H
#define LIGHTRPC_NET_RPC_RPC_CLOSURE_H

#include <google/protobuf/stubs/callback.h>
#include <functional>
#include <memory>
#include "../../common/log.h"
#include "rpc_interface.h"

namespace lightrpc {
class RpcClosure : public google::protobuf::Closure {
 public:
  typedef std::shared_ptr<RpcInterface> it_s_ptr;

  RpcClosure(std::function<void()> cb) : m_cb_(cb) {
    LOG_INFO("RpcClosure");
  }

  ~RpcClosure() {
    LOG_INFO("~RpcClosure");
  }

  void Run() override {
    if (m_cb_ != nullptr) {
        m_cb_();
      }
  }

 private:
  std::function<void()> m_cb_ {nullptr};
};

}
#endif