#include "epoll.h"
#include "socket.h"
#include <iostream>
#include <sys/epoll.h>
#include <unistd.h>
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
    std::cerr << "epoll_create1 failed..." << std::endl;
    return false;
  }
  return true;
}

bool Epoll::add_epoll_server(int fd) {
  struct epoll_event ev {};
  ev.data.fd = fd;
  ev.events = EPOLLIN;
  if (epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev)) {
    std::cerr << "epoll_ctl add failed..." << std::endl;
    return false;
  }
  return true;
}
bool Epoll::delete_epoll(int fd) {
  if (epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr)) {
    std::cerr << "epoll_ctl del failed..." << std::endl;
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
// bool Epoll::events_loop() {
//   epoll_event evs[max_events_];
//   int nfds = epoll_wait(epfd_, evs, MAXEVENTS, -1);
//   if (nfds < 0) {
//     if (errno == EINTR) {
//       //被信号中断
//       std::cerr << "epoll_wait 被信号中断" << std::endl;
//       return false;
//     }
//     std::cerr << "epoll_wait other errors" << std::endl;
//     return false;
//   }
//   for (int i = 0; i < nfds; i++) {
//     if (evs[i].data.fd == server_fd_) { // accep
//       int client_fd = server_socket.accept_socket(evs[i].data.fd);
//       if (client_fd < 0) {
//         if (errno == EAGAIN || errno == EMFILE) {
//           continue; // 资源暂时不可用
//         }
//         std::cerr << "accept failed..." << std::endl;
//         continue;
//       }
//       Socket client_socket{};
//       client_socket.set_nonblock(client_fd);

//       std::cout << "new client connected..." << std::endl;
//       ev.data.fd = clientFd;
//       ev.events = EPOLLIN | EPOLLET;
//       epoll_ctl(epfd, EPOLL_CTL_ADD, ev.data.fd, &ev);
//     } else if (evs[i].events & EPOLLIN) {
//       // read dealing
//       /* 一次性读完（短连接够用） */
//       int fd = evs[i].data.fd;
//       char buf[4096];
//       ssize_t nread = read(fd, buf, sizeof(buf) - 1);
//       if (nread <= 0) { // 对方关闭或出错
//         close(fd);
//         epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
//         continue;
//       }
//       buf[nread] = '\0';
//       if (!buf) { // 格式异常
//         close(fd);
//         epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
//         continue;
//       }
//       std::optional<std::string> ret_path = parseRequestLine(buf);
//       if (!ret_path.has_value()) {
//         close(fd);
//         epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
//       }
//       std::optional<fs::path> requested_path = parse_request_path(ret_path);

//       std::string resp = buildResponse(requested_path.value().string());
//       /* 发回并关闭连接 */
//       ssize_t nwrite = write(fd, resp.c_str(), resp.size());
//       if (nwrite < 0) {
//         perror("read failed...");
//       }
//       close(fd);
//       epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
//     } else if (evs[i].events & EPOLLOUT) // clientFd WRITE events
//     {
//     } else {
//       perror("false");
//       return false;
//     }
//   }
// }
