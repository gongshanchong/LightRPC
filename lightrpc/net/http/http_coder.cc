#include "http_coder.h"
#include <algorithm>

namespace lightrpc {
    // 将 message 对象转化为字节流，写入到 buffer
    void HttpCoder::Encode(std::vector<AbstractProtocol::s_ptr>& messages, TcpBuffer::s_ptr out_buffer){
        for (auto &i : messages) {
            std::shared_ptr<HttpResponse> response =std::dynamic_pointer_cast<HttpResponse>(i);

            std::stringstream ss;
            ss << response->m_response_version_ << " " << response->m_response_code_ << " "
                << response->m_response_info_ << "\r\n" << response->m_response_header_.ToHttpString()
                << "\r\n" << response->m_response_body_;
            std::string http_res = ss.str();
            LOG_DEBUG("msg_id = %s", message->m_msg_id.c_str());
    
            buf->WriteToBuffer(http_res.c_str(), http_res.length());
            LOG_DEBUG("succ encode and write http response [%s] to buffer, writeindex = %d", message->m_msg_id.c_str(), buf->writeIndex());
        }
    }

    // 将 buffer 里面的字节流转换为 message 对象
    void HttpCoder::Decode(std::vector<AbstractProtocol::s_ptr>& out_messages, TcpBuffer::s_ptr buffer){
        std::string strs = "";
        if (!buf || !data) {
            LOG_ERROR("decode error! buf or data nullptr");
            return;
        }
        std::shared_ptr<HttpRequest> request = std::make_shared<HttpRequest>(); 
        if (!request) {
            LOG_ERROR("not httprequest type");
            return;
        }
        
        strs = buf->GetBufferString();
        bool is_parse_request_line = false;
        bool is_parse_request_header = false;
        bool is_parse_request_content = false;
        int read_size = 0;
        std::string tmp(strs);
        int len = tmp.length();
        LOG_DEBUG("pending to parse str: %s, total size = %d", tmp, len);
        
        while (1) {
            // 解析请求行
            if (!is_parse_request_line) {
                size_t i = tmp.find(g_CRLF);
                if (i == tmp.npos) {
                    LOG_DEBUG("not found CRLF in buffer");
                    return;
                }
                if (i == tmp.length() - 2) {
                    LOG_DEBUG("need to read more data");
                    break;
                }
                is_parse_request_line = ParseHttpRequestLine(request, tmp.substr(0, i));
                if (!is_parse_request_line) {
                    return;
                }    
                tmp = tmp.substr(i + 2, len - 2 - i);
                len = tmp.length();
                read_size = read_size + i + 2;
            }
            // 解析请求头
            if (!is_parse_request_header) {
                size_t j  = tmp.find(g_CRLF_DOUBLE);
                if (j == tmp.npos) {
                    LOG_DEBUG("not found CRLF CRLF in buffer");
                    return;
                }
                is_parse_request_header = ParseHttpRequestHeader(request, tmp.substr(0, j));
                if (!is_parse_request_header) {
                    return;
                }
                tmp = tmp.substr(j + 4, len - 4 - j);
                len = tmp.length();
                read_size = read_size + j + 4;
            }
            // 解析请求体
            if (!is_parse_request_content) {
                int content_len = std::atoi(request->m_requeset_header_.m_maps_["Content-Length"].c_str());
                if ((int)strs.length() - read_size < content_len) {
                    LOG_DEBUG("need to read more data");
                    return;
                }
                if (request->m_request_method_ == POST && content_len != 0) {
                    is_parse_request_content = ParseHttpRequestContent(request, tmp.substr(0, content_len));
                    if (!is_parse_request_content) {
                        return;
                    }
                    read_size = read_size + content_len;
                  } else {
                    is_parse_request_content = true;
                  }
            }
            if (is_parse_request_line && is_parse_request_header && is_parse_request_header) {
                LOG_DEBUG("parse http request success, read size is %d bytes", read_size);
                buf->recycleRead(read_size);
                break;
            }
        }
        
        data = request;
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