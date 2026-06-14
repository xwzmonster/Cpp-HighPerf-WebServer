#pragma once
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/tcp.h>
#include<string.h>
#include<errno.h>
#include<unistd.h>
#include"InetAddress.h"

int createNonBlockingFd();

class Socket {
private:
    const int fd_;

public: 
    explicit Socket(int fd);
    ~Socket();
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    int getfd() const; // 返回成员变量fd_
    void SetReUseAddr(bool on); // 设置SO_REUSEADDR选项, true-打开, false-关闭
    void SetReUsePort(bool on); // 设置SO_REUSEPORT选项, true-打开, false-关闭
    void SetKeepAlive(bool on); // 设置SO_KEEPALIVE选项, true-打开, false-关闭
    void SetTCPNoDelay(bool on);   // 设置TCP_NODELAY选项, true-打开, false-关闭

    bool Bind(const InetAddress& server);
    bool Listen(int n);
    int Accept4(InetAddress& clientaddr);

};

