# 问题：

* 部分细节点不明确，比如runtime等；
* 对于makefile编写、编译的产出不是很了解；
* 单例模式的单例最后如何销毁的？；
* rpc_channel如何更优雅的在客户端中使用？；(解决)
* InterFace是否有必要，为了日志输出？（解决，为了更方便的销毁过程中产生的变量，以及业务代码的编写）
* controller是否有必要，辅助信息？；（解决，可以更好的记录代码运行中产生的错误，客户端可以得知本地的错误，服务端可以将服务解析中产生的错误发送给客户端）
* 与redis结合；
* 测试客户端使用http进行通信（服务器与浏览器的通信可以通过/调用begin方法进行初始页面设置，后续以序列化的数据进行通信，需要前端进行反序列化）；
