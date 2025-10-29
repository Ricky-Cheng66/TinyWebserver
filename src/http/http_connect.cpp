#include "http_connect.h"
#include <cstring>
#include <iostream>
#include <unistd.h>

HttpConnect::HttpConnect(int client_socket, const sockaddr_in &client_addr)
    : client_socket_(client_socket), state_(State::READING) {

  // 提取客户端信息
  char ip_str[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, INET_ADDRSTRLEN);
  client_ip_ = std::string(ip_str);
  client_port_ = ntohs(client_addr.sin_port);

  std::cout << "New connection from " << client_ip_ << ":" << client_port_
            << ", state: READING" << std::endl;
}

HttpConnect::~HttpConnect() {
  if (client_socket_ != -1) {
    close(client_socket_);
    std::cout << "Connection closed: " << client_ip_ << ":" << client_port_
              << std::endl;
  }
}

bool HttpConnect::process_read() {
  if (state_ != State::READING) {
    return false;
  }

  // 读取数据
  ssize_t bytes_read = read_from_socket();
  if (bytes_read <= 0) {
    state_ = State::CLOSING;
    return false;
  }

  std::cout << "Read " << bytes_read << " bytes from " << client_ip_
            << std::endl;

  // 使用HttpRequest的增量解析
  auto error_code =
      http_request_.parse_chunk(read_buffer_, read_buffer_.size());
  if (error_code) {
    std::cout << "Parse error from " << client_ip_ << ": "
              << error_code.message() << std::endl;

    // 生成错误响应
    http_response_ = HttpResponse::make_error_response(
        HttpResponse::StatusCode::BAD_REQUEST, "Parse error");
    response_buffer_ = http_response_.build();
    state_ = State::WRITING;
    return true;
  }

  // 检查请求是否解析完成
  if (http_request_.is_complete()) {
    std::cout << "Request complete from " << client_ip_
              << ", method: " << static_cast<int>(http_request_.get_method())
              << ", path: " << http_request_.get_path() << std::endl;
    return handle_request_complete();
  }
  std::cout << "Raw data received:\n" << read_buffer_ << std::endl;
  return true;
}

bool HttpConnect::process_write() {
  if (state_ != State::WRITING || response_buffer_.empty()) {
    return false;
  }

  // 发送响应
  ssize_t sent =
      send(client_socket_, response_buffer_.data(), response_buffer_.size(), 0);

  if (sent > 0) {
    std::cout << "DEBUG process_write: buffer_size=" << response_buffer_.size()
              << ", sent=" << sent << " to " << client_ip_ << std::endl;

    // 🎯 关键修改：检查是否全部发送完成
    if (static_cast<size_t>(sent) == response_buffer_.size()) {
      // 全部发送完成
      response_buffer_.clear();

      if (keep_alive_ && !should_close()) {
        reset();
        std::cout << "Keep-alive connection reset for " << client_ip_
                  << std::endl;
      } else {
        state_ = State::CLOSING;
        std::cout << "Closing connection for " << client_ip_ << std::endl;
        //立即关闭 socket
        if (client_socket_ != -1) {
          close(client_socket_);
          client_socket_ = -1;
          std::cout << "Socket immediately closed for " << client_ip_
                    << std::endl;
        }
      }
    } else {
      // 🎯 关键修改：部分发送，继续发送剩余数据
      response_buffer_.erase(0, sent);
      std::cout << "DEBUG: Partial send, remaining=" << response_buffer_.size()
                << " bytes for " << client_ip_ << std::endl;
      // 保持 WRITING 状态，继续发送
    }
    return true;
  } else if (sent == 0) {
    // 客户端关闭连接
    std::cout << "Client closed connection during send: " << client_ip_
              << std::endl;
    state_ = State::CLOSING;
    return false;
  } else {
    // 发送错误
    std::cerr << "Error sending response to " << client_ip_ << ": "
              << strerror(errno) << std::endl;
    state_ = State::CLOSING;
    return false;
  }
}

ssize_t HttpConnect::read_from_socket() {
  char buffer[4096];
  ssize_t bytes_read = recv(client_socket_, buffer, sizeof(buffer) - 1, 0);

  if (bytes_read > 0) {
    buffer[bytes_read] = '\0';
    read_buffer_.append(buffer, bytes_read);
    return bytes_read;
  } else if (bytes_read == 0) {
    // 客户端关闭连接
    std::cout << "Client closed connection: " << client_ip_ << std::endl;
    return 0;
  } else {
    // 读取错误
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      std::cerr << "Error reading from " << client_ip_ << ": "
                << strerror(errno) << std::endl;
    }
    return -1;
  }
}

bool HttpConnect::handle_request_complete() {
  state_ = State::PROCESSING;

  // 检查是否有请求处理器
  if (!request_handler_) {
    std::cerr << "No request handler set for " << client_ip_ << std::endl;
    http_response_ = HttpResponse::make_error_response(
        HttpResponse::StatusCode::INTERNAL_SERVER_ERROR);
  } else {
    try {
      // 调用用户请求处理器
      http_response_ = request_handler_(http_request_);
      request_processed_ = true;

      std::cout << "Request processed for " << client_ip_ << " "
                << http_request_.get_path() << std::endl;
    } catch (const std::exception &e) {
      std::cerr << "Exception in request handler for " << client_ip_ << ": "
                << e.what() << std::endl;
      http_response_ = HttpResponse::make_error_response(
          HttpResponse::StatusCode::INTERNAL_SERVER_ERROR);
    }
  }

  // 设置连接头
  if (keep_alive_) {
    http_response_.set_header("Connection", "keep-alive");
  } else {
    http_response_.set_header("Connection", "close");
  }

  // 构建响应字符串
  response_buffer_ = http_response_.build();
  if (response_buffer_.empty()) {
    std::cerr << "Failed to build response for " << client_ip_ << std::endl;
    state_ = State::CLOSING;
    return false;
  }

  // 更新keep-alive状态
  keep_alive_ = http_request_.keep_alive();

  std::cout << "Response prepared for " << client_ip_
            << ", size: " << response_buffer_.size()
            << " bytes, keep-alive: " << (keep_alive_ ? "yes" : "no")
            << std::endl;

  state_ = State::WRITING;
  return true;
}

void HttpConnect::reset() {
  std::cout << "DEBUG reset: " << client_ip_
            << ", old_state=" << static_cast<int>(state_) << std::endl;
  if (state_ == State::CLOSING || state_ == State::CLOSED) {
    return;
  }

  // 重置HTTP请求和响应
  http_request_.reset();
  http_response_ = HttpResponse();
  read_buffer_.clear();
  response_buffer_.clear();
  request_processed_ = false;
  state_ = State::READING;

  std::cout << "DEBUG reset: new_state=" << static_cast<int>(state_)
            << ", keep_alive=" << keep_alive_ << std::endl;
}