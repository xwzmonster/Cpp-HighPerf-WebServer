#include<unistd.h>
#include<vector>

#include "Eventloop.h"
#include "Epoll.h"
#include "Channel.h"

Eventloop::Eventloop() : ep_(std::make_unique<Epoll>()){

}

std::vector<Channel*> Eventloop::poll(int timeout) {
    return this->ep_->loop(timeout);
}

bool Eventloop::updateChannel(Channel* chn) {
    return this->ep_->UpdateChannel(chn);
}

bool Eventloop::removeChannel(Channel* chn) {
    return this->ep_->RemoveChannel(chn);
}

// Epoll* Eventloop::getEpoll() {
//     return this->ep_.get();
// }

void Eventloop::setRemoveConnectionCallback(const RemoveConnectionCallback& cb) {
    this->RemoveConnectionCallback_ = cb;
}

void Eventloop::loop() {
    while(1) {
        std::vector<Channel*> chns = this->poll(-1);
        if(chns.empty()) {
            continue;
        }
        for(Channel* chn : chns) {
            int fd = chn->getfd_();
            bool alive = chn->handleEvent();

            if(!alive && this->RemoveConnectionCallback_) {
                this->RemoveConnectionCallback_(fd);
            }
        }
    }
}

Eventloop::~Eventloop() = default;