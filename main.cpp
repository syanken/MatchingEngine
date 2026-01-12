#include <iostream>
#include <csignal>
#include "utils/Logger.h"
#include "network/TcpServer.h"
#include "core/Order.h"
#include "core/OrderBook.h"
#include "core/ExecutionReport.h"
#include "core/MatchingEngine.h"

int main()
{
    Logger::init();
    spdlog::info("Matching Engine started!");

    MatchingEngine engine;
    auto onMessage = [&engine](Connection* conn, MessageType type, const std::vector<uint8_t>& payload) {
        engine.onMessage(conn, type, payload);
    };
    // 启动服务器
    TcpServer server(9999, onMessage);

    // 捕获 Ctrl+C
    signal(SIGINT, [](int)
           {
        spdlog::info("Shutting down...");
        exit(0); });
    server.start();
    return 0;
}
