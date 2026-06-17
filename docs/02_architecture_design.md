# 架构设计说明

## 当前架构

当前项目已经完成到阶段 1C：`Eventloop` 接管事件循环和事件分发

```text
  main
    -> Eventloop
         -> Epoll
         -> loop()
              -> poll(-1)
              -> Channel::handleEvent()
              -> removeConnectionCallback(fd)
    -> TcpServer
         -> listen Socket
         -> listen Channel
         -> unordered_map<int, unique_ptr<TcpConnection>>
                -> TcpConnection
                     -> client Socket
                     -> client Channel
```

## 当前职责划分

| 类 | 当前职责 |
| --- | --- |
| `Socket` | 管理 socket fd，封装 `bind`、`listen`、`accept4`、`setsockopt`，析构时关闭 fd。 |
| `InetAddress` | 封装 `sockaddr_in`，处理 IP、端口和字节序转换。 |
| `Epoll` | 封装 `epoll_create1`、`epoll_ctl`、`epoll_wait`, 只负责底层 IO 多路复用。 |
| `Channel` | 表示一个 fd 关心的事件状态，保存监听事件、实际发生事件和事件回调。 |
| `TcpServer` | 管理监听 socket、监听 channel、客户端连接集合和连接生命周期。 |
| `TcpConnection` | 管理单个客户端连接的 socket、channel、读写缓冲、关闭状态和事件处理。 |

## 当前事件流程

```text
  main
    -> 创建 Eventloop
    -> 创建 TcpServer
    -> TcpServer::start()
    -> Eventloop::loop()
    -> Epoll::loop()
    -> 返回活跃 Channel*
    -> Channel::handleEvent()
    -> 调用 TcpServer 或 TcpConnection 中绑定的回调
```

## 当前连接关闭流程

  ```text
  客户端断开
    -> `epoll_wait` 返回对应 `Channel`
    -> `Eventloop::loop()` 调用 `Channel::handleEvent()`
    -> `TcpConnection::handleRead()` 或 `handleClose()` 返回 false
    -> `Eventloop` 通过 `removeConnectionCallback(fd)` 通知 `TcpServer`
    -> `TcpServer::removeConnection(fd)`
    -> `conns_.erase(fd)`
    -> `unique_ptr<TcpConnection>` 析构
    -> `TcpConnection` 析构时移除 `Channel`
    -> `Socket` 析构时关闭 `fd`
  ```

## 关键设计原则

  1. Socket 拥有 fd，析构时关闭 fd。
  2. Channel 不拥有 fd，只负责 fd 的事件分发。
  3. Epoll 不懂业务，只负责底层事件注册和等待。
  4. Eventloop 是 Reactor 的核心调度对象。
  5. TcpServer 负责连接集合和连接生命周期。
  6. TcpConnection 负责单个客户端连接的读写和关闭状态。
  7. 单线程阶段不加锁，多线程阶段再引入线程归属和跨线程任务队列。

## 下一阶段目标：Acceptor

阶段 2 将抽象 Acceptor：

```text
  TcpServer
    -> Acceptor
         -> listen Socket
         -> listen Channel
         -> handleAccept()
         -> NewConnectionCallback
    -> unordered_map<int, unique_ptr<TcpConnection>>
```

  阶段 2 完成后，TcpServer 不再直接操作监听 socket，也不再直接写 accept 循环。

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
