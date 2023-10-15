#ifndef ROCKET_NET_TCP_TCP_BUFFER_H
#define ROCKET_NET_TCP_TCP_BUFFER_H

#include <vector>
#include <memory>

namespace rocket {

class TcpBuffer {
 public:
  typedef std::shared_ptr<TcpBuffer> s_ptr;

  TcpBuffer(int size);

  // 返回可读字节数
  int readAble();

  // 返回可写的字节数
  int writeAble();

  int readIndex();

  int writeIndex();

  void writeToBuffer(const char* buf, int size);

  void readFromBuffer(std::vector<char>& re, int size);

  void resizeBuffer(int new_size);

  void adjustBuffer();

  void moveReadIndex(int size);

  void moveWriteIndex(int size);

 private:
  int m_read_index {0};
  int m_write_index {0};
  int m_size {0};

 public:
  // 在buffer上维护读写索引，每次写都判断是否需要扩容，每次读都将读后的内容删除
  std::vector<char> m_buffer;
};
}
#endif