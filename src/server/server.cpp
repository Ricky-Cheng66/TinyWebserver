#include "server.h"
#include "../http/request.h"
#include "../http/response.h"
#include "socket.h"
#include "epoll.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
extern Socket server_socket;
extern Epoll ep;
Server::Server()
{

}

Server::~Server()
{

}
bool Server::init(int server_port)
{
    //socket部分
    server_port_ = server_port;
    // create server_fd
    server_fd_ = server_socket.create_server_socket();
    //设置地址重用
    if (!server_socket.set_socket_option(server_fd_)){
        std::cerr << "set_socket_option failed..." << std::endl;
    }
    // set listenFd nonblock
    server_socket.set_nonblock(server_fd_);
    // create epfd and put listenFd into epoll
    ep.create_epoll();
    //add_epoll
    ep.add_epoll_server(server_fd_);
    //blind
    server_socket.bind_server_socket(server_fd_, server_port_);
    return true;
}
bool Server::start()
{
    server_socket.listen_socket(server_fd_);
    while (1) // loop of events
    {
        //epoll_wait
        ep.events_loop();
    }
    return true;
}
