# 架构设计说明

## 当前架构

当前项目已经完成到阶段 2A：抽象 `Acceptor`。

`Eventloop` 负责事件循环和事件分发, `Acceptor` 负责监听 socket、监听 Channel 和 accept 新连接, `TcpServer` 负责连接集合和连接生命周期。

```text
  main
    -> Eventloop
         -> Epoll
         -> loop()
              -> poll(-1)
              -> Channel::handleEvent()
    -> TcpServer
         -> Acceptor
              -> listen Socket
              -> listen Channel
              -> handleAccept()
              -> NewConnectionCallback
         -> unordered_map<int, unique_ptr<TcpConnection>>
              -> TcpConnection
                   -> client Socket
                   -> client Channel
                   -> outbuf_
```

## 当前职责划分

| 类 | 当前职责 |
| --- | --- |
| `Socket` | 管理 socket fd, 封装 `bind`、`listen`、`accept4`、`setsockopt`, 析构时关闭 fd。 |
| `InetAddress` | 封装 `sockaddr_in`, 处理 IP、端口和字节序转换。 |
| `Epoll` | 封装 `epoll_create1`、`epoll_ctl`、`epoll_wait`, 只负责底层 IO 多路复用。 |
| `Channel` | 表示一个 fd 关心的事件状态, 保存监听事件、实际发生事件和事件回调。 |
| `Eventloop` | Reactor 核心调度对象, 负责等待事件、分发事件、注册和移除 Channel |
| `Acceptor` | 管理监听 socket 和监听 Channel, 负责 accept 新连接, 并通过回调通知 TcpServer |
| `TcpServer` | 管理 Acceptor、客户端连接集合和连接生命周期, 不再直接管理监听 socket 和监听 Channel。 |
| `TcpConnection` | 管理单个客户端连接的 socket、channel、读写缓冲、关闭状态和事件处理。 |

## 当前事件流程

```text
main
    -> 创建 Eventloop
    -> 创建 TcpServer
    -> TcpServer 创建 Acceptor
    -> Acceptor 注册监听 Channel
    -> TcpServer::start()
    -> Eventloop::loop()
    -> Epoll::loop()
    -> 返回活跃 Channel*
    -> Channel::handleEvent()
    -> 如果是监听 fd：Acceptor::handleAccept()
    -> Acceptor 通过 NewConnectionCallback 通知 TcpServer
    -> TcpServer::newConnection()
    -> 创建 TcpConnection
    -> 如果是客户端 fd：TcpConnection 处理读写和关闭
```

## 当前连接关闭流程

```text
  客户端断开
    -> epoll_wait 返回对应客户端 Channel
    -> Eventloop::loop() 调用 Channel::handleEvent()
    -> TcpConnection::handleRead() / handleClose() 返回 false
    -> Eventloop 通过 removeConnectionCallback(fd) 通知 TcpServer
    -> TcpServer::removeConnection(fd)
    -> conns_.erase(fd)
    -> unique_ptr<TcpConnection> 析构
    -> TcpConnection 析构时移除 Channel
    -> Socket 析构时关闭 fd
```

## 关键设计原则

  1. Socket 拥有 fd, 析构时关闭 fd。
  2. Channel 不拥有 fd, 只负责 fd 的事件分发。
  3. Epoll 不懂业务, 只负责底层事件注册和等待。
  4. Eventloop 是 Reactor 的核心调度对象。
  5. TcpServer 负责连接集合和连接生命周期。
  6. TcpConnection 负责单个客户端连接的读写和关闭状态。
  7. 单线程阶段不加锁, 多线程阶段再引入线程归属和跨线程任务队列。

## 下一阶段目标：Buffer

阶段 3 将抽象 Buffer：

  TcpConnection
    -> inputBuffer_
    -> outputBuffer_

阶段 3 完成后，TcpConnection 不再只依赖简单的 std::string outbuf_ 处理读写数据。

## muduo 风格目标架构

后续演进目标：

```text
main
  -> EventLoop mainLoop
  -> TcpServer
       -> Acceptor
            -> listen Socket
            -> listen Channel
       -> EventLoopThreadPool
            -> sub EventLoop 1
            -> sub EventLoop 2
       -> TcpConnection
            -> Socket
            -> Channel
            -> inputBuffer
            -> outputBuffer
```
