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

## P1：当前必须做

1. 阶段 2：抽象 `Acceptor`。
    - 完成标准：监听 socket、监听 Channel 和 accept 逻辑从 `TcpServer` 拆出。
    - 验证方法：服务端能启动，多客户端 echo 行为不变，客户端断开后服务端不崩溃。

2. 阶段 2 后更新构建说明。
    - 完成标准：`docs/05_build_run_test.md` 和 `src/Makefile` 都包含 `Acceptor.cpp`。
    - 验证方法：`make clean && make` 成功，语法检查命令包含 `Acceptor.cpp`。

## P2：后续应该做

1. 阶段 3：抽象 `Buffer`。
     - 完成标准：不再直接依赖简单 `std::string outbuf_` 处理全部读写。
     - 验证方法：连续发送、多次发送、大一点的数据包都能正常 echo。

2. 阶段 4：增加用户回调接口。
     - 完成标准：echo 业务从 `TcpConnection` 内部分离。
     - 验证方法：可以在入口文件或业务回调中定义 echo 行为。

## P3：以后做

1. 多线程 Reactor。
     - 完成标准：主 Reactor 负责 accept，子 Reactor 负责连接 IO。
     - 验证方法：多个子线程各自运行 `Eventloop`，连接能稳定分配和处理。

2. 压测和简历材料整理。
     - 完成标准：记录测试方式、并发连接数、吞吐表现、项目亮点。
     - 验证方法：README 和 `docs/06_resume_material.md` 完成更新
