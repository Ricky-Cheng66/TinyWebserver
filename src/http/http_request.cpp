#include "http_request.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <string>
namespace fs = std::filesystem;
HttpRequest::HttpRequest() { reset(); }

std::error_code HttpRequest::parse_chunk(std::string_view data, size_t length) {
  if (state_ == ParseState::COMPLETE || state_ == ParseState::ERROR) {
    return std::make_error_code(std::errc::invalid_argument);
  }
  // 添加到缓冲区
  buffer_.append(data, length);
  size_t consumed = 0;
  std::error_code ec;

  while (consumed < buffer_.length() && state_ != ParseState::COMPLETE &&
         state_ != ParseState::ERROR) {
    size_t prev_consumed = consumed;

    switch (state_) {
    case ParseState::START:
      ec = handle_start(consumed);
      break;
    case ParseState::REQUEST_LINE:
      ec = handle_request_line(buffer_.data() + consumed, consumed);
      break;
    case ParseState::HEADERS:
      ec = handle_headers(buffer_.data() + consumed, consumed);
      break;
    case ParseState::BODY:
      ec = handle_body(buffer_.data() + consumed, buffer_.length() - consumed,
                       consumed);
      break;
    default:
      ec = std::make_error_code(std::errc::invalid_argument);
      break;
    }

    if (ec) {
      state_ = ParseState::ERROR;
      return ec;
    }

    // 如果没有消费任何数据，避免无限循环
    if (consumed == prev_consumed) {
      break;
    }
  }

  // 移除已处理的数据
  if (consumed > 0) {
    buffer_.erase(0, consumed);
  }

  return ec;
}
std::error_code HttpRequest::handle_start(size_t &consumed) {
  state_ = ParseState::REQUEST_LINE;
  consumed = 0;
  return {};
}

std::error_code HttpRequest::handle_request_line(std::string_view data,
                                                 size_t &consumed) {
  // 查找行结束
  const size_t line_end_pos = data.find('\n');
  if (line_end_pos == std::string_view::npos) {
    return {}; // 需要更多数据
  }
  //去除可能的'\r'
  std::string_view line_view = data.substr(0, line_end_pos);
  if (!line_view.empty() && line_view.back() == '\r') {
    line_view.remove_suffix(1); //去除回车符
  }
  // 解析请求行
  const std::string request_line{line_view};
  std::istringstream iss(request_line);
  std::string method_str, path_str, version_str;
  if (!(iss >> method_str >> path_str >> version_str)) {
    return std::make_error_code(std::errc::protocol_error);
  }
  method_ = transfer_to_method(method_str);
  path_ = std::move(path_str);
  version_ = transfer_to_version(version_str);

  if (method_ == Method::UNKNOWN || version_ == Version::UNKNOWN) {
    return std::make_error_code(std::errc::protocol_error);
  }

  // 默认文件
  if (path_ == "/") {
    path_ = "/index.html";
  }

  consumed = line_end_pos + 1; // +1 for '\n'
  state_ = ParseState::HEADERS;
  return {};
}

std::error_code HttpRequest::handle_headers(std::string_view data,
                                            size_t &consumed) {
  size_t pos = 0;
  while (pos < data.length()) {
    // 查找行结束
    size_t line_end_pos = data.find('\n', pos);
    if (line_end_pos == std::string_view::npos) {
      break; // 需要更多数据
    }

    std::string_view line = data.substr(pos, line_end_pos - pos);
    if (!line.empty() && line.back() == '\r') {
      line.remove_suffix(1); // 去掉回车符
    }

    // 空行表示头部结束
    if (line.empty()) {
      consumed = line_end_pos + 1; //消费空行+'\n'

      // 检查是否有body
      content_length_ = get_content_length();
      if (content_length_ > 0) {
        state_ = ParseState::BODY;
      } else {
        state_ = ParseState::COMPLETE;
      }
      return {};
    }

    // 解析头部行
    size_t colon_pos = line.find(':');
    if (colon_pos != std::string_view::npos) {
      std::string_view key_view = line.substr(0, colon_pos);
      std::string_view value_view = line.substr(colon_pos + 1);

      // 清理空格
      key_view = trim_whitespace(key_view);
      value_view = trim_whitespace(value_view);

      //转换键名为小写并存储
      std::string key = to_lower_case(key_view);
      headers_[std::move(key)] = std::string(value_view);
    }
    //移动到下一行
    pos = line_end_pos + 1;
    consumed = pos;
  }

  return {};
}

std::error_code HttpRequest::handle_body(std::string_view data, size_t length,
                                         size_t &consumed) {
  size_t needed = content_length_ - body_.length();
  size_t to_copy = std::min(needed, length);

  body_.append(data, to_copy);
  consumed = to_copy;

  if (body_.length() >= content_length_) {
    state_ = ParseState::COMPLETE;
  }

  return {};
}

