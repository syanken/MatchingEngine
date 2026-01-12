#include "MessageCodec.h"
#include "utils/Logger.h"
#include <spdlog/spdlog.h>
#include <cstring>
#include <cassert>

std::vector<uint8_t> MessageCodec::encode(MessageType type, const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> frame;
    frame.resize(HEADER_SIZE + payload.size());

    // 写 magic (小端)
    uint32_t magic = MAGIC;
    std::memcpy(frame.data(), &magic, 4);

    // 写 length (小端，2字节)
    uint16_t len = static_cast<uint16_t>(payload.size());
    std::memcpy(frame.data() + 4, &len, 2);
    frame[6] =static_cast<uint8_t>(type);
    // 写 payload
    if (!payload.empty()) {
        std::memcpy(frame.data() + HEADER_SIZE, payload.data(), payload.size());
    }

    return frame;
}

std::optional<std::pair<MessageType, std::vector<uint8_t>>> MessageCodec::decode(
    const std::vector<uint8_t>& buffer,
    size_t& readIndex
) {
    size_t available = buffer.size() - readIndex;

    // 1. 至少要有 header
    if (available < HEADER_SIZE) {
        return std::nullopt;
    }

    // 2. 读 magic
    uint32_t magic;
    std::memcpy(&magic, buffer.data() + readIndex, 4);
    spdlog::info("Read magic: {:08x} (bytes: {:02x} {:02x} {:02x} {:02x})", 
    magic,
    buffer[readIndex], buffer[readIndex+1], buffer[readIndex+2], buffer[readIndex+3]);

    if (magic != MAGIC) {
        spdlog::error("Invalid magic: {:08x}", magic);
        readIndex = buffer.size(); // 跳过非法数据
        return std::nullopt;
    }

    // 3. 读 length
    uint16_t payloadLen;
    std::memcpy(&payloadLen, buffer.data() + readIndex + 4, 2);

    uint8_t typeByte = buffer[readIndex + 6];
    MessageType type = static_cast<MessageType>(typeByte);

    // 4. 检查是否有完整 payload
    if (available < HEADER_SIZE + payloadLen) {
        return std::nullopt; // 半包，等待更多数据
    }

    // 5. 提取 payload
    std::vector<uint8_t> payload(payloadLen);
    if (payloadLen > 0) {
        std::memcpy(payload.data(),
                    buffer.data() + readIndex + HEADER_SIZE,
                    payloadLen);
    }

    // 6. 移动 readIndex 到下一个包起点
    readIndex += HEADER_SIZE + payloadLen;

    return std::make_pair(type, payload);
}