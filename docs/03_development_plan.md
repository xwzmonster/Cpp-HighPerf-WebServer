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

目标：把事件循环从 `TcpServer` 中逐步拆出来，让 `Eventloop` 成为 Reactor 的核心调度对象。

### 阶段 1A：Eventloop 接管 Epoll 所有权

目标：让 `Eventloop` 持有 `Epoll`，`TcpServer` 不再自己拥有 `Epoll`。

已完成：

  1. 新增 `Eventloop` 类。
  2. `Eventloop` 内部使用 `std::unique_ptr<Epoll>` 管理 `Epoll` 对象。
  3. `main` 创建 `Eventloop`。
  4. `main` 创建 `TcpServer` 时传入 `Eventloop*`。
  5. `TcpServer` 不再持有 `Epoll`。
  6. `TcpServer` 通过 `Eventloop` 间接使用 `Epoll`。
  7. `Makefile` 的服务端源文件加入 `Eventloop.cpp`。

完成标准：

  1. `make` 能成功。
  2. 服务端能正常启动。
  3. 多客户端 echo 正常。
  4. 客户端断开后服务端不崩溃。
  5. 行为和阶段 0 保持一致。

### 阶段 1B：Channel 持有 Eventloop*

  目标：让 `Channel` 不再直接持有 `Epoll*`，而是持有 `Eventloop*`。

  已完成：

  1. `Channel` 构造函数参数从 `Epoll*` 改为 `Eventloop*`。
  2. `Channel` 内部成员从 `Epoll*` 改为 `Eventloop*`。
  3. `Channel::update()` 通过 `Eventloop::updateChannel()` 转发。
  4. `Channel::remove()` 通过 `Eventloop::removeChannel()` 转发。
  5. `TcpServer` 创建监听 `Channel` 时传入 `Eventloop*`。
  6. `TcpConnection` 创建连接 `Channel` 时传入 `Eventloop*`。
  7. 不再通过 `Eventloop::getEpoll()` 暴露底层 `Epoll`。

  完成标准：

  1. `make` 能成功。
  2. 服务端能正常启动。
  3. 多客户端 echo 正常。
  4. 客户端断开后服务端不崩溃。
  5. `Channel` 不再依赖 `Epoll*`。
  6. `TcpServer` 和 `TcpConnection` 不再绕过 `Eventloop` 直接操作 `Epoll`。

### 阶段 1C：Eventloop 接管事件循环

  目标：让 `Eventloop` 真正负责事件循环和事件分发，`TcpServer` 不再自己写 `while` 循环处理活跃事件。

  已完成：

  1. 给 `Eventloop` 提供真正的 `loop()` 函数。
  2. `Eventloop::loop()` 内部调用 `poll(-1)`。
  3. `Eventloop::loop()` 遍历活跃的 `Channel*`。
  4. `Eventloop::loop()` 调用 `Channel::handleEvent()`。
  5. 连接关闭时，`Eventloop` 通过回调通知 `TcpServer` 删除连接。
  6. `TcpServer::start()` 改为调用 `Eventloop::loop()`。
  7. 连接集合 `conns_` 仍由 `TcpServer` 管理。

  完成标准：

  1. `TcpServer::start()` 不再自己写事件循环。
  2. `TcpServer` 不再直接遍历活跃 `Channel*`。
  3. `Eventloop` 负责等待事件和分发事件。
  4. 多客户端 echo 行为和阶段 1B 完全一致。
  5. 客户端断开后服务端不崩溃。
  6. 已更新 `docs` 和 `development_log`。

当前状态：阶段 1C 已完成。下一步进入阶段 2A：抽象 `Acceptor`，只拆监听 socket、监听 Channel 和 accept 逻辑。

## 阶段 2：抽象 Acceptor

目标：把监听 fd 和接受连接逻辑从 `TcpServer` 中拆出来。

### 阶段 2A：新增 Acceptor

目标：让 `Acceptor` 负责监听 socket、监听 Channel 和 accept 循环，让 `TcpServer` 不再直接操作监听 fd。

已完成：

1. 新增 `Acceptor` 类。
2. `Acceptor` 持有 listen `Socket`。
3. `Acceptor` 持有 listen `Channel`。
4. `Acceptor` 内部处理 `accept4`。
5. `Acceptor` 对外提供 `NewConnectionCallback`。
6. `TcpServer` 创建 `Acceptor`。
7. `TcpServer` 给 `Acceptor` 设置新连接回调。
8. `TcpServer` 在新连接回调中创建 `TcpConnection`。
9. `Makefile` 已加入 `Acceptor.cpp`。
10. 多客户端 echo 和客户端断开测试通过。

完成标准：

