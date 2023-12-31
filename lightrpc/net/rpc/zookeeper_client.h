#pragma once

#include <semaphore.h>
#include <zookeeper/zookeeper.h>
#include <string>
#include "../../common/log.h"
#include "../tcp/net_addr.h"

namespace lightrpc {

class ZookeeperClient
{
public:
    ZookeeperClient();
    ~ZookeeperClient();

    //启动连接--》zkserver
    void start(lightrpc::NetAddr::s_ptr zookeeper_addr);
    //在zkserver 根据指定的path创建znode节点
    void create(const char *path, const char *data, int datalen, int state = 0);
    //根据参数指定的znode节点路径，获取znode节点的值
    std::string get_data(const char *path);

private:
    //zk的客户端句柄
    zhandle_t *zhandle_;
};
}