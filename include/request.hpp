#include "server.hpp"
#include <string>
#include <string_view>
bool parseRequestLine(std::string_view bufView, std::string& pathOut);