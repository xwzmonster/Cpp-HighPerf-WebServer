#pragma once
#include<sys/epoll.h>
#include<functional>

class Epoll;

// 封装“一个 fd 在 epoll 里的状态”. 它保存 fd、想监听的事件、实际发生的事件、回调函数
class Channel {
private:
    int fd_; // Channel拥有的fd, Channel和fd是一对一的关系
    Epoll* ep_; // Channel对应的红黑树, Channel与Epoll是多对一的关系, 一个Channel只对应一个Epoll
    bool isInEpoll_; // Channel是否已添加到epoll树上, 如果已添加, 调用 EPOLL_CTL_MOD；否则调用 EPOLL_CTL_ADD
    uint32_t events_; // 需要监听的事件
    uint32_t revents_; // 实际发生的事件(epoll_wait 返回时, 由 Epoll 类把内核返回的具体事件填充到这里)
    
    typedef std::function<bool()> EventCallback;
    EventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;

public:
    explicit Channel(Epoll* ep, int fd);
    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;
    ~Channel();

    int getfd_();
    void UseET(); // 设置采用边缘触发ET模式
    void enableReading(); // 让epoll_wait监听fd_的读事件
    void set__events_(uint32_t ev); // 设置成员变量events_
    void set__revents_(uint32_t ev); // 设置成员变量revents
    void set__isInepoll_(bool opt); // 设置成员变量inepoll_
    uint32_t get__events_(); // 返回events_成员
    uint32_t get__revents_(); // 返回revents成员
    bool get_isInEpoll_();
    bool update();

    // 把绑定回调
    // 把“事件检测”和“事件处理”分开. Epoll 只知道有事件, Channel 知道这个事件应该调用哪个函数
    void setReadCallback(const EventCallback& cb);
    void setWriteCallback(const EventCallback& cb);
    void setCloseCallback(const EventCallback& cb);
    void setErrorCallback(const EventCallback& cb);

    // 根据 revents_ 调用对应回调
    bool handleEvent(); // 事件处理函数, epoll_wait()返回的时候执行

    bool remove();
};