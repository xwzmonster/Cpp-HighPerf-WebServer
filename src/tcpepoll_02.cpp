#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<errno.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<sys/fcntl.h>
#include<sys/epoll.h>
#include<netinet/tcp.h> // TCP_NODELAY需要包含这个头文件
#include<string.h>
#include<string>
#include<unordered_map>
#include<vector>

#include"TcpServer.h"
#include"Eventloop.h"

int main(int argc, char* argv[]) {
    if(argc != 3) {
        printf("usage: ./epoll <ip> <server_port>\n");
        printf("example: ./epoll 127.0.0.1 8080\n");
        return -1;
    }

    char* end = NULL;
    long port = strtol(argv[2], &end, 10);
    if(*end != '\0' || port <= 0 || port > 65535) {
        printf("invaild port: %s\n", argv[2]);
        return -1;
    }
    Eventloop loop;
    TcpServer server(&loop, argv[1], static_cast<uint16_t>(port));
    server.start();
    return 0;
}