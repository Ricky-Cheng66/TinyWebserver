#include "epoll.h"
#include <sys/epoll.h>
Epoll::Epoll()
{


}
Epoll::~Epoll()
{

}
int Epoll::create_epoll() 
{
    epfd_ = epoll_create(1);
}
void Epoll::add_epoll()
{

}
void Epoll::delete_epoll()
{

}
void Epoll::modify_epoll()
{

}