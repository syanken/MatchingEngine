#include "Connection.h"
#include "protocol/MessageCodec.h"
#include "utils/Logger.h"
#include <spdlog/spdlog.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <spdlog/fmt/bin_to_hex.h>
Connection::Connection(int fd, MessageCallback cb) : sockfd_(fd), messageCallback_(std::move(cb)), recvBuffer_(BUFFER_SIZE)
{
    // 设置非阻塞
    int flags = fcntl(sockfd_, F_GETFL, 0);
    fcntl(sockfd_, F_SETFL, flags | O_NONBLOCK);
}

Connection::~Connection()
{
    close(sockfd_);
    spdlog::info("Connection closed: fd={}", sockfd_);
}

void Connection::handleRead()
{
    while (true)
    {
        if (recvBuffer_.size() - readIndex_ < 1024)
        {
            recvBuffer_.resize(recvBuffer_.size() + BUFFER_SIZE);
        }

        ssize_t n = recv(sockfd_, recvBuffer_.data() + readIndex_, recvBuffer_.size() - readIndex_, 0);

        if (n > 0)
        {

            readIndex_ += n;
            size_t currentOffset = 0; // 从 buffer 起点开始解析

            while (true)
            {
                size_t tempIndex = currentOffset;

                auto result = MessageCodec::decode(recvBuffer_, tempIndex);
                if (result)
                {
                    auto [type, msg] = *result;
                    messageCallback_(this, type, msg);
                    currentOffset = tempIndex; // 更新已处理位置
                }
                else
                {
                    break;
                }
            }

            // 滑动：只保留 [currentOffset, readIndex_) 的未处理数据
            if (currentOffset > 0)
            {
                std::memmove(recvBuffer_.data(),
                             recvBuffer_.data() + currentOffset,
                             readIndex_ - currentOffset);
                readIndex_ -= currentOffset;
                recvBuffer_.resize(std::max(readIndex_, static_cast<size_t>(1024))); // 保留最小空间
            }
        }
        else if (n == 0)
        {
            spdlog::info("Client closed connection: fd={}", sockfd_);
            break;
        }
        else
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break; // 数据已读完
            }
            else
            {
                spdlog::error("Read error on fd={}: {}", sockfd_, strerror(errno));
                break;
            }
        }
    }
}