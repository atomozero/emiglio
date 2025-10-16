#ifndef EMIGLIO_BACKTEST_SIMULATOR_H
#define EMIGLIO_BACKTEST_SIMULATOR_H

#include "Portfolio.h"
#include "BacktestResult.h"
#include "../strategy/SignalGenerator.h"
#include "../strategy/RecipeLoader.h"
#include "../data/DataStorage.h"

namespace Emiglio {
namespace Backtest {

// Backtest configuration
struct BacktestConfig {
	double initialCapital;          // Starting capital
	double commissionPercent;       // Commission per trade (e.g., 0.001 = 0.1%)
	double slippagePercent;         // Slippage per trade (e.g., 0.0005 = 0.05%)
	bool useStopLoss;               // Enable stop-loss from recipe
	bool useTakeProfit;             // Enable take-profit from recipe
	int maxOpenPositions;           // Max concurrent positions (1 = no pyramiding)

	BacktestConfig()
		: initialCapital(1000.0)
		, commissionPercent(0.001)     // 0.1% default
		, slippagePercent(0.0005)      // 0.05% default
		, useStopLoss(true)
		, useTakeProfit(true)
		, maxOpenPositions(1)
	{}
};

// Main backtest simulator
class BacktestSimulator {
public:
	BacktestSimulator(const Recipe& recipe, const BacktestConfig& config);
	~BacktestSimulator();

	// Run backtest on historical data
	BacktestResult run(const std::vector<Candle>& candles);

	// Configuration setters
	void setCommission(double percent);
	void setSlippage(double percent);
	void setInitialCapital(double capital);
	void setMaxOpenPositions(int max);

	// Get last error
	std::string getLastError() const;

private:
	Recipe recipe;
	BacktestConfig config;
	SignalGenerator signalGen;
	Portfolio portfolio;
	BacktestResult result;
	std::string lastError;

	// Processing
	void processCandle(const Candle& candle, size_t index, const std::vector<Candle>& allCandles);
	void checkStopLoss(const Candle& candle);
	void checkTakeProfit(const Candle& candle);
	void updateEquityCurve(const Candle& candle);

	// Helpers
	double calculateCommission(double orderValue) const;
	double calculateSlippage(double price, bool isBuy) const;
	double calculatePositionSize(double price) const;
};

} // namespace Backtest
} // namespace Emiglio

#endif // EMIGLIO_BACKTEST_SIMULATOR_H
