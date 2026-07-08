# TODO

## P0：已完成

1. 阶段 0：单线程 Reactor echo server 稳定运行。
     - 完成标准：能启动、能 accept、能 recv、能 echo、能处理客户端断开。
     - 验证方法：多客户端 echo 测试，客户端 Ctrl+C 断开后服务端不崩溃。

2. 阶段 1A：`Eventloop` 接管 `Epoll` 所有权。
     - 完成标准：`Eventloop` 持有 `Epoll`，`TcpServer` 不再自己持有 `Epoll`。
     - 验证方法：编译通过，多客户端 echo 行为不变。

3. 阶段 1B：`Channel` 改为持有 `Eventloop*`。
     - 完成标准：`Channel::update()` 和 `Channel::remove()` 通过 `Eventloop` 转发。
     - 验证方法：编译通过，多客户端 echo 行为不变。

4. 阶段 1C：`Eventloop` 接管事件循环和事件分发。
     - 完成标准：`TcpServer::start()` 调用 `Eventloop::loop()`，事件等待和事件分发由 `Eventloop` 完成。
     - 验证方法：多客户端 echo、客户端 Ctrl+C 断开、服务端不崩溃。
5. 阶段 2A：抽象 `Acceptor`。
     - 完成标准：`Acceptor` 负责监听 socket、监听 Channel 和 accept 循环；`TcpServer` 不再有效持有监听 socket 和监听 Channel。
     - 验证方法：包含 `Acceptor.cpp` 的语法检查通过，三客户端 echo 正常，客户端 Ctrl+C 断开后服务端不崩溃。

6. 阶段 3A：新增最小 `Buffer` 并接入构建。
     - 完成标准：`Buffer.h`、`Buffer.cpp` 已新增；`Buffer.cpp` 已加入 `Makefile`；`make` 和语法检查通过；原有多客户端 echo 行为不变。
     - 验证方法：三客户端 echo 测试通过，客户端 Ctrl+C 断开后服务端不崩溃。

7. 阶段 3B：用 `Buffer outputBuffer_` 替换 `TcpConnection::outbuf_`。
     - 完成标准：`TcpConnection` 使用 `Buffer outputBuffer_` 管理待发送数据。
     - 验证方法：三客户端 echo、稍长字符串 echo、客户端 Ctrl+C 断开均正常。

8. 阶段 3C：引入输入缓冲 `inputBuffer_`。
     - 完成标准：`TcpConnection` 同时拥有 `inputBuffer_` 和 `outputBuffer_`。
     - 验证方法：三客户端 echo、40/120/240 字节字符串 echo、客户端 Ctrl+C 断开均正常。

9. 阶段 4A：引入最小 `MessageCallback`。
     - 完成标准：echo 业务移动到入口文件，`TcpConnection::handleRead()` 通过消息回调通知业务层。
     - 验证方法：短消息、连续消息、稍长字符串 echo 均正常，客户端断开后服务端不崩溃。

10. 阶段 4B-1：只增加 `ConnectionCallback`。
     - 完成标准：连接建立时，上层业务能收到通知。
     - 验证方法：客户端连接时服务端打印连接建立回调，原有 echo 行为不变。

11. 阶段 4B-2：增加业务层 `CloseCallback`。
     - 完成标准：连接关闭前，上层业务能收到通知。
     - 验证方法：客户端 Ctrl+C 断开时服务端打印关闭回调，原有 echo 和连接建立回调不受影响，服务端不崩溃。

## P1：当前必须做

1. 阶段 5：多线程 Reactor 前置设计。
     - 完成标准：明确主 Reactor、子 Reactor、连接分配、跨线程唤醒机制。
     - 验证方法：先完成设计文档，不急着写代码。

## P2：后续应该做

1. 阶段 5：多线程 Reactor 前置设计。
     - 完成标准：明确主 Reactor、子 Reactor、连接分配、跨线程唤醒机制。
     - 验证方法：先完成设计文档，不急着写代码。

## P3：以后做

1. 多线程 Reactor。
     - 完成标准：主 Reactor 负责 accept，子 Reactor 负责连接 IO。
     - 验证方法：多个子线程各自运行 `Eventloop`，连接能稳定分配和处理。

2. 压测和简历材料整理。
     - 完成标准：记录测试方式、并发连接数、吞吐表现、项目亮点。
     - 验证方法：README 和 `docs/06_resume_material.md` 完成更新
