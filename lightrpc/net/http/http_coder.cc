#include "http_coder.h"
#include <algorithm>

namespace lightrpc {
    // 将 message 对象转化为字节流，写入到 buffer
    void HttpCoder::Encode(std::vector<AbstractProtocol::s_ptr>& messages, TcpBuffer::s_ptr out_buffer){
        HttpResponse* response = dynamic_cast<HttpResponse*>(messages);
        response->encode_succ = false;

        std::stringstream ss;
        ss << response->m_response_version << " " << response->m_response_code << " "
            << response->m_response_info << "\r\n" << response->m_response_header.toHttpString()
            << "\r\n" << response->m_response_body;
        std::string http_res = ss.str();
        DebugLog << "encode http response is:  " << http_res;  

        buf->writeToBuffer(http_res.c_str(), http_res.length());
        DebugLog << "succ encode and write to buffer, writeindex=" << buf->writeIndex();
        response->encode_succ = true;
        DebugLog << "test encode end";
    }

    // 将 buffer 里面的字节流转换为 message 对象
    void HttpCoder::Decode(std::vector<AbstractProtocol::s_ptr>& out_messages, TcpBuffer::s_ptr buffer){

    }

    // 获取协议类型
    ProtocalType HttpCoder::GetProtocalType(){
        return Http_Protocal;
    }

    // 解析http请求
    bool HttpCoder::ParseHttpRequestLine(HttpRequest* requset, const std::string& tmp){
        size_t s1 = tmp.find_first_of(" ");
        size_t s2 = tmp.find_last_of(" ");

        if (s1 == tmp.npos || s2 == tmp.npos || s1 == s2) {
            LOG_ERROR("error read Http Requser Line, space is not 2");
            return false;
        }
        // 获取请求方法
        std::string method = tmp.substr(0, s1);
        std::transform(method.begin(), method.end(), method.begin(), toupper);
        if (method == "GET") {
            requset->m_request_method_ = HttpMethod::GET;
        } else if (method == "POST") {
            requset->m_request_method_ = HttpMethod::POST;
        } else {
            LOG_ERROR("parse http request request line error, not support http method: %s", method)
            return false;
        }
        // 获取请求版本
        std::string version = tmp.substr(s2 + 1, tmp.length() - s2 - 1);
        std::transform(version.begin(), version.end(), version.begin(), toupper);
        if (version != "HTTP/1.1" && version != "HTTP/1.0") {
            LOG_ERROR("parse http request request line error, not support http version: %s", version);
            return false;
        }
        requset->m_request_version_ = version;
        // 获取请求行中的请求url
        std::string url = tmp.substr(s1 + 1, s2 - s1 - 1);
        size_t j = url.find("://");

        if (j != url.npos && j + 3 >= url.length()) {
            LOG_ERROR("parse http request request line error, bad url: %s", url);
            return false;
        }
        int l = 0;
        // 请求根路径，或者根路径下的其他
        if (j == url.npos) {
            LOG_DEBUG("url only have path, url is %s", url);
        } else {
            url = url.substr(j + 3, s2 - s1  - j - 4);
            LOG_DEBUG("delete http prefix, url = %s", url);
            j = url.find_first_of("/");
            l = url.length();
            if (j == url.npos || j == url.length() - 1) {
                LOG_DEBUG("http request root path, and query is empty");
                return true;
            }
            url = url.substr(j + 1, l - j - 1);
        }
        // 请求根路径下的其他
        l = url.length();
        j = url.find_first_of("?");
        if (j == url.npos) {
            requset->m_request_path_ = url;
            LOG_DEBUG("http request path: %s and query is empty", requset->m_request_path_);
            return true;
        }
        requset->m_request_path_ = url.substr(0, j);
        requset->m_request_query_ = url.substr(j + 1, l - j - 1);
        LOG_DEBUG("http request path: %s and query is %s", requset->m_request_path_, requset->m_request_query_);
        SplitStrToMap(requset->m_request_query_, "&", "=", requset->m_query_maps_);
        return true;
    }

    bool HttpCoder::ParseHttpRequestHeader(HttpRequest* requset, const std::string& tmp){
        if (tmp.empty() || tmp.length() < 4 || tmp == "\r\n\r\n") {
            return true;
        }
        SplitStrToMap(tmp, "\r\n", ":", requset->m_requeset_header_.m_maps_);
        return true;
    }

    bool HttpCoder::ParseHttpRequestContent(HttpRequest* requset, const std::string& tmp){
        if (tmp.empty()) {
            return true;
        }
        requset->m_request_body_ = tmp;
        return true;
    }
}