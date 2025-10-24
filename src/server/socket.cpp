#include "socket.h"
#include <iostream>
#include <sys/socket.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
Socket::Socket()
{

}
Socket::~Socket()
{


}
int Socket::create_server_socket()
{
    // create server_fd
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        std::cerr << "socket failed..." << std::endl;
        return false;
    }
    return server_fd;
}

bool Socket::bind_server_socket(int fd, int port)
{
    struct sockaddr_in server_addr_in{};
    server_addr_in.sin_addr.s_addr = INADDR_ANY;
    server_addr_in.sin_family = AF_INET;
    server_addr_in.sin_port = htons(port);
    socklen_t serverLen = sizeof(server_addr_in);
    if (bind(fd, (sockaddr *)&server_addr_in, serverLen) < 0)
    {
        std::cerr << "bind failed..." << std::endl;
        return false;
    }
    return true;
}
int Socket::accept_socket(int server_fd)
{
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (sockaddr *)&client_addr, &client_len);
    if (client_fd < 0){
        std::cerr << "accept failed..." << std::endl;
    }
    return client_fd;
}
bool Socket::listen_socket(int fd)
{
    if(listen(fd, MAXCLIENTFDS) < 0){
        std::cerr << "listen failed..." << std::endl;
        return false;
    }
    return true;
}
bool Socket::set_socket_option(int fd)
{
    //set REUSADDR
    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        std::cerr << "setsockopt failed..." << std::endl;
        return false;
    }
    return true;
}

bool Socket::set_nonblock(int socketFd)
{
    int flags = fcntl(socketFd, F_GETFL, 0);
    if (flags < 0)
    {
        std::cerr << "fcntl failed..." << std::endl;
        return false;
    }
    fcntl(socketFd, F_SETFL, flags | O_NONBLOCK);
    return true;
}