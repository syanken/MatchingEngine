#pragma once
#include <string>
#include <cstdint>

enum class ExecType : uint8_t
{
    NEW = 0,
    PARTIAL_FILL = 1,
    FILL = 2,
    CANCELED = 3
};

struct ExecutionReport
{
    std::string order_id;
    double price;
    int32_t last_shares;
    int32_t leaves_qty;
    ExecType exec_type;
};