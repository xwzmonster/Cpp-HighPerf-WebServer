#include"Channel.h"
// #include"Epoll.h"
#include"Eventloop.h"

Channel::Channel(Eventloop* loop, int fd) {
    // this->ep_ = ep;
    this->loop_ = loop;
    this->fd_ = fd;
    this->isInEpoll_ = false;
    this->events_ = 0; // 需要监听的事件
    this->revents_ = 0; // 已发生的事件 
} 
Channel::~Channel() {
/*
    在析构函数中, 不要销毁ep, 也不能关闭fd, 因为这两个东西
    不属于Channel类, Channel类只是需要它们, 使用它们而已
*/
}

int Channel::getfd_() {
    return this->fd_;
}

void Channel::UseET() { // 设置采用边缘触发ET模式
    this->events_ |= EPOLLET;
}

void Channel::enableReading() { // 让epoll_wait监听fd_的读事件
    this->events_ |= EPOLLIN;
    this->loop_->updateChannel(this);
}

void Channel::set__isInepoll_(bool opt) { // 设置成员变量inepoll_ = true;
    this->isInEpoll_ = opt;
}

void Channel::set__events_(uint32_t ev) { // 设置成员变量events_
    this->events_ = ev;
}

void Channel::set__revents_(uint32_t ev) { // 设置成员变量revents
    this->revents_ = ev;
}

uint32_t Channel::get__events_() { // 返回events_成员
    return this->events_;
}

uint32_t Channel::get__revents_() { // 返回revents成员
    return this->revents_;
}

bool Channel::get_isInEpoll_() {
    return this->isInEpoll_;
}

bool Channel::update() {
    return this->loop_->updateChannel(this);
}

void Channel::setReadCallback(const EventCallback& cb) {
    this->readCallback_ = cb;
}

void Channel::setWriteCallback(const EventCallback& cb) {
    this->writeCallback_ = cb;
}

void Channel::setCloseCallback(const EventCallback& cb) {
    this->closeCallback_ = cb;
}

void Channel::setErrorCallback(const EventCallback& cb) {
    this->errorCallback_ = cb;
}

bool Channel::handleEvent() {
    uint32_t events = this->revents_;

    EventCallback readCallback = this->readCallback_;
    EventCallback writeCallback = this->writeCallback_;
    EventCallback closeCallback = this->closeCallback_;
    EventCallback errorCallback = this->errorCallback_;

    if((events & EPOLLERR) && errorCallback) {
        return errorCallback();
    }
    if((events & (EPOLLIN | EPOLLPRI)) && readCallback) {
        if(!readCallback()) {
            return false;
        }
    }
    if((events & (EPOLLRDHUP | EPOLLHUP)) && closeCallback) {
        if(!closeCallback()) {
            return false;
        }
    }
    if((events & EPOLLOUT) && writeCallback) {
        if(!writeCallback()) {
            return false;
        }
    }
    return true;
}

bool Channel::remove() {
    return this->loop_->removeChannel(this);
}