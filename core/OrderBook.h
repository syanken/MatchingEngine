#pragma once
#include <map>
#include <list>
#include <unordered_map>
#include <functional>
#include "Order.h"
#include "ExecutionReport.h"
class OrderBook
{
public:
    using MatchCallback = std::function<void(const ExecutionReport &)>;
    OrderBook();
    ~OrderBook() = default;

    bool matchOrder(Order order, MatchCallback callback);
    bool cancelOrder(const std::string &order_id, MatchCallback callback);
    double getLastTradedPrice()
    {
        return lastTradedPrice;
    }

private:
    template <typename BookType, typename TradePredicate>
    void matchAgainstBook(Order &order, BookType &book, TradePredicate canTrade, MatchCallback &callback);
    void generateReport(
        const Order &order,
        int32_t last_shares,
        ExecType type,
        MatchCallback &callback);
    double lastTradedPrice = 0.0;
    std::map<double, std::list<Order>, std::greater<double>> buyBook;
    std::map<double, std::list<Order>, std::less<double>> sellBook;

    struct OrderHandle
    {
        double price;
        std::list<Order>::iterator iter;
        OrderSide side;
    };
    std::unordered_map<std::string, OrderHandle> orderIndex;
};