#pragma once
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
class HttpRequest {
public:
  HttpRequest() = default;
  ~HttpRequest() = default;

  enum class Method { GET, POST, UNKNOWN };
  enum class Version { HTTP1_0, HTTP1_1, UNKNOWN };

  //解析接口
  bool parse(const std::string_view raw_request);
  void reset();

  //获取解析结果
  Method get_method() const { return method_; };
  const std::string &get_path() const { return path_; };
  Version get_version() const { return version_; };
  const std::unordered_map<std::string, std::string> &get_headers() const {
    return headers_;
  };
  //便捷方法
  std::string get_header(const std::string &key) const;
  bool keep_alive() const;
  size_t get_content_length() const;

  //路径解析
  std::optional<std::filesystem::path> parse_request_path();
  // std::optional<fs::path> parse_request_path(std::optional<std::string>
  // path);

private:
  //解析步骤
  bool parse_request_line(const std::string_view request_line);
  bool parse_request_header(const std::string_view request_header);
  bool parse_request_body(const std::string_view request_body);

  //辅助方法
  static Method transfer_to_method(const std::string_view method);
  static Version transfer_to_version(const std::string_view version);

  //成员变量
  Method method_ = Method::UNKNOWN;
  std::string path_;
  Version version_ = Version::UNKNOWN;
  std::unordered_map<std::string, std::string> headers_;
  std::string body_;
  //解析状态
  bool parsed_ = false;
};
