#ifndef BINANCE_WEBSOCKET_H
#define BINANCE_WEBSOCKET_H

#include <string>
#include <functional>
#include <memory>
#include <vector>

namespace Emiglio {

// WebSocket message types
enum class WSMessageType {
	TICKER,       // 24hr ticker statistics
	TRADE,        // Individual trade
	KLINE,        // Candlestick data
	DEPTH,        // Order book updates
	AGG_TRADE,    // Aggregated trades
	ERROR
};

// Ticker update (24hr statistics)
struct TickerUpdate {
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

// Trade update
struct TradeUpdate {
	std::string symbol;
	long tradeId;
	double price;
	double quantity;
	time_t timestamp;
	bool isBuyerMaker;
};

// Kline (candlestick) update
struct KlineUpdate {
	std::string symbol;
	std::string interval;
	time_t openTime;
	time_t closeTime;
	double open;
	double high;
	double low;
	double close;
	double volume;
	bool isClosed;  // true if kline is closed
};

// Callback types
using TickerCallback = std::function<void(const TickerUpdate&)>;
using TradeCallback = std::function<void(const TradeUpdate&)>;
using KlineCallback = std::function<void(const KlineUpdate&)>;
using ErrorCallback = std::function<void(const std::string&)>;

// Binance WebSocket client
class BinanceWebSocket {
public:
	BinanceWebSocket();
	~BinanceWebSocket();

	// Initialize connection
	bool connect();
	void disconnect();
	bool isConnected() const;

	// Subscribe to streams
	bool subscribeTicker(const std::string& symbol, TickerCallback callback);
	bool subscribeTrades(const std::string& symbol, TradeCallback callback);
	bool subscribeKlines(const std::string& symbol, const std::string& interval, KlineCallback callback);

	// Unsubscribe from streams
	void unsubscribeTicker(const std::string& symbol);
	void unsubscribeTrades(const std::string& symbol);
	void unsubscribeKlines(const std::string& symbol, const std::string& interval);

	// Set error callback
	void setErrorCallback(ErrorCallback callback);

	// Process incoming messages (call from main thread periodically)
	void processMessages();

private:
	class Impl;
	std::unique_ptr<Impl> pImpl;
};

} // namespace Emiglio

#endif // BINANCE_WEBSOCKET_H
