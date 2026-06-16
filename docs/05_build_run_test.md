# 构建、运行与测试说明

## 构建

建议服务端源文件包含：

```makefile
SERVER_SRC = tcpepoll_02.cpp InetAddress.cpp Socket.cpp Epoll.cpp Channel.cpp TcpServer.cpp TcpConnection.cpp Eventloop.cpp
```

如果入口文件改名为 `tcpepoll.cpp`，则使用：

```makefile
SERVER_SRC = tcpepoll.cpp TcpServer.cpp TcpConnection.cpp InetAddress.cpp Socket.cpp Epoll.cpp Channel.cpp
```

编译命令：

```bash
cd src
make
```

只做语法检查：

```bash
g++ -std=c++17 -Wall -Wextra -fsyntax-only tcpepoll_02.cpp InetAddress.cpp Socket.cpp Epoll.cpp Channel.cpp TcpServer.cpp TcpConnection.cpp Eventloop.cpp
```

## 运行

启动服务端：

```bash
./epoll 127.0.0.1 8080
```

启动客户端：

```bash
./client 127.0.0.1 8080
```

## 基础测试

1. 客户端输入短字符串，确认服务端 echo。
2. 多开几个客户端，确认多个连接都能工作。
3. 客户端直接退出，确认服务端不崩溃。
4. 连续输入多次，确认不会丢数据。

## 后续压力测试

可使用以下方式逐步增加压力：

1. 多终端运行多个客户端。
2. 编写批量连接测试客户端。
3. 使用 `nc` 快速连接测试。
4. 后续支持 HTTP 后再使用 `wrk` 或 `ab`。
