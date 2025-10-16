#ifndef EMIGLIO_BACKTEST_RESULT_H
#define EMIGLIO_BACKTEST_RESULT_H

#include "Trade.h"
#include <string>
#include <vector>
#include <ctime>

namespace Emiglio {
namespace Backtest {

// Single point in equity curve
struct EquityPoint {
	time_t timestamp;            // Timestamp
	double equity;               // Total equity (cash + position value)
	double cash;                 // Available cash
	double positionValue;        // Value of open positions

	EquityPoint()
		: timestamp(0)
		, equity(0.0)
		, cash(0.0)
		, positionValue(0.0)
	{}

	EquityPoint(time_t t, double e, double c, double pv)
		: timestamp(t)
		, equity(e)
		, cash(c)
		, positionValue(pv)
	{}
};

// Complete backtest result
struct BacktestResult {
	// Metadata
	std::string recipeName;      // Strategy name
	std::string symbol;          // Trading symbol
	time_t startTime;            // Backtest start
	time_t endTime;              // Backtest end
	int totalCandles;            // Number of candles processed

	// Initial state
	double initialCapital;       // Starting capital
	double finalEquity;          // Final equity
	double peakEquity;           // Highest equity reached

	// Trades
	std::vector<Trade> trades;   // All trades (open + closed)
	int totalTrades;             // Total number of trades
	int winningTrades;           // Number of winning trades
	int losingTrades;            // Number of losing trades

	// Equity curve
	std::vector<EquityPoint> equityCurve;

	// Costs
	double totalCommission;      // Total commission paid
	double totalSlippage;        // Total slippage cost

	// Performance metrics (calculated by PerformanceAnalyzer)
	double totalReturn;          // Total return %
	double totalReturnPercent;   // Same as totalReturn
	double annualizedReturn;     // Annualized return %
	double sharpeRatio;          // Sharpe ratio
	double sortinoRatio;         // Sortino ratio
	double maxDrawdown;          // Max drawdown %
	double maxDrawdownPercent;   // Same as maxDrawdown
	double winRate;              // Win rate %
	double profitFactor;         // Total wins / Total losses
	double expectancy;           // Average P&L per trade
	double averageWin;           // Average winning trade
	double averageLoss;          // Average losing trade

	// Constructor
	BacktestResult()
		: startTime(0)
		, endTime(0)
		, totalCandles(0)
		, initialCapital(0.0)
		, finalEquity(0.0)
		, peakEquity(0.0)
		, totalTrades(0)
		, winningTrades(0)
		, losingTrades(0)
		, totalCommission(0.0)
		, totalSlippage(0.0)
		, totalReturn(0.0)
		, totalReturnPercent(0.0)
		, annualizedReturn(0.0)
		, sharpeRatio(0.0)
		, sortinoRatio(0.0)
		, maxDrawdown(0.0)
		, maxDrawdownPercent(0.0)
		, winRate(0.0)
		, profitFactor(0.0)
		, expectancy(0.0)
		, averageWin(0.0)
		, averageLoss(0.0)
	{}
};

} // namespace Backtest
} // namespace Emiglio

#endif // EMIGLIO_BACKTEST_RESULT_H
