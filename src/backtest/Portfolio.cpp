#include "Portfolio.h"
#include "../utils/Logger.h"
#include <sstream>
#include <algorithm>

namespace Emiglio {
namespace Backtest {

Portfolio::Portfolio(double initialCapital)
	: initialCapital(initialCapital)
	, cash(initialCapital)
	, nextTradeId(1)
{
	LOG_INFO("Portfolio initialized with capital: $" + std::to_string(initialCapital));
}

Portfolio::~Portfolio() {
}

std::string Portfolio::generateTradeId() {
	std::ostringstream oss;
	oss << "T" << nextTradeId++;
	return oss.str();
}

bool Portfolio::openPosition(Trade& trade, double commission, double slippage) {
	// Calculate total cost
	double positionCost = trade.entryPrice * trade.quantity;
	double totalCost = positionCost + commission + slippage;

	// Check if we have enough cash
	if (totalCost > cash) {
		LOG_WARNING("Insufficient cash to open position: need $" +
		            std::to_string(totalCost) + ", have $" + std::to_string(cash));
		return false;
	}

	// Generate trade ID if not set
	if (trade.id.empty()) {
		trade.id = generateTradeId();
	}

	// Set trade details
	trade.status = TradeStatus::OPEN;
	trade.commission = commission;  // Entry commission
	trade.slippage = slippage;      // Entry slippage

	// Deduct cash
	cash -= totalCost;

	// Add to open trades
	openTrades.push_back(trade);

	LOG_INFO("Opened " + std::string(trade.type == TradeType::LONG ? "LONG" : "SHORT") +
	         " position: " + trade.id + " @ $" + std::to_string(trade.entryPrice) +
	         " qty: " + std::to_string(trade.quantity));

	return true;
}

bool Portfolio::closePosition(const std::string& tradeId, double exitPrice,
                              const std::string& reason, double commission, double slippage) {
	// Find the trade in open trades
	auto it = std::find_if(openTrades.begin(), openTrades.end(),
	                       [&tradeId](const Trade& t) { return t.id == tradeId; });

	if (it == openTrades.end()) {
		LOG_WARNING("Trade not found: " + tradeId);
		return false;
	}

	Trade& trade = *it;

	// Set exit details
	trade.exitPrice = exitPrice;
	trade.exitReason = reason;
	trade.status = TradeStatus::CLOSED;
	trade.exitTime = std::time(nullptr);  // Should be set by simulator

	// Add exit commission and slippage
	trade.commission += commission;
	trade.slippage += slippage;

	// Calculate P&L
	double positionValue = exitPrice * trade.quantity;
	double entryValue = trade.entryPrice * trade.quantity;

	if (trade.type == TradeType::LONG) {
		// LONG: profit = (exit - entry) * quantity - costs
		trade.pnl = (exitPrice - trade.entryPrice) * trade.quantity - trade.commission - trade.slippage;
	} else {
		// SHORT: profit = (entry - exit) * quantity - costs
		trade.pnl = (trade.entryPrice - exitPrice) * trade.quantity - trade.commission - trade.slippage;
	}

	trade.pnlPercent = (trade.pnl / entryValue) * 100.0;

	// Add cash from closing position
	cash += positionValue - commission - slippage;

	// Move trade to closed trades
	closedTrades.push_back(trade);
	openTrades.erase(it);

	LOG_INFO("Closed position: " + tradeId + " @ $" + std::to_string(exitPrice) +
	         " | P&L: $" + std::to_string(trade.pnl) + " (" + std::to_string(trade.pnlPercent) + "%)" +
	         " | Reason: " + reason);

	return true;
}

double Portfolio::getEquity(double currentPrice) const {
	double equity = cash;

	// Add value of open positions
	for (const auto& trade : openTrades) {
		double positionValue = 0.0;

		if (currentPrice > 0.0) {
			// Use provided current price
			positionValue = currentPrice * trade.quantity;
		} else {
			// Use entry price (unrealized P&L = 0)
			positionValue = trade.entryPrice * trade.quantity;
		}

		equity += positionValue;
	}

	return equity;
}

double Portfolio::getCash() const {
	return cash;
}

double Portfolio::getPositionValue(double currentPrice) const {
	double totalValue = 0.0;

	for (const auto& trade : openTrades) {
		double price = (currentPrice > 0.0) ? currentPrice : trade.entryPrice;
		totalValue += price * trade.quantity;
	}

	return totalValue;
}

std::vector<Trade> Portfolio::getOpenTrades() const {
	return openTrades;
}

std::vector<Trade> Portfolio::getClosedTrades() const {
	return closedTrades;
}

// Fixed: Changed from pointer return (use-after-free risk) to index return
// Usage: int idx = portfolio.getOpenTradeIndex("T1"); if (idx >= 0) { Trade& t = openTrades[idx]; }
int Portfolio::getOpenTradeIndex(const std::string& tradeId) const {
	auto it = std::find_if(openTrades.begin(), openTrades.end(),
	                       [&tradeId](const Trade& t) { return t.id == tradeId; });

	if (it != openTrades.end()) {
		return std::distance(openTrades.begin(), it);
	}

	return -1;  // Not found
}

bool Portfolio::canOpenPosition(double requiredCash) const {
	return cash >= requiredCash;
}

double Portfolio::getMaxPositionSize() const {
	// Return available cash (can be modified for margin/leverage)
	return cash;
}

int Portfolio::getTotalTrades() const {
	return openTrades.size() + closedTrades.size();
}

int Portfolio::getOpenTradesCount() const {
	return openTrades.size();
}

int Portfolio::getClosedTradesCount() const {
	return closedTrades.size();
}

void Portfolio::reset(double newInitialCapital) {
	initialCapital = newInitialCapital;
	cash = newInitialCapital;
	openTrades.clear();
	closedTrades.clear();
	nextTradeId = 1;

	LOG_INFO("Portfolio reset with capital: $" + std::to_string(newInitialCapital));
}

} // namespace Backtest
} // namespace Emiglio
