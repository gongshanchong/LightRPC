#ifndef LIGHTRPC_NET_RPC_RPC_INTERFACE_H
#define LIGHTRPC_NET_RPC_RPC_INTERFACE_H 

#include <memory>
#include <google/protobuf/message.h>
#include "rpc_controller.h"
#include "../../common/log.h"

namespace lightrpc {

class RpcClosure;

/*
 * Rpc Interface Base Class
 * All interface should extend this abstract class
*/

class RpcInterface : public std::enable_shared_from_this<RpcInterface> {
 public:

  RpcInterface(const google::protobuf::Message* req, google::protobuf::Message* rsp, RpcClosure* done, RpcController* controller);

  virtual ~RpcInterface();

  // reply to client
  void Reply();

  // free resourse
  void Destroy();

  // core business deal method
  virtual void Run() = 0;

  // set error code and error into to controller to write response message
  virtual void SetError(int code, const std::string& err_info) = 0;

 protected:

  const google::protobuf::Message* m_req_base_ {NULL};

  google::protobuf::Message* m_rsp_base_ {NULL};

  RpcClosure* m_done_ {NULL};        // callback

  RpcController* m_controller_ {NULL};
};

}
#endif