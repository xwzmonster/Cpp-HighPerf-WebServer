CXX = g++
# 编译选项：-Wall 开启所有警告, -g 开启调试信息(方便后期用 gdb 调试死锁或段错误)
CXXFLAGS = -std=c++17 -Wall -Wextra -g

# 服务端配置
SERVER_SRC = tcpepoll_02.cpp InetAddress.cpp Socket.cpp Epoll.cpp Channel.cpp TcpServer.cpp TcpConnection.cpp
SERVER_TARGET = epoll

# 客户端配置
CLIENT_SRC = client.cpp
CLIENT_TARGET = client

# 默认目标: 输入 make 时默认执行 all, 即同时编译两个程序
all: $(SERVER_TARGET) $(CLIENT_TARGET)

# 编译服务端
$(SERVER_TARGET): $(SERVER_SRC)
	$(CXX) $(CXXFLAGS) $(SERVER_SRC) -o $(SERVER_TARGET)

# 编译客户端
$(CLIENT_TARGET): $(CLIENT_SRC)
	$(CXX) $(CXXFLAGS) $(CLIENT_SRC) -o $(CLIENT_TARGET)

# 清理生成的文件
clean: 
	rm -f $(SERVER_TARGET) $(CLIENT_TARGET)