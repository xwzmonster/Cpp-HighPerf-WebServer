#pragma once

#include <cstddef>
#include<vector>

/*
    帮程序临时保存已经从 socket 读出来、但业务还没处理完的数据;
    或者保存业务想发送、但暂时还没写进内核的数据
    这里的buffer是用户态的
    
    以“客户端发数据给服务端”为例
            客户端用户态数据
                ↓ send()
            客户端内核发送缓冲区
                ↓ TCP/IP 网络传输
            服务端内核接收缓冲区
                ↓ read()/recv()
            服务端用户态临时数组 / Buffer（在这层）
                ↓ 业务逻辑解析
            服务端业务处理
*/ 
class Buffer {
private:
    std::vector<char> buffer_; // 创建一块初始大小的内存, 并把读写位置初始化好
    size_t readerIndex_; // 记录已经读到哪里
    size_t writerIndex_;  // 表示已经写到哪里

public:
    Buffer();
/*
    A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
   
    +-------------------+------------------+------------------+
    | prependable bytes |  readable bytes  |  writable bytes  |
    | 已经读过的区域     |  还没处理的数据   |还可以写入的空闲区 |
    +-------------------+------------------+------------------+
    |                   |                  |                  |
    0      <=      readerIndex   <=   writerIndex    <=     size   
    
    区间[readerIndex_, writeIndex_): 表示当前有效数据
    readerIndex_ == writeIndex_: 表示没有数据可读
*/
    // 返回当前 Buffer 里有多少字节是有效数据, 表示还有多少数据可以读
    size_t readableBytes() const; 

    // 返回当前 Buffer 后面还有多少空闲空间可以直接写入, 表示还有多少数据可以写
    size_t writeableBytes() const;

    // 拿到当前可读数据的起始地址, 即buffer_.data() + readerIndex_ 
    const char* peek() const; 

    // 消费掉 len 个字节, 表示前 len 个字节已经处理完了, 可以丢掉
    // 注意, 这里的“丢掉”通常不是立刻从 vector 里删除, 而是移动 readerIndex_
    void retrieve(size_t len);

    // 清空所有可读数据
    void retrieveAll();

    // 表示追加网络收到的数据, 把新收到的数据追加到 Buffer 的可写区域
    // append 要保证空间足够，然后把 data 复制进去
    void append(const char* data, size_t len); 

    ~Buffer();
};