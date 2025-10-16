#ifndef DATASTORAGE_H
#define DATASTORAGE_H

#include <string>
#include <vector>
#include <memory>
#include <ctime>

namespace Emiglio {

// OHLCV candle data structure
struct Candle {
	std::string exchange;
	std::string symbol;
	std::string timeframe;
	time_t timestamp;
	double open;
	double high;
	double low;
	double close;
	double volume;

	Candle()
		: timestamp(0), open(0), high(0), low(0), close(0), volume(0) {}
};

// Trade record structure (used for both backtesting and market trades)
struct Trade {
	int64_t id;                   // Database ID (backtesting) or market trade ID
	std::string strategyName;     // Used for backtesting
	std::string backtestId;       // Used for backtesting
	time_t timestamp;
	std::string symbol;
	std::string side;             // "buy" or "sell"
	double price;
	double quantity;
	double commission;            // Used for backtesting
	double pnl;                   // Used for backtesting
	double portfolioValue;        // Used for backtesting
	bool isBuyerMaker;            // Used for market trades (true if maker was buyer)

	Trade()
		: id(0), timestamp(0), price(0), quantity(0), commission(0), pnl(0), portfolioValue(0), isBuyerMaker(false) {}
};

// Backtest result structure
struct BacktestResult {
	std::string id;
	std::string recipeName;
	time_t startDate;
	time_t endDate;
	double initialCapital;
	double finalCapital;
	double totalReturn;
	double sharpeRatio;
	double maxDrawdown;
	double winRate;
	int totalTrades;
	time_t createdAt;
	std::string config; // JSON config

	BacktestResult()
		: startDate(0), endDate(0), initialCapital(0), finalCapital(0),
		  totalReturn(0), sharpeRatio(0), maxDrawdown(0), winRate(0),
		  totalTrades(0), createdAt(0) {}
};

// DataStorage class for SQLite operations
class DataStorage {
public:
	DataStorage();
	~DataStorage();

	// Initialize database
	bool init(const std::string& dbPath);

	// Close database
	void close();

	// Candle operations
	bool insertCandle(const Candle& candle);
	bool insertCandles(const std::vector<Candle>& candles);
	std::vector<Candle> getCandles(const std::string& exchange,
	                                const std::string& symbol,
	                                const std::string& timeframe,
	                                time_t startTime,
	                                time_t endTime);
	int getCandleCount(const std::string& exchange,
	                   const std::string& symbol,
	                   const std::string& timeframe);

	// Trade operations
	bool insertTrade(const Trade& trade);
	std::vector<Trade> getTrades(const std::string& strategyName,
	                              time_t startTime,
	                              time_t endTime);
	std::vector<Trade> getTradesByBacktest(const std::string& backtestId);

	// Backtest result operations
	bool insertBacktestResult(const BacktestResult& result);
	BacktestResult getBacktestResult(const std::string& id);
	std::vector<BacktestResult> getAllBacktestResults();

	// Utility operations
	bool clearCandles(const std::string& exchange,
	                  const std::string& symbol,
	                  const std::string& timeframe);
	bool vacuum(); // Optimize database

private:
	class Impl;
	std::unique_ptr<Impl> pImpl;
};

} // namespace Emiglio

#endif // DATASTORAGE_H
