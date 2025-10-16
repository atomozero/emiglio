#include "PaperPortfolio.h"
#include "../utils/Logger.h"

#include <sstream>
#include <iomanip>
#include <cmath>

namespace Emiglio {
namespace Paper {

PaperPortfolio::PaperPortfolio(double initialBalance)
	: initialBalance(initialBalance)
	, balance(initialBalance)
	, feeRate(0.001)           // 0.1% fee
	, defaultSlippage(0.0005)  // 0.05% slippage
{
	LOG_INFO("PaperPortfolio created with balance: $" + std::to_string(initialBalance));
}

PaperPortfolio::~PaperPortfolio() {
}

bool PaperPortfolio::buy(const std::string& symbol, double quantity, double price, double slippage) {
	// Apply slippage (buy at slightly higher price)
	double executionPrice = price * (1.0 + slippage);
	double cost = quantity * executionPrice;
	double fee = calculateFee(quantity, executionPrice);
	double totalCost = cost + fee;

	// Check if we have enough balance
	if (totalCost > balance) {
		LOG_ERROR("Insufficient balance for BUY order: " + symbol);
		return false;
	}

	// Deduct from balance
	balance -= totalCost;

	// Update or create position
	auto it = positions.find(symbol);
	if (it != positions.end()) {
		// Average up the position
		PaperPosition& pos = it->second;
		double totalQuantity = pos.quantity + quantity;
		pos.entryPrice = ((pos.entryPrice * pos.quantity) + (executionPrice * quantity)) / totalQuantity;
		pos.quantity = totalQuantity;
		pos.currentPrice = price;
	} else {
		// Create new position
		PaperPosition pos;
		pos.symbol = symbol;
		pos.side = "LONG";
		pos.entryPrice = executionPrice;
		pos.currentPrice = price;
		pos.quantity = quantity;
		pos.openTime = std::time(nullptr);
		pos.unrealizedPnL = 0.0;
		pos.unrealizedPnLPercent = 0.0;
		positions[symbol] = pos;
	}

	// Record trade
	PaperTrade trade;
	trade.symbol = symbol;
	trade.side = "BUY";
	trade.price = executionPrice;
	trade.quantity = quantity;
	trade.fee = fee;
	trade.timestamp = std::time(nullptr);
	trade.orderId = generateOrderId();
	tradeHistory.push_back(trade);

	LOG_INFO("BUY executed: " + symbol + " qty=" + std::to_string(quantity) +
	         " price=$" + std::to_string(executionPrice));

	return true;
}

bool PaperPortfolio::sell(const std::string& symbol, double quantity, double price, double slippage) {
	// Check if we have the position
	auto it = positions.find(symbol);
	if (it == positions.end()) {
		LOG_ERROR("No position to sell: " + symbol);
		return false;
	}

	PaperPosition& pos = it->second;

	// Check if we have enough quantity
	if (quantity > pos.quantity) {
		LOG_ERROR("Insufficient quantity to sell: " + symbol);
		return false;
	}

	// Apply slippage (sell at slightly lower price)
	double executionPrice = price * (1.0 - slippage);
	double proceeds = quantity * executionPrice;
	double fee = calculateFee(quantity, executionPrice);
	double netProceeds = proceeds - fee;

	// Add to balance
	balance += netProceeds;

	// Calculate realized P&L
	double realizedPnL = (executionPrice - pos.entryPrice) * quantity - fee;

	// Update position
	pos.quantity -= quantity;

	// Remove position if fully closed
	if (pos.quantity < 0.0001) {  // Close to zero
		positions.erase(it);
	}

	// Record trade
	PaperTrade trade;
	trade.symbol = symbol;
	trade.side = "SELL";
	trade.price = executionPrice;
	trade.quantity = quantity;
	trade.fee = fee;
	trade.timestamp = std::time(nullptr);
	trade.orderId = generateOrderId();
	tradeHistory.push_back(trade);

	LOG_INFO("SELL executed: " + symbol + " qty=" + std::to_string(quantity) +
	         " price=$" + std::to_string(executionPrice) +
	         " PnL=$" + std::to_string(realizedPnL));

	return true;
}

PaperPosition* PaperPortfolio::getPosition(const std::string& symbol) {
	auto it = positions.find(symbol);
	if (it != positions.end()) {
		return &it->second;
	}
	return nullptr;
}

std::vector<PaperPosition> PaperPortfolio::getAllPositions() const {
	std::vector<PaperPosition> result;
	for (const auto& pair : positions) {
		result.push_back(pair.second);
	}
	return result;
}

void PaperPortfolio::updatePrice(const std::string& symbol, double newPrice) {
	auto it = positions.find(symbol);
	if (it != positions.end()) {
		PaperPosition& pos = it->second;
		pos.currentPrice = newPrice;

		// Calculate unrealized P&L
		if (pos.side == "LONG") {
			pos.unrealizedPnL = (newPrice - pos.entryPrice) * pos.quantity;
		} else {  // SHORT
			pos.unrealizedPnL = (pos.entryPrice - newPrice) * pos.quantity;
		}

		pos.unrealizedPnLPercent = (pos.unrealizedPnL / (pos.entryPrice * pos.quantity)) * 100.0;
	}
}

void PaperPortfolio::closePosition(const std::string& symbol, double price) {
	auto it = positions.find(symbol);
	if (it != positions.end()) {
		PaperPosition& pos = it->second;
		sell(symbol, pos.quantity, price);
	}
}

double PaperPortfolio::getEquity() const {
	double equity = balance;

	// Add unrealized P&L from all positions
	for (const auto& pair : positions) {
		const PaperPosition& pos = pair.second;
		equity += pos.unrealizedPnL;
	}

	return equity;
}

double PaperPortfolio::getTotalPnL() const {
	return getEquity() - initialBalance;
}

double PaperPortfolio::getTotalPnLPercent() const {
	return (getTotalPnL() / initialBalance) * 100.0;
}

double PaperPortfolio::getUsedMargin() const {
	double used = 0.0;

	for (const auto& pair : positions) {
		const PaperPosition& pos = pair.second;
		used += pos.entryPrice * pos.quantity;
	}

	return used;
}

double PaperPortfolio::getAvailableMargin() const {
	return balance;
}

void PaperPortfolio::reset(double newBalance) {
	initialBalance = newBalance;
	balance = newBalance;
	positions.clear();
	tradeHistory.clear();

	LOG_INFO("PaperPortfolio reset with balance: $" + std::to_string(newBalance));
}

double PaperPortfolio::calculateFee(double quantity, double price) const {
	return quantity * price * feeRate;
}

std::string PaperPortfolio::generateOrderId() {
	static int orderCounter = 1;
	std::ostringstream oss;
	oss << "PAPER" << std::setw(8) << std::setfill('0') << orderCounter++;
	return oss.str();
}

} // namespace Paper
} // namespace Emiglio
