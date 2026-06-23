#include"Buffer.h"
#include <algorithm>

Buffer::Buffer() : buffer_(1024), readerIndex_(0), writerIndex_(0) {
    
}

size_t Buffer::readableBytes() const {
    return (this->writerIndex_ - this->readerIndex_);
}

size_t Buffer::writeableBytes() const {
    return (this->buffer_.size() - this->writerIndex_);
}

const char* Buffer::peek() const {
    return (this->buffer_.data() + this->readerIndex_);
}

void Buffer::retrieve(size_t len) {
    if(len < readableBytes()) {
        this->readerIndex_ += len;
    } else {
        retrieveAll();
    }
}

void Buffer::retrieveAll() {
    this->readerIndex_ = 0;
    this->writerIndex_ = 0;
}

void Buffer::append(const char* data, size_t len) {
    if(writeableBytes() < len) {
        buffer_.resize(writerIndex_ + len);
    }
    std::copy(data, data + len, buffer_.begin() + writerIndex_);
    this->writerIndex_ += len;
}

Buffer::~Buffer() = default;