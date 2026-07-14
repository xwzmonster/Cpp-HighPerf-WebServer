# 构建、运行与测试说明

## 构建

建议服务端源文件包含：

```makefile
SERVER_SRC = tcpepoll_02.cpp InetAddress.cpp Socket.cpp Epoll.cpp Channel.cpp TcpServer.cpp TcpConnection.cpp Eventloop.cpp Acceptor.cpp Buffer.cpp
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
g++ -std=c++17 -Wall -Wextra -fsyntax-only tcpepoll_02.cpp InetAddress.cpp Socket.cpp Epoll.cpp Channel.cpp TcpServer.cpp TcpConnection.cpp Eventloop.cpp Acceptor.cpp Buffer.cpp
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

### 阶段 1C 验证

1. 启动服务端：

```bash
    cd src
    ./epoll 127.0.0.1 8080
```

1. 打开第一个客户端：
  
```bash
    cd src
    ./client 127.0.0.1 8080
```

1. 打开第二个客户端：

```bash
    cd src
    ./client 127.0.0.1 8080
```

1. 两个客户端分别输入不同字符串，确认都能收到 echo。
1. 两个客户端分别按 Ctrl+C 断开。
1. 服务端应打印 disconnected，且不崩溃。
1. 断开后可以重新启动客户端，确认服务端仍能继续 accept 新连接。

### 当前通过标准

  1. make 能成功。
  2. 服务端能正常启动。
  3. 多客户端能同时连接。
  4. 多客户端能正常 echo。
  5. 客户端断开后服务端不崩溃。
  6. Eventloop::loop() 负责事件等待和事件分发

## 阶段 1C 结构检查

用以下命令检查事件循环是否已经移动到 `Eventloop`：

  ```bash
    rg -n "TcpServer::loop|Eventloop::loop|poll\\(-1\\)|handleEvent" src
  ```

  期望结果：

  1. Eventloop::loop 应该存在。
  2. poll(-1) 和 handleEvent() 应该出现在 Eventloop.cpp 中。
  3. TcpServer::loop 不应该再作为有效函数存在。

### 阶段 2A 验证

阶段 2A 用于验证 `Acceptor` 是否已经接管监听 socket、监听 Channel 和 accept 逻辑。

  结构检查：

```bash
  rg -n "Acceptor|listenSock_|listenChannel_|handleAccept|newConnection" src
```

语法检查：

cd src
g++ -std=c++17 -Wall -Wextra -fsyntax-only tcpepoll_02.cpp InetAddress.cpp Socket.cpp Epoll.cpp Channel.cpp TcpServer.cpp TcpConnection.cpp Eventloop.cpp Acceptor.cpp

运行验证：

1. 启动服务端。
2. 启动三个客户端。
3. 三个客户端分别发送不同字符串。
4. 确认三个客户端都能收到 echo。
5. 三个客户端分别 Ctrl+C 断开。
6. 服务端应打印 disconnected，且不崩溃。

期望结果：

1. `Acceptor` 持有监听 socket 和监听 `Channel`。
2. `Acceptor::handleAccept()` 负责 `accept` 新连接。
3. `TcpServer` 持有 `std::unique_ptr<Acceptor>`。
4. `TcpServer` 通过 `newConnection()` 创建 `TcpConnection`。
5. `TcpServer` 不再有效持有 `listenSock_`和 `listenChannel_`。

### 阶段 3A 验证

阶段 3A 用于验证最小 `Buffer` 类已经加入工程，但暂不替换 `TcpConnection::outbuf_`。

 结构检查：

```bash
  rg -n "class Buffer|Buffer::append|Buffer::retrieve|Buffer.cpp|outbuf_" src
```

期望结果：

1. Buffer.h 中存在 Buffer 类。
2. Buffer.cpp 中实现 append()、retrieve()、retrieveAll()。
3. Makefile 的 SERVER_SRC 包含 Buffer.cpp。
4. TcpConnection 暂时仍使用 std::string outbuf_，阶段 3B 再替换。

语法检查：

```bash
  cd src
  g++ -std=c++17 -Wall -Wextra -fsyntax-only tcpepoll_02.cpp InetAddress.cpp Socket.cpp Epoll.cpp Channel.cpp TcpServer.cpp TcpConnection.cpp
  Eventloop.cpp Acceptor.cpp Buffer.cpp
```

期望结果：

1. `Acceptor` 持有监听 socket 和监听 `Channel`。
2. `Acceptor::handleAccept()` 负责 `accept` 新连接。
3. `TcpServer` 持有 `std::unique_ptr<Acceptor>`。
4. `TcpServer` 通过 `newConnection()` 创建 `TcpConnection`。
5. `TcpServer` 不再有效持有 `listenSock_`和 `listenChannel_`。

### 阶段 3B 验证

  阶段 3B 用于验证 `TcpConnection` 是否已经使用 `Buffer` 替换原来的 `std::string outbuf_`。

  结构检查：

```bash
  rg -n "outbuf_|outputBuffer_|Buffer" src/TcpConnection.h src/TcpConnection.cpp src/Buffer.h src/Buffer.cpp
