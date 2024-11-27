## 项目介绍

    该项目基于C++11实现的仿muduo网络库

## 目录结构

```shell
myTinyMuduo/
|
|-- base/
| |-- chat-client.cpp # 聊天客户端
| |-- test-client.cpp # 测试客户端
| |-- server.cpp      # 服务器
|
|-- img/ # 自述文件依赖的图片
|
|-- include/
| |-- Acceptor.h
| |-- Buffer.h
| |-- Channel.h
| |-- Connection.h
| |-- EchoServer.h    # 实现的简易Echo服务器, 用来测试网络库
| |-- Epoll.h
| |-- EventLoop.h
| |-- InetAddress.h
| |-- Logger.h
| |-- Socket.h
| |-- TcpServer.h
| |-- ThreadPool.h
| |-- TimeStamp.h
|
|-- src/
| |-- Acceptor.cpp
| |-- Buffer.cpp
| |-- Channel.cpp
| |-- Connection.cpp
| |-- EchoServer.cpp
| |-- Epoll.cpp
| |-- EventLoop.cpp
| |-- InetAddress.cpp
| |-- Logger.cpp
| |-- Socket.cpp
| |-- TcpServer.cpp
| |-- ThreadPool.cpp
| |-- TimeStamp.cpp
|
|-- makefile
|-- .gitignore
```

## 参考资料
    1. 陈硕的muduo库: https://github.com/chenshuo/muduo
    2. TinyWebServer: https://github.com/qinguoyi/TinyWebServer
    3. 博客: https://www.cnblogs.com/S1mpleBug/p/16712003.html
    4. 书籍:《Linux多线程服务端编程: 使用muduo C++网络库》