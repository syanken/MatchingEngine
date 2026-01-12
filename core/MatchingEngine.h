#pragma once
#include <unordered_map>
#include <functional>
#include "OrderBook.h"
#include "network/Connection.h"
#include "protocol/MessageType.h"
class MatchingEngine {
public:
    void onMessage(Connection* conn, MessageType type, const std::vector<uint8_t>& payload);

private:
    void handleNewOrder(Connection* conn, const std::vector<uint8_t>& payload);
    void handleCancelOrder(Connection* conn, const std::vector<uint8_t>& payload);
    void sendExecutionReport(Connection* conn, const ExecutionReport& report);

    OrderBook orderBook_;

};