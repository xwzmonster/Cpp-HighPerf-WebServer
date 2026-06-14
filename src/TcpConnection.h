#pragma once
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include<string>
#include<unordered_map>
#include<vector>
#include<memory>
#include"InetAddress.h"

class TcpServer;
class Channel;
class Epoll;
class Socket;

// 保存每个客户端连接的状态
class TcpConnection {
private:
    // TcpServer* server_;
    int fd_;
    std::unique_ptr<Socket> sock_;
    std::unique_ptr<Channel> channel_;
    std::string outbuf_; // 输出缓冲区
    bool peerClosed_; // 对端关闭写端后, 先把待发送数据发完再关闭连接
    
    bool tryWrite();
    bool refresh(); // 根据当前是否还有数据没发完，决定是否继续监听 EPOLLOUT

public: 
    TcpConnection(Epoll* ep, int fd);
    ~TcpConnection();

    bool handleRead();
    bool handleWrite();
    bool handleClose();
    bool handleError();
};