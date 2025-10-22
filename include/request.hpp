#include "server.hpp"
#include <string>
#include <string_view>
#include <fstream>
#include <sstream>
#include <optional>
#include <filesystem>
namespace fs = std::filesystem;
std::optional<std::string> parseRequestLine(std::string_view bufView);
std::optional<fs::path> parse_request_path(std::optional<std::string> path);