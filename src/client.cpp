#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<time.h>

bool sendAll(int sockfd, const char* buf, size_t len) {
    size_t total = 0;

    while(total < len) {
        ssize_t n = send(sockfd, buf + total, len - total, MSG_NOSIGNAL);
        if(n > 0) {
            total += static_cast<size_t>(n);
            continue;
        }

        if(n == -1 && errno == EINTR) {
            continue;
        }

        return false;
    }

    return true;
}

ssize_t recvAll(int sockfd, char* buf, size_t len) {
    size_t total = 0;

    while(total < len) {
        ssize_t n = recv(sockfd, buf + total, len - total, 0);
        if(n > 0) {
            total += static_cast<size_t>(n);
            continue;
        }

        if(n == 0) {
            return static_cast<ssize_t>(total);
        }

        if(errno == EINTR) {
            continue;
        }

        return -1;
    }

    return static_cast<ssize_t>(total);
}

int main(int argc, char* argv[]) {
    if(argc != 3) {
        printf("usage: ./client <ip> <port>\n");
        printf("example: ./client 127.0.0.1 8080\n");
        printf("Please try again");
        return -1;
    }

    char buf[1024];
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        perror("socket: ");
        return -1;
    }

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    if(inet_pton(AF_INET, argv[1], &server.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        return -1;
    }
    char* end = NULL;
    long port = strtol(argv[2], &end, 10);
    if(*end != '\0' || port <= 0 || port > 65535) {
        printf("invalid port: %s\n", argv[2]);
        close(sockfd);
        return -1;
    }
    server.sin_port = htons(static_cast<uint16_t>(port));
    server.sin_family = AF_INET;

    if(connect(sockfd, (struct sockaddr*)&server, sizeof(server)) == -1) {
        perror("connect: ");
        printf("connect(%s:%s) failed.\n", argv[1], argv[2]);
        close(sockfd);
        return -1;
    }

    printf("connect success\n");

    for(int i = 0; i < 200000; i++) {
        memset(buf, 0, sizeof(buf));
        printf("input: ");
        if(scanf("%1023s", buf) != 1) {
            printf("input closed.\n");
            break;
        }

        size_t len = strlen(buf);
        if(!sendAll(sockfd, buf, len)) {
            perror("send: ");
            printf("send() failed.\n");
            close(sockfd);
            return -1;
        }

        char recvbuf[1024];
        memset(recvbuf, 0, sizeof(recvbuf));
        ssize_t n = recvAll(sockfd, recvbuf, len);
        if(n == -1) {
            perror("recv: ");
            printf("recv() failed.\n");
            close(sockfd);
            return -1;
        }
        if(static_cast<size_t>(n) < len) {
            printf("server closed connection.\n");
            close(sockfd);
            return 0;
        }
        printf("recv: %.*s\n", (int)n, recvbuf);
    }
    close(sockfd);
    return 0;
}
