#include"Epoll.h"
#include"Channel.h"

/*
    Epoll();
    ~Epoll();

    void AddFd(int fd, epoll_event ev); // 把fd和它需要监视的事件添加到红黑树上
    std::vector<epoll_event> loop(int timeout); // 运行epoll_wait(), 等待事件发生, 已发生的事件用vector容器返回

*/

Epoll::Epoll() {
    this->epollfd_ = epoll_create1(0);
    if(epollfd_ == -1) {
        perror("epoll_create1");
    }
}

Epoll::~Epoll() {
    if(epollfd_ >= 0) {
        close(this->epollfd_);
    }
}

// bool Epoll::AddFd(int fd, uint32_t op) {
//     epoll_event ev;
//     ev.data.fd = fd;
//     ev.events = op;
//     if(epoll_ctl(this->epollfd_, EPOLL_CTL_ADD, fd, &ev) == -1) {
//         perror("epoll_ctl: ");
//         return false;
//     }
//     return true;
// }
bool Epoll::UpdateChannel(Channel* chn){ // 把channel添加/更新到红黑树上, channel中有fd, 也有需要监视的事件
    epoll_event ev;
    memset(&ev, 0, sizeof(epoll_event));
    ev.data.ptr = chn;
    ev.events = chn->get__events_(); 

    if(chn->get_isInEpoll_()) { // channel 已经在树上
        if(epoll_ctl(this->getepollfd_(), EPOLL_CTL_MOD, chn->getfd_(), &ev) == -1) {
            perror("epoll_ctl: ");
            return false;
        }
        return true;
    } else { // 不在树上
        bool opt = false;
        if(epoll_ctl(this->getepollfd_(), EPOLL_CTL_ADD, chn->getfd_(), &ev) == -1) {
            perror("epoll_ctl: ");
            return false;
        }
        opt = true;
        chn->set__isInepoll_(opt);
        return true;
    }
}

int Epoll::getepollfd_() {
    return this->epollfd_;
}

// std::vector<epoll_event> Epoll::loop(int timeout) {
//     std::vector<epoll_event>evs; // 存放epoll_wait返回的事件

//     int infds = epoll_wait(this->epollfd_, this->events_, MAXEVENTS, timeout);
//     if(infds < 0) {
//         if(errno == EINTR || errno == EWOULDBLOCK) {
//             return evs;
//         }
//         perror("epoll_wait:");
//         return evs;
//     }

//     if(infds == 0) { // 超时
//         printf("epoll_wait timeout.\n");
//         return evs;
//     }

//     for(int i = 0; i < infds; i++) {
//         evs.push_back(this->events_[i]);
//     }

//     return evs;
// }

std::vector<Channel*> Epoll::loop(int timeout) {
    std::vector<Channel*> chns;
    int infds = epoll_wait(this->epollfd_, this->events_, this->MAXEVENTS, timeout);

    if(infds < 0) {
        if(errno == EINTR || errno == EWOULDBLOCK) {
            return chns;
        }
        perror("epoll_wait: ");
        return chns;
    }
    if(infds == 0) { // 超时
        printf("timeout!\n");
        return chns;
    }
    for(int i = 0; i < infds; i++) {
        Channel* chn = (Channel*)this->events_[i].data.ptr;
        chn->set__revents_(this->events_[i].events);
        chns.push_back(chn);
    }
    return chns;
}

bool Epoll::RemoveChannel(Channel* chn) {
    if(!chn->get_isInEpoll_()) {
        return true;
    }
    if(epoll_ctl(this->epollfd_, EPOLL_CTL_DEL, chn->getfd_(), nullptr) == -1) {
        perror("epoll_ctl del");
        return false;
    }
    chn->set__isInepoll_(false);
    return true;
}

