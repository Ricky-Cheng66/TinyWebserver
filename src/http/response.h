#pragma once
#include "../server/server.h"
#include <fstream>
#include <sstream>
#include <optional>
#include <sys/mman.h>
#include <span>
class Response
{
public:
    Response();
    ~Response();
    std::string build_response(std::optional<std::string> opt);
};


