#ifndef BINANCEAPI_H
#define BINANCEAPI_H

#include "ExchangeAPI.h"
#include <memory>

namespace Emiglio {

// Binance API implementation using Haiku native BHttpRequest
class BinanceAPI : public ExchangeAPI {
public:
	BinanceAPI();
	~BinanceAPI() override;

	// Initialize connection
	bool init(const std::string& apiKey, const std::string& apiSecret) override;

	// Connection management
	bool testConnection() override;
	bool ping() override;
	time_t getServerTime() override;

	// Market data (public)
	Ticker getTicker(const std::string& symbol) override;

	// Batch operations (more efficient)
	std::vector<Ticker> getAllTickers();  // Get all symbols at once (1 request vs N)

	std::vector<Candle> getCandles(const std::string& symbol,
	                                const std::string& timeframe,
	                                time_t startTime,
	                                time_t endTime,
	                                int limit = 500) override;
	OrderBook getOrderBook(const std::string& symbol, int limit = 100) override;
	std::vector<Trade> getRecentTrades(const std::string& symbol, int limit = 100) override;

	// Account data (private - requires authentication)
	std::vector<Balance> getBalances() override;
	Balance getBalance(const std::string& asset) override;

	// Trading (private - requires authentication)
	Order createOrder(const std::string& symbol,
	                  OrderSide side,
	                  OrderType type,
	                  double quantity,
	                  double price = 0.0) override;
	bool cancelOrder(const std::string& symbol, const std::string& orderId) override;
	Order getOrder(const std::string& symbol, const std::string& orderId) override;
	std::vector<Order> getOpenOrders(const std::string& symbol = "") override;
	std::vector<Order> getAllOrders(const std::string& symbol, int limit = 100) override;

	// Utility
	std::string getName() const override;
	std::string getExchangeInfo() override;
	std::vector<std::string> getAllSymbols();  // Get list of all available trading pairs

private:
	class Impl;
	std::unique_ptr<Impl> pImpl;
};

} // namespace Emiglio

#endif // BINANCEAPI_H
