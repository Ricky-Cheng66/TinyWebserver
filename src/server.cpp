#include "server.hpp"
#include "request.hpp"
#include "response.hpp"
const int MAXCLIENTFD = 1024;
const int MAXEVENTS = 10;
bool setNonBlock(int socketFd)
{
    int flags = fcntl(socketFd, F_GETFL, 0);
    if (flags < 0)
    {
        perror("fcntl failed...");
        return false;
    }
    fcntl(socketFd, F_SETFL, flags | O_NONBLOCK);
    return true;
}
int main()
{
    // create serverFd
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serverAddr{};
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    socklen_t serverLen = sizeof(serverAddr);
    // set listenFd nonblock
    setNonBlock(serverFd);
    // create epfd and put listenFd into epoll
    int epfd = epoll_create(1);
    epoll_event ev{};
    ev.data.fd = serverFd;
    ev.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, serverFd, &ev);
    if (bind(serverFd, (sockaddr *)&serverAddr, serverLen) < 0)
    {
        perror("bind failed...");
        return false;
    }
    listen(serverFd, MAXCLIENTFD);
    while (1) // loop of events
    {

        epoll_event evs[MAXEVENTS];
        int nfds = epoll_wait(epfd, evs, MAXEVENTS, 0);
        for (int i = 0; i < nfds; i++)
        {
            if (evs[i].data.fd == serverFd) // accept
            {
                sockaddr_in clientAddr{};
                socklen_t clientLen = sizeof(clientAddr);
                int clientFd = accept(serverFd, (sockaddr *)&clientAddr, &clientLen);
                setNonBlock(clientFd);
                if (clientFd < 0)
                {
                    if (errno == EAGAIN || errno == EMFILE)
                        continue; // 资源暂时不可用
                    perror("accept");
                    continue;
                }
                std::cout << "new client connected..." << std::endl;
                ev.data.fd = clientFd;
                ev.events = EPOLLIN | EPOLLET;
                epoll_ctl(epfd, EPOLL_CTL_ADD, ev.data.fd, &ev);
            }
            else if (evs[i].events & EPOLLIN)
            {
                // read dealing
                /* 一次性读完（短连接够用） */
                int fd = evs[i].data.fd;
                char buf[4096];
                ssize_t nread = read(fd, buf, sizeof(buf) - 1);
                if (nread <= 0)
                { // 对方关闭或出错
                    close(fd);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                    continue;
                }
                buf[nread] = '\0';
                if (!buf)
                { // 格式异常
                    close(fd);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                    continue;
                }
                std::optional<std::string> opt = parseRequestLine(buf);
                if (!opt.has_value())
                {
                    close(fd);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                }
                std::string resp = buildResponse(opt);
                /* 发回并关闭连接 */
                ssize_t nwrite = write(fd, resp.c_str(), resp.size());
                if (nwrite < 0)
                {
                    perror("read failed...");
                }
                close(fd);
                epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
            }
            else if (evs[i].events & EPOLLOUT) // clientFd WRITE events
            {

            }
            else
            {
                perror("false");
                return false;
            }
        }
    }
    return 0;
}