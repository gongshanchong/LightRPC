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
#include "../lightrpc/net/rpc/string_codec.h"
#include "../lightrpc/net/rpc/abstract_protocol.h"
#include "../lightrpc/net/tinypb/tinypb_codec.h"
#include "../lightrpc/net/tinypb/tinypb_protocol.h"
#include "../lightrpc/net/tcp/net_addr.h"
#include "../lightrpc/net/tcp/tcp_server.h"
#include "../lightrpc/net/rpc/rpc_dispatcher.h"
#include "../lightrpc/net/rpc/rpc_controller.h"
#include "../lightrpc/net/rpc/rpc_channel.h"
#include "../lightrpc/net/rpc/rpc_closure.h"

#include "order.pb.h"

void test_rpc_channel() {
  NEWMESSAGE(makeOrderRequest, request);
  NEWMESSAGE(makeOrderResponse, response);
  // 获取服务端通信
  std::shared_ptr<lightrpc::RpcChannel> channel = std::make_shared<lightrpc::RpcChannel>(std::make_shared<lightrpc::IPNetAddr>("127.0.0.1:2181"));
  std::shared_ptr<Order_Stub> stub = std::make_shared<Order_Stub>(channel.get());
  // 请求与响应
  request->set_price(100);
  request->set_goods("apple");
  // 相关辅助参数设置
  NEWRPCCONTROLLER(controller);
  controller->SetMsgId("99998888");
  controller->SetTimeout(10000);
  // 设置http请求报文的相关参数
  controller->SetProtocol(lightrpc::ProtocalType::HTTP);
  controller->SetCallMethod(lightrpc::HttpMethod::GET);
  lightrpc::HttpHeaderComm http_header;
  http_header.SetKeyValue("Content-Type",lightrpc::content_type_lightrpc);
  http_header.SetKeyValue("Connection", "Keep-Alive");
  controller->SetHttpHeader(http_header);
  controller->SetHttpVersion("HTTP/1.1");
  // 回调函数设置
  std::shared_ptr<lightrpc::RpcClosure> closure = std::make_shared<lightrpc::RpcClosure>([request, response]() mutable {
    LOG_INFO("call rpc success, request[%s], response[%s]", request->ShortDebugString().c_str(), response->ShortDebugString().c_str());
    // 执行业务逻辑
    if (response->order_id() == "xxx") {
      // xx
    }
  });
  // 远程调用，可以通过controller来进行相关的控制（如连接超时时间、错误、调用完成。。。）
  stub->makeOrder(controller.get(), request.get(), response.get(), closure.get());
}

int main() {

  lightrpc::Config::SetGlobalConfig(NULL);

  lightrpc::Logger::InitGlobalLogger(0);

  // test_tcp_client();
  test_rpc_channel();

  LOG_INFO("test_rpc_channel end");

  return 0;
}