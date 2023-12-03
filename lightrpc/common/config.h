#ifndef LIGHTRPC_COMMON_CONFIG_H
#define LIGHTRPC_COMMON_CONFIG_H

#include <map>
#include <string>
#include <algorithm>
#include <tinyxml/tinyxml.h>
#include "../net/tcp/net_addr.h"
#include "../net/rpc/abstract_protocol.h"


namespace lightrpc {

struct RpcStub {
  std::string name_;
  NetAddr::s_ptr addr_;
  ProtocalType protocal_;
  int timeout_ {2000};
};

struct RpcZookeeper {
  std::string name_;
  NetAddr::s_ptr addr_;
};

class Config {
public:
  Config(const char* xmlfile);
  Config();

  ~Config();

public:
  static Config* GetGlobalConfig();
  static void SetGlobalConfig(const char* xmlfile);

public:
  // 日志的设置
  int m_log_type_;               // 日志是否输出到文件
  std::string m_log_level_;      // 日志级别
  std::string m_log_file_name_;  // 日志名称
  std::string m_log_file_path_;  // 日志目录
  int m_log_max_file_size_ {0};  // 单个日志文件最大大小
  int m_log_sync_inteval_ {0};   // 日志同步间隔，ms
  // 线程数
  int m_io_threads_ {0};

  TiXmlDocument* m_xml_document_{NULL};
  // 服务配置中心
  std::map<std::string, RpcZookeeper> m_rpc_zookeepers_;
  // rpc服务的名字及其对应的地址和超时设置
  std::map<std::string, RpcStub> m_rpc_stubs_;
};

}

#endif