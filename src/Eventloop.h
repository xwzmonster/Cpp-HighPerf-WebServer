#pragma once
#include<unistd.h>
#include<sys/epoll.h>
#include<memory>
#include<vector>

class Epoll;
class Channel;

class Eventloop { // 负责事件循环和Epoll, Eventloop拥有Epoll
private:  
    std::unique_ptr<Epoll> ep_;

public:
    Eventloop();
    Eventloop(const Eventloop&) = delete;
    Eventloop& operator=(const Eventloop&) = delete;
    
    // 运行epoll_wait(), 等待事件发生, 已发生的事件用vector容器返回
    std::vector<Channel*> poll(int timeout); 
    
    // 把channel添加/更新到红黑树上, channel中有fd, 也有需要监视的事件
    bool updateChannel(Channel* chn);
    
    // 删除Channel
    bool removeChannel(Channel* chn);

    ~Eventloop();
};