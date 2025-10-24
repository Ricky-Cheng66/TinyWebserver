#pragma once
#include <sys/epoll.h>
class Epoll
{
public:
    Epoll();
    ~Epoll();
    int create_epoll();
    bool add_epoll_server(int);
    bool delete_epoll(int);
    void modify_epoll();
    bool events_loop();
private:
    const int MAXEVENTS = 64;
    int epfd_{};
    int server_fd_{};
    epoll_event ev{};
}ep;