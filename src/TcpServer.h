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
// #include"Socket.h"

// class Epoll;
// class Channel;
class TcpServer;
class TcpConnection;
class Eventloop;
class Acceptor;

class TcpServer { // 负责连接管理, 创建conn, 移除conn
private:
    // std::unique_ptr<Epoll> ep_;
    // std::unique_ptr<Socket> listenSock_;
    // std::unique_ptr<Channel> listenChannel_;
    std::unordered_map<int, std::unique_ptr<TcpConnection>> conns_;
    bool initialized_ = false;
    Eventloop* loop_;
    std::unique_ptr<Acceptor> acceptor_;

    // bool handleAccept();
    bool removeConnection(int fd);

public:
    TcpServer(Eventloop* loop, const std::string& ip, uint16_t port);
    void start();
    // void loop();
    // Epoll* getep_();
    void newConnection(int clientfd, const InetAddress& clientaddr);
    ~TcpServer();
};
