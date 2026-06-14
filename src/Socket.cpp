#include"Socket.h"

int createNonBlockingFd() {
    int listenfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if(listenfd == -1) {
        perror("socket: ");
        return -1;
    }
    return listenfd;
}

Socket::Socket(int fd) : fd_(fd){
}

Socket::~Socket() { 
    ::close(this->fd_);
}

int Socket::getfd() const {
    return this->fd_;
}

void Socket::SetReUseAddr(bool on) { // 设置SO_REUSEADDR选项, true-打开, false-关闭
    int opt = on ? 1 : 0;
    if(::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt SO_REUSEADDR failed");
    }
}
void Socket::SetReUsePort(bool on){ // 设置SO_REUSEPORT选项, true-打开, false-关闭
    int opt = on ? 1 : 0;
    if(::setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        perror("setsockopt SO_REUSEPORT failed");
    }
}
void Socket::SetKeepAlive(bool on){ // 设置SO_KEEPALIVE选项, true-打开, false-关闭
    int opt = on ? 1 : 0;
    if(::setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt)) < 0) {
        perror("setsockopt SO_KEEPALIVE failed");
    }
}
void Socket::SetTCPNoDelay(bool on){   // 设置TCP_NODELAY选项, true-打开, false-关闭
    int opt = on ? 1 : 0;
    if(::setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0) {
        perror("setsockopt TCP_NODELAY failed");
    }
}

bool Socket::Bind(const InetAddress& server) {
    if(::bind(fd_, server.addr(), sizeof(sockaddr_in)) == -1) {
        perror("bind: ");
        return false;
    }
    return true;
}
bool Socket::Listen(int n) {
    if(::listen(fd_, n) == -1) {
        perror("listen: ");
        return false;
    }
    return true;
}
int Socket::Accept4(InetAddress& clientaddr) {
    struct sockaddr_in peeraddr;
    memset(&peeraddr, 0, sizeof(peeraddr));
    
    socklen_t len = sizeof(peeraddr);
    int clientfd = accept4(fd_, (struct sockaddr*)&peeraddr, &len, SOCK_NONBLOCK);
    if(clientfd >= 0) {
        clientaddr.setaddr(peeraddr);
    }

    return clientfd;
}
