#include "http_response.h"
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <system_error>
#include <unistd.h>

namespace fs = std::filesystem;

// 🏗️ 构造函数：设置默认服务器标识
HttpResponse::HttpResponse() { set_header("Server", "TinyWebServer/1.0"); }

// 🧹 析构函数：确保资源正确释放
HttpResponse::~HttpResponse() { cleanup_mmap(); }

// 🎯 设置HTTP状态码
HttpResponse &HttpResponse::set_status(StatusCode code) {
  status_code_ = code;
  return *this; // 返回引用支持链式调用
}

// 🏷️ 设置HTTP头部（使用string_view避免字符串拷贝）
HttpResponse &HttpResponse::set_header(std::string_view key,
                                       std::string_view value) {
  // 将string_view转换为string存储（因为unordered_map需要固定字符串）
  headers_[std::move(std::string(key))] = std::move(std::string(value));
  return *this;
}

// 📄 设置内容类型
HttpResponse &HttpResponse::set_content_type(std::string_view content_type) {
  return set_header("Content-Type", content_type);
}

// 🔄 设置连接保持选项
HttpResponse &HttpResponse::set_keep_alive(bool keep_alive) {
  set_header("Connection", keep_alive ? "keep-alive" : "close");
  if (keep_alive) {
    set_header("Keep-Alive", "timeout=5, max=100"); // 设置keep-alive参数
  }
  return *this;
}

// 📝 设置响应体内容
HttpResponse &HttpResponse::set_body(std::string_view body) {
  cleanup_mmap();                       // 清除之前可能的内存映射
  body_ = std::move(std::string(body)); // 转换为string存储
  use_mmap_ = false;
  auto_set_content_type(); // 自动检测内容类型
  return *this;
}

// 📁 从文件读取内容作为响应体
HttpResponse &HttpResponse::set_body_from_file(const fs::path &file_path) {
  std::cout << "DEBUG: Reading file: " << file_path << std::endl;
  cleanup_mmap();

  if (!read_file_content(file_path)) {
    // 文件读取失败，返回404错误
    std::cout << "DEBUG: File read failed: " << file_path << std::endl;
    set_status(StatusCode::NOT_FOUND);
    set_text_body("File not found: " + file_path.string());
    return *this;
  }

  // 🔍 根据文件扩展名自动设置Content-Type
  std::string extension = file_path.extension().string();
  if (extension == ".html" || extension == ".htm") {
    set_content_type("text/html; charset=utf-8");
  } else if (extension == ".css") {
    set_content_type("text/css; charset=utf-8");
  } else if (extension == ".js") {
    set_content_type("application/javascript; charset=utf-8");
  } else if (extension == ".json") {
    set_content_type("application/json; charset=utf-8");
  } else if (extension == ".png") {
    set_content_type("image/png");
  } else if (extension == ".jpg" || extension == ".jpeg") {
    set_content_type("image/jpeg");
  } else if (extension == ".gif") {
    set_content_type("image/gif");
  } else {
    set_content_type("application/octet-stream"); // 二进制流
  }

  return *this;
}

// 🎯 便捷方法：设置JSON格式响应体
HttpResponse &HttpResponse::set_json_body(std::string_view json) {
  set_content_type("application/json; charset=utf-8");
  return set_body(json);
}

// 🎯 便捷方法：设置HTML格式响应体
HttpResponse &HttpResponse::set_html_body(std::string_view html) {
  set_content_type("text/html; charset=utf-8");
  return set_body(html);
}

// 🎯 便捷方法：设置纯文本响应体
HttpResponse &HttpResponse::set_text_body(std::string_view text) {
  set_content_type("text/plain; charset=utf-8");
  return set_body(text);
}

// 🔀 设置重定向响应
HttpResponse &HttpResponse::set_redirect(std::string_view location,
                                         StatusCode code) {
  set_status(code);
  set_header("Location", location);
  return set_body(""); // 重定向响应通常没有内容体
}

// 🏗️ 构建完整的HTTP响应字符串
std::string HttpResponse::build() const {
  std::ostringstream response;

  // 1. 📋 状态行：HTTP/1.1 状态码 状态描述
  response << "HTTP/1.1 " << static_cast<int>(status_code_) << " "
           << get_status_text() << "\r\n";

  // 2. 📊 自动设置Content-Length（如果未手动设置）
  if (headers_.find("Content-Length") == headers_.end() && !use_mmap_) {
    response << "Content-Length: " << body_.length() << "\r\n";
  }

  // 3. 🏷️ 输出所有HTTP头部
  for (const auto &[key, value] : headers_) {
    response << key << ": " << value << "\r\n";
  }

  // 4. ➕ 空行分隔头部和主体
  response << "\r\n";

  // 5. 📝 输出响应体（如果不是内存映射的文件）
  if (!use_mmap_ && !body_.empty()) {
    response << body_;
  }

  return response.str();
}

