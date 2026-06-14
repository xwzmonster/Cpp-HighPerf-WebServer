# 分阶段开发计划

## 阶段 0：稳定当前单线程 echo server

目标：项目能稳定编译、启动、接受连接、回显数据、关闭连接。

必须完成：

1. 修正 `Makefile` 的服务端源文件列表。
2. 确认入口文件只负责解析参数、创建 `TcpServer`、调用 `start()`。
3. 给 `TcpServer` 增加初始化状态，避免构造失败后继续运行。
4. 清理 `TcpConnection` 中暂时未使用的 `server_`，或者明确它的用途。
5. 保持 `Channel::handleEvent()` 的事件分发逻辑简单清晰。

完成标准：

1. `make` 能成功。
2. `./epoll 127.0.0.1 8080` 能启动。
3. `./client 127.0.0.1 8080` 能发送并收到 echo。
4. 客户端断开后服务端不崩溃、不死循环。

## 阶段 1：抽象 EventLoop

目标：把事件循环从 `TcpServer` 中拆出来。

计划：

1. 新增 `EventLoop` 类。
2. `EventLoop` 持有 `Epoll`。
3. `EventLoop::loop()` 负责 `epoll_wait` 和 `Channel::handleEvent()`。
4. `Channel` 持有 `EventLoop*` 或继续持有 `Epoll*`，本阶段建议先持有 `EventLoop*`，由 `EventLoop` 转发 update/remove。
5. `TcpServer` 不再直接调用 `ep_->loop()`，而是调用 `loop_->loop()`。

完成标准：

1. 行为和阶段 0 完全一致。
2. `TcpServer` 不直接控制底层 epoll 等待。

## 阶段 2：抽象 Acceptor

目标：把监听 fd 和接受连接逻辑从 `TcpServer` 中拆出来。

计划：

1. 新增 `Acceptor` 类。
2. `Acceptor` 拥有 listen `Socket` 和 listen `Channel`。
3. `Acceptor` 内部处理 `accept4`。
4. `Acceptor` 对外提供 `NewConnectionCallback`。
5. `TcpServer` 收到新连接回调后创建 `TcpConnection`。

完成标准：

1. `TcpServer` 不直接操作 listen socket。
2. 新连接创建流程更接近 muduo。

## 阶段 3：完善 Buffer

目标：替换直接使用 `std::string outbuf_` 的粗糙写法。

计划：

1. 新增 `Buffer` 类。
2. 支持 readable/writable/prepend 区间。
3. 支持从 fd 读取数据。
4. 支持向 fd 写出数据。
5. `TcpConnection` 拥有 `inputBuffer_` 和 `outputBuffer_`。

完成标准：

1. 支持任意二进制数据。
2. 不依赖 `%s` 风格字符串输出。
3. 大包和半包处理基础更清晰。

## 阶段 4：用户回调接口

目标：让业务逻辑从 `TcpConnection` 内部分离。

计划：

1. 定义 `ConnectionCallback`。
2. 定义 `MessageCallback`。
3. 定义 `CloseCallback`。
4. `TcpServer` 对外暴露设置业务回调的接口。
5. echo 逻辑移动到 `main` 或示例业务函数中。

完成标准：

1. `TcpConnection` 只负责网络连接，不写死 echo 业务。
2. 可以很容易替换成其他业务。

## 阶段 5：多线程 Reactor

目标：主 Reactor 接受连接，子 Reactor 处理连接读写。

计划：

1. 新增 `Thread` 或直接使用 `std::thread`。
2. 新增 `EventLoopThread`。
3. 新增 `EventLoopThreadPool`。
4. 主线程只处理 accept。
5. 新连接按轮询分配到子 `EventLoop`。
6. 引入跨线程唤醒机制，建议使用 `eventfd`。

完成标准：

1. 多个子线程各自运行一个 `EventLoop`。
2. 每个 `TcpConnection` 只属于一个固定 `EventLoop`。
3. 跨线程操作通过任务队列完成。

## 阶段 6：工程化与简历增强

目标：把项目整理成可展示的工程。

计划：

1. 增加 README。
2. 增加压测说明。
3. 增加架构图。
4. 增加常见问题记录。
5. 使用 `ab`、`wrk`、自写客户端或 `nc` 进行测试。
6. 记录 QPS、并发连接数、CPU 占用和瓶颈分析。

