#include "Order.h"
#include "utils/Logger.h"

std::vector<uint8_t> Order::serialize() const
{
    std::vector<uint8_t> buf(73, 0);
    size_t offset = 0;

    size_t uid_len = std::min(user_id.size(), static_cast<size_t>(15));
    memcpy(buf.data(), user_id.data(), uid_len);
    offset += 16;

    size_t orderid_len = std::min(order_id.size(), static_cast<size_t>(31));
    memcpy(buf.data()+offset, order_id.data(), orderid_len);
    offset += 32;

    buf[offset++] = static_cast<uint8_t>(side);

    std::memcpy(buf.data() + offset, &price, sizeof(double));
    offset += sizeof(double);

    std::memcpy(buf.data() + offset, &quantity, sizeof(int32_t));
    offset += sizeof(int32_t);

    std::memcpy(buf.data() + offset, &remaining_quantity, sizeof(int32_t));
    offset += sizeof(int32_t);

    std::memcpy(buf.data() + offset, &timestamp, sizeof(uint64_t));

    return buf;
}

std::optional<Order> Order::deserialize(const std::vector<uint8_t> &data)
{
    if (data.size() != 73)
    {
        spdlog::error("Invalid order binary size: {}", data.size());
        return std::nullopt;
    }

    Order order;
    size_t offset = 0;

    std::string uid(reinterpret_cast<const char *>(data.data() + offset), 16);
    uid.resize(uid.find('\0')); // 截断到第一个 \0
    order.user_id = std::move(uid);
    offset += 16;

    std::string oid(reinterpret_cast<const char *>(data.data() + offset), 32);
    oid.resize(oid.find('\0'));
    order.order_id = std::move(oid);
    offset += 32;

    uint8_t side_val = data[offset++];
    if (side_val > 1)
    {
        spdlog::error("Invalid order side: {}", side_val);
        return std::nullopt;
    }
    order.side = static_cast<OrderSide>(side_val);

    std::memcpy(&order.price, data.data() + offset, sizeof(double));
    offset += sizeof(double);

    std::memcpy(&order.quantity, data.data() + offset, sizeof(int32_t));
    offset += sizeof(int32_t);

    std::memcpy(&order.remaining_quantity, data.data() + offset, sizeof(int32_t));
    offset += sizeof(int32_t);

    std::memcpy(&order.timestamp, data.data() + offset, sizeof(uint64_t));

    // 简单校验
    if (order.price <= 0 || order.quantity <= 0)
    {
        spdlog::error("Invalid order: price={}, qty={}", order.price, order.quantity);
        return std::nullopt;
    }

    return order;
}