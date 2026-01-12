#pragma once
#include <cstdint>

enum class MessageType : uint8_t {
    NEW_ORDER = 1,
    CANCEL_ORDER = 2,
    HEARTBEAT = 3,
    EXECUTION_REPORT = 4  // 服务端 → 客户端
};