#pragma once
#include <string>
#include <string_view>
#include <fstream>
#include <sstream>
#include <optional>
#include <filesystem>
namespace fs = std::filesystem;
class Request
{
public:
    Request();
    ~Request();
    std::optional<std::string> parse_request_line(std::string_view bufView);
    std::optional<fs::path> parse_request_path(std::optional<std::string> path);
};

