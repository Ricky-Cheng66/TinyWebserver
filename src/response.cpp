#include "response.hpp"
std::string buildResponse(const std::string& path)
{
    /* 读文件 */
    std::string file = "www" + path;
    int filefd = open(file.c_str(), O_RDONLY);
    std::string body;
    if (filefd < 0)
    {
        body = "<h1>404 Not Found</h1>";
    }
    else
    {
        struct stat st;
        fstat(filefd, &st);
        body.resize(st.st_size);
        ssize_t n = read(filefd, &body[0], st.st_size);
        if (n < 0)
        {
            perror("read failed...");
        }
        close(filefd);
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