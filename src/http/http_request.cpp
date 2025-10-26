#include "http_request.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
namespace fs = std::filesystem;
bool HttpRequest::parse(const std::string_view raw_request) {
  reset();
  std::cout << "调试: 收到数据长度=" << raw_request.length() << std::endl;
  std::cout << "调试: 数据=[" << raw_request << "]" << std::endl;
  //分割请求行 请求头 请求体
  auto request_line_end = raw_request.find("\r\n");
  if (request_line_end == std::string::npos) {
    std::cout << "错误: 找不到请求行结束" << std::endl;
    return false;
  }
  //解析请求行
  auto request_line = raw_request.substr(0, request_line_end);
  if (!parse_request_line(request_line)) {
    std::cout << "错误: 请求行解析失败" << std::endl;
    return false;
  }
  //解析请求头
  auto request_header_start = request_line_end + 2; // 2->\r\n
  if (request_header_start >= raw_request.length()) {
    // 只有请求行，没有头部 - 合法情况
    parsed_ = true;
    return true;
  }
  auto request_header_end = raw_request.find("\r\n\r\n", request_header_start);
  if (request_header_end == std::string::npos) {
    // 尝试其他结束标记
    request_header_end = raw_request.find("\n\n", request_header_start);
    if (request_header_end == std::string::npos) {
      // 如果没有明确的结束标记，认为整个剩余部分都是头部
      request_header_end = raw_request.length();
    }
  }
  auto request_header = raw_request.substr(
      request_header_start, request_header_end - request_header_start);
  if (!parse_request_header(request_header)) {
    std::cout << "错误: 头部解析失败" << std::endl;
    return false;
  }
  //解析请求体
  size_t body_start =
      request_header_end +
      (raw_request.substr(request_header_end, 2) == "\r\n\r\n" ? 4 : 2);
  if (body_start < raw_request.length()) {
    if (!parse_request_body(raw_request.substr(body_start))) {
      std::cout << "错误: 主体解析失败" << std::endl;
      return false;
    }
  }
  std::cout << "调试： 解析成功" << std::endl;
  parsed_ = true;
  return true;
}

bool HttpRequest::parse_request_line(const std::string_view request_line) {
  std::cout << "调试请求行: [" << request_line << "]" << std::endl;
  /* 极简解析：method path version */
  auto sp1 = request_line.find(' ');
  auto sp2 = request_line.find(' ', sp1 + 1);
  std::cout << "sp1=" << sp1 << ", sp2=" << sp2 << std::endl;
  if (sp1 == std::string::npos || sp2 == std::string::npos) {
    std::cout << "错误: 找不到空格分隔符" << std::endl;
    return false;
  }
  auto method{request_line.substr(0, sp1)};
  auto path{request_line.substr(sp1 + 1, sp2 - sp1 - 1)};
  auto version{request_line.substr(sp2 + 1)};
  std::cout << "method=[" << method << "], path=[" << path << "], version=["
            << version << "]" << std::endl;
  method_ = transfer_to_method(method);
  path_ = static_cast<std::string>(path);
  version_ = transfer_to_version(version);
  std::cout << "转换后: method=" << static_cast<int>(method_)
            << ", path=" << path_ << ", version=" << static_cast<int>(version_)
            << std::endl;

  /* 默认文件 */
  if (path_ == "/") {
    path_ = "/index.html";
  }

  return (method_ != Method::UNKNOWN && version_ != Version::UNKNOWN);
}
bool HttpRequest::parse_request_header(const std::string_view request_header) {
  std::istringstream iss(static_cast<std::string>(request_header));
  std::string line;
  while (std::getline(iss, line)) {
    if (line.back() == '\r')
      line.pop_back();
    size_t colon_pos = line.find(':');
    if (colon_pos != std::string::npos) {
      std::string key = line.substr(0, colon_pos);
      std::string value = line.substr(colon_pos + 1);

      // 清理空格
      key.erase(0, key.find_first_not_of(" \t"));
      key.erase(key.find_last_not_of(" \t") + 1);
      value.erase(0, value.find_first_not_of(" \t"));
      value.erase(value.find_last_not_of(" \t") + 1);

      headers_[key] = value;
    }
  }
  return true;
}
bool HttpRequest::parse_request_body(const std::string_view request_body) {
  body_ = request_body;
  // 验证 Content-Length
  size_t content_length = get_content_length();
  if (content_length > 0 && body_.length() != content_length) {
    return false; // 数据长度不匹配
  }
  return true;
}
HttpRequest::Method
HttpRequest::transfer_to_method(const std::string_view method) {
  std::cout << "转换方法: [" << method << "]" << std::endl;
  if (method == "GET") {
    return Method::GET;
  }
  if (method == "POST") {
    return Method::POST;
  }
  std::cout << "未知方法: " << method << std::endl;
  return Method::UNKNOWN;
}
HttpRequest::Version
HttpRequest::transfer_to_version(const std::string_view version) {
  std::cout << "转换版本: [" << version << "]" << std::endl;
  if (version == "HTTP/1.0") {
    return Version::HTTP1_0;
  }
  if (version == "HTTP/1.1") {
    return Version::HTTP1_1;
  }
  std::cout << "未知版本: " << version << std::endl;
  return Version::UNKNOWN;
}
std::string HttpRequest::get_header(const std::string &key) const {
  auto it = headers_.find(key);
  return (it != headers_.end()) ? it->second : "";
}
bool HttpRequest::keep_alive() const {
  if (version_ == Version::HTTP1_1) {
    std::string connection = get_header("Connection");
    return connection != "close";
  }

  return false; // HTTP/1.0默认短连接
}
size_t HttpRequest::get_content_length() const {
  std::string len_str = get_header("Content-Length");
  return len_str.empty() ? 0 : std::stoul(len_str);
}
void HttpRequest::reset() {
  method_ = Method::UNKNOWN;
  path_.clear();
  version_ = Version::UNKNOWN;
  headers_.clear();
  body_.clear();
  parsed_ = false;
}
std::optional<fs::path> HttpRequest::parse_request_path() {
  if (path_.empty()) {
    std::cout << " 错误 路径为空 " << std::endl;
    return std::nullopt;
  }
  auto path = path_;
  if (path[0] == '/') {
    path = path.substr(1);
  }
  // 路径拼接
  try {
    fs::path root_path{"www"};
    fs::path get_path{path};
    fs::path request_path = root_path / get_path;
    fs::path canonical_request_path = fs::canonical(request_path);
    fs::path canonical_root_path = fs::canonical(root_path);
    auto request_path_str = canonical_request_path.string();
    auto root_path_str = canonical_root_path.string();
    if (request_path_str.find(root_path_str) != 0) // 不是以根目录开头
    {
      std::cout << "路径错误 不以根目录开头" << std::endl;
      return std::nullopt;
    }
    if (!fs::is_regular_file(canonical_request_path)) //
    //检查是否为普通文件
    {
      std::cout << "错误 不是普通文件" << std::endl;
      return std::nullopt;
    }
    // 所有检查通过
    return canonical_request_path;
  } catch (const fs::filesystem_error &e) {
    std::cerr << e.what() << '\n';
    return std::nullopt;
  }
}