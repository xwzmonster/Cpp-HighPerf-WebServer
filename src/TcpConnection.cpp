#include<string>
#include<memory>
#include<errno.h>
#include<unistd.h>
#include<sys/epoll.h>
#include<unordered_map>
#include <sys/types.h>
#include<sys/socket.h>


#include"TcpConnection.h"
#include"TcpServer.h"
#include"Channel.h"
#include"Socket.h"

bool TcpConnection::tryWrite() {
    if(this->outputBuffer_.readableBytes() == 0) {
        return true;
    }
    // std::string& out = this->outputBuffer_;

    /*
        把用户态outputBuffer_里的数据, 尽可能写入 fd 对应 socket 的内核发送缓冲区
        数据流
                服务器用户态 outputBuffer_
                        |
                        | send()
                        v
                服务器内核中 clientfd 对应的 TCP 发送缓冲区
                        |
                        | TCP 协议栈 / 网卡 / 网络
                        v
                客户端内核 TCP 接收缓冲区
                        |
                        | 客户端 read()
                        v
                客户端用户态 buffer
    */
    while(outputBuffer_.readableBytes() > 0) {
        // data()是std::string类的另一个成员函数, 它返回指向字符串内容的指针
        // s.data() 返回一个 const char* 类型的指针, 指向字符串的第一个字符
        // size()是std::string类的一个成员函数, 用于获取字符串中的字符数, 不包括\0
        /*
            用MSG_NOSIGNAL是因为:
            当往一个已经关闭的socket写数据时,Linux默认可能产生SIGPIPE信号,这个信号默认会导致进程退出
            加上:
                MSG_NOSIGNAL
                可以避免 SIGPIPE 杀死服务器进程
        */
        ssize_t n = ::send(this->fd_, outputBuffer_.peek(), outputBuffer_.readableBytes(), MSG_NOSIGNAL);
        if(n > 0) {
            this->outputBuffer_.retrieve(static_cast<size_t>(n));
            continue;
        }

        // 在 socket 发送缓冲区满了, 暂时不能继续写
        if(n == -1 && errno == EINTR) {
            continue;
        }

        if(n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return true;
        }
        perror("send:");
        return false;
    }
    return true;
}

// 根据当前是否还有数据没发完，决定是否继续监听 EPOLLOUT
bool TcpConnection::refresh() {
    // 对端关闭, 且数据发送完
    if(this->peerClosed_ && this->outputBuffer_.readableBytes() == 0) {
        return false;
    }

    // 对端已经关闭读方向后, 只关心把剩余数据写完
    uint32_t events = EPOLLET;
    if(!this->peerClosed_) {
        events |= EPOLLIN | EPOLLRDHUP; 
    }
    if(this->outputBuffer_.readableBytes() > 0) {
        events |= EPOLLOUT;
    } 
    channel_->set__events_(events);
    return channel_->update();
}

TcpConnection::TcpConnection(Eventloop* loop, int fd) 
        : fd_(fd), 
        sock_(std::make_unique<Socket>(fd)), 
        channel_(std::make_unique<Channel>(loop, fd)),
        inputBuffer_(),
        outputBuffer_(), 
        peerClosed_(false) {
    sock_->SetTCPNoDelay(true);

    this->channel_->setReadCallback([this](){
        return this->handleRead();
    });
    this->channel_->setWriteCallback([this](){
        return this->handleWrite();
    });
    this->channel_->setCloseCallback([this]() {
        return this->handleClose();
    });
    this->channel_->setErrorCallback([this]() {
        return this->handleError();
    });

    channel_->set__events_(EPOLLIN | EPOLLET | EPOLLRDHUP);
    channel_->update();
}

TcpConnection::~TcpConnection() {
    if(this->channel_) {
        this->channel_->remove();
    }
}

bool TcpConnection::handleRead() {
    bool needWrite = false;
    /*
        数据流
            客户端用户态 buffer
                    |
                    | send()
                    v
            客户端内核 TCP 发送缓冲区
                    |
                    | TCP 协议栈 / 网络
                    v
            服务端内核中 clientfd 对应的 TCP 接收缓冲区
                    |
                    | 服务端 read(clientfd, ...)
                    v
            服务端用户态 buf
    */
    /*
        加上 while, 这样在第一次 EPOLLIN 触发时就循环读取所有可用数据,
        直到 read 返回 EAGAIN → 这时缓冲区空了, ET 的规则就不会漏数据 
    */
    while(1) {
        char buf[1024];
        ssize_t n = read(this->fd_, buf, sizeof(buf));
        if(n > 0) {
            /*
                通常我们打印字符串只用 %s, 它只需要一个参数(字符串的指针). 
                但是 %s 有一个致命的特点: 它会一直往后打印, 直到遇到字符串结束符 \0 为止
                而 %.*s. (.点号) 后面的数字: 表示“精度”, 对于字符串来说, 意思是“最多打印多少个字符”
                比如 %.5s 意思就是最多只打印 5 个字符
                * (星号):这是一个动态占位符. 它告诉 printf: “不想把数字写死在代码里，具体的数字会在后面的参数里传给你
                
                %.*s 组合起来的意思就是: 先从参数里读取一个整数作为长度, 
                然后再读取一个指针, 打印这个指针指向的字符串, 但最多只打印刚才指定的长度
            */
            printf("recv from fd = %d, len = %zd, content = %.*s\n", this->fd_, n, (int)n, buf);
            fflush(stdout);

            this->inputBuffer_.append(buf, static_cast<size_t>(n));
            if(this->messageCallback_) {
                messageCallback_(this, &inputBuffer_);
            } else {
                outputBuffer_.append(inputBuffer_.peek(), inputBuffer_.readableBytes());
                inputBuffer_.retrieveAll();
            }

            // // 把 buf 里面前 n 个字节的数据, 安全地追加（拼接）到 inputBuffer_ 这个字符串的末尾
            // this->outputBuffer_.append(this->inputBuffer_.peek(), this->inputBuffer_.readableBytes());
            // this->inputBuffer_.retrieveAll();

            needWrite = true;
        } else if(n == 0) {
            // 对端正常关闭写端
            printf("clientfd(client fd = %d) disconnected.\n", this->fd_);
            this->peerClosed_ = true;
            break;
        } else {
            if(errno == EINTR) {
                continue;
            }
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                // ET模式下, 必须读到这里才算读干净
                break;
            }
            perror("read");
            return false;
        }
    }
    if(needWrite) {
        if(!this->tryWrite()) {
            return false;
        }
    }
    return this->refresh();
}
bool TcpConnection::handleWrite() {
    if(!this->tryWrite()) {
        return false;
    }
    return this->refresh();
}
bool TcpConnection::handleClose() {
    printf("clientfd(client fd = %d) disconnected.\n", this->fd_);
    this->peerClosed_ = true;
    return this->refresh();
}
bool TcpConnection::handleError() {
    return false;
}

void TcpConnection::setMessageCallback(const MessageCallback& cb) {
    this->messageCallback_ = cb;
}

void TcpConnection::send(const char* data, size_t len) {
    this->outputBuffer_.append(data, len);
    tryWrite();
    refresh();
}