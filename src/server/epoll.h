#pragma once
#include <sys/epoll.h>
class Epoll {
public:
  //单例访问方法
  static Epoll &get_instance() {
    static Epoll instance{};
    return instance;
  }
  //删除拷构造函数和赋值操作符
  Epoll(const Epoll &) = delete;
  Epoll &operator=(const Epoll &) = delete;
  //成员函数
  bool initialize();
  bool add_epoll_server(int fd);
  bool delete_epoll(int fd);
  void modify_epoll();
  int wait_events(struct epoll_event *evs, int timeout);
  // bool events_loop();
  //获取内部状态
  int get_epoll_fd() const { return epfd_; }
  bool is_initialized() const { return (epfd_ != -1); }
  int get_epoll_max_events() const { return max_events_; };

private:
  Epoll() = default;
  ~Epoll();
  int max_events_ = 64;
  int epfd_ = -1;
  int server_fd_{};
};