// 📋 获取状态码对应的描述文本
std::string_view HttpResponse::get_status_text() const {
  // 静态映射表：状态码 -> 描述文本
  static const std::unordered_map<StatusCode, std::string> status_texts = {
      {StatusCode::OK, "OK"},
      {StatusCode::CREATED, "Created"},
      {StatusCode::NO_CONTENT, "No Content"},
      {StatusCode::MOVED_PERMANENTLY, "Moved Permanently"},
      {StatusCode::FOUND, "Found"},
      {StatusCode::BAD_REQUEST, "Bad Request"},
      {StatusCode::UNAUTHORIZED, "Unauthorized"},
      {StatusCode::FORBIDDEN, "Forbidden"},
      {StatusCode::NOT_FOUND, "Not Found"},
      {StatusCode::METHOD_NOT_ALLOWED, "Method Not Allowed"},
      {StatusCode::INTERNAL_SERVER_ERROR, "Internal Server Error"},
      {StatusCode::NOT_IMPLEMENTED, "Not Implemented"},
      {StatusCode::SERVICE_UNAVAILABLE, "Service Unavailable"}};

  static const std::string unknown_status = "Unknown Status";
  auto it = status_texts.find(status_code_);
  return it != status_texts.end() ? std::string_view(it->second)
                                  : std::string_view(unknown_status);
}

// 🔍 自动检测并设置内容类型
void HttpResponse::auto_set_content_type() {
  // 如果已经手动设置了Content-Type，跳过自动检测
  if (headers_.find("Content-Type") != headers_.end()) {
    return;
  }

  // 简单的启发式内容类型检测
  std::string_view body_view(body_);
  if (body_view.substr(0, 15) == "<!DOCTYPE html>" ||
      body_view.substr(0, 6) == "<html>") {
    set_content_type("text/html; charset=utf-8");
  } else if (body_view.substr(0, 1) == "{" || body_view.substr(0, 1) == "[") {
    // 简单的JSON检测（以{或[开头）
    set_content_type("application/json; charset=utf-8");
  } else {
    set_content_type("text/plain; charset=utf-8");
  }
}

// 📖 读取文件内容到内存
bool HttpResponse::read_file_content(const fs::path &file_path) {
  try {
    // 检查文件是否存在且是普通文件
    if (!fs::exists(file_path) || !fs::is_regular_file(file_path)) {
      return false;
    }

    size_t file_size = fs::file_size(file_path);

    // 对于大文件使用内存映射，小文件直接读取
    if (file_size > 64 * 1024) { // 64KB以上使用mmap
      int fd = open(file_path.c_str(), O_RDONLY);
      if (fd == -1) {
        return false;
      }

      // 🗺️ 创建内存映射
      mmap_addr_ = mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
      close(fd);

      if (mmap_addr_ == MAP_FAILED) {
        mmap_addr_ = nullptr;
        return false;
      }

      mmap_size_ = file_size;
      use_mmap_ = true;

      // 设置Content-Length头部
      set_header("Content-Length", std::to_string(file_size));

    } else {
      // 小文件直接读取到内存
      std::ifstream file(file_path, std::ios::binary);
      if (!file) {
        return false;
      }

      body_.assign(std::istreambuf_iterator<char>(file),
                   std::istreambuf_iterator<char>());
      use_mmap_ = false;
    }

    return true;

  } catch (const fs::filesystem_error &) {
    return false;
  }
}

// 🧹 清理内存映射资源
void HttpResponse::cleanup_mmap() {
  if (mmap_addr_ != nullptr) {
    munmap(mmap_addr_, mmap_size_);
    mmap_addr_ = nullptr;
    mmap_size_ = 0;
    use_mmap_ = false;
  }
}

// ❌ 创建错误响应（静态工厂方法）
HttpResponse HttpResponse::make_error_response(StatusCode code,
                                               std::string_view message) {
  HttpResponse response;
  response.set_status(code);

  std::string body = "<html><body><h1>" +
                     std::to_string(static_cast<int>(code)) + " " +
                     std::string(response.get_status_text()) + "</h1>";
  if (!message.empty()) {
    body += "<p>" + std::string(message) + "</p>";
  }
  body += "</body></html>";

  return response.set_html_body(body);
}

// 404错误响应快捷方法
HttpResponse HttpResponse::make_404_response() {
  return make_error_response(StatusCode::NOT_FOUND,
                             "The requested resource was not found.");
}

// 500错误响应快捷方法
HttpResponse HttpResponse::make_500_response() {
  return make_error_response(StatusCode::INTERNAL_SERVER_ERROR,
                             "Internal server error occurred.");
}