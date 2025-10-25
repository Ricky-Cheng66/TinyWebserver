#pragma once
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
namespace fs = std::filesystem;
class Request {
public:
  Request() = default;
  ~Request() = default;
  std::optional<std::string> parse_request_line(std::string_view bufView);
  std::optional<fs::path> parse_request_path(std::optional<std::string> path);
};
