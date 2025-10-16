#ifndef EMIGLIO_BACKTEST_TRADE_H
#define EMIGLIO_BACKTEST_TRADE_H

#include <string>
#include <ctime>

namespace Emiglio {
namespace Backtest {

// Trade types for backtesting
enum class TradeType {
	LONG,   // Buy/Long position
	SHORT   // Sell/Short position
};

// Trade status for backtesting
enum class TradeStatus {
	OPEN,      // Position is open
	CLOSED,    // Position closed normally
	CANCELLED  // Position cancelled (e.g., insufficient margin)
};

// Individual backtest trade record
struct Trade {
	// Identification
	std::string id;              // Unique trade ID
	std::string symbol;          // Trading pair (e.g., "BTCUSDT")

	// Trade type and status
	TradeType type;              // LONG or SHORT
	TradeStatus status;          // OPEN, CLOSED, CANCELLED

	// Prices and quantity
	double entryPrice;           // Entry price
	double exitPrice;            // Exit price (0.0 if still open)
	double quantity;             // Position size (in base currency)

	// Timing
	time_t entryTime;            // Entry timestamp
	time_t exitTime;             // Exit timestamp (0 if still open)

	// Costs
	double commission;           // Total commission paid (entry + exit)
	double slippage;             // Total slippage cost

	// P&L
	double pnl;                  // Realized profit/loss (in quote currency)
	double pnlPercent;           // P&L as percentage of entry value

	// Reasons
	std::string entryReason;     // Why entered (e.g., "RSI < 30")
	std::string exitReason;      // Why exited (e.g., "Stop-Loss", "Signal", "Take-Profit")

	// Stop-loss and take-profit levels
	double stopLossPrice;        // Stop-loss price (0 = no SL)
	double takeProfitPrice;      // Take-profit price (0 = no TP)

	// Constructor
	Trade()
		: type(TradeType::LONG)
		, status(TradeStatus::OPEN)
		, entryPrice(0.0)
		, exitPrice(0.0)
		, quantity(0.0)
		, entryTime(0)
		, exitTime(0)
		, commission(0.0)
		, slippage(0.0)
		, pnl(0.0)
		, pnlPercent(0.0)
		, stopLossPrice(0.0)
		, takeProfitPrice(0.0)
	{}
};

} // namespace Backtest
} // namespace Emiglio

#endif // EMIGLIO_BACKTEST_TRADE_H
