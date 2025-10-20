#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

bool setNonBlock(int socketFd);