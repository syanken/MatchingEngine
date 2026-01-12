#include "OrderBook.h"
#include "utils/Logger.h"
OrderBook::OrderBook() = default;
template <typename BookType, typename TradePredicate>
void OrderBook::matchAgainstBook(Order &order, BookType &book, TradePredicate canTrade, MatchCallback &callback)
{
    auto it = book.begin();
    while (order.remaining_quantity > 0 &&
           it != book.end() &&
           canTrade(order.price, it->first))
    {

        auto &list = it->second;
        auto &front_order = list.front();

        int32_t trade_quantity = std::min(order.remaining_quantity, front_order.remaining_quantity);
        // 执行成交

        double trade_price = it->first;
        lastTradedPrice = trade_price;

        order.remaining_quantity -= trade_quantity;
        front_order.remaining_quantity -= trade_quantity;
        generateReport(order, trade_quantity,
                       front_order.remaining_quantity == 0 ? ExecType::FILL : ExecType::PARTIAL_FILL,
                       callback);
        // 如果单被完全吃掉
        if (front_order.remaining_quantity == 0)
        {
            orderIndex.erase(front_order.order_id);
            list.pop_front(); // 移除第一个元素
            if (list.empty())
            {
                it = book.erase(it); // erase 返回下一个迭代器
            }
        }
        else
        {
            break;
        }
    }
    if (order.quantity - order.remaining_quantity > 0)
    {
        int32_t filled = order.quantity - order.remaining_quantity;
        generateReport(order, filled,
                       order.remaining_quantity == 0 ? ExecType::FILL : ExecType::PARTIAL_FILL,
                       callback);
    }
}

bool OrderBook::matchOrder(Order order, MatchCallback callback)
{
    double price = order.price;
    std::string order_id = order.order_id;
    if (order.side == OrderSide::BUY)
    {
        matchAgainstBook(order, sellBook, [](double buyPx, double sellPx)
                         { return buyPx >= sellPx; }, callback);
        if (order.remaining_quantity > 0)
        {
            buyBook[price].push_back(std::move(order));
            orderIndex[order_id] = {
                price,
                std::prev(buyBook[price].end()),
                OrderSide::BUY};
            generateReport(
                *orderIndex[order_id].iter,
                0,
                (order.quantity == order.remaining_quantity) ? ExecType::NEW : ExecType::PARTIAL_FILL,
                callback); // 未成交的单
        }
    }
    else
    {
        matchAgainstBook(order, buyBook, [](double sellPx, double buyPx)
                         { return buyPx >= sellPx; }, callback);
        if (order.remaining_quantity > 0)
        {
            sellBook[price].push_back(std::move(order));
            orderIndex[order_id] = {
                price,
                std::prev(sellBook[price].end()),
                OrderSide::SELL};
            generateReport(
                *orderIndex[order_id].iter,
                0,
                (order.quantity == order.remaining_quantity) ? ExecType::NEW : ExecType::PARTIAL_FILL,
                callback); // 未成交的单
        }
    }
    return true;
}

bool OrderBook::cancelOrder(const std::string &order_id, MatchCallback callback)
{
    auto it = orderIndex.find(order_id);
    if (it == orderIndex.end())
    {
        spdlog::warn("Cancel failed: order not found: {}", order_id);
        return false;
    }

    const auto &handle = it->second;

    // 从订单簿中删除
    if (handle.side == OrderSide::BUY)
    {
        buyBook[handle.price].erase(handle.iter);
        // 清理空档位
        if (buyBook[handle.price].empty())
        {
            buyBook.erase(handle.price);
        }
    }
    else
    {
        sellBook[handle.price].erase(handle.iter);
        if (sellBook[handle.price].empty())
        {
            sellBook.erase(handle.price);
        }
    }
    //撤单通知
    ExecutionReport report;
    report.order_id = order_id;
    report.exec_type = ExecType::CANCELED;
    report.leaves_qty = 0;
    callback(report);
    spdlog::info("Order canceled: {}", order_id);
    return true;
}

void OrderBook::generateReport(
    const Order &order,
    int32_t last_shares,
    ExecType type,
    MatchCallback &callback)
{
    ExecutionReport report;
    report.order_id = order.order_id;
    report.price = lastTradedPrice;
    report.last_shares = last_shares;
    report.leaves_qty = (type == ExecType::CANCELED) ? 0 : order.remaining_quantity;
    report.exec_type = type;
    callback(report);
}