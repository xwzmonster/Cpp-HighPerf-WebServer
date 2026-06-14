# TODO

## P0：必须先完成

1. 修正 `Makefile` 服务端源文件列表。
2. 确认 `make` 成功。
3. 服务端和客户端完成一次 echo 测试。
4. 给 `TcpServer` 增加初始化状态保护。
5. 处理 `TcpConnection::server_` 未使用问题。

## P1：单线程 Reactor 完整化

1. 新增 `EventLoop`。
2. 让 `EventLoop` 管理 `Epoll`。
3. 让 `TcpServer` 不直接调用 `epoll_wait`。
4. 新增 `Acceptor`。
5. 让 `TcpServer` 只接收新连接回调。

## P2：网络库能力增强

1. 新增 `Buffer`。
2. 将 echo 业务从 `TcpConnection` 中移出。
3. 增加用户回调接口。
4. 增加更完整的连接状态管理。

## P3：多线程 Reactor

1. 新增 `EventLoopThread`。
2. 新增 `EventLoopThreadPool`。
3. 使用 `eventfd` 实现跨线程唤醒。
4. 主线程处理 accept，子线程处理连接 IO。

