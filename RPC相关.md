# RPC

## RPC简介

RPC （Remote Procedure Call）从字面上理解，就是调用一个方法，但是这个方法不是运行在本地，而是运行在远程的服务器上。 也就是说，客户端应用可以像调用本地函数一样，直接调用运行在远程服务器上的方法。

## RPC框架所面临的问题

1. 函数调用时，数据结构的约定问题；
2. 数据传输时，序列化与反序列化问题；
   1. 使用protobuf；
3. 网络通行问题；
   1. http、自定义协议；
4. 跨语言；

## protobuf介绍

### 简介

Protobuf是Protocol Buffers的简称，它是 Google 开发的一种跨语言、跨平台、可扩展的用于序列化数据协议，可以用于结构化数据序列化（串行化），它序列化出来的数据量少，再加上以 K-V 的方式来存储数据，非常适用于在网络通讯中的数据载体。

只要遵守一些简单的使用规则，可以做到非常好的兼容性和扩展性，可用于通讯协议、数据存储等领域的语言无关、平台无关、可扩展的序列化结构数据格式。

Protobuf 中最基本的数据单元是message，并且在message中可以多层嵌套message或其它的基础数据类型的成员。

protobuf 序列化之后的数据，全部是二进制的。Protobuf 是一种灵活，高效，自动化机制的结构数据序列化方法，可模拟 XML，但是比 XML 更小（3 ~ 10倍）、更快（20 ~ 100倍）、更简单，而且它支持 Java、C++、Python 等多种语言。

### 基本使用

RPC框架概览：

