#include "server/server.h"
#include <iostream>
int main() {
  //初始化Server
  Server server{};
  if (!server.init(8080)) {
    std::cerr << "server init failed..." << std::endl;
    return -1;
  }
  //启动Server
  if (!server.start()) {
    std::cerr << "server start failed" << std::endl;
    return -1;
  }
  return 0;
}