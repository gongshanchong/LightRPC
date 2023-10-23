#ifndef LIGHTRPC_NET_HTTP_HTTP_CODER_H
#define LIGHTRPC_NET_HTTP_HTTP_CODER_H

#include <map>
#include <string>
#include <sstream>
#include "../rpc/abstract_protocol.h"
#include "../rpc/abstract_coder.h"
#include "../../common/log.h"
#include "../../common/msg_id_util.h"
#include "http_protocol.h"

namespace lightrpc {

class HttpCoder : public AbstractCoder {
public:
    HttpCoder() = default;

    ~HttpCoder() = default;

    // 将 message 对象转化为字节流，写入到 buffer
    void Encode(std::vector<AbstractProtocol::s_ptr>& messages, TcpBuffer::s_ptr out_buffer);

    // 将 buffer 里面的字节流转换为 message 对象
    void Decode(std::vector<AbstractProtocol::s_ptr>& out_messages, TcpBuffer::s_ptr buffer);

private:
    // 解析http请求
    bool ParseHttpRequest(std::vector<AbstractProtocol::s_ptr>& out_messages, TcpBuffer::s_ptr buffer, const std::string& tmp);
    bool ParseHttpRequestLine(std::shared_ptr<HttpRequest> requset, const std::string& tmp);
    bool ParseHttpRequestHeader(std::shared_ptr<HttpRequest> requset, const std::string& tmp);
    bool ParseHttpRequestContent(std::shared_ptr<HttpRequest> requset, const std::string& tmp);
    // 解析http响应
    bool ParseHttpResponse(std::vector<AbstractProtocol::s_ptr>& out_messages, TcpBuffer::s_ptr buffer, const std::string& tmp);
    bool ParseHttpResponseLine(std::shared_ptr<HttpResponse> response, const std::string& tmp);
    bool ParseHttpResponseHeader(std::shared_ptr<HttpResponse> response, const std::string& tmp);
    bool ParseHttpResponseContent(std::shared_ptr<HttpResponse> response, const std::string& tmp);
};

} 


#endif