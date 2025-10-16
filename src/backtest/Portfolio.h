#ifndef EMIGLIO_BACKTEST_PORTFOLIO_H
#define EMIGLIO_BACKTEST_PORTFOLIO_H

#include "Trade.h"
#include <vector>
#include <string>

namespace Emiglio {
namespace Backtest {

// Portfolio manager for backtest simulation
class Portfolio {
public:
	Portfolio(double initialCapital);
	~Portfolio();

	// Position management
	bool openPosition(Trade& trade, double commission, double slippage);
	bool closePosition(const std::string& tradeId, double exitPrice,
	                   const std::string& reason, double commission, double slippage);

	// Queries
	double getEquity(double currentPrice = 0.0) const;  // Current equity (cash + position value)
	double getCash() const;                              // Available cash
	double getPositionValue(double currentPrice) const;  // Value of open positions
	std::vector<Trade> getOpenTrades() const;
	std::vector<Trade> getClosedTrades() const;
	// Fixed: Changed from pointer return (use-after-free risk) to index return
	int getOpenTradeIndex(const std::string& tradeId) const;  // Returns -1 if not found

	// Risk management
	bool canOpenPosition(double requiredCash) const;
	double getMaxPositionSize() const;

	// Statistics
	int getTotalTrades() const;
	int getOpenTradesCount() const;
	int getClosedTradesCount() const;

	// Reset
	void reset(double newInitialCapital);

private:
	double initialCapital;
	double cash;
	std::vector<Trade> openTrades;
	std::vector<Trade> closedTrades;
	int nextTradeId;

	// Helper
	std::string generateTradeId();
};

} // namespace Backtest
} // namespace Emiglio

#endif // EMIGLIO_BACKTEST_PORTFOLIO_H
