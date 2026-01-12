#include "TcpServer.h"
#include "utils/Logger.h"
#include <spdlog/spdlog.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <iostream>

TcpServer::TcpServer(int port, MessageCallback cb)
    : port_(port), messageCallback_(std::move(cb))
{
    // 创建监听 socket
    listenFd_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (listenFd_ == -1)
    {
        spdlog::critical("Failed to create socket");
        exit(1);
    }

    // 设置 SO_REUSEADDR,立即重启服务而不被 TIME_WAIT 阻塞
    int opt = 1;
    setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 绑定
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(listenFd_, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        spdlog::critical("Bind failed: {}", strerror(errno));
        exit(1);
    }

    // 监听
    if (listen(listenFd_, 128) == -1)
    {
        spdlog::critical("Listen failed");
        exit(1);
    }

    // 创建 epoll
    epollFd_ = epoll_create1(EPOLL_CLOEXEC);
    if (epollFd_ == -1)
    {
        spdlog::critical("Epoll create failed");
        exit(1);
    }

    // 注册监听 socket 到 epoll（ET 模式）
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = listenFd_;
    if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, listenFd_, &ev) == -1)
    {
        spdlog::critical("Epoll_ctl add listen fd failed");
        exit(1);
    }

    spdlog::info("TcpServer listening on port {}", port_);
}

TcpServer::~TcpServer()
{
    close(listenFd_);
    close(epollFd_);
}

void TcpServer::handleAccept()
{
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    int clientFd;

    while ((clientFd = accept4(listenFd_, (struct sockaddr *)&clientAddr, &clientLen, SOCK_NONBLOCK)) != -1)
    {
        spdlog::info("New connection from {}:{}", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

        // 设置客户端 socket 为非阻塞（accept4 已设置，双重保险）
        int flags = fcntl(clientFd, F_GETFL, 0);
        fcntl(clientFd, F_SETFL, flags | O_NONBLOCK);

        // 注册到 epoll
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
        ev.data.fd = clientFd;
        if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, clientFd, &ev) == -1)
        {
            spdlog::error("Epoll_ctl add client fd failed: {}", strerror(errno));
            close(clientFd);
            continue;
        }

        // 保存连接
        connections_[clientFd] = std::make_unique<Connection>(clientFd, messageCallback_);
    }

    if (errno != EAGAIN && errno != EWOULDBLOCK)
    {
        spdlog::error("Accept error: {}", strerror(errno));
    }
}

void TcpServer::runEventLoop()
{
    std::vector<epoll_event> events(MAX_EVENTS);
    spdlog::info("Starting event loop...");
    std::cout<<"Starting event loop...\n";
    while (true)
    {
        int nfds = epoll_wait(epollFd_, events.data(), MAX_EVENTS, -1);
        
        if (nfds == -1)
        {
            if (errno == EINTR)
                continue;
            spdlog::critical("Epoll_wait failed: {}", strerror(errno));
            break;
        }

        for (int i = 0; i < nfds; ++i)
        {
            int fd = events[i].data.fd;

            if (fd == listenFd_)
            {
                handleAccept();
            }
            else
            {
                auto it = connections_.find(fd);
                if (it == connections_.end())
                    continue;

                if (events[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
                {
                    spdlog::info("Client disconnected: fd={}", fd);
                    connections_.erase(it);
                    continue;
                }

                if (events[i].events & EPOLLIN)
                {
                    it->second->handleRead();
                }
            }
        }
    }
}

void TcpServer::start()
{
    runEventLoop();
}