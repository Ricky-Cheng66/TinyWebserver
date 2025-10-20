#include "request.hpp"

bool parseRequestLine(const char* firstLine, std::string& pathOut)
{
     /* 极简解析：method path version */
     std::string line(firstLine), method, path, version;
     size_t sp1 = line.find(' ');
     size_t sp2 = line.find(' ', sp1 + 1);
     if (sp1 == std::string::npos || sp2 == std::string::npos)
     {
          return false;
     }
     method = line.substr(0, sp1);
     path = line.substr(sp1 + 1, sp2 - sp1 - 1);
     version = line.substr(sp2 + 1);

     /* 默认文件 */
     if (path == "/")
     {
          path = "/index.html";
     }
          pathOut = path;
     return true;
}