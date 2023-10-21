#include <assert.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <memory>
#include <unistd.h>
#include <google/protobuf/service.h>
#include "../lightrpc/common/log.h"
#include "../lightrpc/common/config.h"
#include "../lightrpc/common/log.h"
#include "../lightrpc/net/tcp/tcp_client.h"
#include "../lightrpc/net/tcp/net_addr.h"
#include "../lightrpc/net/rpc/string_coder.h"
#include "../lightrpc/net/rpc/abstract_protocol.h"
#include "../lightrpc/net/tinypb/tinypb_coder.h"
#include "../lightrpc/net/tinypb/tinypb_protocol.h"
#include "../lightrpc/net/tcp/net_addr.h"
#include "../lightrpc/net/tcp/tcp_server.h"
#include "../lightrpc/net/rpc/rpc_dispatcher.h"

#include "orderimpl.h"

int main(int argc, char* argv[]) {

  if (argc != 2) {
    printf("Start test_rpc_server error, argc not 2 \n");
    printf("Start like this: \n");
    printf("./test_rpc_server ../conf/lightrpc.xml \n");
    return 0;
  }

  // 初始化日志和配置文件
  lightrpc::Config::SetGlobalConfig(argv[1]);
  lightrpc::Logger::InitGlobalLogger();

  lightrpc::TcpServer tcp_server(lightrpc::Config::GetGlobalConfig()->m_rpc_stubs_["default"].addr_, 
  lightrpc::Config::GetGlobalConfig()->m_rpc_stubs_["default"].protocal_, 
  lightrpc::Config::GetGlobalConfig()->m_rpc_stubs_["default"].timeout_);
  // 依据配置文件中的服务的协议进行相关的操作
  if(lightrpc::Config::GetGlobalConfig()->m_rpc_stubs_["default"].protocal_ == "HTTP"){

  }else{
    std::shared_ptr<OrderImpl> service = std::make_shared<OrderImpl>();
    lightrpc::RpcDispatcher::GetRpcDispatcher()->RegisterService(service);
  }

  tcp_server.Start();

  return 0;
}