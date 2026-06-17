#include "TcpServer.h"
#include "TcpConnection.h"
// #include "Epoll.h"
#include "Channel.h"
#include "Eventloop.h"

#include <vector>
#include <memory>
#include <cstdio>
#include <cstdlib>



TcpServer::TcpServer(Eventloop* loop, const std::string& ip, uint16_t port) : loop_(loop){
    if(this->loop_ == nullptr) {
        return;
    }
    int listenfd = createNonBlockingFd();   
    if(listenfd == -1) {
        return;
    }
    this->listenSock_ = std::make_unique<Socket>(listenfd);
    
    this->listenSock_->SetReUseAddr(true);
    this->listenSock_->SetReUsePort(true);
    this->listenSock_->SetKeepAlive(true);

    InetAddress serveraddr(ip, port);
    if(!listenSock_->Bind(serveraddr)) {
        return;
    }
    if(!listenSock_->Listen(128)) {
        return;
    }
    // this->ep_ = std::make_unique<Epoll>();
    this->listenChannel_ = std::make_unique<Channel>(this->loop_, listenSock_->getfd());
    /*
        [this] 的意思是: 回调函数以后发生时，调用当前这个 TcpServer 对象的 handleAccept()
    */
    this->listenChannel_->setReadCallback([this]() {
        return this->handleAccept();
    });
    listenChannel_->set__events_(EPOLLIN);
    if(!listenChannel_->update()) {
        return;
    }

    this->loop_->setRemoveConnectionCallback([this](int fd) {
        // 监听 fd 不是普通客户端连接, 不在 conns_里, TcpServer只应该删除客户端连接
        if(this->listenSock_ && fd != this->listenSock_->getfd()) {
            this->removeConnection(fd);
        }
    });

    this->initialized_ = true;
}

void TcpServer::start() {
    if(!this->initialized_) {
        fprintf(stderr, "TcpServer init failed.\n");
        return;
    }
    this->loop_->loop();
}

// void TcpServer::loop() {
//     while(1) {
//         std::vector<Channel*> chns = this->loop_->poll(-1);
//         if(chns.empty()) {
//             continue;
//         }
//         for(Channel* chn : chns) {
//             int fd = chn->getfd_();
//             bool alive = chn->handleEvent();

//             if(!alive && fd != this->listenSock_->getfd()) {
//                 removeConnection(fd);
//             }
//         }
//     }
// }

bool TcpServer::handleAccept() {
    while(1) {
        InetAddress clientaddr;
        int clientfd = this->listenSock_->Accept4(clientaddr);
        if(clientfd < 0) {
            if(errno == EINTR) {
                continue;
            }
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            perror("accept4");
            return false;
        }
        std::string ip = clientaddr.get_str_ip();
        printf("accept client(fd = %d, ip = %s, port = %d) ok\n", clientfd, ip.c_str(), clientaddr.getport());
        
        this->conns_[clientfd] = std::make_unique<TcpConnection>(this->loop_, clientfd);
    }
    return true;
}

/*
这里的设计是：TcpConnection::handleRead/Write/Close/Error() 返回 false，表示这个连接该删了；真正删除由 TcpServer 从 conns_
里 erase。erase 后 unique_ptr<TcpConnection> 析构，连接对象自动释放。
*/
bool TcpServer::removeConnection(int fd) {
    std::unordered_map<int, std::unique_ptr<TcpConnection>>::iterator it = this->conns_.find(fd);
    if(it == this->conns_.end()) {
        return false;
    }
    this->conns_.erase(it);
    return true;
}

TcpServer::~TcpServer() = default;


