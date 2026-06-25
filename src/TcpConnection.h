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
#include"Buffer.h"

class TcpServer;
class Channel;
// class Epoll;
class Socket;
class Eventloop;
class Buffer;

// 保存每个客户端连接的状态
class TcpConnection {
private:
    // TcpServer* server_;
    int fd_;
    std::unique_ptr<Socket> sock_;
    std::unique_ptr<Channel> channel_;

    Buffer inputBuffer_; // “从 socket 读到但还没被业务处理的数据”
    // std::string outbuf_; // 输出缓冲区
    Buffer outputBuffer_; // “准备发送但还没完全写入内核的数据”
    bool peerClosed_; // 对端关闭写端后, 先把待发送数据发完再关闭连接
    
    bool tryWrite();
    bool refresh(); // 根据当前是否还有数据没发完，决定是否继续监听 EPOLLOUT

public: 
    TcpConnection(Eventloop* loop, int fd);
    ~TcpConnection();

    bool handleRead();
    bool handleWrite();
    bool handleClose();
    bool handleError();
};