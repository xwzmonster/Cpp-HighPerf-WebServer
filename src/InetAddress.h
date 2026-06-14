#pragma once

#include<arpa/inet.h>
#include<netinet/in.h>
#include<string>

// socket的地址协议族
class InetAddress {
private:
    sockaddr_in addr_; // 表示地址协议的结构体

public:
    // 空参
    InetAddress();
    // 如果是监听的fd, 用这个构造函数
    InetAddress(const std::string& ip, uint16_t port); 
    // 客户端fd
    InetAddress(const sockaddr_in addr);
    ~InetAddress();

    const std::string get_str_ip()const; // 返回字符串表示的地址, 例如192.168.150.128
    uint16_t getport()const; // 返回整数表示的端口, 例如8080, 80
    const sockaddr* addr()const; // 返回addr_成员的地址, 由sockaddr_in转换成了sockaddr
    void setaddr(struct sockaddr_in addr);

};