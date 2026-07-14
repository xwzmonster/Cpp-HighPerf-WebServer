# 架构设计说明

## 当前架构

当前项目已经完成到阶段 4B-2：`TcpConnection` 已同时拥有输入缓冲、输出缓冲和最小消息回调 `MessageCallback`，`TcpServer` 已支持连接建立回调 `ConnectionCallback` 和连接关闭回调 `CloseCallback`。echo 业务已经从 `TcpConnection::handleRead()` 移动到入口文件。

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
                   -> inputBuffer_
                   -> outputBuffer_
                   -> MessageCallback
     -> Buffer
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

业务 CloseCallback 必须在 conns_.erase() 前执行。执行 erase() 后，`unique_ptr<TcpConnection>` 管理的连接对象会被销毁，之前取得的TcpConnection* 随即失效，业务层不能继续访问或长期保存该指针。

## 当前回调分类

### 底层事件回调

由 `Channel` 保存和触发，包括读、写、关闭和错误事件回调。

```text
  epoll_wait
    -> Channel::handleEvent()
    -> readCallback_ / writeCallback_ / closeCallback_ / errorCallback_
```

### 框架内部通知回调

用于不同框架对象之间传递事件：

1. Acceptor::NewConnectionCallback：把新客户端 fd 通知给 TcpServer。
2. Eventloop::RemoveConnectionCallback：把需要移除的 fd 通知给 TcpServer。

### 上层业务回调

由入口文件设置，由 TcpServer 保存：

1. ConnectionCallback：连接建立后执行。
2. MessageCallback：收到数据后执行。
3. CloseCallback：连接销毁前执行。

setXXXCallback() 只负责保存可调用对象，不会立即执行。真正执行回调的位置分别位于连接建立、收到消息和连接关闭的事件处理路径中。
业务回调收到的 `TcpConnection*` 和 `Buffer*` 都是借用指针。业务层不能删除这些对象，也不能把这些指针长期保存到异步任务或成员变量中。

## 关键设计原则

  1. Socket 拥有 fd, 析构时关闭 fd。
  2. Channel 不拥有 fd, 只负责 fd 的事件分发。
  3. Epoll 不懂业务, 只负责底层事件注册和等待。
  4. Eventloop 是 Reactor 的核心调度对象。
  5. TcpServer 负责连接集合和连接生命周期。
  6. TcpConnection 负责单个客户端连接的读写和关闭状态。
  7. 单线程阶段不加锁, 多线程阶段再引入线程归属和跨线程任务队列。

## 下一阶段目标：阶段 4C 回调机制复盘

当前不增加新功能，也不进入多线程 Reactor。

阶段 4C 需要区分三类回调：

1. `Channel` 保存的读、写、关闭和错误事件回调。
2. `Acceptor`、`Eventloop` 用于跨对象通知的框架内部回调。
3. `TcpServer` 对外提供的连接、消息和关闭业务回调。

阶段 4C 完成后，再设计阶段 4D，处理默认 echo 仍然存在、删除连接职责过于集中和 `[this]` 生命周期约束等问题。

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
