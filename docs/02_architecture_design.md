# 架构设计说明

## 当前架构

当前源码已经从单文件版本逐步拆成以下对象：

### 阶段 1A 当前架构

```text
main
  -> TcpServer
       -> Epoll
       -> listen Socket
       -> listen Channel
       -> unordered_map<int, unique_ptr<TcpConnection>>
              -> client Socket
              -> client Channel
```

## 当前职责划分

| 类 | 当前职责 |
| --- | --- |
| `Socket` | 管理 socket fd，封装 `bind`、`listen`、`accept4`、`setsockopt`，析构时关闭 fd。 |
| `InetAddress` | 封装 `sockaddr_in`，处理 IP、端口和字节序转换。 |
| `Epoll` | 封装 `epoll_create1`、`epoll_ctl`、`epoll_wait`。 |
| `Channel` | 表示一个 fd 关心的事件和实际发生的事件，并保存事件回调。 |
| `TcpServer` | 管理监听 socket、监听 channel、epoll 和所有客户端连接。 |
| `TcpConnection` | 管理单个客户端连接的 socket、channel、读写缓冲、关闭状态和事件处理。 |

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

## 关键设计原则

1. `Channel` 不拥有 fd，只负责 fd 事件分发。
2. `Socket` 拥有 fd，析构时关闭 fd。
3. `Epoll` 不懂业务，只返回活跃的 `Channel`。
4. `TcpConnection` 负责单个连接的状态。
5. `TcpServer` 负责连接集合和连接生命周期。
6. 单线程阶段不加锁，多线程阶段再引入线程归属和跨线程任务队列。

## 阶段 1B 当前架构

```text
  main
    -> Eventloop
         -> Epoll
    -> TcpServer
         -> listen Socket
         -> listen Channel -> Eventloop
         -> unordered_map<int, unique_ptr<TcpConnection>>
                -> client Socket
                -> client Channel -> Eventloop
```

当前职责：

  1. Eventloop 拥有 Epoll。
  2. Channel 不再直接持有 Epoll*，而是持有 Eventloop*。
  3. Channel 通过 Eventloop::updateChannel() 和 Eventloop::removeChannel() 间接操作 epoll。
  4. TcpServer 仍然负责主事件循环和连接删除。
  5. 下一步需要让 Eventloop 进一步接管事件分发逻辑。
