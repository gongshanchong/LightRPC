#include "../../lightrpc/common/log.h"
#include "../../lightrpc/net/http/http_servlet.h"
#include "../order.pb.h"

class BlockCallHttpServlet : public lightrpc::HttpServlet {
 public:
  BlockCallHttpServlet() = default;
  ~BlockCallHttpServlet() = default;

  void Handle(std::shared_ptr<lightrpc::HttpRequest> req, std::shared_ptr<lightrpc::HttpResponse> res) {
    LOG_APPDEBUG("BlockCallHttpServlet get request");
    LOG_APPDEBUG("BlockCallHttpServlet success recive http request, now to get http response");
    SetHttpCode(res, lightrpc::HTTP_OK);
    SetHttpContentType(res, "text/html;charset=utf-8");

    queryAgeReq rpc_req;
    queryAgeRes rpc_res;
    AppDebugLog ("now to call QueryServer TinyRPC server to query who's id is %s", req->m_query_maps["id"].c_str());
    rpc_req.set_id(std::atoi(req->m_query_maps["id"].c_str()));

    tinyrpc::TinyPbRpcChannel channel(addr);
    QueryService_Stub stub(&channel);

    tinyrpc::TinyPbRpcController rpc_controller;
    rpc_controller.SetTimeout(5000);

    AppDebugLog("BlockCallHttpServlet end to call RPC");
    stub.query_age(&rpc_controller, &rpc_req, &rpc_res, NULL);
    AppDebugLog("BlockCallHttpServlet end to call RPC");

    if (rpc_controller.ErrorCode() != 0) {
      char buf[512];
      sprintf(buf, html, "failed to call QueryServer rpc server");
      setHttpBody(res, std::string(buf));
      return;
    }

    if (rpc_res.ret_code() != 0) {
      std::stringstream ss;
      ss << "QueryServer rpc server return bad result, ret = " << rpc_res.ret_code() << ", and res_info = " << rpc_res.res_info();
      AppDebugLog(ss.str().c_str());
      char buf[512];
      sprintf(buf, html, ss.str().c_str());
      setHttpBody(res, std::string(buf));
      return;
    }

    std::stringstream ss;
    ss << "Success!! Your age is," << rpc_res.age() << " and Your id is " << rpc_res.id();

    char buf[512];
    sprintf(buf, html, ss.str().c_str());
    setHttpBody(res, std::string(buf));

  }

  std::string GetServletName() {
    return "BlockCallHttpServlet";
  }

};