```

期望结果：

1. TcpConnection.h 中不再有效持有 std::string outbuf_。
2. TcpConnection.h 中存在 Buffer outputBuffer_。
3. TcpConnection::handleRead() 把读取到的数据追加到 outputBuffer_。
4. TcpConnection::tryWrite() 从 outputBuffer_ 取数据发送。
5. TcpConnection::refresh() 根据 outputBuffer_.readableBytes() 判断是否监听 EPOLLOUT。

语法检查：

```bash
  cd src
  g++ -std=c++17 -Wall -Wextra -fsyntax-only tcpepoll_02.cpp InetAddress.cpp Socket.cpp Epoll.cpp Channel.cpp TcpServer.cpp TcpConnection.cpp
  Eventloop.cpp Acceptor.cpp Buffer.cpp
```

运行验证：

1. 启动服务端。
2. 启动三个客户端。
3. 三个客户端分别发送短字符串。
4. 三个客户端连续发送多条消息。
5. 发送一条稍长字符串。
6. 确认所有客户端都能收到 echo。
7. 客户端 Ctrl+C 断开后，服务端不崩溃。

### 阶段 3C 验证

结构检查：

```bash
  rg -n "inputBuffer_|outputBuffer_|retrieveAll|append" src/TcpConnection.h src/TcpConnection.cpp
```

期望结果：

1. TcpConnection.h 中存在 Buffer inputBuffer_。
2. TcpConnection.h 中存在 Buffer outputBuffer_。
3. handleRead() 先追加数据到 inputBuffer_。
4. inputBuffer_的可读数据会追加到 outputBuffer_。
5. 处理完成后会清空或消费 inputBuffer_。

运行验证：

1. 三客户端 echo 正常。
2. 40、120、240 字节字符串 echo 正常。
3. 客户端 Ctrl+C 断开后服务端不崩溃。

### 阶段 4A 验证

结构检查：

```bash
  rg -n "MessageCallback|setMessageCallback|messageCallback_|::send" src/TcpServer.h src/TcpServer.cpp src/TcpConnection.h src/TcpConnection.cpp src/tcpepoll_02.cpp
```

期望结果：

1. TcpServer 能保存 MessageCallback。
2. TcpConnection 能保存 MessageCallback。
3. TcpServer::newConnection() 会把消息回调设置到新连接。
4. TcpConnection::handleRead() 读到数据后调用消息回调。
5. tcpepoll_02.cpp 中通过 lambda 设置 echo 业务。
6. TcpConnection::tryWrite() 使用 ::send(...) 调用系统发送接口。

运行验证：

1. cd src && make
2. 启动服务端：./epoll 127.0.0.1 8080
3. 启动三个客户端：./client 127.0.0.1 8080
4. 分别发送短字符串和稍长字符串。
5. 确认所有客户端都能收到 echo。
6. Ctrl+C 断开客户端后，服务端不崩溃。

### 阶段 4B-1 验证

结构检查：

```bash
  rg -n "ConnectionCallback|setConnectionCallback|connectionCallback_" src/TcpServer.h src/TcpServer.cpp src/tcpepoll_02.cpp
```

运行验证：

1. cd src && make
2. 启动服务端：./epoll 127.0.0.1 8080
3. 启动三个客户端。
4. 确认每个客户端连接时服务端打印连接建立回调。
5. 分别发送 40、80、120 字节字符串。
6. 确认 echo 正常，客户端断开后服务端不崩溃。

### 阶段 4B-2 验证

结构检查：

```bash
  rg -n "CloseCallback|setCloseCallback|closeCallback_" src/TcpServer.h src/TcpServer.cpp src/tcpepoll_02.cpp
```

运行验证：

1. cd src && make
2. 启动服务端：./epoll 127.0.0.1 8080
3. 启动三个客户端。
4. 分别发送短字符串和稍长字符串。
5. Ctrl+C 断开客户端。
6. 确认服务端打印关闭回调。
7. 确认服务端不崩溃，原有 echo 和连接建立回调行为不变。

### 阶段 4C 验证

阶段 4C 不增加功能，主要验证回调调用链、职责边界和生命周期。

结构检查：

```bash
  rg -n "ConnectionCallback|MessageCallback|CloseCallback|set.*Callback|removeConnection|messageCallback_" \
    src/TcpServer.h src/TcpServer.cpp \
    src/TcpConnection.h src/TcpConnection.cpp \
    src/Eventloop.h src/Eventloop.cpp \
    src/Channel.h src/Channel.cpp \
    src/tcpepoll_02.cpp
```

检查标准：

1. setXXXCallback() 只保存回调，不立即调用。
2. ConnectionCallback 在 TcpServer::newConnection() 中调用。
3. MessageCallback 从 TcpServer 下发给 TcpConnection。
4. MessageCallback 在 TcpConnection::handleRead() 中调用。
5. 业务 CloseCallback 在 TcpServer::removeConnection() 中调用。
6. 业务 CloseCallback 必须在 conns_.erase() 前调用。
7. 能区分 Channel::setCloseCallback() 和 TcpServer::setCloseCallback()。
8. 回调收到的 TcpConnection*、Buffer* 不能被长期保存。

运行验证：

1. 使用 -Wall -Wextra -Wpedantic 完成语法检查。
2. 启动三个客户端并分别发送不同长度的消息。
3. 验证连接建立回调、消息回调和 echo。
4. 逐个断开客户端，确认关闭回调在每个连接上执行一次。
5. 全部客户端断开后重新连接，确认服务端仍能 accept 和 echo。
6. 服务端运行过程中不得出现崩溃、死循环或 sanitizer 报错。

## 后续压力测试

可使用以下方式逐步增加压力：

1. 多终端运行多个客户端。
2. 编写批量连接测试客户端。
3. 使用 `nc` 快速连接测试。
4. 后续支持 HTTP 后再使用 `wrk` 或 `ab`。
