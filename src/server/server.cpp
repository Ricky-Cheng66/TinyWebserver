#include "server.h"
#include "../http/request.h"
#include "../http/response.h"
#include "epoll.h"
#include "socket.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <optional>
#include <string_view>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

bool Server::init(int server_port) {
  // socket部分
  server_port_ = server_port;
  // create server_fd
  Socket server_socket{};
  server_fd_ = server_socket.create_server_socket();
  //设置地址重用
  if (!server_socket.set_socket_option(server_fd_)) {
    std::cerr << "set_socket_option failed..." << std::endl;
  }
  // set listenFd nonblock
  server_socket.set_nonblock(server_fd_);
  // create epfd and put listenFd into epoll
  //获取Epoll单例
  Epoll &ep = Epoll::get_instance();
  if (!ep.initialize()) {
    return false;
  }
  // add_epoll
  ep.add_epoll_server(server_fd_);
  // blind
  server_socket.bind_server_socket(server_fd_, server_port_);
  return true;
}
bool Server::start() {
  Socket server_socket{};
  server_socket.listen_socket(server_fd_);
  //获取Epoll单例
  Epoll &ep = Epoll::get_instance();
  if (!ep.initialize()) {
    return false;
  }
  int max_events = ep.get_epoll_max_events();
  struct epoll_event evs[max_events];
  while (1) {
    int nfds = ep.wait_events(evs, -1);
    if (nfds < 0) {
      if (errno == EINTR) {
        //被信号中断
        std::cerr << "epoll_wait 被信号中断" << std::endl;
        continue;
      }
      std::cerr << "epoll_wait other errors" << std::endl;
      continue;
    }
    for (int i = 0; i < nfds; i++) {
      if (evs[i].data.fd == server_fd_) { // accep
        int client_fd = server_socket.accept_socket(evs[i].data.fd);
        if (client_fd < 0) {
          if (errno == EAGAIN || errno == EMFILE) {
            continue; // 资源暂时不可用
          }
          std::cerr << "accept failed..." << std::endl;
          continue;
        }
        Socket client_socket{};
        client_socket.set_nonblock(client_fd);
        std::cout << "new client connected..." << std::endl;
        struct epoll_event ev {};
        ev.data.fd = client_fd;
        ev.events = EPOLLIN | EPOLLET;
        ep.add_epoll_server(ev.data.fd);
      } else if (evs[i].events & EPOLLIN) { //处理客户端事件
        // read dealing
        /* 一次性读完（短连接够用） */
        int fd = evs[i].data.fd;
        char buf[4096];
        ssize_t nread = read(fd, buf, sizeof(buf) - 1);
        if (nread <= 0) { // 对方关闭或出错
          ep.delete_epoll(fd);
          close(fd);
          continue;
        }
        buf[nread] = '\0';
        Request request{};
        std::optional<std::string> ret_path = request.parse_request_line(buf);
        if (!ret_path.has_value()) {
          ep.delete_epoll(fd);
          close(fd);
          continue;
        }
        std::optional<fs::path> requested_path =
            request.parse_request_path(ret_path);
        Response response;
        std::string resp =
            response.build_response(requested_path.value().string());
        /* 发回并关闭连接 */
        ssize_t nwrite = write(fd, resp.c_str(), resp.size());
        if (nwrite < 0) {
          std::cerr << "write failed..." << std::endl;
          continue;
        }
        ep.delete_epoll(fd);
        close(fd);
      } else if (evs[i].events & EPOLLOUT) { // clientFd WRITE events

      } else {
        std::cerr << "other faults" << std::endl;
        return false;
      }
    }
  }
  return true;
}
