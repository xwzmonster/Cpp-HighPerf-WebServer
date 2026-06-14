#pragma once
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<strings.h>
#include<string.h>
#include<sys/epoll.h>
#include<vector>
#include<unistd.h>

class Channel;

// Epoll 不关心业务，它只负责“等事件”和“注册事件”
class Epoll { 
private:
    static const int MAXEVENTS = 10;
    int epollfd_ = -1; // 创建epoll句柄（红黑树）, 构造函数中创建
    epoll_event events_[MAXEVENTS];  // 存放实际发生的事件

public:
    Epoll();
    ~Epoll();
    Epoll(const Epoll&) = delete;
    Epoll& operator=(const Epoll&) = delete;

    int getepollfd_();

    // bool AddFd(int fd, uint32_t op); // 把fd和它需要监视的事件添加到红黑树上
    bool UpdateChannel(Channel* chn); // 把channel添加/更新到红黑树上, channel中有fd, 也有需要监视的事件
    
    // std::vector<epoll_event> loop(int timeout); // 运行epoll_wait(), 等待事件发生, 已发生的事件用vector容器返回
    std::vector<Channel*> loop(int timeout);// 运行epoll_wait(), 等待事件发生, 已发生的事件用vector容器返回

    // 删除Channel
    bool RemoveChannel(Channel* chn);
};