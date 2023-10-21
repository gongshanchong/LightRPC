#include "../../lightrpc/common/log.h"
#include "../../lightrpc/net/http/http_servlet.h"
#include "../order.pb.h"

class NonBlockCallHttpServlet: public lightrpc::HttpServlet {
 public:
  NonBlockCallHttpServlet() = default;
  ~NonBlockCallHttpServlet() = default;

  void Handle(std::shared_ptr<lightrpc::HttpRequest> req, std::shared_ptr<lightrpc::HttpResponse> res) {
    AppInfoLog("NonBlockCallHttpServlet get request");
    AppDebugLog("NonBlockCallHttpServlet success recive http request, now to get http response");
    setHttpCode(res, tinyrpc::HTTP_OK);
    setHttpContentType(res, "text/html;charset=utf-8");

    std::shared_ptr<queryAgeReq> rpc_req = std::make_shared<queryAgeReq>();
    std::shared_ptr<queryAgeRes> rpc_res = std::make_shared<queryAgeRes>();
    AppDebugLog("now to call QueryServer TinyRPC server to query who's id is %s", req->m_query_maps["id"].c_str());
    rpc_req->set_id(std::atoi(req->m_query_maps["id"].c_str()));

    std::shared_ptr<tinyrpc::TinyPbRpcController> rpc_controller = std::make_shared<tinyrpc::TinyPbRpcController>();
    rpc_controller->SetTimeout(10000);

    AppDebugLog("NonBlockCallHttpServlet begin to call RPC async");


    tinyrpc::TinyPbRpcAsyncChannel::ptr async_channel = 
      std::make_shared<tinyrpc::TinyPbRpcAsyncChannel>(addr);

    auto cb = [rpc_res]() {
      printf("call succ, res = %s\n", rpc_res->ShortDebugString().c_str());
      AppDebugLog("NonBlockCallHttpServlet async call end, res=%s", rpc_res->ShortDebugString().c_str());
    };

    std::shared_ptr<tinyrpc::TinyPbRpcClosure> closure = std::make_shared<tinyrpc::TinyPbRpcClosure>(cb); 
    async_channel->saveCallee(rpc_controller, rpc_req, rpc_res, closure);

    QueryService_Stub stub(async_channel.get());

    stub.query_age(rpc_controller.get(), rpc_req.get(), rpc_res.get(), NULL);
    AppDebugLog("NonBlockCallHttpServlet async end, now you can to some another thing");

    async_channel->wait();
    AppDebugLog("wait() back, now to check is rpc call succ");

    if (rpc_controller->ErrorCode() != 0) {
      AppDebugLog("failed to call QueryServer rpc server");
      char buf[512];
      sprintf(buf, html, "failed to call QueryServer rpc server");
      setHttpBody(res, std::string(buf));
      return;
    }

    if (rpc_res->ret_code() != 0) {
      std::stringstream ss;
      ss << "QueryServer rpc server return bad result, ret = " << rpc_res->ret_code() << ", and res_info = " << rpc_res->res_info();
      char buf[512];
      sprintf(buf, html, ss.str().c_str());
      setHttpBody(res, std::string(buf));
      return;
    }

    std::stringstream ss;
    ss << "Success!! Your age is," << rpc_res->age() << " and Your id is " << rpc_res->id();

    char buf[512];
    sprintf(buf, html, ss.str().c_str());
    setHttpBody(res, std::string(buf));
  }

  std::string GetServletName() {
    return "NonBlockCallHttpServlet";
  }

};