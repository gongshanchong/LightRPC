#ifndef LIGHTRPC_NET_HTTP_HTTP_PROTOCOL_H
#define LIGHTRPC_NET_HTTP_HTTP_PROTOCOL_H

#include <string>
#include <memory>
#include <map>
#include <vector>

#include "../../common/log.h"
#include "../rpc/abstract_protocol.h"

namespace lightrpc {

enum HttpMethod {
  GET = 1,
  POST = 2, 
};

enum HttpCode {
  HTTP_OK = 200,
  HTTP_BADREQUEST = 400,
  HTTP_FORBIDDEN = 403,
  HTTP_NOTFOUND = 404,
  HTTP_INTERNALSERVERERROR = 500,
};

extern std::string g_CRLF;
extern std::string g_CRLF_DOUBLE;

extern std::string content_type_text;
extern const char* default_html_template;

// http协议的头部信息
class HttpHeaderComm {
public:
    HttpHeaderComm() = default;

    virtual ~HttpHeaderComm() = default;
    // 获取http头的长度
    int GetHeaderTotalLength();
    // 获取http头的中key对应的内容
    std::string GetValue(const std::string& key);
    // 对http头中的数据进行存储
    void SetKeyValue(const std::string& key, const std::string& value);
    // 将http头转换为字符串
    std::string ToHttpString();

public:
    // 将http头以字典的形式进行存储
    std::map<std::string, std::string> m_maps_;
};

class HttpRequest : public AbstractProtocol {
public:
    typedef std::shared_ptr<HttpRequest> ptr; 

public:
    HttpMethod m_request_method_;       // 请求的方法 
    std::string m_request_path_;        // 请求行请求的路径
    std::string m_request_query_;       // 请求内容
    std::string m_request_version_;     // 请求的http版本
    HttpHeaderComm m_requeset_header_;  // 请求头
    std::string m_request_body_;        // 请求体

    std::map<std::string, std::string> m_query_maps_;
};

class HttpResponse : public AbstractProtocol {
public:
    typedef std::shared_ptr<HttpResponse> ptr; 

public:
    std::string m_response_version_;    // 响应版本
    int m_response_code_;               // 响应码
    std::string m_response_info_;       // 响应消息
    HttpHeaderComm m_response_header_;  // 响应头
    std::string m_response_body_;       // 响应体
};

// 将状态码转换为字符串
const char* HttpCodeToString(const int code);

// split a string to map
// for example:  str is a=1&tt=2&cc=3  split_str = '&' joiner='=' 
// get res is {"a":"1", "tt":"2", "cc", "3"}
void SplitStrToMap(const std::string& str, const std::string& split_str, 
const std::string& joiner, std::map<std::string, std::string>& res);

// split a string to vector 
// for example:  str is a=1&tt=2&cc=3  split_str = '&' 
// get res is {"a=1", "tt=2", "cc=3"}
void SplitStrToVector(const std::string& str, const std::string& split_str, 
std::vector<std::string>& res);

void SetHttpCode(std::shared_ptr<HttpResponse> res, const int code);
  
void SetHttpContentType(std::shared_ptr<HttpResponse> res, const std::string& content_type);

void SetHttpBody(std::shared_ptr<HttpResponse> res, const std::string& body);

void SetCommParam(std::shared_ptr<HttpRequest> req, std::shared_ptr<HttpResponse> res);

void SetNotFoundHttp(std::shared_ptr<HttpResponse> res);

void SetInternalErrorHttp(std::shared_ptr<HttpResponse> res, const std::string error_info);

}

#endif