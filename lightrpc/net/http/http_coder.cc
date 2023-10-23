#include "http_coder.h"
#include "http_protocol.h"
#include <algorithm>
#include <cstdio>

namespace lightrpc {
    // 将 message 对象转化为字节流，写入到 buffer
    void HttpCoder::Encode(std::vector<AbstractProtocol::s_ptr>& messages, TcpBuffer::s_ptr out_buffer){
        for (auto &i : messages) {
            std::stringstream ss;
            std::string http_content;
            if(i->http_type_ == HttpType::RESPONSE){
                std::shared_ptr<HttpResponse> response = std::dynamic_pointer_cast<HttpResponse>(i);
                response->protocol_ = ProtocalType::HTTP;

                ss << response->m_response_version_ << " " << response->m_response_code_ << " "
                    << response->m_response_info_ << "\r\n" << response->m_response_header_.ToHttpString()
                    << "\r\n" << response->m_response_body_;
                http_content = ss.str();
                LOG_DEBUG("msg_id = %s | succ encode and write http response [%s] to buffer, writeindex = %d", i->m_msg_id_.c_str(), response->m_msg_id_.c_str(), out_buffer->WriteIndex());
            }else{
                std::shared_ptr<HttpRequest> resquest = std::dynamic_pointer_cast<HttpRequest>(i);
                resquest->protocol_ = ProtocalType::HTTP;
               
                ss << resquest->m_request_method_ << " " << resquest->m_request_path_ << " "
                    << resquest->m_request_version_ << "\r\n" << resquest->m_requeset_header_.ToHttpString()
                    << "\r\n" << resquest->m_request_body_;
                http_content = ss.str();
                LOG_DEBUG("msg_id = %s | succ encode and write http request [%s] to buffer, writeindex = %d", i->m_msg_id_.c_str(), resquest->m_msg_id_.c_str(), out_buffer->WriteIndex());
            }
            
            // 将响应写入写缓冲区
            out_buffer->WriteToBuffer(http_content.c_str(), http_content.length());
        }
    }

    // 将 buffer 里面的字节流转换为 message 对象
    void HttpCoder::Decode(std::vector<AbstractProtocol::s_ptr>& out_messages, TcpBuffer::s_ptr buffer){
        while (1) {
            std::string strs = buffer->GetBufferString();
            if(strs.empty()){ break; }
            // 记录已读的长度
            int read_size = 0;
            std::string tmp(strs);
            int len = tmp.length();
            bool parse_success = false;
            bool is_parse_request_line = false;
            bool is_parse_request_header = false;
            bool is_parse_request_content = false;
            std::shared_ptr<HttpRequest> request = std::make_shared<HttpRequest>();
            request->protocol_ = ProtocalType::HTTP;
            // 生成个msg_id，便于跟踪
            request->m_msg_id_ = MsgIDUtil::GenMsgID();
            if (!request) {
                LOG_ERROR("not httprequest type");
                return;
            }

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
                LOG_DEBUG("parse http request[%s] success, read size is %d bytes", request->m_msg_id_.c_str(), read_size);
                buffer->MoveReadIndex(read_size);
                out_messages.push_back(request);
                continue;
            }
        }
    }

    // 解析http请求
    bool HttpCoder::ParseHttpRequestLine(std::shared_ptr<HttpRequest> requset, const std::string& tmp){
        LOG_DEBUG("request line: %s", tmp.c_str());
        std::istringstream lineStream(tmp);
        std::string method;
        std::string rquest_resourse;
        std::string http_version;
        lineStream >> method;
        lineStream >> rquest_resourse;
        lineStream >> http_version;
        // 获取请求方法
        std::transform(method.begin(), method.end(), method.begin(), toupper);
        std::transform(method.begin(), method.end(), method.begin(), toupper);
        if (method == "GET") {
            requset->m_request_method_ = HttpMethod::GET;
        } else if (method == "POST") {
            requset->m_request_method_ = HttpMethod::POST;
        } else {
            LOG_ERROR("parse http request request line error, not support http method: %s", method)
            return false;
        }
        // 获取http版本
        std::transform(http_version.begin(), http_version.end(), http_version.begin(), toupper);
        if (http_version != "HTTP/1.1" && http_version != "HTTP/1.0") {
            LOG_ERROR("parse http request request line error, not support http version: %s", http_version.c_str());
            return false;
        }
        requset->m_request_version_ = http_version;
        // 获取请求资源
        int l = 0;
        size_t j;
        l = rquest_resourse.length();
        j = rquest_resourse.find_first_of("?");
        if (j == rquest_resourse.npos) {
            requset->m_request_path_ = rquest_resourse;
            LOG_DEBUG("http request path: %s and query is empty", requset->m_request_path_.c_str());
            return true;
        }
        requset->m_request_path_ = rquest_resourse.substr(0, j);
        requset->m_request_query_ = rquest_resourse.substr(j + 1, l - j - 1);
        LOG_DEBUG("http request path: %s and query is %s", requset->m_request_path_.c_str(), requset->m_request_query_.c_str());
        SplitStrToMap(requset->m_request_query_, "&", "=", requset->m_query_maps_);

        return true;
    }

    bool HttpCoder::ParseHttpRequestHeader(std::shared_ptr<HttpRequest> requset, const std::string& tmp){
        if (tmp.empty() || tmp.length() < 4 || tmp == "\r\n\r\n") {
            return true;
        }
        SplitStrToMap(tmp, "\r\n", ":", requset->m_requeset_header_.m_maps_);
        return true;
    }

    bool HttpCoder::ParseHttpRequestContent(std::shared_ptr<HttpRequest> requset, const std::string& tmp){
        if (tmp.empty()) {
            return true;
        }
        requset->m_request_body_ = tmp;
        return true;
    }
}
