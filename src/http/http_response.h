#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <sys/mman.h>
#include <system_error>
#include <unordered_map>

class HttpResponse {
public:
  // HTTP 状态码枚举
  enum class StatusCode {
    OK = 200,                    // 请求成功
    CREATED = 201,               // 资源创建成功
    NO_CONTENT = 204,            // 请求成功但无内容返回
    MOVED_PERMANENTLY = 301,     // 永久重定向
    FOUND = 302,                 // 临时重定向
    BAD_REQUEST = 400,           // 客户端请求错误
    UNAUTHORIZED = 401,          // 未授权
    FORBIDDEN = 403,             // 禁止访问
    NOT_FOUND = 404,             // 资源未找到
    METHOD_NOT_ALLOWED = 405,    // 方法不允许
    INTERNAL_SERVER_ERROR = 500, // 服务器内部错误
    NOT_IMPLEMENTED = 501,       // 未实现的功能
    SERVICE_UNAVAILABLE = 503    // 服务不可用
  };

  HttpResponse();
  ~HttpResponse();

  // 🔗 链式设置方法（返回引用支持连续调用）
  HttpResponse &set_status(StatusCode code);

  // 🏷️ 设置HTTP头部（使用string_view避免拷贝）
  HttpResponse &set_header(std::string_view key, std::string_view value);

  // 📄 设置内容类型
  HttpResponse &set_content_type(std::string_view content_type);

  // 🔄 设置连接保持
  HttpResponse &set_keep_alive(bool keep_alive);

  // 📝 设置响应体
  HttpResponse &set_body(std::string_view body);

  // 📁 从文件设置响应体
  HttpResponse &set_body_from_file(const std::filesystem::path &file_path);

  // 🎯 便捷方法：设置特定格式的响应体
  HttpResponse &set_json_body(std::string_view json);
  HttpResponse &set_html_body(std::string_view html);
  HttpResponse &set_text_body(std::string_view text);

  // 🔀 重定向
  HttpResponse &set_redirect(std::string_view location,
                             StatusCode code = StatusCode::FOUND);

  // 🏗️ 构建完整的HTTP响应字符串
  std::string build() const;

  // 📊 获取响应信息
  StatusCode get_status() const { return status_code_; }
  std::string_view get_status_text() const;
  size_t get_content_length() const { return body_.size(); }

  // ❌ 错误响应快捷方法（静态工厂方法）
  static HttpResponse make_error_response(StatusCode code,
                                          std::string_view message = "");
  static HttpResponse make_404_response();
  static HttpResponse make_500_response();

private:
  // 🔧 内部辅助方法
  void auto_set_content_type(); // 自动检测内容类型
  std::string_view
  get_default_status_text(StatusCode code) const; // 获取状态码描述
  bool
  read_file_content(const std::filesystem::path &file_path); // 读取文件内容
  void cleanup_mmap(); // 清理内存映射

  // 💾 成员变量
  StatusCode status_code_ = StatusCode::OK;              // 状态码
  std::unordered_map<std::string, std::string> headers_; // HTTP头部
  std::string body_;                                     // 响应体内容

  // 🗺️ 内存映射相关（用于高效处理大文件）
  void *mmap_addr_ = nullptr; // 内存映射地址
  size_t mmap_size_ = 0;      // 映射大小
  bool use_mmap_ = false;     // 是否使用内存映射
};
