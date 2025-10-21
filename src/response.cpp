#include "response.hpp"
std::string buildResponse(std::optional<std::string> opt)
{
    if (!opt.has_value())
    {
        return "<h1>404 Not Found</h1>";
    }
    std::string resp =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " +
        std::to_string(opt.value().size()) +
        "\r\nConnection: close\r\n\r\n" + opt.value();
    return resp;
}