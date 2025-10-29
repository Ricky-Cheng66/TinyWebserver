#pragma once
#include "../http/http_connect.h"
#include "epoll.h"
#include <string>
#include <unordered_map>
class Server {
public:
  Server() = default;

  ~Server() = default;

  bool init(int server_port);

  bool start();

private:
  // 请求处理函数
  HttpResponse handle_http_request(const HttpRequest &request);
  // 处理客户端事件
  void handle_client_event(epoll_event &event, Epoll &ep);

  const int MAXCLIENTFDS = 1024;
  int server_fd_{};
  std::string server_addr_{};
  int server_port_{};
  std::unordered_map<int, std::shared_ptr<HttpConnect>>
      active_connections_; // 新增：连接管理
};
