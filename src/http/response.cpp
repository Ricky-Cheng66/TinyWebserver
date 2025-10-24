#include "response.h"
Response::Response()
{

}
Response::~Response()
{

}
std::string Response::build_response(std::optional<std::string> opt)
{
        if (!opt.has_value())
    {
        return "<h1>404 Not Found</h1>";
    }
    // int fd = open(opt.value().c_str(), O_RDONLY);
    // if(fd = -1){
    //     std::cout << "文件不存在 " << std::endl;
    //     std::cerr << errno << std::endl;
    //     return{};
    // }
    // struct stat st{};
    // fstat(fd, &st);
    // if(st.st_size > 64){
    //     void* mmap_addr = mmap(nullptr, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    //     std::span<const char> view{static_cast<const char *>(mmap_addr), 
    //                                static_cast<long unsigned int>(st.st_size)};
        
    //     return view;
    // }
    std::string resp =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " +
        std::to_string(opt.value().size()) +
        "\r\nConnection: close\r\n\r\n" + opt.value();
    // std::span<const char> view{resp};
    return resp;
}
