#include"Acceptor.h"
#include"Channel.h"

Acceptor::Acceptor(Eventloop* loop, const std::string& ip, uint16_t port) : loop_(loop) {
    int listenfd = createNonBlockingFd();
    if(listenfd == -1) {
        return;
    }
    this->listenSock_ = std::make_unique<Socket>(listenfd);
    int opt = 1;
    this->listenSock_->SetReUseAddr(opt);
    this->listenSock_->SetReUsePort(opt);
    this->listenSock_->SetTCPNoDelay(opt);

    InetAddress serveraddr(ip, port);
    if(!this->listenSock_->Bind(serveraddr)) {
        return;
    }
    if(!this->listenSock_->Listen(128)) {
        return;
    }
    this->listenChannel_ = std::make_unique<Channel>(loop, listenfd);
    this->listenChannel_->setReadCallback([this]() {
        return this->handleAccept();
    });
    this->listenChannel_->set__events_(EPOLLIN);
    if(!this->listenChannel_->update()) {
        return;
    }
    this->initialized_ = true;
}

bool Acceptor::handleAccept() {
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
        printf("accept clientfd(fd = %d, ip = %s, port = %d) ok\n", clientfd, ip.c_str(), clientaddr.getport());

        if(this->newConnectionCallback_) {
            this->newConnectionCallback_(clientfd, clientaddr);
        }
    }
    return true;
}

void Acceptor::setNewConnectionCallback(const NewConnectionCallback& cb) {
    this->newConnectionCallback_ = cb;
}
bool Acceptor::initialized() const {
    return this->initialized_;
}

Acceptor::~Acceptor() {
    if(this->listenChannel_) {
        this->listenChannel_->remove();
    }
}
