#pragma once
#include <string>
#include <cstdint>
#include <vector>
#include <optional>

enum class OrderSide : uint8_t
{
    BUY = 1,
    SELL = 0
};

struct Order
{
    std::string user_id;        // 16
    std::string order_id;       // 32
    OrderSide side;             // 1
    double price;               // 8
    int32_t quantity;           // 4
    int32_t remaining_quantity; // 4
    uint64_t timestamp;         // 8

    std::vector<uint8_t> serialize() const;

    static std::optional<Order> deserialize(const std::vector<uint8_t> &data);
};