![](https://pic4.zhimg.com/v2-86ffa7522cc02801fbbc6acb1b38719f_r.jpg)

* Step1：创建 .proto 文件，定义数据结构（成员、方法）

  * ```
    essage EchoRequest {
    	string message = 1;
    }

    message EchoResponse {
    	string message = 1;
    }

    message AddRequest {
    	int32 a = 1;
    	int32 b = 2;
    }

    message AddResponse {
    	int32 result = 1;
    }

    service EchoService {
    	rpc Echo(EchoRequest) returns(EchoResponse);
    	rpc Add(AddRequest) returns(AddResponse);
    }
    ```
* Step2： 使用 protoc 工具，编译 .proto 文档，生成界面（类以及相应的方法）

  * `protoc echo_service.proto -I./ --cpp_out=./`
  * 执行以上命令，即可生成两个文件：在这2个文件中，定义了2个重要的类，echo_service.pb.h, echo_service.pb.c。
  * ```
    class EchoService : public ::PROTOBUF_NAMESPACE_ID::Service {
        virtual void Echo(RpcController* controller, 
                        EchoRequest* request,
                        EchoResponse* response,
                        Closure* done);

       virtual void Add(RpcController* controller,
                        AddRequest* request,
                        AddResponse* response,
                        Closure* done);

        void CallMethod(MethodDescriptor* method,
                      RpcController* controller,
                      Message* request,
                      Message* response,
                      Closure* done);
    }
    class EchoService_Stub : public EchoService {
     public:
      EchoService_Stub(RpcChannel* channel);

      void Echo(RpcController* controller,
                EchoRequest* request,
                EchoResponse* response,
                Closure* done);

      void Add(RpcController* controller,
                AddRequest* request,
                AddResponse* response,
                Closure* done);

    private:
        // 成员变量，比较关键
        RpcChannel* channel_;
    };
    ```
* Step3：服务端程序实现接口中定义的方法，提供服务;客户端调用接口函数，调用远程的服务。

  * 服务端：EchoService：EchoService类中的两个方法 Echo 和 Add 都是虚函数，我们需要继承这个类，定义一个业务层的服务类 EchoServiceImpl，然后实现这两个方法，以此来提供远程调用服务。
  * 客户端：EchoService_Stub：EchoService_Stub就相当于是客户端的代理，应用程序只要把它"当做"远程服务的替身，直接调用其中的函数就可以了。（Stub可以理解为木头人，意为替身）
    * protobuf自动生成的实现代码：channel 就像一个通道，是用来解决数据传输问题的。 也就是说方法会把所有的数据结构序列化之后，通过网络发送给服务器。
    * ```
      void EchoService_Stub::Echo(RpcController* controller,
                                  EchoRequest* request,
                                  EchoResponse* response,
                                  Closure* done) {
        channel_->CallMethod(descriptor()->method(0),
                             controller, 
                             request, 
                             response, 
                             done);
      }

      void EchoService_Stub::Add(RpcController* controller,
                                  AddRequest* request,
                                  AddResponse* response,
                                  Closure* done) {
        channel_->CallMethod(descriptor()->method(1),
                             controller, 
                             request, 
                             response, 
                             done);
      }
      ```
  * RpcChannel 是用来解决网络通信问题的，因此客户端和服务端都需要它们来提供数据的接收和发送。客户端使用的RpcChannelClient， 是服务端使用的RpcChannelServer，它俩都是继承自 protobuf 提供的RpcChannel。
  * protobuf只是提供了网络通讯的策略，通讯的机制是什么需要由RPC框架来决定和实现。
  * protobuf 提供了一个基类，其中定义了方法。 我们的 RPC 框架中，客户端和服务端实现的 Channel必须继承protobuf 中的RpcChannel，然后重载CallMethod这个方法。
  * CallMethod 方法的几个参数特别重要，通过这些参数，来利用 protobuf 实现序列化、控制函数调用等操作，也就是说这些参数就是一个纽带，把我们写的代码与 protobuf 提供的功能，连接在一起。

## 网络库libevet

### 简介

Libevent 是一个用C 语言编写的、轻量级、高性能、基于事件的网络库。

主要有以下几个亮点：

> 事件驱动（ event-driven），高性能; 轻量级，专注于网络;源代码相当精炼、易读; 跨平台，支持 Windows、 Linux、*BSD 和 Mac Os; 支持多种 I/O 多路复用技术， epoll、 poll、 dev/poll、 select 和 kqueue 等; 支持 I/O，定时器和信号等事件;注册事件优先顺序。

从用户的角度来看，libevent 库提供了以下功能：当一个文件描述符的特定事件（如可读，可写或出错）发生了，或一个定时事件发生了， libevent 就会自动执行用户注册的回调函数，来接收数据或者处理事件。

* 把 fd 读写、信号、DNS、定时器甚至idle（空闲） 都抽象化成了event（事件）。

## RPC框架

### 基本框架构思

![](https://pic3.zhimg.com/80/v2-f696188e042207efef085a6a486d4a6e_720w.webp)

RPC 框架需要实现的部分，功能简述如下：

> EchoService：服务端界面类，定义需要实现哪些方法;
>
> EchoService_Stub： 继承自 EchoService，是客户端的本地代理;
>
> RpcChannelClient： 用户处理客户端网络通讯，继承自 RpcChannel;
>
> RpcChannelServer： 用户处理服务端网络通讯，继承自 RpcChannel;

应用程序 ：

> EchoServiceImpl：服务端应用层需要实现的类，继承自 EchoService;
>
> ClientApp： 客户端应用程序，调用 EchoService_Stub 中的方法;

服务发现：

> 在一些分步式应用场景中，可能会有一个服务发现流程。
>
> 每一个服务都注册到「服务发现服务器」上，然后客户端在调用远程服务的之前，并不知道服务提供商在什么位置。
>
> 客户端首先到服务发现服务器中查询，拿到了某个服务提供者的网络地址之后，再向该服务提供者发送远程调用请求。

### 元数据的设计

定义与业务层相关的数据结构，以及与RPC消息的数据结构。

* 客户端：
  * 首先，构造一个 RpcMessage 变量，填入各种元数据（type， id， service， method， error）;
  * 然后，序列化客户端传入的请求对象（EchoRequest）， 得到请求数据的字节码;
  * 再然后，把请求数据的字节插入到RpcMessage中的request字段;
  * 最后，把RpcMessage变量序列化之后，通过TCP发送出去。
  * ![img](https://pic3.zhimg.com/80/v2-f38a978eec84ce03ffc1190091b6328e_720w.webp)
* 服务端：
  * 首先，把接收到的 TCP 数据反序列化，得到一个 RpcMessage 变量;
  * 然后，根据其中的 type 字段，得知这是一个调用请求，于是根据 service 和 method 字位，构造出两个类实例：EchoRequest 和 EchoResponse（利用了 C++ 中的原型模式）;
  * 最后，从RpcMessage消息中的request字位反序列化，来填充EchoRequest实例;
  * ![img](https://pic3.zhimg.com/80/v2-18324ddb5fd30c29a903669324e4da52_720w.webp)

### 客户端发送请求数据

![](https://pic4.zhimg.com/80/v2-326d42213de33b688be9621c6ac2cb4b_720w.webp)

* Step1：业务级客户端调用 Echo（）函数
  * ```
    // ip, port 是服务端地址
    RpcChannel *rpcChannel = new RpcChannelClient(ip, port);
    EchoService_Stub *serviceStub = new EchoService_Stub(rpcChannel);
    serviceStub->Echo(...);
    ```
  * EchoService_Stub中的 Echo 方法，会调用其成员变量 channel_ 的 CallMethod方法，因此，**需要提前把实现好的RpcChannelClient实例，作为构造函数的参数，注册到 EchoService_Stub中**。
* Step2：EchoService_Stub 调用 channel_. CallMethod（） 方法
  * 这个方法在RpcChannelClient （继承自 protobuf 中的 RpcChannel 类）中实现，它主要的任务就是：把 EchoRequest 请求数据，包装在 RPC 元数据中，然后序列化得到二进制数据。
  * ```
    // 创建 RpcMessage
    RpcMessage message;

    // 填充元数据
    message.set_type(RPC_TYPE_REQUEST);
    message.set_id(1);
    message.set_service("EchoService");
    message.set_method("Echo");

    // 序列化请求变数，填充 request 栏位 
    // (这里的 request 变数，是客户端程式传进来的)
    message.set_request(request->SerializeAsString());

    // 把 RpcMessage 序列化
    std::string message_str;
    message.SerializeToString(&message_str);
    ```
* Step3： 通过 libevent 接口函数发送 TCP 数据
  * ```
    bufferevent_write(m_evBufferEvent, [二进位数据]);
    ```

### 服务端接收请求数据

![](https://pic4.zhimg.com/80/v2-b20bb452e27acb4055595389f2748a4f_720w.webp)

* Step4： 第一次反序列化数据
  * RpcChannelServer是负责处理服务端的网络数据，当它接收到 TCP 数据之后，首先进行第一次反序列化，得到 RpcMessage 变量，这样就获得了 RPC 元数据，包括：消息类型（请求RPC_TYPE_REQUEST）、消息 Id、Service 名称（"EchoServcie"）、Method 名称（"Echo"）。
  * ```
    RpcMessage rpcMsg;

    // 第一次反序列化
    rpcMsg.ParseFromString(tcpData); 

    // 创建请求和响应实例
    auto *serviceDesc = service->GetDescriptor();
    auto *methodDesc = serviceDesc->FindMethodByName(rpcMsg.method());
    ```
  * 从请求数据中获取到请求服务的Service名称（serviceDesc）之后，就可以查找到服务对象EchoService了，因为我们也拿到了请求方法的名称（methodDesc），此时利用 C++ 中的原型模式，构造出这个方法所需要的请求对象和响应对象，如下：
    * C++的原型模式：用原型实例指定创建对象的种类，并通过拷贝这些原型创建新的对象，简单理解就是“ **克隆指定对象** ”
  * ```
    // 构造 request & response 对象
    auto *echoRequest = service->GetRequestPrototype(methodDesc).New();
    auto *echoResponse = service->GetResponsePrototype(methodDesc).New();
    ```
  * 构造出请求对象 echoRequest 之后，就可以用 TCP 数据中的请求字段（即： rpcMsg.request）来第二次反序列化了，此时就还原出了这次方法调用中的参数，如下
  * ```
    // 第二次反序列化:
    request->ParseFromString(rpcMsg.request());
    ```
  * EchoService 服务是如何被查找到的？在服务端有一个Service 服务对象池，当 RpcChannelServer 接收到调用请求后，到这个池子中查找相应的 Service 对象，对于我们的示例来说，就是要查找 EchoServcie 对象，例如：
  * ```
    std::map<std::string, google::protobuf::Service *> m_spServiceMap;

    // 服务端启动的时候，把一个 EchoServcie 实例注册到池子中
    EchoService *echoService = new EchoServiceImpl();
    m_spServiceMap->insert("EchoService", echoService);
    ```
* Step5： 调用 EchoServiceImpl 中的 Echo（） 方法
  * 查找到服务对象之后，就可以调用其中的 Echo（） 这个方法了，但不是直接调用，而是用一个中间函数EchoService->CallMethod来进行过渡。
  * ```
    // 查找到 EchoService 对象
    service->CallMethod(...)
    ```
  * 在echo_servcie.pb.cc中，这个 CallMethod（） 方法的实现为：protobuf 是利用固定（写死）的索引，来定位一个 Service 服务中所有的 method 的，也就是说顺序很重要！
  * ```
    void EchoService::CallMethod(...)
    {
        switch(method->index())
        {
            case 0: 
                Echo(...);
                break;

            case 1:
                Add(...);
                break;
        }
    }
    ```
* Step6： 调用 EchoServiceImpl 中的 Echo （）方法
  * EchoServiceImpl 类继承自EchoService，并实现了其中的虚函数 Echo 和 Add，因此 Step5 中在调用 Echo 方法时，根据 C++ 的多态，就进入了业务层中实现的 Echo 方法。
  * 当查找到EchoServcie服务对象之后，就可以调用其中的指定方法了。（服务发现）
  * ![img](https://pic4.zhimg.com/80/v2-ee7bcf1f47999657e30c29a41f6a1f87_720w.webp)

### 服务端发送响应数据

![](https://pic4.zhimg.com/80/v2-730d6283095656555dc3933c25c4257b_720w.webp)

* Step7： 业务层处理完毕，回调 RpcChannelServer 中的回调对象

  * 在上面的 Step4 中，我们通过原型模式构造了 2 个对象：请求对象（echoRequest）和响应对象（echoResponse）:

    * ```
      // 构造 request & response 对象
      auto *echoRequest = service->GetRequestPrototype(methodDesc).New();
      auto *echoResponse = service->GetResponsePrototype(methodDesc).New();
      ```
  * 在Step5中，调用的时候，传递参数如下：service->CallMethod(...)：

    * ```
      service->CallMethod([参数1:先不管], [参数2:先不管], echoRequest, echoResponse, respDone);

      // this position
      ```
    * 按照一般的函数调用流程，在CallMethod中调用 Echo（） 函数，业务层处理完之后，会回到上面 this position这个位置。 然后再把 echoResponse 响应数据序列化，最后通过 TCP 发送出去。
    * 但是 protobuf 的设计并不是如此，这里利用了 C++ 中的闭包(closure)的可调用特性，构造了respDone这个变量，这个变量会一直作为参数传递到业务层的 Echo（） 方法中。
    * ```
      auto respDone = google::protobuf::NewCallback(this, &RpcChannelServer::onResponseDoneCB, echoResponse);
      ```
    * 因此，通过NewCallBack这个模板方法，就可以创建一个可调用对象respDone，并且这个对象中保存了传入的参数：一个函数，这个函数接收的参数。当在以后某个时候，调用respDone这个对象的 Run 方法时，这个方法就会调用它保存的那个函数，并且传入保存的参数。
    * sadad
  * 看一下业务层的 Echo（） 代码 ：

    * ```
      void EchoServiceImpl::Echo(protobuf::RpcController* controller,
                         EchoRequest* request,
                         EchoResponse* response,
                         protobuf::Closure* done)
      {
      	response->set_message(request->message() + ", welcome!");
      	done->Run();
      }
      ```
    * 在Echo 方法处理完毕之后，只调用了done->Run()方法，这个方法会调用之前作为参数注册进去的RpcChannelServer::onResponseDoneCB方法，并且把响应对象echoResponse作为参数传递进去。方法中一定是进行了 2 个操作：

      * 反序列化数据; 发送TCP数据;
* Step8： 序列化得到二进制字节码，发送TCP数据

  * 首先，构造 RPC 元数据，把响应对象序列化之后，设置到 response 字段。
    * ```
      void RpcChannelImpl::onResponseDoneCB(Message *response)
      {
          // 构造外层的 RPC 元数据
      	RpcMessage rpcMsg;
      	rpcMsg.set_type(RPC_TYPE_RESPONSE);
      	rpcMsg.set_id([消息 Id]]);
      	rpcMsg.set_error(RPC_ERR_SUCCESS);

      	// 把响应对象序列化，设置到 response 栏位。
      	rpcMsg.set_response(response->SerializeAsString());
      }
      ```
  * 然后，序列化数据，通过libevent发送TCP数据。
    * ```
      std::string message_str;
      rpcMsg.SerializeToString(&message_str);
      bufferevent_write(m_evBufferEvent, message_str.c_str(), message_str.size());
      ```

### 客户端接收响应数据

![](https://pic2.zhimg.com/80/v2-c9ba1eb4e6dcf4d9a159f10fdcce9209_720w.webp)

* Step9： 反序列化接收到的 TCP 数据

  * RpcChannelClient 是负责客户端的网络通讯，因此当它接收到 TCP 数据之后，首先进行第一次反序列化，构造出 RpcMessage 变量，其中的 response 栏位就存放着服务端的函数处理结果，只不过此时它是二进制数据。
    * ```
      RpcMessage rpcMsg;
      rpcMsg.ParseFromString(tcpData);

      // 此时，rpcMsg.reponse 中存储的就是 Echo() 函数处理结果的二进位数据。
      ```
* Step10： 调用业务层客户端的函数来处理 RPC 结果

  * Step1 中，业务层客户端在调用serviceStub->Echo(...)方法的时候，我没有列出传递的参数，这里把它补全：

    * ```
      // 定义请求对象
      EchoRequest request;
      request.set_message("hello, I am client");

      // 定义响应对象
      EchoResponse *response = new EchoResponse;


      auto doneClosure = protobuf::NewCallback(
      				    &doneEchoResponseCB, 
      					response 
      				    );

      // 第一个参数先不用关心
      serviceStub->Echo(rpcController, &request, response, doneClosure);
      ```
    * 这里同样利用了 protobuf 提供的NewCallback模板方法，来创建一个可调用对象（闭包doneClosure），并且让这个闭包保存了 2 个参数：一个回调函数（doneEchoResponseCB）和response 对象（应该说是指针更准确）。
    * 当回调函数doneEchoResponseCB被调用的时候，会自动把response对象作为参数传递进去。这个可调用对象（doneClosure闭包） 和 response 对象，被作为参数一路传递到 EchoService_Stub –>RpcChannelClient
    * ![img](https://pic1.zhimg.com/80/v2-13fc15c547bd6ac193a38de307da26f0_720w.webp)
  * 因此当RpcChannelClient接收到 RPC 远程调用结果时，就把二进位的 TCP 数据，反序列化到response对象上，然后再调用doneClosure->Run（）方法，Run（） 方法中执行 ，就调用了业务层中的回调函数，也把参数传递进去了。

    * ```
      void doneEchoResponseCB(EchoResponse *response)
      {
      	cout << "response.message = " << response->message() << endl;
      	delete response;
      }
      ```

### 总结

**protobuf 的核心：**

* 通过以上的分析，可以看出 protobuf 主要是为我们解决了序列化和反序列化的问题。
* 然后又通过RpcChannel这个类，来完成业务层的用户代码与protobuf 代码的整合问题。

**参考：[利用protobuf实现RPC框架 - 知乎 (zhihu.com)](https://zhuanlan.zhihu.com/p/373237728)**
