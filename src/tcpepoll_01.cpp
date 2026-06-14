#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<errno.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<sys/fcntl.h>
#include<sys/epoll.h>
#include<netinet/tcp.h> // TCP_NODELAY需要包含这个头文件
#include<string.h>
#include<string>
#include<unordered_map>
#include<vector>

#include"InetAddress.h"
#include"Socket.h"
#include"Epoll.h"
#include"Channel.h"
#include"TcpServer.h"
#include"TcpConnection.h"

using namespace std;

// 保存每个客户端连接的状态
struct Conn {
    std::string outbuf; // 输出缓冲区
    bool peerClosed = false; // 对端关闭写端后, 先把待发送数据发完再关闭连接
    Channel* channel = NULL;
};

/* 
    用 fd 找到连接状态 
    key: 客户端 fd
    value: 这个客户端对应的 Conn 状态
*/
std::unordered_map<int, Conn> conns; 

// 设置非阻塞IO
// int setnonblocking(int fd) {
//     int flags = fcntl(fd, F_GETFL, 0);
//     if (flags == -1) {
//         return -1;
//     }
//     // 写fcntl(fd, F_SETFL, O_NONBLOCK);就直接覆盖原来的flags
//     return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
// }

void closeConn(int epollfd, int fd) {
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);
    std::unordered_map<int, Conn>::iterator it = conns.find(fd);
    if(it != conns.end()) {
        delete it->second.channel;
        conns.erase(it);
    }
}

