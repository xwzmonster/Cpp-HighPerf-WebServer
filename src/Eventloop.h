#pragma once
#include<unistd.h>
#include<sys/epoll.h>

class Channel;
class Epoll;
class TcpServer;

class Eventloop {
private:  
    TcpServer tcpServer;

public:
    void loop();
    
};