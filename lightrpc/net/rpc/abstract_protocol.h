#ifndef LIGHTRPC_NET_RPC_ABSTRACT_PROTOCOL_H
#define LIGHTRPC_NET_RPC_ABSTRACT_PROTOCOL_H

#include <memory>
#include <string>

namespace lightrpc {
// std::enable_shared_from_this 能让其一个对象（假设其名为 t ，且已被一个 std::shared_ptr 对象 pt 管理）安全地生成其他额外的 std::shared_ptr 实例（假设名为 pt1, pt2, … ），它们与 pt 共享对象 t 的所有权。
struct AbstractProtocol : public std::enable_shared_from_this<AbstractProtocol> {
 public:
  typedef std::shared_ptr<AbstractProtocol> s_ptr;

  virtual ~AbstractProtocol() {}

 public:
  std::string m_msg_id_;     // 请求号，唯一标识一个请求或者响应
};
}
#endif