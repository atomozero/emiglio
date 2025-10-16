#ifndef EXCHANGEAPI_H
#define EXCHANGEAPI_H

#include <string>
#include <vector>
#include <map>
#include "../data/DataStorage.h"

namespace Emiglio {

// Order side
enum class OrderSide {
	BUY,
	SELL
};

// Order type
enum class OrderType {
	MARKET,
	LIMIT,
	STOP_LOSS,
	STOP_LOSS_LIMIT,
	TAKE_PROFIT,
	TAKE_PROFIT_LIMIT
};

// Order status
enum class OrderStatus {
	NEW,
	PARTIALLY_FILLED,
	FILLED,
	CANCELED,
	PENDING_CANCEL,
	REJECTED,
	EXPIRED
};

// Market ticker (24h stats)
struct Ticker {
	std::string symbol;
	double lastPrice;
	double priceChange;
	double priceChangePercent;
	double highPrice;
	double lowPrice;
	double volume;
	double quoteVolume;
	time_t timestamp;
};

// Order book level
struct OrderBookLevel {
	double price;
	double quantity;
};

// Order book
struct OrderBook {
	std::string symbol;
	std::vector<OrderBookLevel> bids;
	std::vector<OrderBookLevel> asks;
	time_t timestamp;
};

// Account balance
struct Balance {
	std::string asset;
	double free;
	double locked;
	double total;
};

// Order information
struct Order {
	std::string orderId;
	std::string symbol;
	OrderSide side;
	OrderType type;
	OrderStatus status;
	double price;
	double origQuantity;
	double executedQuantity;
	time_t timestamp;
	time_t updateTime;
};

// Abstract base class for all exchanges
class ExchangeAPI {
public:
	virtual ~ExchangeAPI() = default;

	// Initialize connection
	virtual bool init(const std::string& apiKey, const std::string& apiSecret) = 0;

	// Connection management
	virtual bool testConnection() = 0;
	virtual bool ping() = 0;
	virtual time_t getServerTime() = 0;

	// Market data (public)
	virtual Ticker getTicker(const std::string& symbol) = 0;
	virtual std::vector<Candle> getCandles(const std::string& symbol,
	                                        const std::string& timeframe,
	                                        time_t startTime,
	                                        time_t endTime,
	                                        int limit = 500) = 0;
	virtual OrderBook getOrderBook(const std::string& symbol, int limit = 100) = 0;
	virtual std::vector<Trade> getRecentTrades(const std::string& symbol, int limit = 100) = 0;

	// Account data (private - requires authentication)
	virtual std::vector<Balance> getBalances() = 0;
	virtual Balance getBalance(const std::string& asset) = 0;

	// Trading (private - requires authentication)
	virtual Order createOrder(const std::string& symbol,
	                          OrderSide side,
	                          OrderType type,
	                          double quantity,
	                          double price = 0.0) = 0;
	virtual bool cancelOrder(const std::string& symbol, const std::string& orderId) = 0;
	virtual Order getOrder(const std::string& symbol, const std::string& orderId) = 0;
	virtual std::vector<Order> getOpenOrders(const std::string& symbol = "") = 0;
	virtual std::vector<Order> getAllOrders(const std::string& symbol, int limit = 100) = 0;

	// Utility
	virtual std::string getName() const = 0;
	virtual std::string getExchangeInfo() = 0;

protected:
	std::string apiKey;
	std::string apiSecret;
	std::string baseUrl;
	bool initialized;
};

} // namespace Emiglio

#endif // EXCHANGEAPI_H
