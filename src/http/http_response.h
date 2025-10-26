#pragma once
#include "../server/server.h"
#include <fstream>
#include <optional>
#include <span>
#include <sstream>
#include <sys/mman.h>
class HttpResponse {
public:
  HttpResponse() = default;
  ~HttpResponse() = default;
  std::string build_response(std::optional<std::string> opt);
};
