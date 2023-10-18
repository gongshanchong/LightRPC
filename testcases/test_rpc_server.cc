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
#include "../lightrpc/net/tinypb/string_coder.h"
#include "../lightrpc/net/tinypb/abstract_protocol.h"
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

  lightrpc::Config::SetGlobalConfig(argv[1]);

  lightrpc::Logger::InitGlobalLogger();

  std::shared_ptr<OrderImpl> service = std::make_shared<OrderImpl>();
  lightrpc::RpcDispatcher::GetRpcDispatcher()->RegisterService(service);

  lightrpc::IPNetAddr::s_ptr addr = std::make_shared<lightrpc::IPNetAddr>("127.0.0.1", lightrpc::Config::GetGlobalConfig()->m_port_);

  lightrpc::TcpServer tcp_server(addr);

  tcp_server.Start();

  return 0;
}