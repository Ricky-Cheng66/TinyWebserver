#pragma once
#include <string>

class Server {
public:
  Server() = default;

  ~Server() = default;

  bool init(int server_port);

  bool start();

private:
  const int MAXCLIENTFDS = 1024;
  int server_fd_{};
  std::string server_addr_{};
  int server_port_{};
};