1. `TcpServer` 不再直接持有 `listenSock_`。
2. `TcpServer` 不再直接持有 `listenChannel_`。
3. `TcpServer` 不再直接实现 accept 循环。
4. `Acceptor` 负责监听 fd 的创建、绑定、监听和新连接接收。
5. 多客户端 echo 行为和阶段 1C 保持一致。
6. 客户端断开后服务端不崩溃。
7. `Makefile` 已加入 `Acceptor.cpp`。
8. 已更新 `docs` 和 `development_log`。

当前状态：阶段 2A 已完成。`TcpServer` 不再有效持有监听 socket 和监听 Channel，监听 fd、监听 Channel 和 accept 循环已经迁移到 `Acceptor`。下一步进入阶段 3A：设计并引入最小 `Buffer`。

## 阶段 3：完善 Buffer

目标：先新增 `Buffer` 类，并接入构建系统，暂时不替换 `TcpConnection` 的读写逻辑。

已完成：

1. 新增 `Buffer.h`。
2. 新增 `Buffer.cpp`。
3. `Buffer` 内部使用 `std::vector<char>` 管理连续内存。
4. `Buffer` 使用 `readerIndex_` 表示可读数据起点。
5. `Buffer` 使用 `writerIndex_` 表示可写数据起点。
6. `Buffer::append()` 能把外部数据复制到缓冲区。
7. `Buffer::retrieve()` 能消费已读数据。
8. `Buffer::retrieveAll()` 能清空可读数据。
9. `Makefile` 已加入 `Buffer.cpp`。
10. 当前暂不替换 `TcpConnection::outbuf_`。

完成标准：

1. `make` 能成功。
2. 包含 `Buffer.cpp` 的语法检查通过。
3. 服务端原有 echo 行为不变。
4. 多客户端连接、发送、回显、断开仍然正常。

当前状态：阶段 3A 已完成。`Buffer` 已加入工程并通过构建、语法检查和三客户端 echo 运行验证。下一步进入阶段 3B：用 `Buffer` 替换 `TcpConnection::outbuf_`。

### 阶段 3B：用 Buffer 替换 TcpConnection 输出缓冲

目标：让 `TcpConnection` 使用 `Buffer` 管理待发送数据，替换当前的 `std::string outbuf_`。

计划：

1. `TcpConnection.h` 引入 `Buffer.h`。
2. 将 `std::string outbuf_` 替换为 `Buffer outputBuffer_`。
3. 修改 `TcpConnection::handleRead()`，把读取到的数据 append 到 `outputBuffer_`。
4. 修改 `TcpConnection::tryWrite()`，从 `outputBuffer_` 中取出可读数据发送。
5. 修改 `TcpConnection::refresh()`，根据 `outputBuffer_.readableBytes()` 判断是否监听 `EPOLLOUT`。
6. 保持 echo 行为不变。

完成标准：

1. `make` 能成功。
2. 多客户端 echo 正常。
3. 客户端 Ctrl+C 断开后服务端不崩溃。
4. 连续发送多条消息正常。
5. 发送稍大一点的数据仍能正常回显。

当前状态：阶段 3B 已完成。`TcpConnection` 已使用 `Buffer outputBuffer_` 替换原来的 `std::string outbuf_`，三客户端 echo 和稍长字符串 echo 验
  证通过。下一步进入阶段 3C：引入 `Buffer inputBuffer_`。

### 阶段 3C：引入输入缓冲 inputBuffer_

目标：让 `TcpConnection` 同时拥有输入缓冲和输出缓冲。

计划：

1. 在 `TcpConnection` 中新增 `Buffer inputBuffer_`。
2. `handleRead()` 先把 socket 读到的数据追加到 `inputBuffer_`。
3. 当前 echo 业务中，把 `inputBuffer_` 的可读数据追加到 `outputBuffer_`。
4. 处理完后清空或消费 `inputBuffer_`。
5. 保持现有 echo 行为不变。

完成标准：

1. `make` 能成功。
2. 三客户端 echo 正常。
3. 连续发送正常。
4. 稍长字符串 echo 正常。
5. 客户端断开后服务端不崩溃。

### 阶段 3C 验证

阶段 3C 用于验证 `TcpConnection` 是否已经同时拥有输入缓冲和输出缓冲。

结构检查：

```bash
  rg -n "inputBuffer_|outputBuffer_|retrieveAll|append" src/TcpConnection.h src/TcpConnection.cpp
```

期望结果：

1. TcpConnection.h 中存在 Buffer inputBuffer_。
2. TcpConnection.h 中存在 Buffer outputBuffer_。
3. handleRead() 先追加数据到 inputBuffer_。
4. 当前 echo 逻辑中，inputBuffer_的可读数据会追加到 outputBuffer_。
5. 处理完成后会消费或清空 inputBuffer_。

