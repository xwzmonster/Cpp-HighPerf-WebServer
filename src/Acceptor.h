#pragma once
#include<unistd.h>
#include<functional>
#include<memory>
#include"InetAddress.h"
#include"Socket.h"

class Channel;
class Eventloop;


using NewConnectionCallback = std::function<void(int, const InetAddress&)>;

/*
    Acceptor 的职责：
    1. 创建监听 socket
    2. bind 地址
    3. listen
    4. 创建 listenChannel，监听读事件
    5. 有新连接时 accept
    6. accept 成功后通过回调把 connfd 和 peerAddr 交给 TcpServer

    它需要：
       - Eventloop*，因为要注册 Channel
       - Socket，表示监听 socket
       - Channel，表示监听 fd 对应的事件
       - NewConnectionCallback，通知 TcpServer 新连接
*/
// 创建监听 socket, 绑定地址, 监听端口, 把监听 fd 交给 Channel 关注读事件; 当有新连接到来时, 调用 accept, 拿到 connfd，然后通过回调通知 TcpServer
class Acceptor { // 只负责监听 socket, 只负责 accept 新连接
private:
    Eventloop* loop_;
    std::unique_ptr<Socket> listenSock_;
    std::unique_ptr<Channel> listenChannel_;
    NewConnectionCallback newConnectionCallback_;
    bool initialized_ = false;  

    bool handleAccept();

public:
    Acceptor(Eventloop* loop, const std::string& ip, uint16_t port);
    void setNewConnectionCallback(const NewConnectionCallback& cb);
    bool initialized() const; // 告诉 TcpServer: Acceptor 初始化是否成功

    ~Acceptor();
};