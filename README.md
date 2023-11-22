# A C++ Light Remote Procedure Call（LightRPC）

本项目为C++11编写的小型远程调用框架，基于**主从Reactor架构**，底层采用**epoll**实现IO多路复用。应用层使用**http协议**和**自定义的tinyrpc协议**通过**protobuf**进行通信，通过**zookeeper**实现**服务发现**。

## 目录

|      Part Ⅰ      |      Part Ⅱ      |      Part Ⅲ      |      Part Ⅳ      |            Part Ⅴ            |   Part Ⅵ   |
| :----------------: | :----------------: | :----------------: | :----------------: | :----------------------------: | :---------: |
| [环境配置](#环境配置) | [前置知识](#前置知识) | [整体框架](#整体框架) | [各个模块](#各个模块) | [使用及功能展示](#使用及功能展示) | [遇到的问题]() |

## 环境配置

本项目使用了智能指针，如std::make_unique，还有其他一些C++新特性，本项目的环境部署在腾讯云，使用版本如下：

1. 服务端环境：
   * C++17及以上
   * ubuntu 20.04，CPU 2核 - 内存 2GB
2. 依赖库安装：
   1. protobuf：[protobuf安装](https://zhuanlan.zhihu.com/p/631291781)，安装到/usr/local目录
   2. tinyxml：
      要生成 libtinyxml.a 静态库，需要简单修改 makefile 如下:

      ```
      # 84 行修改为如下
      OUTPUT := libtinyxml.a

      # 93 行修改为如下
      SRCS := tinyxml.cpp tinyxmlparser.cpp tinyxmlerror.cpp tinystr.cpp 

      # 注释掉129行# 93 行xmltest.o: tinyxml.h tinystr.h

      # 104行修改如下
      ${OUTPUT}: ${OBJS}
          ${AR} $@ ${LDFLAGS} ${OBJS} ${LIBS} ${EXTRA_LIBS}
      ```

      安装过程如下：

      ```
      wget https://udomain.dl.sourceforge.net/project/tinyxml/tinyxml/2.6.2/tinyxml_2_6_2.zip

      unzip tinyxml_2_6_2.zip

      cd tinyxml
      make -j4

      # copy 库文件到系统库文件搜索路径下
      cp libtinyxml.a /usr/lib/

      # copy 头文件到系统头文件搜索路径下 
      mkdir /usr/include/tinyxml
      cp *.h /usr/include/tinyxml
      ```
   3. zookeeper：

      1. 安装zookeeper之前需要安装java（[JAVA安装](https://blog.csdn.net/qq_43329216/article/details/118385502)）；
      2. [Linux下 zookeeper 的安装](https://blog.csdn.net/shenmingxueIT/article/details/115755444?ops_request_misc=%257B%2522request%255Fid%2522%253A%2522169865410516800226512242%2522%252C%2522scm%2522%253A%252220140713.130102334.pc%255Fblog.%2522%257D&request_id=169865410516800226512242&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~blog~first_rank_ecpm_v1~rank_v31_ecpm-4-115755444-null-null.nonecase&utm_term=zookeeper&spm=1018.2226.3001.4450)；
      3. 可能出现的报错（[Linux安装zookeeper原生C API接口出现的make编译错误](https://blog.csdn.net/weixin_43604792/article/details/103879578)）；

## 前置知识

**Protobuf：**

* [Protobuf实现rpc的简介](123)

**zookeeper：**

* [zookeeper入门——基础知识](https://blog.csdn.net/shenmingxueIT/article/details/116135815)

## 整体概览

**整体流程：**

![1700660750094](image/README/1700660750094.png)

**RPC整体框架：**

![1700668167972](image/README/1700668167972.png)

## 各个模块

## 使用及功能展示

## 遇到的问题
