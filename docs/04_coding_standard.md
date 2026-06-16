# 编码规范

## 资源管理

1. fd 必须由 `Socket` 或专门 RAII 类拥有。
2. 禁止多个对象同时拥有同一个 fd。
3. `Channel` 不关闭 fd。
4. `Epoll` 只管理 epoll fd。
5. 使用 `std::unique_ptr` 表示独占所有权。

## 回调设计

1. `Channel` 只保存事件回调，不写业务逻辑。
2. 读、写、关闭、错误分别使用独立回调。
3. 回调返回 `false` 表示连接需要关闭。
4. lambda 捕获 `this` 时，必须保证 `Channel` 生命周期短于所属对象，或者在析构前从 epoll 删除。

## 命名建议

1. 类名使用大驼峰：`TcpServer`、`TcpConnection`。
2. 成员变量使用尾下划线：`fd_`、`events_`。
3. 函数名建议统一风格，后续可逐步改为小驼峰：`setReadCallback`、`handleEvent`。
4. 避免 `set__events_` 这类不自然命名，后续可改为 `setEvents`。

## 错误处理

1. 系统调用失败后必须检查 `errno`。
2. `EINTR` 通常重试。
3. 非阻塞 IO 遇到 `EAGAIN/EWOULDBLOCK` 不算错误。
4. 构造失败不能让对象进入可运行状态。

## 修改原则

1. 每次只修改一个主题。
2. 修改后立即编译。
3. 编译通过后再运行功能测试。
4. 不在重构时同时改变业务行为。

