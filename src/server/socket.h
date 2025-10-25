#pragma once
class Socket {
public:
  // 服务器Socket单例
  // static Socket &get_server_socket_instance() {
  //   static Socket server_socket{};
  //   return server_socket;
  // }
  Socket() = default;
  ~Socket() = default;
  int create_server_socket();
  bool bind_server_socket(int fd, int port);
  int accept_socket(int server_fd);
  bool listen_socket(int fd);
  bool set_socket_option(int fd);
  bool set_nonblock(int fd);

private:
  int max_clients = 1024;
};
