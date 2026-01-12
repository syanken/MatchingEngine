// core/MatchingEngine.cpp
#include "MatchingEngine.h"
#include "protocol/MessageCodec.h"
#include "protocol/MessageType.h"
#include "utils/Logger.h"
#include <spdlog/spdlog.h>

void MatchingEngine::onMessage(Connection *conn, MessageType type, const std::vector<uint8_t> &payload)
{
    switch (type)
    {
    case MessageType::NEW_ORDER:
        handleNewOrder(conn, payload);
        break;
    case MessageType::CANCEL_ORDER:
        handleCancelOrder(conn, payload);
        break;
    case MessageType::HEARTBEAT:
        spdlog::debug("Heartbeat from fd={}", conn->fd());
        break;
    default:
        spdlog::warn("Unknown message type: {}", static_cast<int>(type));
    }
}

void MatchingEngine::handleNewOrder(Connection *conn, const std::vector<uint8_t> &payload)
{
    auto order = Order::deserialize(payload);
    if (!order)
    {
        spdlog::error("Invalid new order from fd={}", conn->fd());
        return;
    }

    auto callback = [this, conn](const ExecutionReport &rpt)
    {
        this->sendExecutionReport(conn, rpt);
    };

    spdlog::info(
        "Received order: user={} order_id={} side={} price={} qty={}",
        order->user_id,
        order->order_id,
        (order->side == OrderSide::BUY ? "BUY" : "SELL"),
        order->price,
        order->quantity);

    orderBook_.matchOrder(*order, callback);
}

void MatchingEngine::handleCancelOrder(Connection *conn, const std::vector<uint8_t> &payload)
{
    if (payload.size() < 32)
    {
        spdlog::error("Cancel order: payload too short");
        return;
    }

    std::string order_id(reinterpret_cast<const char *>(payload.data()), 32);
    order_id.resize(order_id.find('\0'));

    auto callback = [this, conn](const ExecutionReport &rpt)
    {
        this->sendExecutionReport(conn, rpt);
    };

    orderBook_.cancelOrder(order_id, callback);
}

void MatchingEngine::sendExecutionReport(Connection *conn, const ExecutionReport &report)
{
    // 序列化 ExecutionReport（简化：只发 order_id + type + leaves_qty）
    std::vector<uint8_t> payload;
    payload.resize(32 + 1 + 4); // order_id(32) + type(1) + leaves(4)

    std::string oid = report.order_id;
    oid.resize(32, '\0');
    std::memcpy(payload.data(), oid.data(), 32);
    payload[32] = static_cast<uint8_t>(report.exec_type);
    std::memcpy(payload.data() + 33, &report.leaves_qty, 4);

    auto frame = MessageCodec::encode(MessageType::EXECUTION_REPORT, payload);
    send(conn->fd(), frame.data(), frame.size(), 0);
    spdlog::info("Sent {} bytes (frame)", frame.size());
}
