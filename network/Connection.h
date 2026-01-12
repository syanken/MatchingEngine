#pragma once
#include <sys/socket.h>
#include <memory>
#include <vector>
#include <functional>
#include "protocol/MessageType.h" 

// class MessageCodec;

class Connection {
public:
    using MessageCallback = std::function<void(Connection*, MessageType,const std::vector<uint8_t>&)>;
    explicit Connection(int fd, MessageCallback cb);
    ~Connection();

    void handleRead();

    int fd() const { return sockfd_; }

private:
    int sockfd_;
    std::vector<uint8_t> recvBuffer_;

    size_t readIndex_ = 0; 
    static const size_t BUFFER_SIZE = 4096;
    
    MessageCallback messageCallback_;
};