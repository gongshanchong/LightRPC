#include <memory>
#include "http_servlet.h"

namespace lightrpc {

extern const char* default_html_template;
extern std::string content_type_text;

void HttpServlet::SetHttpCode(std::shared_ptr<HttpResponse> res, const int code) {
    res->m_response_code_ = code;
    res->m_response_info_ = std::string(HttpCodeToString(code));
}

void HttpServlet::SetHttpContentType(std::shared_ptr<HttpResponse> res, const std::string& content_type) {
    res->m_response_header_.m_maps_["Content-Type"] = content_type;
}

void HttpServlet::SetHttpBody(std::shared_ptr<HttpResponse> res, const std::string& body) {
    res->m_response_body_ = body;
    res->m_response_header_.m_maps_["Content-Length"]= std::to_string(res->m_response_body_.length());
}

void HttpServlet::SetCommParam(std::shared_ptr<HttpRequest> req, std::shared_ptr<HttpResponse> res) {
    LOG_DEBUG("set response version = %s", req->m_request_version_);
    res->m_response_version_ = req->m_request_version_;
    res->m_response_header_.m_maps_["Connection"]= req->m_requeset_header_.m_maps_["Connection"];
}

void NotFoundHttpServlet::Handle(std::shared_ptr<HttpRequest> req, std::shared_ptr<HttpResponse> res) {
    HandleNotFound(req, res);
}

void NotFoundHttpServlet::HandleNotFound(std::shared_ptr<HttpRequest> req, std::shared_ptr<HttpResponse> res) {
    LOG_DEBUG("return 404 html");
    SetHttpCode(res, HTTP_NOTFOUND);
    char buf[512];
    sprintf(buf, default_html_template, std::to_string(HTTP_NOTFOUND).c_str(), HttpCodeToString(HTTP_NOTFOUND));
    res->m_response_body_ = std::string(buf);
    res->m_response_header_.m_maps_["Content-Type"] = content_type_text;
    res->m_response_header_.m_maps_["Content-Length"]= std::to_string(res->m_response_body_.length());
}

std::string NotFoundHttpServlet::GetServletName() {
    return "NotFoundHttpServlet";
}

}