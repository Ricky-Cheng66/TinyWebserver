#include "request.hpp"

std::optional<std::string> parseRequestLine(std::string_view bufView)
{
     /* 只取第一行 */
     bufView.remove_suffix(2);//去除\r\n
     /* 极简解析：method path version */
     auto sp1 = bufView.find(' ');
     auto sp2 = bufView.find(' ', sp1 + 1);
     if (sp1 == std::string::npos || sp2 == std::string::npos)
     {
          return std::nullopt;
     }
     auto method{bufView.substr(0, sp1)};
     std::string path{bufView.substr(sp1 + 1, sp2 - sp1 - 1)};
     auto version{bufView.substr(sp2 + 1)};
     /* 默认文件 */
     if (path == "/")
     {
          path = "/index.html";
     }
     /* 读文件 */
     std::string file = "www" + path;
     std::ifstream in(file, std::ios::binary);
     if (!in)
     {
          return std::nullopt;
     }

     std::ostringstream tmp;
     tmp << in.rdbuf();

     return tmp.str();
}