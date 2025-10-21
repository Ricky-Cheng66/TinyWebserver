#include "response.hpp"
std::string buildResponse(const std::string& path)
{
    /* 读文件 */
    std::string file = "www" + path;
    std::ifstream in(file, std::ios::binary);
    std::string body;
    if (!in)
    {
        body = "<h1>404 Not Found</h1>";
    }
    else
    {
        std::ostringstream tmp;
        tmp << in.rdbuf();
        std::string body = tmp.str();
    }
    /* 构造响应 */
    std::string resp =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " +
        std::to_string(body.size()) +
        "\r\nConnection: close\r\n\r\n" + body;
    return resp;
}