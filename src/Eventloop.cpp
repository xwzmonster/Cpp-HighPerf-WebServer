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

Eventloop::~Eventloop() = default;