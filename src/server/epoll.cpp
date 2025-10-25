#include "epoll.h"
#include "socket.h"
#include <iostream>
#include <sys/epoll.h>
#include <unistd.h>
#include <system_error>
Epoll::~Epoll() {
  if (epfd_ != -1) {
    close(epfd_);
    epfd_ = -1;
  }
}
bool Epoll::initialize() {
  if (epfd_ != -1) {
    std::cerr << "Epoll already initialized..." << std::endl;
    return true; //已经初始化过了
  }
  epfd_ = epoll_create1(0);
  if (epfd_ < 0) {
    std::error_code ec(errno, std::system_category());
    std::cerr << "epoll_create1 failed..." << ec.message() << std::endl;
    return false;
  }
  return true;
}

bool Epoll::add_epoll_server(int fd) {
  std::lock_guard<std::mutex> lock(epoll_mutex_);
  struct epoll_event ev {};
  ev.data.fd = fd;
  ev.events = EPOLLIN;
  if (epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev)) {
    std::error_code ec(errno, std::system_category());
    std::cerr << "epoll_ctl add failed..." << ec.message() <<  std::endl;
    return false;
  }
  return true;
}
bool Epoll::delete_epoll(int fd) {
  std::lock_guard<std::mutex> lock(epoll_mutex_);
  if (epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr)) {
    std::error_code ec(errno, std::system_category());
    std::cerr << "epoll_ctl del failed..." << ec.message() << std::endl;
    return false;
  }
  return true;
}
void Epoll::modify_epoll() {}
int Epoll::wait_events(struct epoll_event *evs, int timeout) {
  if (epfd_ < 0) {
    return -1;
  }
  timeout = -1;
  int nfds = epoll_wait(epfd_, evs, max_events_, timeout);
  if (nfds < 0) {
    if (errno == EINTR) {
      //被信号中断
      std::cerr << "epoll_wait 被信号中断" << std::endl;
      return -1;
    }
    std::cerr << "epoll_wait other errors" << std::endl;
    return -1;
  }
  return nfds;
}
