#include "BacktestSimulator.h"
#include "../utils/Logger.h"
#include <algorithm>
#include <cmath>

namespace Emiglio {
namespace Backtest {

BacktestSimulator::BacktestSimulator(const Recipe& recipe, const BacktestConfig& config)
	: recipe(recipe)
	, config(config)
	, signalGen()
	, portfolio(config.initialCapital)
{
	signalGen.loadRecipe(recipe);
	LOG_INFO("BacktestSimulator initialized for strategy: " + recipe.name);
}

BacktestSimulator::~BacktestSimulator() {
}

void BacktestSimulator::setCommission(double percent) {
	config.commissionPercent = percent;
}

void BacktestSimulator::setSlippage(double percent) {
	config.slippagePercent = percent;
}

void BacktestSimulator::setInitialCapital(double capital) {
	config.initialCapital = capital;
	portfolio.reset(capital);
}

void BacktestSimulator::setMaxOpenPositions(int max) {
	config.maxOpenPositions = max;
}

std::string BacktestSimulator::getLastError() const {
	return lastError;
}

double BacktestSimulator::calculateCommission(double orderValue) const {
	return orderValue * config.commissionPercent;
}

double BacktestSimulator::calculateSlippage(double price, bool isBuy) const {
	// Slippage: buy higher, sell lower
	if (isBuy) {
		return price * config.slippagePercent;
	} else {
		return price * config.slippagePercent;
	}
}

double BacktestSimulator::calculatePositionSize(double price) const {
	// Fixed: Added validation to prevent division by zero
	if (price <= 0.0) {
		LOG_ERROR("Invalid price for position size calculation: " + std::to_string(price));
		return 0.0;
	}

	// Position size based on recipe's position_size_percent
	double availableCash = portfolio.getCash();
	if (availableCash <= 0.0) {
		LOG_WARNING("No cash available for position: " + std::to_string(availableCash));
		return 0.0;
	}

	double positionPercent = recipe.capital.positionSizePercent / 100.0;
	if (positionPercent <= 0.0 || positionPercent > 1.0) {
		LOG_ERROR("Invalid position percent: " + std::to_string(positionPercent * 100.0) + "%");
		return 0.0;
	}

	double targetValue = availableCash * positionPercent;

	// Calculate quantity
	double quantity = targetValue / price;

	return quantity;
}

void BacktestSimulator::checkStopLoss(const Candle& candle) {
	if (!config.useStopLoss) return;

	auto openTrades = portfolio.getOpenTrades();

	for (auto& trade : openTrades) {
		if (trade.stopLossPrice <= 0.0) continue;

		bool hitStopLoss = false;

		if (trade.type == TradeType::LONG) {
			// LONG: stop-loss triggers if price drops below SL
			if (candle.low <= trade.stopLossPrice) {
				hitStopLoss = true;
			}
		} else {
			// SHORT: stop-loss triggers if price rises above SL
			if (candle.high >= trade.stopLossPrice) {
				hitStopLoss = true;
			}
		}

		if (hitStopLoss) {
			double exitPrice = trade.stopLossPrice;
			double commission = calculateCommission(exitPrice * trade.quantity);
			double slippage = calculateSlippage(exitPrice, false);  // Selling

			portfolio.closePosition(trade.id, exitPrice, "Stop-Loss", commission, slippage);
		}
	}
}

void BacktestSimulator::checkTakeProfit(const Candle& candle) {
	if (!config.useTakeProfit) return;

	auto openTrades = portfolio.getOpenTrades();

	for (auto& trade : openTrades) {
		if (trade.takeProfitPrice <= 0.0) continue;

		bool hitTakeProfit = false;

		if (trade.type == TradeType::LONG) {
			// LONG: take-profit triggers if price rises above TP
			if (candle.high >= trade.takeProfitPrice) {
				hitTakeProfit = true;
			}
		} else {
			// SHORT: take-profit triggers if price drops below TP
			if (candle.low <= trade.takeProfitPrice) {
				hitTakeProfit = true;
			}
		}

		if (hitTakeProfit) {
			double exitPrice = trade.takeProfitPrice;
			double commission = calculateCommission(exitPrice * trade.quantity);
			double slippage = calculateSlippage(exitPrice, false);  // Selling

			portfolio.closePosition(trade.id, exitPrice, "Take-Profit", commission, slippage);
		}
	}
}

void BacktestSimulator::updateEquityCurve(const Candle& candle) {
	double currentPrice = candle.close;
	double equity = portfolio.getEquity(currentPrice);
	double cash = portfolio.getCash();
	double positionValue = portfolio.getPositionValue(currentPrice);

	EquityPoint point(candle.timestamp, equity, cash, positionValue);
	result.equityCurve.push_back(point);

	// Update peak equity
	if (equity > result.peakEquity) {
		result.peakEquity = equity;
	}
}

void BacktestSimulator::processCandle(const Candle& candle, size_t index,
                                      const std::vector<Candle>& allCandles) {
	// Check stop-loss and take-profit first (on candle open/high/low)
	checkStopLoss(candle);
	checkTakeProfit(candle);

	// Generate signal at current index (indicators already pre-calculated)
	Signal signal = signalGen.generateSignalAt(index, allCandles);

	// Process signal
	if (signal.type == SignalType::BUY) {
		// Check if we can open a new position
		if (portfolio.getOpenTradesCount() >= config.maxOpenPositions) {
			// Already at max positions
			return;
		}

		// Calculate position size
		double entryPrice = candle.close;  // Enter at close price
		double quantity = calculatePositionSize(entryPrice);

		if (quantity <= 0.0) {
			LOG_WARNING("Invalid quantity calculated: " + std::to_string(quantity));
			return;
		}

		// Calculate costs
		double orderValue = entryPrice * quantity;
		double commission = calculateCommission(orderValue);
		double slippage = calculateSlippage(entryPrice, true);  // Buying

		// Create trade
		Trade trade;
		trade.symbol = candle.symbol;
		trade.type = TradeType::LONG;
		trade.entryPrice = entryPrice;
		trade.quantity = quantity;
		trade.entryTime = candle.timestamp;
		trade.entryReason = signal.reason;

		// Set stop-loss and take-profit
		if (config.useStopLoss && recipe.risk.stopLossPercent > 0.0) {
			trade.stopLossPrice = entryPrice * (1.0 - recipe.risk.stopLossPercent / 100.0);
		}
		if (config.useTakeProfit && recipe.risk.takeProfitPercent > 0.0) {
			trade.takeProfitPrice = entryPrice * (1.0 + recipe.risk.takeProfitPercent / 100.0);
		}

		// Open position
		bool success = portfolio.openPosition(trade, commission, slippage);
		if (success) {
			result.totalCommission += commission;
			result.totalSlippage += slippage;
		}

	} else if (signal.type == SignalType::SELL) {
		// Close all open LONG positions
		auto openTrades = portfolio.getOpenTrades();

		for (auto& trade : openTrades) {
			if (trade.type == TradeType::LONG) {
				double exitPrice = candle.close;
				double commission = calculateCommission(exitPrice * trade.quantity);
				double slippage = calculateSlippage(exitPrice, false);  // Selling

				portfolio.closePosition(trade.id, exitPrice, "Exit Signal", commission, slippage);

				result.totalCommission += commission;
				result.totalSlippage += slippage;
			}
		}
	}

	// Update equity curve
	updateEquityCurve(candle);
}

BacktestResult BacktestSimulator::run(const std::vector<Candle>& candles) {
	// Reset result
	result = BacktestResult();
	result.recipeName = recipe.name;
	result.initialCapital = config.initialCapital;

	if (candles.empty()) {
		lastError = "No candles provided";
		LOG_ERROR(lastError);
		return result;
	}

	result.symbol = candles[0].symbol;
	result.startTime = candles.front().timestamp;
	result.endTime = candles.back().timestamp;
	result.totalCandles = candles.size();

	LOG_INFO("Starting backtest: " + recipe.name + " on " + result.symbol);
	LOG_INFO("  Period: " + std::to_string(result.totalCandles) + " candles");
	LOG_INFO("  Capital: $" + std::to_string(config.initialCapital));
	LOG_INFO("  Commission: " + std::to_string(config.commissionPercent * 100) + "%");
	LOG_INFO("  Slippage: " + std::to_string(config.slippagePercent * 100) + "%");

	// Reset portfolio
	portfolio.reset(config.initialCapital);

	// OPTIMIZATION: Pre-calculate all indicators once (instead of recalculating for each candle)
	LOG_INFO("Pre-calculating indicators...");
	if (!signalGen.precalculateIndicators(candles)) {
		lastError = "Failed to pre-calculate indicators";
		LOG_ERROR(lastError);
		return result;
	}
	LOG_INFO("Indicators pre-calculated successfully");

	// Process each candle
	for (size_t i = 0; i < candles.size(); i++) {
		processCandle(candles[i], i, candles);
	}

	// Close any remaining open positions at final price
	auto openTrades = portfolio.getOpenTrades();
	if (!openTrades.empty()) {
		LOG_INFO("Closing " + std::to_string(openTrades.size()) + " open positions at end of backtest");

		double finalPrice = candles.back().close;
		for (auto& trade : openTrades) {
			double commission = calculateCommission(finalPrice * trade.quantity);
			double slippage = calculateSlippage(finalPrice, false);

			portfolio.closePosition(trade.id, finalPrice, "End of Backtest", commission, slippage);

			result.totalCommission += commission;
			result.totalSlippage += slippage;
		}
	}

	// Collect trades
	result.trades = portfolio.getClosedTrades();
	result.totalTrades = result.trades.size();

	// Count winning/losing trades
	for (const auto& trade : result.trades) {
		if (trade.pnl > 0.0) {
			result.winningTrades++;
		} else if (trade.pnl < 0.0) {
			result.losingTrades++;
		}
	}

	// Final equity
	result.finalEquity = portfolio.getEquity(candles.back().close);

	// Calculate basic metrics
	result.totalReturn = result.finalEquity - result.initialCapital;
	result.totalReturnPercent = (result.totalReturn / result.initialCapital) * 100.0;

	if (result.totalTrades > 0) {
		result.winRate = (static_cast<double>(result.winningTrades) / result.totalTrades) * 100.0;
	}

	LOG_INFO("Backtest completed:");
	LOG_INFO("  Total trades: " + std::to_string(result.totalTrades));
	LOG_INFO("  Win rate: " + std::to_string(result.winRate) + "%");
	LOG_INFO("  Final equity: $" + std::to_string(result.finalEquity));
	LOG_INFO("  Total return: $" + std::to_string(result.totalReturn) +
	         " (" + std::to_string(result.totalReturnPercent) + "%)");

	return result;
}

} // namespace Backtest
} // namespace Emiglio
