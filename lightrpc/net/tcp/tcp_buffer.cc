#include <memory>
#include <string.h>
#include "../../common/log.h"
#include "tcp_buffer.h"

namespace lightrpc {
TcpBuffer::TcpBuffer(int size) : m_size_(size) {
  m_buffer_.resize(size);
}

std::string TcpBuffer::GetBufferString(){
  std::string re(readAble(), '0');
  memcpy(&re[0],  &m_buffer[m_read_index], ReadAble());
  return re;
}

// 返回可读字节数
int TcpBuffer::ReadAble() {
  return m_write_index_ - m_read_index_;
}

// 返回可写的字节数
int TcpBuffer::WriteAble() {
  return m_buffer_.size() - m_write_index_;
}

int TcpBuffer::ReadIndex() {
  return m_read_index_;
}

int TcpBuffer::WriteIndex() {
  return m_write_index_;
}

void TcpBuffer::WriteToBuffer(const char* buf, int size) {
  if (size > WriteAble()) {
    // 调整 buffer 的大小，扩容
    int new_size = (int)(1.5 * (m_write_index_ + size));
    ResizeBuffer(new_size);
  }
  memcpy(&m_buffer_[m_write_index_], buf, size);
  m_write_index_ += size; 
}


void TcpBuffer::ReadFromBuffer(std::vector<char>& re, int size) {
  if (ReadAble() == 0) {
    return;
  }
  // 确定可读的大小
  int read_size = ReadAble() > size ? size : ReadAble();
  std::vector<char> tmp(read_size);
  memcpy(&tmp[0], &m_buffer_[m_read_index_], read_size);

  re.swap(tmp); 
  m_read_index_ += read_size;

  AdjustBuffer();
}

void TcpBuffer::ResizeBuffer(int new_size) {
  std::vector<char> tmp(new_size);
  int count = std::min(new_size, ReadAble());
  
  memcpy(&tmp[0], &m_buffer_[m_read_index_], count);
  m_buffer_.swap(tmp);

  m_read_index_ = 0;
  m_write_index_ = m_read_index_ + count;
}

void TcpBuffer::AdjustBuffer() {
  // 调整m_read_index_的位置，去除已经读取的内容
  if (m_read_index_ < int(m_buffer_.size() / 3)) {
    return;
  }
  std::vector<char> buffer(m_buffer_.size());
  int count = ReadAble();

  memcpy(&buffer[0], &m_buffer_[m_read_index_], count);
  m_buffer_.swap(buffer);
  m_read_index_ = 0;
  m_write_index_ = m_read_index_ + count;

  buffer.clear();
}

void TcpBuffer::MoveReadIndex(int size) {
  size_t j = m_read_index_ + size;
  if (j >= m_buffer_.size()) {
    LOG_ERROR("moveReadIndex error, invalid size %d, old_read_index %d, buffer size %d", size, m_read_index_, m_buffer_.size());
    return;
  }
  m_read_index_ = j;
  AdjustBuffer();
}

void TcpBuffer::MoveWriteIndex(int size) {
  size_t j = m_write_index_ + size;
  if (j >= m_buffer_.size()) {
    LOG_ERROR("moveWriteIndex error, invalid size %d, old_read_index %d, buffer size %d", size, m_read_index_, m_buffer_.size());
    return;
  }
  m_write_index_ = j;
  AdjustBuffer();
}
}