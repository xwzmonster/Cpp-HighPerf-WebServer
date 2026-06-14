#include"InetAddress.h"

// 空参
InetAddress::InetAddress() : addr_{} {
    
}

// listenfd用的构造函数
InetAddress::InetAddress(const std::string& ip, uint16_t port) : addr_{} {
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    if(inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr) <= 0) {
        perror("inet_pton: ");
    }
}

// 客户端连接的fd用的构造函数
InetAddress::InetAddress(const sockaddr_in addr) : addr_(addr) {
    
}

InetAddress::~InetAddress() {
    
}

// 返回字符串表示的地址, 例如192.168.150.128
const std::string InetAddress::get_str_ip()const { 
    char ip[INET_ADDRSTRLEN] = {0};
    const char* p = inet_ntop(AF_INET, &addr_.sin_addr, ip, sizeof(ip));
    if(p == NULL) {
        return "unknown";
    }
    return ip;
}

// 返回整数表示的端口, 例如8080, 80
uint16_t InetAddress::getport()const { 
    return ntohs(addr_.sin_port);
}

// 返回addr_成员的地址
const sockaddr* InetAddress:: addr()const { 
    return (sockaddr*)&addr_;
}

void InetAddress::setaddr(struct sockaddr_in addr) {
    this->addr_ = addr;
}

