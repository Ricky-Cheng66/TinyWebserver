#pragma once
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
class HttpRequest {
public:
  HttpRequest();
  ~HttpRequest() = default;

  enum class Method { GET, POST, PUT, DELETE, HEAD, OPTIONS, PATCH, UNKNOWN };
  enum class Version { HTTP1_0, HTTP1_1, HTTP2_0, UNKNOWN };
  enum class ParseState { START, REQUEST_LINE, HEADERS, BODY, COMPLETE, ERROR };

  //增量解析
  std::error_code parse_chunk(std::string_view data, size_t length);
  bool is_complete() const { return state_ == ParseState::COMPLETE; };
  bool has_error() const { return state_ == ParseState::ERROR; };

  //解析接口
  bool parse(const std::string_view raw_request);
  void reset();

  //获取解析结果
  Method get_method() const { return method_; };
  const std::string &get_path() const { return path_; };
  const std::string &get_query_string() const { return query_string_; };
  Version get_version() const { return version_; };
  const std::unordered_map<std::string, std::string> &get_headers() const {
    return headers_;
  };
  const std::string &get_body() const { return body_; };

  //便捷方法
  std::string get_header(const std::string &key) const;
  bool keep_alive() const;
  size_t get_content_length() const;

  //路径解析
  std::optional<std::filesystem::path> parse_request_path();
  // std::optional<fs::path> parse_request_path(std::optional<std::string>
  // path);

  //状态检查
  bool is_valid() const { return parsed_ && method_ != Method::UNKNOWN; };
  bool has_body() const { return !body_.empty(); };

private:
  //状态机处理
  std::error_code handle_start(std::string_view data, size_t length,
                               size_t &consumed);
  std::error_code handle_request_line(std::string_view data, size_t length,
                                      size_t &consumed);
  std::error_code handle_headers(std::string_view data, size_t length,
                                 size_t &consumed);
  std::error_code handle_body(std::string_view data, size_t length,
                              size_t &consumed);
  //解析步骤
  bool parse_request_line(const std::string_view request_line);
  bool parse_request_header(const std::string_view request_header);
  bool parse_request_body(const std::string_view request_body);

  //辅助方法
  static Method transfer_to_method(const std::string_view method);
  static Version transfer_to_version(const std::string_view version);
  void parse_query_string();
  static std::string to_lower_case(std::string_view str);
  std::string_view trim_whitespace(std::string_view s);
  //解析缓冲区
  std::string buffer_;
  size_t content_length_ = 0;
  std::string current_header_name_;
  //成员变量
  ParseState state_ = ParseState::START;
  Method method_ = Method::UNKNOWN;
  std::string path_;
  std::string query_string_;
  Version version_ = Version::UNKNOWN;
  std::unordered_map<std::string, std::string> headers_;
  std::string body_;
  std::unordered_map<std::string, std::string> query_params_;
  bool parsed_ = false;
};
