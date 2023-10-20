#ifndef LIGHTRPC_NET_HTTP_HTTP_SERVLET_H
#define LIGHTRPC_NET_HTTP_HTTP_SERVLET_H

#include <memory>
#include "http_protocol.h"
#include "../../common/log.h"

namespace lightrpc {

class HttpServlet : public std::enable_shared_from_this<HttpServlet> {
 public:
  typedef std::shared_ptr<HttpServlet> ptr;

  HttpServlet() = default;

  virtual ~HttpServlet();

  virtual void Handle(std::shared_ptr<HttpRequest> req, std::shared_ptr<HttpResponse> res) = 0;

  virtual std::string GetServletName() = 0;

  void SetHttpCode(std::shared_ptr<HttpResponse> res, const int code);
  
  void SetHttpContentType(std::shared_ptr<HttpResponse> res, const std::string& content_type);
  
  void SetHttpBody(std::shared_ptr<HttpResponse> res, const std::string& body);

  void SetCommParam(std::shared_ptr<HttpRequest> req, std::shared_ptr<HttpResponse> res);

};

class NotFoundHttpServlet: public HttpServlet {
 public:

  NotFoundHttpServlet() = default;

  ~NotFoundHttpServlet() = default;

  void Handle(std::shared_ptr<HttpRequest> req, std::shared_ptr<HttpResponse> res);

  void HandleNotFound(std::shared_ptr<HttpRequest> req, std::shared_ptr<HttpResponse> res);

  std::string GetServletName();
};

}

#endif