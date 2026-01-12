#pragma once
#include <sys/epoll.h>
#include <unordered_map>
#include <memory>
#include "Connection.h"

using MessageCallback = std::function<void(Connection *, MessageType, const std::vector<uint8_t> &)>;

class TcpServer
{
public:
    explicit TcpServer(int port, MessageCallback cb);
    ~TcpServer();
    void start();

private:
    void handleAccept();
    void runEventLoop();
    MessageCallback messageCallback_;
    int listenFd_;
    int epollFd_;
    int port_;
    std::unordered_map<int, std::unique_ptr<Connection>> connections_;
    static const int MAX_EVENTS = 1024;
};