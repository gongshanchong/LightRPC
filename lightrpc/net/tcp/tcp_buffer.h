#ifndef LIGHTRPC_NET_TCP_TCP_BUFFER_H
#define LIGHTRPC_NET_TCP_TCP_BUFFER_H

#include <vector>
#include <memory>

namespace lightrpc {

class TcpBuffer {
 public:
  typedef std::shared_ptr<TcpBuffer> s_ptr;

  TcpBuffer(int size);

  // 返回可读字节数
  int ReadAble();

  // 返回可写的字节数
  int WriteAble();

  int ReadIndex();

  int WriteIndex();

  void WriteToBuffer(const char* buf, int size);

  void ReadFromBuffer(std::vector<char>& re, int size);

  void ResizeBuffer(int new_size);

  void AdjustBuffer();

  void MoveReadIndex(int size);

  void MoveWriteIndex(int size);

 private:
  int m_read_index_ {0};
  int m_write_index_ {0};
  int m_size_ {0};

 public:
  // 在buffer上维护读写索引，每次写都判断是否需要扩容，每次读都将读后的内容删除
  std::vector<char> m_buffer;
};
}
#endif