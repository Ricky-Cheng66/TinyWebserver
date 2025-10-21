#include "request.hpp"

bool parseRequestLine(std::string_view bufView, std::string& pathOut)
{
     /* 只取第一行 */
     bufView.remove_suffix(2);//去除\r\n
     /* 极简解析：method path version */
     auto sp1 = bufView.find(' ');
     auto sp2 = bufView.find(' ', sp1 + 1);
     if (sp1 == std::string::npos || sp2 == std::string::npos)
     {
          return false;
     }
     auto method{bufView.substr(0, sp1)};
     auto path{bufView.substr(sp1 + 1, sp2 - sp1 - 1)};
     auto version{bufView.substr(sp2 + 1)};
     /* 默认文件 */
     if (path == "/")
     {
          path = "/index.html";
     }
     pathOut = path;
     
     return true;
}