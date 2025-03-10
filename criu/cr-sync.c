#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/un.h>

#include "cr-sync.h"

enum STATE now_state = READY;

int syncServerInit(char *ip, int port){
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    char *ack = "ACK from Server";

    // create socket fd
    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // bind socket to port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = port;

    if(bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // listen for connections
    if(listen(server_fd, 3) < 0){
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // accept connection
    if((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0){
        perror("accept");
        exit(EXIT_FAILURE);
    }

    return new_socket;
}

int syncClientInit(char *ip, int port){
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    // create socket fd
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = port;

    // convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, ip, &serv_addr.sin_addr)<=0){
        perror("inet_pton");
        exit(EXIT_FAILURE);
    }

    // connect to server
    if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        perror("connect");
        exit(EXIT_FAILURE);
    }
    return sock;
}

int sync_wait(int sockfd){
    int read_bytes = 0, write_bytes = 0;
    char buffer[20];

    write_bytes = write(sockfd, "wait", 4);
    if (write_bytes < 0){
        perror("write");
        exit(EXIT_FAILURE);
    }

    read_bytes = read(sockfd, buffer, 4);
    if (read_bytes < 0){
        perror("read");
        exit(EXIT_FAILURE);
    }

    if (write_bytes == read_bytes)
        return 0;
    else
        return 1;
}


int wait_state(int sockfd, enum STATE state){
    int ret = 0;
    int now_state;
    while (1){
        ret = recv(sockfd, &now_state, sizeof(state), 0);
        if (now_state >= state && ret != 0){
            break;
        }
    }
    return 0;
}

int notify_peer(int sockfd, enum STATE state){
    int ret = 0;
    // 切换到目标状态上去，再通知对方
    ret = send(sockfd, &state, sizeof(state), 0);
    return ret;
}

int update_state(int sockfd, enum STATE state){
    int ret = 0;
    now_state = state;
    ret = notify_peer(sockfd, state);
    return ret;
}

// 服务器函数
int syncServerInit_unix(char *socket_path) {
    int server_fd, new_socket;
    struct sockaddr_un address;
    int addrlen = sizeof(address);

    // 创建 socket fd
    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // 设置 socket 地址
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, socket_path, sizeof(address.sun_path) - 1);

    // 绑定 socket 到地址
    unlink(socket_path); // 删除旧的 socket 文件，确保不会冲突
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // 监听连接
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // 接受连接
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    return new_socket;
}

// 客户端函数
int syncClientInit_unix(char *socket_path) {
    int sock = 0;
    struct sockaddr_un serv_addr;

    // 创建 socket fd
    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // 设置服务器地址
    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, socket_path, sizeof(serv_addr.sun_path) - 1);

    // 连接到服务器
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    return sock;
}








