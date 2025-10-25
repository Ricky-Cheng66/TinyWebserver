#include "request.h"
#include <iostream>
std::optional<std::string>
Request::parse_request_line(std::string_view bufView) {
  /* 只取第一行 */
  bufView.remove_suffix(2); // 去除\r\n
  /* 极简解析：method path version */
  auto sp1 = bufView.find(' ');
  auto sp2 = bufView.find(' ', sp1 + 1);
  if (sp1 == std::string::npos || sp2 == std::string::npos) {
    return std::nullopt;
  }
  auto method{bufView.substr(0, sp1)};
  std::optional<std::string> path{bufView.substr(sp1 + 1, sp2 - sp1 - 1)};
  auto version{bufView.substr(sp2 + 1)};
  /* 默认文件 */
  if (path == "/") {
    path = "/index.html";
  }

  return path;
}
std::optional<fs::path>
Request::parse_request_path(std::optional<std::string> path) {
  if (!path.has_value()) {
    std::cout << " 错误 路径为空 " << std::endl;
    return std::nullopt;
  }
  // 将path.value()开头的/去掉
  if (path.value()[0] == '/') {
    path.value() = path.value().substr(1);
  }
  // 路径拼接
  fs::path root_path{"www"};
  fs::path get_path{path.value()};
  fs::path request_path = root_path / get_path;
  try {
    fs::path canonical_request_path = fs::canonical(request_path);
    fs::path canonical_root_path = fs::canonical(root_path);
    auto request_path_str = canonical_request_path.string();
    auto root_path_str = canonical_root_path.string();
    if (request_path_str.find(root_path_str) != 0) // 不是以根目录开头
    {
      std::cout << "路径错误 不以根目录开头" << std::endl;
      return std::nullopt;
    }
    if (!fs::is_regular_file(canonical_request_path)) // 检查是否为普通文件
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
std::optional<std::string> parseRequestLine(std::string_view bufView) {
  /* 只取第一行 */
  bufView.remove_suffix(2); //去除\r\n
  /* 极简解析：method path version */
  auto sp1 = bufView.find(' ');
  auto sp2 = bufView.find(' ', sp1 + 1);
  if (sp1 == std::string::npos || sp2 == std::string::npos) {
    return std::nullopt;
  }
  auto method{bufView.substr(0, sp1)};
  std::optional<std::string> path{bufView.substr(sp1 + 1, sp2 - sp1 - 1)};
  auto version{bufView.substr(sp2 + 1)};
  /* 默认文件 */
  if (path == "/") {
    path = "/index.html";
  }

  return path;
}
std::optional<fs::path> parse_request_path(std::optional<std::string> path) {
  if (!path.has_value()) {
    std::cout << " 错误 路径为空 " << std::endl;
    return std::nullopt;
  }
  //将path.value()开头的/去掉
  if (path.value()[0] == '/') {
    path.value() = path.value().substr(1);
  }
  //路径拼接
  fs::path root_path{"www"};
  fs::path get_path{path.value()};
  fs::path request_path = root_path / get_path;
  try {
    fs::path canonical_request_path = fs::canonical(request_path);
    fs::path canonical_root_path = fs::canonical(root_path);
    auto request_path_str = canonical_request_path.string();
    auto root_path_str = canonical_root_path.string();
    if (request_path_str.find(root_path_str) != 0) //不是以根目录开头
    {
      std::cout << "路径错误 不以根目录开头" << std::endl;
      return std::nullopt;
    }
    if (!fs::is_regular_file(canonical_request_path)) //检查是否为普通文件
    {
      std::cout << "错误 不是普通文件" << std::endl;
      return std::nullopt;
    }
    //所有检查通过
    return canonical_request_path;

  } catch (const fs::filesystem_error &e) {
    std::cerr << e.what() << '\n';
    return std::nullopt;
  }
}
