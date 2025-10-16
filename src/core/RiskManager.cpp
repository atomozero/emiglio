#include "RiskManager.h"
#include "Logger.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <cstdio>
#include <cstdarg>

using Emiglio::Logger;

namespace Emiglio {
namespace Core {

// Helper functions for formatted logging
namespace {
	void LogInfo(const char* format, ...) {
		char buffer[1024];
		va_list args;
		va_start(args, format);
		vsnprintf(buffer, sizeof(buffer), format, args);
		va_end(args);
		Logger::getInstance().info(buffer);
	}

	void LogWarning(const char* format, ...) {
		char buffer[1024];
		va_list args;
		va_start(args, format);
		vsnprintf(buffer, sizeof(buffer), format, args);
		va_end(args);
		Logger::getInstance().warning(buffer);
	}

	void LogDebug(const char* format, ...) {
		char buffer[1024];
		va_list args;
		va_start(args, format);
		vsnprintf(buffer, sizeof(buffer), format, args);
		va_end(args);
		Logger::getInstance().debug(buffer);
	}
}

RiskManager::RiskManager()
	: totalCapital(0.0)
	, tradingEnabled(true)
	, emergencyStopLoss(false)
	, lastResetTime(0)
{
	// Initialize with safe defaults
	config.stopLossPercent = 2.0;
	config.takeProfitPercent = 5.0;
	config.maxDailyLossPercent = 5.0;
	config.maxOpenPositions = 1;

	resetDailyStats();
}

RiskManager::~RiskManager()
{
}

// Configuration
void RiskManager::configure(const RiskConfig& newConfig)
{
	config = newConfig;
	LogInfo("RiskManager configured: SL=%.2f%%, TP=%.2f%%, MaxDailyLoss=%.2f%%, MaxPos=%d",
		config.stopLossPercent, config.takeProfitPercent,
		config.maxDailyLossPercent, config.maxOpenPositions);
}

void RiskManager::setCapital(double capital)
{
	totalCapital = capital;
	dailyStats.startingCapital = capital;
	dailyStats.currentCapital = capital;
	LogInfo("RiskManager capital set to: %.2f", capital);
}

// Position validation
bool RiskManager::canOpenPosition(const std::string& symbol, double price, double quantity) const
{
	if (!tradingEnabled) {
		LogWarning("Cannot open position: trading disabled");
		return false;
	}

	if (isMaxPositionsReached()) {
		LogWarning("Cannot open position: max positions reached (%d)", config.maxOpenPositions);
		return false;
	}

	double positionValue = price * quantity;
	if (!isWithinRiskLimits(positionValue)) {
		LogWarning("Cannot open position: exceeds risk limits (value=%.2f)", positionValue);
		return false;
	}

	if (!isWithinDailyLossLimit()) {
		LogWarning("Cannot open position: daily loss limit reached");
		return false;
	}

	double availableCap = getAvailableCapital();
	if (positionValue > availableCap) {
		LogWarning("Cannot open position: insufficient capital (need=%.2f, available=%.2f)",
			positionValue, availableCap);
		return false;
	}

	return true;
}

bool RiskManager::isWithinRiskLimits(double positionValue) const
{
	double maxPosValue = getMaxPositionValue();
	return positionValue <= maxPosValue;
}

bool RiskManager::isMaxPositionsReached() const
{
	return getOpenPositionsCount() >= config.maxOpenPositions;
}

bool RiskManager::isWithinDailyLossLimit() const
{
	const_cast<RiskManager*>(this)->autoResetIfNewDay();

	if (dailyStats.realizedPnL >= 0) {
		return true;  // In profit, no limit
	}

	double lossPercent = (std::abs(dailyStats.realizedPnL) / dailyStats.startingCapital) * 100.0;
	return lossPercent < config.maxDailyLossPercent;
}

// Position sizing
double RiskManager::calculatePositionSize(double currentPrice, double totalCap) const
{
	// Use recipe's position size percent
	// This would typically come from Recipe, but we can access via config if needed
	// For now, calculate max position size based on risk limits
	double maxPosValue = getMaxPositionValue();
	return std::min(maxPosValue, totalCap * 0.1);  // Default 10% if not specified
}

double RiskManager::calculateQuantity(double positionValue, double currentPrice) const
{
	if (currentPrice <= 0.0) {
		return 0.0;
	}
	return positionValue / currentPrice;
}

// Stop-loss and take-profit calculation
double RiskManager::calculateStopLossPrice(double entryPrice, bool isLong) const
{
	if (config.stopLossPercent <= 0.0) {
		return 0.0;  // No stop-loss
	}

	double slDistance = entryPrice * (config.stopLossPercent / 100.0);

	if (isLong) {
		return entryPrice - slDistance;
	} else {
		return entryPrice + slDistance;
	}
}

double RiskManager::calculateTakeProfitPrice(double entryPrice, bool isLong) const
{
	if (config.takeProfitPercent <= 0.0) {
		return 0.0;  // No take-profit
	}

	double tpDistance = entryPrice * (config.takeProfitPercent / 100.0);

	if (isLong) {
		return entryPrice + tpDistance;
	} else {
		return entryPrice - tpDistance;
	}
}

// Position tracking
void RiskManager::addPosition(const Position& position)
{
	openPositions.push_back(position);
	positionIndex[position.id] = openPositions.size() - 1;

	LogInfo("Position added: %s, symbol=%s, qty=%.8f, entry=%.2f",
		position.id.c_str(), position.symbol.c_str(),
		position.quantity, position.entryPrice);
}

void RiskManager::removePosition(const std::string& positionId)
{
	auto it = positionIndex.find(positionId);
	if (it == positionIndex.end()) {
		LogWarning("Cannot remove position: not found (%s)", positionId.c_str());
		return;
	}

	int index = it->second;

	// Remove from vector (swap with last element for O(1) removal)
	if (index < (int)openPositions.size() - 1) {
		std::swap(openPositions[index], openPositions.back());
		// Update index for swapped element
		positionIndex[openPositions[index].id] = index;
	}

	openPositions.pop_back();
	positionIndex.erase(positionId);

	LogInfo("Position removed: %s", positionId.c_str());
}

void RiskManager::updatePosition(const std::string& positionId, double currentPrice)
{
	Position* pos = getPosition(positionId);
	if (!pos) {
		LogWarning("Cannot update position: not found (%s)", positionId.c_str());
		return;
	}

	updatePositionPnL(*pos, currentPrice);

	// Update highest price for trailing stop
	if (pos->trailingStopEnabled && currentPrice > pos->highestPrice) {
		pos->highestPrice = currentPrice;

		// Recalculate trailing stop-loss
		double trailingDistance = pos->highestPrice * (pos->trailingStopPercent / 100.0);
		double newStopLoss = pos->highestPrice - trailingDistance;

		if (newStopLoss > pos->stopLossPrice) {
			pos->stopLossPrice = newStopLoss;
			LogDebug("Trailing stop updated for %s: new SL=%.2f (highest=%.2f)",
				positionId.c_str(), newStopLoss, pos->highestPrice);
		}
	}
}

std::vector<Position> RiskManager::getOpenPositions() const
{
	return openPositions;
}

Position* RiskManager::getPosition(const std::string& positionId)
{
	auto it = positionIndex.find(positionId);
	if (it == positionIndex.end()) {
		return nullptr;
	}
	return &openPositions[it->second];
}

int RiskManager::getOpenPositionsCount() const
{
	return openPositions.size();
}

// Stop-loss/take-profit monitoring
RiskManager::TriggerResult RiskManager::checkStopLoss(const Position& position, double currentPrice) const
{
	TriggerResult result;
	result.triggered = false;

	if (position.stopLossPrice <= 0.0) {
		return result;  // No stop-loss set
	}

	// For long positions: trigger if price drops below SL
	// For short positions: trigger if price rises above SL
	bool isLong = true;  // Assume long for now (would check position type in real implementation)

	if (isLong && currentPrice <= position.stopLossPrice) {
		result.triggered = true;
		result.reason = "stop-loss";
		result.exitPrice = position.stopLossPrice;
		LogWarning("Stop-loss triggered for %s: price=%.2f, SL=%.2f",
			position.id.c_str(), currentPrice, position.stopLossPrice);
	}

	return result;
}

RiskManager::TriggerResult RiskManager::checkTakeProfit(const Position& position, double currentPrice) const
{
	TriggerResult result;
	result.triggered = false;

	if (position.takeProfitPrice <= 0.0) {
		return result;  // No take-profit set
	}

	// For long positions: trigger if price rises above TP
	bool isLong = true;  // Assume long for now

	if (isLong && currentPrice >= position.takeProfitPrice) {
		result.triggered = true;
		result.reason = "take-profit";
		result.exitPrice = position.takeProfitPrice;
		LogInfo("Take-profit triggered for %s: price=%.2f, TP=%.2f",
			position.id.c_str(), currentPrice, position.takeProfitPrice);
	}

	return result;
}

RiskManager::TriggerResult RiskManager::checkTrailingStop(Position& position, double currentPrice)
{
	TriggerResult result;
	result.triggered = false;

	if (!position.trailingStopEnabled) {
		return result;
	}

	// Update highest price first
	if (currentPrice > position.highestPrice) {
		position.highestPrice = currentPrice;
	}

	// Check if trailing stop hit
	double trailingDistance = position.highestPrice * (position.trailingStopPercent / 100.0);
	double trailingStopPrice = position.highestPrice - trailingDistance;

	if (currentPrice <= trailingStopPrice) {
		result.triggered = true;
		result.reason = "trailing-stop";
		result.exitPrice = trailingStopPrice;
		LogInfo("Trailing stop triggered for %s: price=%.2f, TS=%.2f (highest=%.2f)",
			position.id.c_str(), currentPrice, trailingStopPrice, position.highestPrice);
	}

	return result;
}

RiskManager::TriggerResult RiskManager::shouldClosePosition(Position& position, double currentPrice)
{
	// Check in priority order: stop-loss > trailing-stop > take-profit

	TriggerResult result = checkStopLoss(position, currentPrice);
	if (result.triggered) {
		return result;
	}

	result = checkTrailingStop(position, currentPrice);
	if (result.triggered) {
		return result;
	}

	result = checkTakeProfit(position, currentPrice);
	return result;
}

// Daily statistics
void RiskManager::recordTrade(double pnl, bool isWinner)
{
	autoResetIfNewDay();

	dailyStats.realizedPnL += pnl;
	dailyStats.tradesExecuted++;

	if (isWinner) {
		dailyStats.winningTrades++;
	} else {
		dailyStats.losingTrades++;
	}

	LogInfo("Trade recorded: PnL=%.2f, Winner=%d, DailyPnL=%.2f",
		pnl, isWinner, dailyStats.realizedPnL);
}

void RiskManager::updateDailyStats(double currentCapital)
{
	autoResetIfNewDay();

	dailyStats.currentCapital = currentCapital;

	// Calculate drawdown
	double drawdown = dailyStats.startingCapital - currentCapital;
	if (drawdown > dailyStats.maxDrawdown) {
		dailyStats.maxDrawdown = drawdown;
	}
}

void RiskManager::resetDailyStats()
{
	dailyStats.date = getTodayMidnight();
	dailyStats.startingCapital = totalCapital;
	dailyStats.currentCapital = totalCapital;
	dailyStats.realizedPnL = 0.0;
	dailyStats.maxDrawdown = 0.0;
	dailyStats.tradesExecuted = 0;
	dailyStats.winningTrades = 0;
	dailyStats.losingTrades = 0;

	lastResetTime = time(nullptr);

	LogInfo("Daily stats reset: capital=%.2f", totalCapital);
}

DailyStats RiskManager::getDailyStats() const
{
	const_cast<RiskManager*>(this)->autoResetIfNewDay();
	return dailyStats;
}

// Portfolio metrics
double RiskManager::getTotalExposure() const
{
	double exposure = 0.0;
	for (const auto& pos : openPositions) {
		exposure += calculatePositionValue(pos);
	}
	return exposure;
}

double RiskManager::getAvailableCapital() const
{
	return totalCapital - getTotalExposure();
}

double RiskManager::getUsedCapitalPercent() const
{
	if (totalCapital <= 0.0) {
		return 0.0;
	}
	return (getTotalExposure() / totalCapital) * 100.0;
}

double RiskManager::getTotalUnrealizedPnL() const
{
	double totalPnL = 0.0;
	for (const auto& pos : openPositions) {
		totalPnL += pos.currentPnL;
	}
	return totalPnL;
}

double RiskManager::getDailyPnL() const
{
	const_cast<RiskManager*>(this)->autoResetIfNewDay();
	return dailyStats.realizedPnL;
}

double RiskManager::getDailyPnLPercent() const
{
	if (dailyStats.startingCapital <= 0.0) {
		return 0.0;
	}
	return (dailyStats.realizedPnL / dailyStats.startingCapital) * 100.0;
}

// Risk metrics
double RiskManager::getMaxPositionValue() const
{
	// Allow up to 20% of capital per position by default
	// This could be made configurable via RiskConfig
	return totalCapital * 0.2;
}

double RiskManager::getRemainingDailyLoss() const
{
	const_cast<RiskManager*>(this)->autoResetIfNewDay();

	double maxLossAmount = totalCapital * (config.maxDailyLossPercent / 100.0);
	double currentLoss = std::abs(std::min(0.0, dailyStats.realizedPnL));
	return maxLossAmount - currentLoss;
}

bool RiskManager::isDailyLossLimitHit() const
{
	return !isWithinDailyLossLimit();
}

// Emergency controls
void RiskManager::enableTrading()
{
	tradingEnabled = true;
	LogInfo("Trading ENABLED");
}

void RiskManager::disableTrading()
{
	tradingEnabled = false;
	LogWarning("Trading DISABLED");
}

bool RiskManager::isTradingEnabled() const
{
	return tradingEnabled;
}

void RiskManager::setEmergencyStopLoss(bool enabled)
{
	emergencyStopLoss = enabled;
	if (enabled) {
		LogWarning("EMERGENCY STOP-LOSS ACTIVATED - Will force close all positions on limit");
	}
}

// Reset
void RiskManager::reset()
{
	clearPositions();
	resetDailyStats();
	tradingEnabled = true;
	emergencyStopLoss = false;
	LogInfo("RiskManager reset");
}

void RiskManager::clearPositions()
{
	openPositions.clear();
	positionIndex.clear();
	LogInfo("All positions cleared");
}

// Getters
const RiskConfig& RiskManager::getRiskConfig() const
{
	return config;
}

double RiskManager::getTotalCapital() const
{
	return totalCapital;
}

// Private helpers
bool RiskManager::isNewDay() const
{
	time_t today = getTodayMidnight();
	return today > dailyStats.date;
}

void RiskManager::autoResetIfNewDay()
{
	if (isNewDay()) {
		LogInfo("New day detected - resetting daily stats");
		resetDailyStats();
	}
}

time_t RiskManager::getTodayMidnight() const
{
	time_t now = time(nullptr);
	struct tm* tm_info = localtime(&now);
	tm_info->tm_hour = 0;
	tm_info->tm_min = 0;
	tm_info->tm_sec = 0;
	return mktime(tm_info);
}

double RiskManager::calculatePositionValue(const Position& pos) const
{
	// Current value = quantity * current entry price
	// In real implementation, would use current market price
	return pos.quantity * pos.entryPrice;
}

void RiskManager::updatePositionPnL(Position& pos, double currentPrice)
{
	// Calculate unrealized P&L
	double entryValue = pos.quantity * pos.entryPrice;
	double currentValue = pos.quantity * currentPrice;

	pos.currentPnL = currentValue - entryValue;

	if (entryValue > 0.0) {
		pos.currentPnLPercent = (pos.currentPnL / entryValue) * 100.0;
	} else {
		pos.currentPnLPercent = 0.0;
	}
}

} // namespace Core
} // namespace Emiglio
