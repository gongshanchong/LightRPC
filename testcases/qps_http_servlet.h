#include "../lightrpc/common/log.h"
#include "../order.pb.h"

class QPSHttpServlet : public lightrpc::HttpServlet {
 public:
  QPSHttpServlet() = default;
  ~QPSHttpServlet() = default;

  void Handle(std::shared_ptr<lightrpc::HttpRequest> req, std::shared_ptr<lightrpc::HttpResponse> res) {
    LOG_APPDEBUG("QPSHttpServlet get request");
    SetHttpCode(res, lightrpc::HTTP_OK);
    SetHttpContentType(res, "text/html;charset=utf-8");

    std::stringstream ss;
    ss << "QPSHttpServlet Echo Success!! Your id is," << req->m_query_maps_["id"];
    char buf[512];
    sprintf(buf, lightrpc::default_html_template, "Hello!", ss.str().c_str());
    SetHttpBody(res, std::string(buf));
    LOG_APPDEBUG(ss.str().c_str());
  }

  std::string getServletName() {
    return "QPSHttpServlet";
  }

};
