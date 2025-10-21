#include "server.hpp"
#include <string>
#include <string_view>
#include <fstream>
#include <sstream>
#include <optional>
std::optional<std::string> parseRequestLine(std::string_view bufView);