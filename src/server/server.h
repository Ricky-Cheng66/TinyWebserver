#pragma once
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <string_view>

class Server
{
public:
    Server();

    ~Server();

    bool init(int server_port);

    bool start();

private:
    const int MAXCLIENTFDS = 1024;
    int server_fd_{};
    std::string server_addr_{};
    int server_port_{};

};


