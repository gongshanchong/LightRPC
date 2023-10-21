#include "http_protocol.h"
#include <string>
#include <sstream>

namespace lightrpc {

std::string g_CRLF = "\r\n";
std::string g_CRLF_DOUBLE = "\r\n\r\n";

std::string content_type_text = "text/html;charset=utf-8";
const char* default_html_template = "<html><body><h1>%s</h1><p>%s</p></body></html>";

std::string HttpHeaderComm::GetValue(const std::string& key) {
    return m_maps_[key.c_str()];
}

int HttpHeaderComm::GetHeaderTotalLength() {
    int len = 0;
    for (auto it : m_maps_) {
        len += it.first.length() + 1 + it.second.length() + 2;
    }
    return len;
}

void HttpHeaderComm::SetKeyValue(const std::string& key, const std::string& value) {
    m_maps_[key.c_str()] = value;
}

std::string HttpHeaderComm::ToHttpString() {
    std::stringstream ss;
    for (auto it : m_maps_) {
        ss << it.first << ":" << it.second << "\r\n";
    }
    return ss.str();
}

const char* HttpCodeToString(const int code) {
    switch (code)
    {
    case HTTP_OK: 
        return "OK"; 
    
    case HTTP_BADREQUEST:
        return "Bad Request";
    
    case HTTP_FORBIDDEN:
        return "Forbidden";
    
    case HTTP_NOTFOUND:
        return "Not Found";
    
    case HTTP_INTERNALSERVERERROR:
        return "Internal Server Error";
    
    default:
        return "UnKnown code";
    }
}

void SplitStrToMap(const std::string& str, const std::string& split_str, 
    const std::string& joiner, std::map<std::string, std::string>& res) {

  if (str.empty() || split_str.empty() || joiner.empty()) {
    LOG_DEBUG("str or split_str or joiner_str is empty");
    return;
  }
  std::string tmp = str;

  std::vector<std::string> vec;
  SplitStrToVector(tmp, split_str, vec);
  for (auto i : vec) {
    if (!i.empty()) {
      size_t j = i.find_first_of(joiner);
      if (j != i.npos && j != 0) {
        std::string key = i.substr(0, j);
        std::string value = i.substr(j + joiner.length(), i.length() - j - joiner.length());
        LOG_DEBUG("insert key = %s, value = %s", key, value);
        res[key.c_str()] = value;
      }
    }
  }

}

void SplitStrToVector(const std::string& str, const std::string& split_str, 
    std::vector<std::string>& res) {

  if (str.empty() || split_str.empty()) {
    LOG_DEBUG("str or split_str is empty");
    return;
  }
  std::string tmp = str;
  if (tmp.substr(tmp.length() - split_str.length(), split_str.length()) != split_str) {
    tmp += split_str;
  }

  while (1) {
    size_t i = tmp.find_first_of(split_str);
    if (i == tmp.npos) {
      return;
    }
    int l = tmp.length();
    std::string x = tmp.substr(0, i);
    tmp = tmp.substr(i + split_str.length(), l - i - split_str.length());
    if (!x.empty()) {
      res.push_back(std::move(x));
    }
  }

}

void SetHttpCode(std::shared_ptr<HttpResponse> res, const int code) {
    res->m_response_code_ = code;
    res->m_response_info_ = std::string(HttpCodeToString(code));
}

void SetHttpContentType(std::shared_ptr<HttpResponse> res, const std::string& content_type) {
    res->m_response_header_.m_maps_["Content-Type"] = content_type;
}

void SetHttpBody(std::shared_ptr<HttpResponse> res, const std::string& body) {
    res->m_response_body_ = body;
    res->m_response_header_.m_maps_["Content-Length"]= std::to_string(res->m_response_body_.length());
}

void SetCommParam(std::shared_ptr<HttpRequest> req, std::shared_ptr<HttpResponse> res) {
    LOG_DEBUG("set response version = %s", req->m_request_version_);
    res->m_response_version_ = req->m_request_version_;
    res->m_response_header_.m_maps_["Connection"]= req->m_requeset_header_.m_maps_["Connection"];
}

void SetNotFoundHttp(std::shared_ptr<HttpResponse> res){
  SetHttpCode(res, HTTP_NOTFOUND);
  SetHttpContentType(res, content_type_text);
  char buf[512];
  sprintf(buf, default_html_template, std::to_string(HTTP_NOTFOUND).c_str(), HttpCodeToString(HTTP_NOTFOUND));
  SetHttpBody(res, buf);
}

void SetInternalErrorHttp(std::shared_ptr<HttpResponse> res, const std::string error_info){
  SetHttpCode(res, HTTP_INTERNALSERVERERROR);
  SetHttpContentType(res, content_type_text);
  char buf[512];
  sprintf(buf, default_html_template, std::to_string(HTTP_INTERNALSERVERERROR).c_str(), error_info.c_str());
  SetHttpBody(res, buf);
}

}