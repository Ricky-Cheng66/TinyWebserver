#pragma once
class Socket
{
public:
    Socket();
    ~Socket();
    int create_server_socket();
    bool bind_server_socket(int fd, int port);
    int accept_socket(int server_fd);
    bool listen_socket(int fd);
    bool set_socket_option(int fd);
    bool set_nonblock(int socketFd);
private:
    const int MAXCLIENTFDS = 1024;

};