bool HttpRequest::parse(const std::string_view raw_request) {
  std::cout << "调试: 收到数据长度=" << raw_request.length() << std::endl;
  std::cout << "调试: 数据=[" << raw_request << "]" << std::endl;
  //分割请求行 请求头 请求体
  auto request_line_end = raw_request.find("\r\n");
  if (request_line_end == std::string_view::npos) {
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
  if (request_header_end == std::string_view::npos) {
    // 尝试其他结束标记
    request_header_end = raw_request.find("\n\n", request_header_start);
    if (request_header_end == std::string_view::npos) {
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
  if (sp1 == std::string_view::npos || sp2 == std::string_view::npos) {
    std::cout << "错误: 找不到空格分隔符" << std::endl;
    return false;
  }
  auto method{request_line.substr(0, sp1)};
  method_ = transfer_to_method(method);

  auto path{request_line.substr(sp1 + 1, sp2 - sp1 - 1)};
  //分离查询字符串
  size_t query_pos = path.find('?');
  if (query_pos != std::string_view::npos) {
    path_ = static_cast<std::string>(path.substr(0, query_pos));
    query_string_ = static_cast<std::string>(path.substr(query_pos));
  } else {
    path_ = static_cast<std::string>(path);
    query_string_.clear();
  }

  auto version{request_line.substr(sp2 + 1)};
  version_ = transfer_to_version(version);

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
    if (colon_pos != std::string_view::npos) {
      std::string key = line.substr(0, colon_pos);
      std::string value = line.substr(colon_pos + 1);

      // 清理空格
      key.erase(0, key.find_first_not_of(" \t"));
      key.erase(key.find_last_not_of(" \t") + 1);
      value.erase(0, value.find_first_not_of(" \t"));
      value.erase(value.find_last_not_of(" \t") + 1);

      // 将key转换为小写
      std::transform(key.begin(), key.end(), key.begin(), ::tolower);

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
  std::string method_lower = to_lower_case(method);
  if (method_lower == "get")
    return Method::GET;
  if (method_lower == "post")
    return Method::POST;
  if (method_lower == "put")
    return Method::PUT;
  if (method_lower == "delete")
    return Method::DELETE;
  if (method_lower == "head")
    return Method::HEAD;
  if (method_lower == "options")
    return Method::OPTIONS;
  if (method_lower == "patch")
    return Method::PATCH;
  std::cout << "未知方法: " << method << std::endl;
  return Method::UNKNOWN;
}
HttpRequest::Version
HttpRequest::transfer_to_version(const std::string_view version) {
  std::cout << "转换版本: [" << version << "]" << std::endl;
  std::string version_upper = version.data();
  std::transform(version_upper.begin(), version_upper.end(),
                 version_upper.begin(), ::toupper);
  if (version == "HTTP/1.0") {
    return Version::HTTP1_0;
  }
  if (version == "HTTP/1.1") {
    return Version::HTTP1_1;
  }
  std::cout << "未知版本: " << version << std::endl;
  return Version::UNKNOWN;
}
std::string HttpRequest::to_lower_case(std::string_view str) {
  std::string result = str.data();
  std::transform(result.begin(), result.end(), result.begin(), ::tolower);
  return result;
}
std::string_view HttpRequest::trim_whitespace(std::string_view s) {
  auto is_space = [](unsigned char c) { return std::isspace(c); };

  auto start = std::find_if_not(s.begin(), s.end(), is_space);
  auto end = std::find_if_not(s.rbegin(), s.rend(), is_space).base();

  return (start < end) ? s.substr(start - s.begin(), end - start)
                       : std::string_view{};
}
std::string HttpRequest::get_header(const std::string &key) const {
  std::string lower_key = to_lower_case(key);
  auto it = headers_.find(lower_key);
  return (it != headers_.end()) ? it->second : "";
}
bool HttpRequest::keep_alive() const {
  if (version_ == Version::HTTP1_1) {
    std::string connection = to_lower_case(get_header("Connection"));
    return connection != "close";
  }

  return false; // HTTP/1.0默认短连接
}
size_t HttpRequest::get_content_length() const {
  std::string len_str = get_header("Content-Length");
  if (len_str.empty())
    return 0;
  try {
    return std::stoul(len_str);
  } catch (...) {
    return 0;
  }
}
void HttpRequest::reset() {
  state_ = ParseState::START;
  method_ = Method::UNKNOWN;
  path_.clear();
  version_ = Version::UNKNOWN;
  headers_.clear();
  body_.clear();
  buffer_.clear();
  content_length_ = 0;
  current_header_name_.clear();
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