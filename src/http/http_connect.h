#pragma once

#include "http_request.h"
#include "http_response.h"
#include <arpa/inet.h>
#include <functional>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

class HttpConnect {
public:
  // 连接状态
  enum class State {
    READING,    // 正在读取数据
    PROCESSING, // 正在处理请求
    WRITING,    // 正在写入响应
    CLOSING,    // 连接正在关闭
    CLOSED      // 连接已关闭
  };

  // 构造函数
  HttpConnect(int client_socket, const sockaddr_in &client_addr);
  ~HttpConnect();

  // 禁止拷贝
  HttpConnect(const HttpConnect &) = delete;
  HttpConnect &operator=(const HttpConnect &) = delete;

  // 🎯 核心方法 - 处理读事件（增量解析）
  bool process_read();

  // 🎯 核心方法 - 处理写事件
  bool process_write();

  // 📊 状态查询
  State get_state() const { return state_; }
  bool is_keep_alive() const { return keep_alive_; }
  bool should_close() const {
    return state_ == State::CLOSING || state_ == State::CLOSED;
  }
  bool has_response() const {
    std::cout << "DEBUG has_response: " << !response_buffer_.empty()
              << " (size: " << response_buffer_.size() << ")" << std::endl;
    return !response_buffer_.empty();
  }
  int get_socket() const { return client_socket_; }
  const std::string &get_client_ip() const { return client_ip_; }

  // 🔧 配置方法
  void set_request_handler(
      std::function<HttpResponse(const HttpRequest &)> handler) {
    request_handler_ = handler;
  }

  // 📤 获取响应数据
  const std::string &get_response_buffer() const { return response_buffer_; }
  void clear_response_buffer() { response_buffer_.clear(); }

  // 🔄 重置连接状态（用于keep-alive）
  void reset();

private:
  // 网络操作
  ssize_t read_from_socket();

  // 处理步骤
  bool handle_request_complete();

  // 成员变量
  int client_socket_;
  std::string client_ip_;
  int client_port_;

  State state_ = State::READING;

  std::string read_buffer_;     // 读取缓冲区
  std::string response_buffer_; // 响应缓冲区

  HttpRequest http_request_;
  HttpResponse http_response_;
  std::function<HttpResponse(const HttpRequest &)> request_handler_;

  bool keep_alive_ = true;
  bool request_processed_ = false;
};