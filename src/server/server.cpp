#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <optional>
#include <string_view>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <system_error>
#include <unistd.h>
#include <unordered_map>

#include "../http/http_connect.h" // 新增头文件
#include "../http/http_request.h"
#include "../http/http_response.h"
#include "epoll.h"
#include "server.h"
#include "socket.h"
// 请求处理函数实现
HttpResponse Server::handle_http_request(const HttpRequest &request) {
  std::cout << "Handling request: " << static_cast<int>(request.get_method())
            << " " << request.get_path() << std::endl;
  // // 🎯 临时方案：直接返回固定响应，排除文件读取问题
  // HttpResponse response;

  // // 使用固定内容而不是文件读取
  // std::string fixed_content = "<!doctype html>\n"
  //                             "<title>TinyWebServer</title>\n"
  //                             "<h1>It works!</h1>\n";

  // response.set_body(fixed_content);
  // response.set_content_type("text/html; charset=utf-8");
  // response.set_keep_alive(request.keep_alive());

  // std::cout << "DEBUG: Response prepared with fixed content" << std::endl;
  // return response;
  auto file_path = request.parse_request_path();
  if (!file_path) {
    std::cout << "File not found: " << request.get_path() << std::endl;
    return HttpResponse::make_404_response();
  }

  HttpResponse response;
  response.set_body_from_file(file_path.value());
  response.set_keep_alive(request.keep_alive());

  return response;
}
bool Server::init(int server_port) {
  // socket部分
  server_port_ = server_port;
  // create server_fd
  Socket server_socket{};
  server_fd_ = server_socket.create_server_socket();
  //设置地址重用
  if (!server_socket.set_socket_option(server_fd_)) {
    std::error_code ec(errno, std::system_category());
    std::cerr << "set_socket_option failed..." << ec.message() << std::endl;
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
  ep.add_epoll(server_fd_, EPOLLIN | EPOLLET);
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
    std::cout << "DEBUG: epoll_wait returned " << nfds << " events"
              << std::endl;
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
      std::cout << "DEBUG: Event " << i << ": fd=" << evs[i].data.fd
                << ", events=0x" << std::hex << evs[i].events << std::dec
                << std::endl;
      if (evs[i].data.fd == server_fd_) {
        // accept新连接
        int client_fd = server_socket.accept_socket(evs[i].data.fd);
        if (client_fd < 0) {
          if (errno == EAGAIN || errno == EMFILE) {
            continue; // 资源暂时不可用
          }
          std::error_code ec(errno, std::system_category());
          std::cerr << "accept failed..." << ec.message() << std::endl;
          continue;
        }

        Socket client_socket{};
        client_socket.set_nonblock(client_fd);

        // 获取客户端地址
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        getpeername(client_fd, (struct sockaddr *)&client_addr, &client_len);

        // 创建HttpConnect对象
        auto connection = std::make_shared<HttpConnect>(client_fd, client_addr);
        connection->set_request_handler(
            [this](const HttpRequest &req) -> HttpResponse {
              return this->handle_http_request(req);
            });
        // 存储连接对象
        active_connections_[client_fd] = connection;
        std::cout << "DEBUG: Connection created, total="
                  << active_connections_.size() << std::endl;

        // 注册到epoll（ET模式）
        ep.add_epoll(client_fd, EPOLLIN | EPOLLET | EPOLLRDHUP);

        std::cout << "New client connected: " << connection->get_client_ip()
                  << ":" << client_addr.sin_port << std::endl;

      } else {
        // 处理客户端事件
        handle_client_event(evs[i], ep);
      }
    }
  }
  return true;
}

void Server::handle_client_event(epoll_event &event, Epoll &ep) {
  int client_fd = event.data.fd;

  // 检查连接是否已关闭
  if (event.events & EPOLLRDHUP) {
    std::cout << "Client closed connection: " << client_fd << std::endl;
    active_connections_.erase(client_fd);
    ep.delete_epoll(client_fd);

    return;
  }

  // 获取连接对象
  auto it = active_connections_.find(client_fd);
  if (it == active_connections_.end()) {
    ep.delete_epoll(client_fd);
    close(client_fd);
    return;
  }

  auto &connection = it->second;

  try {
    if (event.events & EPOLLIN) {
      // 处理读事件
      if (connection->process_read()) {
        std::cout << "DEBUG: Has response, modifying to EPOLLOUT" << std::endl;
        if (connection->has_response()) {
          // 有响应数据，注册写事件
          struct epoll_event ev {};
          ev.data.fd = client_fd;
          ev.events = EPOLLOUT | EPOLLET | EPOLLRDHUP;
          if (!ep.modify_epoll(client_fd, ev.events)) {
            std::cout << "DEBUG: modify_epoll failed!" << std::endl;
          }
        }
        // 如果没有响应，继续等待更多数据（保持EPOLLIN）
      } else {
        // 处理失败，关闭连接
        std::cout << "Read processing failed, closing: "
                  << connection->get_client_ip() << std::endl;
        active_connections_.erase(client_fd);
        ep.delete_epoll(client_fd);
        std::cout << "DEBUG: Connection deleted, total="
                  << active_connections_.size() << std::endl;
      }
    } else if (event.events & EPOLLOUT) {
      // 处理写事件
      if (connection->process_write()) {
        if (connection->get_state() == HttpConnect::State::READING) {
          // 回到读状态，重新注册读事件
          struct epoll_event ev {};
          ev.data.fd = client_fd;
          ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
          ep.modify_epoll(client_fd, ev.events);
        } else if (connection->should_close()) {
          // 需要关闭连接
          active_connections_.erase(client_fd);
          ep.delete_epoll(client_fd);
          std::cout << "DEBUG: Connection deleted, total="
                    << active_connections_.size() << std::endl;
        }
        // 如果还是WRITING状态，保持EPOLLOUT（可能响应数据没发完）
      } else {
        // 发送失败，关闭连接
        std::cout << "Write processing failed, closing: "
                  << connection->get_client_ip() << std::endl;
        active_connections_.erase(client_fd);
        ep.delete_epoll(client_fd);
        std::cout << "DEBUG: Connection deleted, total="
                  << active_connections_.size() << std::endl;
      }
    }
  } catch (const std::exception &e) {
    std::cerr << "Exception handling client event: " << e.what() << std::endl;
    active_connections_.erase(client_fd);
    ep.delete_epoll(client_fd);
    std::cout << "DEBUG: Connection deleted, total="
              << active_connections_.size() << std::endl;
  }
}