// 尽可能把 conns[fd].outbuf 里的数据发送给客户端
bool tryWrite(int epollfd, int fd) {
    /*
        it 是一个迭代器. 可以粗略理解成:
        it 是一个“指向 unordered_map 中某个元素的位置指针”
        但它不是普通指针, 它是容器提供的“类指针对象”. 
        unordered_map<int, Conn> 里面每一个元素长这样：
            std::pair<const int, Conn>

        也就是说, map 里面每一项可以理解成：
                {
                    key: int,
                    value: Conn
                }
        比如：
                key: fd = 5  ->  value: Conn{ outbuf = "...", peerClosed = false }
                key: fd = 6  ->  value: Conn{ outbuf = "...", peerClosed = false }
                key: fd = 7  ->  value: Conn{ outbuf = "...", peerClosed = true  }

        所以如果 it 找到了某个元素, 那么：
                it->first
        就是 key, 也就是 fd

                it->second
        就是 value, 也就是 Conn 对象.
    */
    std::unordered_map<int, Conn>::iterator it = conns.find(fd); // auto it == std::unordered_map<int, Conn>::iterator it

    // conns.end()表示 没有找到/越过最后一个元素的位置, 最后一个元素的下一个位置
    if(it == conns.end()) {
        return false;
    }

    std::string& out = it->second.outbuf;

    /*
        把用户态outbuf里的数据, 尽可能写入 fd 对应 socket 的内核发送缓冲区
        数据流
                服务器用户态 outbuf
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
    while(!out.empty()) {
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
        ssize_t n = send(fd, out.data(), out.size(), MSG_NOSIGNAL);

        if(n > 0) {
            out.erase(0, n);
            continue;
        }

        if(n == -1 && errno == EINTR) {
            continue;
        }

        // 在 socket 发送缓冲区满了, 暂时不能继续写
        if(n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return true;
        }
        perror("send");
        closeConn(epollfd, fd);
        return false;
    }
    return true;
}

// 修改某个客户端 fd 在 epoll 中关心的事件
bool modEvent(Channel* chn, uint32_t events) {
    chn->set__events_(events);
    return chn->update();
}

bool refreshConn(int epollfd, int fd) {
    std::unordered_map<int, Conn>::iterator it = conns.find(fd);
    if(it == conns.end()) {
        return false;
    }

    if(it->second.peerClosed && it->second.outbuf.empty()) {
        closeConn(epollfd, fd);
        return false;
    }

    uint32_t newEvent = EPOLLIN | EPOLLET | EPOLLRDHUP;
    if(!it->second.outbuf.empty()) {
        newEvent |= EPOLLOUT;
    }

    if(it->second.channel == NULL || !modEvent(it->second.channel, newEvent)) {
        perror("epoll_ctl mod clientfd");
        closeConn(epollfd, fd);
        return false;
    }

    return true;
}

// 处理客户端读事件
bool handleClientRead(int epollfd, int fd) {
    std::unordered_map<int, Conn>::iterator it = conns.find(fd);
    if(it == conns.end()) {
        return false;
    }
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
        ssize_t n = read(fd, buf, sizeof(buf));
        if(n > 0) {
            printf("recv from fd = %d, len = %zd, 读到的内容是: %.*s\n", fd, n, (int)n, buf);
            fflush(stdout);
            conns[fd].outbuf.append(buf, n);
            needWrite = true;
        } else if(n == 0) {
            // 对端正常关闭写端
            printf("clientfd(eventfd = %d) disconnected.\n", fd);
            conns[fd].peerClosed = true;
            break;
        } else {
            if(errno == EINTR) {
                continue;
            }
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                // ET模式下, 读到这里才算干净
                break;
            }

            perror("read");
            closeConn(epollfd, fd);
            return false;
        }
    }
    if(needWrite) {
        if(!tryWrite(epollfd, fd)) {
            return false;
        }
    }
    return refreshConn(epollfd, fd);
}

// 处理客户端写事件---lambda
bool handleClientWrite(int epollfd, int fd) {
    if(!tryWrite(epollfd, fd)) {
        return false;
    }
    return refreshConn(epollfd, fd);
}

// 处理客户端关闭事件
bool handleClientClose(int epollfd, int fd) {
    std::unordered_map<int, Conn>::iterator it = conns.find(fd);
    if(it == conns.end()) {
        return false;
    }
    it->second.peerClosed = true;
    return refreshConn(epollfd, fd);
}

// 处理客户端错误事件
bool handleClientError(int epollfd, int fd) {
    closeConn(epollfd, fd);
    return false;
}

// 给客户端Channel绑定固定回调
void setClientCallback(Channel* chn, int epollfd, int fd) {
    chn->setReadCallback([epollfd, fd](){
        return handleClientRead(epollfd, fd);
    });

    chn->setWriteCallback([epollfd, fd]() {
        return handleClientWrite(epollfd, fd);
    });

    chn->setCloseCallback([epollfd, fd](){
        return handleClientClose(epollfd, fd);
    });
    
    chn->setErrorCallback([epollfd, fd](){
        return handleClientError(epollfd, fd);
    });
}

bool handleAccept(Epoll* ep, Socket* socketServer, int epollfd, int opt) {
    while(1) {
        InetAddress clientaddr;
        Socket* sockClient = new Socket(socketServer->Accept4(clientaddr));
        int clientfd = sockClient->getfd();
        if(clientfd < 0) {
            if(errno == EINTR) {
                continue;
            }
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            perror("accept4");
            break;
        }
        setsockopt(clientfd, IPPROTO_TCP, TCP_NODELAY, &opt, static_cast<socklen_t>(sizeof(opt)));
        conns[clientfd] = Conn{};
        std::string ip = clientaddr.get_str_ip();
        printf("accept client(fd = %d, ip = %s, port = %d) ok\n", clientfd, ip.c_str(), clientaddr.getport());

        Channel* clientChn = new Channel(ep, clientfd);
        clientChn->set__events_(EPOLLIN | EPOLLET | EPOLLRDHUP);
        setClientCallback(clientChn, epollfd, clientfd);
        if(!clientChn->update()) {
            perror("epoll_ctl clientfd");
            close(clientfd);
            delete clientChn;
            conns.erase(clientfd);
            continue;
        }
        conns[clientfd].channel = clientChn;
    }

    return true;
}

int main(int argc, char* argv[]) {
    if(argc != 3) {
        printf("usage: ./epoll <ip> <server_port>\n");
        printf("example: ./epoll 127.0.0.1 8080\n");
        return -1;
    }

    // 把服务端fd设置为非阻塞
    // int listenfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    // if(listenfd == -1) {
    //     perror("socket: ");
    //     return -1;
    // }
    int listenfd = createNonBlockingFd();
    if(listenfd == -1) {
        return -1;
    }
    Socket sockServer(listenfd);

    int opt = 1;
    /* 
        1. SOL_REUSEADDR: 端口复用
    */
    // setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, static_cast<socklen_t>(sizeof(opt)));
    sockServer.SetReUseAddr(opt);
    
    /*  2. SOL_REUSEPOT: 
            允许多个独立的进程或线程同时绑定并监听同一个端口, 常用于多进程负载均衡
        Reactor中意义不大
    */
    // setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, &opt, static_cast<socklen_t>(sizeof(opt)));
    sockServer.SetReUsePort(opt);

    /* 
        3. SOL_KEEPALIVE: 
                心跳测试
                防止客户端突然断开连接(比如突然断电), 但是服务端不知道, 所以如果这个连接很久没动静, 内核会自动发一个探测包给对方
        用于检测长时间无数据的死连接. 不过默认检测周期很长, 实际项目里常常还会做应用层心跳
        效果: 
                对方回了: 说明还在, 继续维持:
                对方没回（或报错）: 说明连接已断, 内核会自动回收这个 Socket:
        TCP 默认的检测周期非常长（通常是 2 小时）, 所以很多应用层（如 Websocket 或游戏）会自己写心跳包, 不完全依赖这个
    */
    // setsockopt(listenfd, SOL_SOCKET, SO_KEEPALIVE, &opt, static_cast<socklen_t>(sizeof(opt)));
    sockServer.SetKeepAlive(opt);

    // struct sockaddr_in server;
    // memset(&server, 0, sizeof(server));
    // server.sin_family = AF_INET;
    char* end = NULL;
    long port = strtol(argv[2], &end, 10); // Convert a string to a long integer.
    if(*end != '\0' || port <= 0 || port > 65535) {
        printf("invalid port: %s\n", argv[2]);
        return -1;
    }
    // server.sin_port = htons(static_cast<uint16_t>(port));
    // if(inet_pton(AF_INET, argv[1], &server.sin_addr) <= 0) {
    //     perror("inet_pton");
    //     close(listenfd);
    //     return -1;
    // }
    InetAddress server(argv[1], port);

    // if(bind(listenfd, server.addr(), sizeof(server)) < 0) {
    //     perror("bind: ");
    //     close(listenfd);
    //     return -1;
    // }
    if(!sockServer.Bind(server)) {
        return -1;
    }

    // if(listen(listenfd, 128) < 0) {
    //     perror("listen: ");
    //     close(listenfd);
    //     return -1;
    // }
    if(!sockServer.Listen(128)) { // 让 socket 进入监听状态, 开始接收 TCP 连接请求
        return -1;
    }

    // int epollfd = epoll_create1(0); // 创建epoll句柄（红黑树）
    // if(epollfd == -1) {
    //     perror("epoll_create1");
    //     close(sockServer.getfd());
    //     close(listenfd);
    //     return -1;
    // }

    // epoll_event ev;
    // ev.data.fd = sockServer.getfd();
    // ev.events = EPOLLIN; // 采用水平触发

    // if(epoll_ctl(epollfd, EPOLL_CTL_ADD, sockServer.getfd(), &ev) == -1) {
    //     perror("epoll_ctl listenfd");
    //     close(epollfd);
    //     close(sockServer.getfd());
    //     close(listenfd);
    //     return -1;
    // }
    // epoll_event evs[10];
    Epoll ep;
    int epollfd = ep.getepollfd_();
    if(epollfd == -1) {
        return -1;
    }
    
    // 一个Channel对应一个epoll，一个fd， epoll和channel绑定
    Channel* serverChn = new Channel(&ep, sockServer.getfd()); // listenfd, epollfd
    // serverChn->set__events_(EPOLLIN); // 把监听 fd 加入 epoll 
    // if(!serverChn->update()) {        // 并监听读事件
    //     delete serverChn;
    //     return -1;
    // }
    /* [&ep, &sockServer, epollfd, opt] 是捕获列表. 
       ep 和 sockServer 按引用捕获, epollfd 和 opt 按值捕获. 
       之后 Channel::handleEvent() 不需要知道业务细节, 只需要调用 readCallback_()
       它的意思不是立刻执行 handleAccept(), 而是以后 listenfd有读事件时, 再调用这个函数
    */
    serverChn->setReadCallback([&ep, &sockServer, epollfd, opt](){
        return handleAccept(&ep, &sockServer, epollfd, opt);
    }); 
    serverChn->set__events_(EPOLLIN);
    if(!serverChn->update()) {  // 更新，注册到内核
        delete serverChn;
        return -1;
    }

    while(1) { // 事件循环
        // int infds = epoll_wait(epollfd, evs, 10, -1); // 等待监听的fd有事件发生

        // if(infds < 0) {
        //     if(errno == EINTR || errno == EWOULDBLOCK) {
        //         continue;
        //     }
        //     perror("epoll_wait: ");
        //     break;
        // }
        // if(infds == 0) { // 超时
        //     printf("epoll_wait timeout.\n");
        //     continue;
        // }
        std::vector<Channel*>chns = ep.loop(-1);
        if(chns.empty()) {
            continue;
        }
        // for(size_t i = 0; i < chns.size(); i++) {
        //     // listenfd 有事件, 表示有新连接到来
        //     if(chns[i]->getfd_() == sockServer.getfd()) { 
        //         while(1) {
        //             // struct sockaddr_in peeraddr;
        //             // socklen_t len = sizeof(peeraddr);
        //             // int clientfd = accept4(listenfd, (struct sockaddr*)&peeraddr, &len, SOCK_NONBLOCK);
        //             InetAddress clientaddr;
        //             // 注意, clientsock只能new出来, 不能在栈上, 否则析构函数会关闭fd
        //             // 还有, 这里new出来的对象没有释放, 这个问题以后再解决
        //             Socket* sockClient = new Socket(sockServer.Accept4(clientaddr));

        //             int clientfd = sockClient->getfd();
        //             if(clientfd < 0) {
        //                 if(errno == EINTR) {
        //                     continue;
        //                 }
        //                 if(errno == EAGAIN || errno == EWOULDBLOCK) {
        //                     break;
        //                 }
        //                 perror("accept4");
        //                 break;
        //             }
                    
        //             /*
        //                 4. TCP_NODELAY
        //                 默认情况下, TCP 使用Nagle 算法,如果只发了几个字节（比如点了一下鼠标）, 它会等一会儿, 想凑够一大波数据再发, 以此节省带宽
        //                 开启这个选项TCP_NODELAY即禁用 Nagle 算法, 有多少发多少, 降低延迟
        //             */
        //             setsockopt(clientfd, IPPROTO_TCP, TCP_NODELAY, &opt, static_cast<socklen_t>(sizeof(opt)));

        //             conns[clientfd] = Conn{};

        //             // char ip[INET_ADDRSTRLEN] = {0};
        //             // if(inet_ntop(AF_INET, &clientaddr.sin_addr, ip, sizeof(ip)) == NULL) {
        //             //     snprintf(ip, sizeof(ip), "unknown");
        //             // }
        //             printf("accept client(fd = %d, ip = %s, port = %d) ok.\n", clientfd, clientaddr.get_str_ip().c_str(), clientaddr.getport());
                    
        //             // 为新客户端准备读事件, 添加到epoll中
        //             // epoll_event ev;
        //             // ev.data.fd = clientfd;
        //             // ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP; // 边缘触发
        //             // if(epoll_ctl(epollfd, EPOLL_CTL_ADD, clientfd, &ev) == -1) {
        //             //     perror("epoll_ctl clientfd");
        //             //     close(clientfd);
        //             //     conns.erase(clientfd);
        //             //     continue;
        //             // }
        //             Channel* clientChn = new Channel(&ep, clientfd);
        //             clientChn->set__events_(EPOLLIN | EPOLLET | EPOLLRDHUP);
        //             if(!clientChn->update()) {
        //                 perror("epoll_ctl clientfd");
        //                 close(clientfd);
        //                 delete clientChn;
        //                 conns.erase(clientfd);
        //                 continue;
        //             }
        //             conns[clientfd].channel = clientChn;
        //         }
        //     } else { // 客户端连接的fd有事件
        //         uint32_t events = chns[i]->get__revents_();
        //         int fd = chns[i]->getfd_();

        //         if(events & EPOLLERR) {
        //             closeConn(epollfd, fd);
        //             continue;
        //         }
        //         // 避免以后出现“fd 已经被关闭, 但事件数组里还有旧事件”的情况
        //         auto it = conns.find(fd);
        //         if(it == conns.end()) {
        //             continue;
        //         }

        //         bool alive = true;
        //         bool needWrite = false;

        //         // 2. 有数据可读, 必须读到 EAGAIN
        //         if(events & (EPOLLIN | EPOLLPRI)) {
        //             /*
        //                 数据流
        //                         客户端用户态 buffer
        //                                 |
        //                                 | send()
        //                                 v
        //                         客户端内核 TCP 发送缓冲区
        //                                 |
        //                                 | TCP 协议栈 / 网络
        //                                 v
        //                         服务端内核中 clientfd 对应的 TCP 接收缓冲区
        //                                 |
        //                                 | 服务端 read(clientfd, ...)
        //                                 v
        //                         服务端用户态 buf
        //             */
        //             /*
        //                 加上 while, 这样在第一次 EPOLLIN 触发时就循环读取所有可用数据,
        //                 直到 read 返回 EAGAIN → 这时缓冲区空了, ET 的规则就不会漏数据 
        //             */
        //             while(1) {
        //                 char buf[1024];
        //                 ssize_t n = read(fd, buf, sizeof(buf));
        //                 if(n > 0) {
        //                     printf("recv from fd %d, len = %zd: %.*s\n", fd, n, (int)n, buf);
        //                     fflush(stdout);
        //                     conns[fd].outbuf.append(buf, n);
        //                     needWrite = true;
        //                 } else if(n == 0) {
        //                     // 对端正常关闭写端
        //                     printf("clientfd(eventfd = %d) disconnected.\n", fd);
        //                     conns[fd].peerClosed = true;
        //                     break;
        //                 } else {
        //                     if(errno == EINTR) {
        //                         continue;
        //                     }
        //                     // 内核接收缓冲区已经被读空了
        //                     if(errno == EAGAIN || errno == EWOULDBLOCK) {
        //                         // ET 模式下，读到这里才算读干净
        //                         break;
        //                     }

        //                     perror("read");
        //                     closeConn(epollfd, fd);
        //                     alive = false;
        //                     break;
        //                 }
        //             }
        //         }

        //         if(!alive) {
        //             continue;
        //         }

        //         // 3. 对端关闭 / 半关闭，不一定马上 close
        //         if(events & (EPOLLRDHUP | EPOLLHUP)) { // 对方关闭连接
        //             conns[fd].peerClosed = true;
        //         }

        //         // 4. 有数据要写，或者 fd 当前可写, 就尝试写
        //         if(needWrite || (events & EPOLLOUT)) {
        //             if(!tryWrite(epollfd, fd)) {
        //                 continue;
        //             }
        //         }

        //         // 5. tryWrite 可能关闭 fd, 所以重新找一次
        //         it = conns.find(fd);
        //         if(it == conns.end()) {
        //             continue;
        //         }

        //         // 6. 如果对端已经关闭, 并且服务器也没有待发送数据了, 就关闭连接
        //         if(it->second.peerClosed && it->second.outbuf.empty()) {
        //             closeConn(epollfd, fd);
        //             continue;
        //         }

        //         // 7. 根据是否还有待写数据, 重新设置关心的事件
        //         uint32_t newEvents = EPOLLIN | EPOLLET | EPOLLRDHUP;

        //         if(!it->second.outbuf.empty()) {
        //             newEvents |= EPOLLOUT;
        //         }

        //         if(!modEvent(chns[i], newEvents)) {
        //             perror("epoll_ctl mod clientfd");
        //             closeConn(epollfd, fd);
        //         }
        //     }
        // }

        for(size_t i = 0; i < chns.size(); i++) {
            chns[i]->handleEvent();
        }
        
    }
    return 0;
}