运行验证：

1. 启动服务端。
2. 启动三个客户端。
3. 分别发送短字符串。
4. 分别发送 40、120、240 字节左右的字符串。
5. 确认所有客户端都能收到 echo。
6. Ctrl+C 断开后服务端不崩溃。

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

### 阶段 4A：最小 MessageCallback

目标：先只引入消息回调，让 echo 业务从 `TcpConnection::handleRead()` 移动到入口文件。

  已完成：

1. `TcpServer` 定义并保存 `MessageCallback`。
2. `TcpConnection` 定义并保存 `MessageCallback`。
3. `TcpServer::newConnection()` 创建连接后下发消息回调。
4. `TcpConnection::handleRead()` 读到数据后触发消息回调。
5. `tcpepoll_02.cpp` 通过 lambda 设置 echo 业务。
6. `TcpConnection::tryWrite()` 中使用 `::send(...)` 调用 Linux 系统调用。

完成标准：

1. `make` 能成功。
2. 三客户端 echo 正常。
3. 稍长字符串 echo 正常。
4. 客户端 Ctrl+C 断开后服务端不崩溃。
5. echo 业务不再写死在 `TcpConnection::handleRead()` 中。

当前状态：阶段 4A 已完成。`MessageCallback` 已接入，echo 业务已经从 `TcpConnection::handleRead()` 移动到 `tcpepoll_02.cpp`。下一步进入阶段 4B-1：只增加 `ConnectionCallback`。

### 阶段 4B-1：只增加 ConnectionCallback

目标：连接建立后，让上层业务能够收到通知。

计划：

1. `TcpServer` 定义并保存 `ConnectionCallback`。
2. `TcpServer` 对外提供 `setConnectionCallback()`。
3. `TcpServer::newConnection()` 创建 `TcpConnection` 后触发连接建立回调。
4. `tcpepoll_02.cpp` 设置连接建立回调，先只打印日志。
5. 保持原有 echo 行为不变。

完成标准：

1. `make` 能成功。
2. 客户端连接时服务端能打印连接建立回调。
3. 原有 `MessageCallback` echo 行为不变。
4. 客户端断开后服务端不崩溃。

当前状态：阶段 4B-1 已完成。`ConnectionCallback` 已接入，客户端连接建立时业务层可以收到通知。下一步进入阶段 4B-2：增加业务层
  `CloseCallback`。

### 阶段 4B-2：增加 CloseCallback

目标：连接关闭前，让业务层能够收到通知。

计划：

1. `TcpServer` 定义并保存 `CloseCallback`。
2. `TcpServer` 对外提供 `setCloseCallback()`。
3. `TcpServer::removeConnection()` 在 erase 前触发关闭回调。
4. `tcpepoll_02.cpp` 设置关闭回调，先只打印日志。
5. 保持原有 echo、连接建立回调和断开行为不变。

完成标准：

1. `make` 能成功。
2. 客户端 Ctrl+C 断开时服务端能打印关闭回调。
3. 服务端不崩溃。

当前状态：阶段 4B-2 已完成。`TcpServer` 已支持业务层 `CloseCallback`，并且在 `TcpServer::removeConnection()` 中于 `conns_.erase()` 前触发关闭回调。三客户端 echo、连接建立回调、关闭回调和客户端断开测试通过。

### 阶段 4C：回调机制复盘和文档补齐

目标：不增加新功能，理解当前项目中的底层事件回调、框架内部回调和业务回调。

当前状态：进行中。

完成标准：

1. 能说明 `setXXXCallback()` 只负责保存函数，不会立即执行。
2. 能说明 `ConnectionCallback` 在哪里设置、保存和调用。
3. 能说明 `MessageCallback` 在哪里设置、下发、保存和调用。
4. 能说明业务 `CloseCallback` 在哪里设置、保存和调用。
5. 能区分 `Channel::setCloseCallback()` 和 `TcpServer::setCloseCallback()`。
6. 能说明关闭回调为什么必须在 `conns_.erase()` 前执行。
7. 能说明业务回调收到的裸指针不能被长期保存。
8. 已更新架构文档、测试文档和开发日志。

下一阶段：阶段 4C 完成后，进入阶段 4D，逐步收口回调职责和生命周期边界。暂不进入多线程 Reactor。

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

## 阶段 6：工程化

目标：把项目整理成可展示的工程。

计划：

1. 增加 README。
2. 增加压测说明。
3. 增加架构图。
4. 增加常见问题记录。
5. 使用 `ab`、`wrk`、自写客户端或 `nc` 进行测试。
6. 记录 QPS、并发连接数、CPU 占用和瓶颈分析。
