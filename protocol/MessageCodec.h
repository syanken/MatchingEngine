#pragma once
#include <vector>
#include <cstdint>
#include <optional>
#include "MessageType.h"
class MessageCodec
{
public:
    // 编码：将 payload 打包成完整帧
    static std::vector<uint8_t> encode(MessageType type, const std::vector<uint8_t>& payload);

    // 解码：返回 (type, payload)
    static std::optional<std::pair<MessageType, std::vector<uint8_t>>> decode(
        const std::vector<uint8_t>& buffer,
        size_t& readIndex
    );

private:
    static constexpr uint32_t MAGIC = 0xABCDEF00;
    static constexpr size_t HEADER_SIZE = 7; // 4 (magic) + 2 (length)+1 (type)
};