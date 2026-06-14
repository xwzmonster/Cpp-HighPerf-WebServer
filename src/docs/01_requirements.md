# 项目需求说明

## 项目定位

本项目目标是实现一个基于 Linux `epoll` 的高并发 TCP Reactor 服务器，学习和复刻 muduo 网络库的核心思想，但不直接照搬 muduo 的完整复杂度。

## 功能需求

1. 支持 TCP 服务端启动、绑定 IP/端口、监听连接。
2. 使用非阻塞 socket 和 `epoll` 实现事件驱动。
3. 支持多个客户端同时连接。
4. 支持连接建立、读事件、写事件、关闭事件、错误事件处理。
5. 当前业务逻辑先实现 echo server：收到客户端数据后原样返回。
6. 后续支持 Reactor 抽象拆分：`EventLoop`、`Channel`、`Poller/Epoll`、`Acceptor`、`TcpServer`、`TcpConnection`。

## 非功能需求

1. 每个阶段都必须可编译、可运行。
2. 每次只做小步重构，不同时修改太多模块。
3. 不使用全局连接表管理业务状态。
4. fd 生命周期必须由 RAII 管理，避免 fd 泄漏和重复关闭。
5. 回调函数必须有清晰的对象归属。
6. 单线程版本稳定后，再进入多线程 Reactor。

## 当前阶段边界

当前阶段只做单线程 Reactor。暂时不实现：

1. 线程池。
2. `EventLoopThreadPool`。
3. 定时器。
4. HTTP 协议解析。
5. 大规模日志系统。

