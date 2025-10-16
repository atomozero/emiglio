#include "SignalGenerator.h"
#include "../utils/Logger.h"
#include <cmath>

namespace Emiglio {

SignalGenerator::SignalGenerator() {
}

SignalGenerator::~SignalGenerator() {
}

// Load recipe
bool SignalGenerator::loadRecipe(const Recipe& recipe) {
	this->recipe = recipe;
	indicatorCache.clear();

	LOG_INFO("Loaded recipe: " + recipe.name);
	LOG_INFO("  Market: " + recipe.market.exchange + " " + recipe.market.symbol + " " + recipe.market.timeframe);
	LOG_INFO("  Indicators: " + std::to_string(recipe.indicators.size()));
	LOG_INFO("  Entry rules: " + std::to_string(recipe.entryConditions.rules.size()));
	LOG_INFO("  Exit rules: " + std::to_string(recipe.exitConditions.rules.size()));

	return true;
}

// Calculate all indicators for the recipe
bool SignalGenerator::calculateIndicators(const std::vector<Candle>& candles) {
	if (candles.empty()) {
		lastError = "No candles provided";
		LOG_ERROR(lastError);
		return false;
	}

	indicatorCache.clear();

	// Extract price data
	std::vector<double> closes = Indicators::getClosePrices(candles);

	// Always calculate closing prices (used by many rules)
	indicatorCache["close"] = closes;

	// Calculate each indicator defined in recipe
	for (const auto& indConfig : recipe.indicators) {
		std::string name = indConfig.name;
		int period = indConfig.period;

		LOG_DEBUG("Calculating indicator: " + name + " (period=" + std::to_string(period) + ")");

		if (name == "sma") {
			indicatorCache["sma"] = Indicators::sma(closes, period);

		} else if (name == "ema") {
			indicatorCache["ema"] = Indicators::ema(closes, period);

		} else if (name == "rsi") {
			indicatorCache["rsi"] = Indicators::rsi(closes, period);

		} else if (name == "macd") {
			int fastPeriod = 12;
			int slowPeriod = 26;
			int signalPeriod = 9;

			// Allow custom periods from params
			if (indConfig.params.count("fast_period")) fastPeriod = static_cast<int>(indConfig.params.at("fast_period"));
			if (indConfig.params.count("slow_period")) slowPeriod = static_cast<int>(indConfig.params.at("slow_period"));
			if (indConfig.params.count("signal_period")) signalPeriod = static_cast<int>(indConfig.params.at("signal_period"));

			auto macdResult = Indicators::macd(closes, fastPeriod, slowPeriod, signalPeriod);
			indicatorCache["macd"] = macdResult.macdLine;
			indicatorCache["macd_signal"] = macdResult.signalLine;
			indicatorCache["macd_histogram"] = macdResult.histogram;

		} else if (name == "bollinger" || name == "bbands") {
			double multiplier = 2.0;
			if (indConfig.params.count("multiplier")) {
				multiplier = indConfig.params.at("multiplier");
			}

			auto bbResult = Indicators::bollingerBands(closes, period, multiplier);
			indicatorCache["bb_upper"] = bbResult.upper;
			indicatorCache["bb_middle"] = bbResult.middle;
			indicatorCache["bb_lower"] = bbResult.lower;

		} else if (name == "atr") {
			indicatorCache["atr"] = Indicators::atr(candles, period);

		} else if (name == "stochastic" || name == "stoch") {
			int kPeriod = period;
			int dPeriod = 3;
			if (indConfig.params.count("d_period")) {
				dPeriod = static_cast<int>(indConfig.params.at("d_period"));
			}

			auto stochResult = Indicators::stochastic(candles, kPeriod, dPeriod);
			indicatorCache["stoch_k"] = stochResult.k;
			indicatorCache["stoch_d"] = stochResult.d;

		} else if (name == "obv") {
			indicatorCache["obv"] = Indicators::obv(candles);

		} else if (name == "adx") {
			indicatorCache["adx"] = Indicators::adx(candles, period);

		} else if (name == "cci") {
			indicatorCache["cci"] = Indicators::cci(candles, period);

		} else {
			LOG_WARNING("Unknown indicator: " + name);
		}
	}

	return true;
}

// Get indicator value at specific index
double SignalGenerator::getIndicatorValue(const std::string& indicatorName, size_t index) {
	if (indicatorCache.count(indicatorName) == 0) {
		LOG_WARNING("Indicator not found in cache: " + indicatorName);
		return NAN;
	}

	const auto& values = indicatorCache[indicatorName];
	if (index >= values.size()) {
		LOG_WARNING("Index out of range for indicator: " + indicatorName);
		return NAN;
	}

	return values[index];
}

// Check if indicator crosses above threshold
bool SignalGenerator::crossesAbove(const std::string& indicator, double threshold, size_t index) {
	if (index == 0) return false;

	double current = getIndicatorValue(indicator, index);
	double previous = getIndicatorValue(indicator, index - 1);

	if (std::isnan(current) || std::isnan(previous)) return false;

	return (previous <= threshold && current > threshold);
}

// Check if indicator crosses below threshold
bool SignalGenerator::crossesBelow(const std::string& indicator, double threshold, size_t index) {
	if (index == 0) return false;

	double current = getIndicatorValue(indicator, index);
	double previous = getIndicatorValue(indicator, index - 1);

	if (std::isnan(current) || std::isnan(previous)) return false;

	return (previous >= threshold && current < threshold);
}

// Compare two values based on operator
bool SignalGenerator::compareValues(double left, const std::string& op, double right) {
	if (std::isnan(left) || std::isnan(right)) return false;

	if (op == ">") return left > right;
	if (op == "<") return left < right;
	if (op == ">=") return left >= right;
	if (op == "<=") return left <= right;
	if (op == "==") return std::abs(left - right) < 1e-6; // Floating point equality

	LOG_WARNING("Unknown operator: " + op);
	return false;
}

// Evaluate a single rule
bool SignalGenerator::evaluateRule(const TradingRule& rule, size_t index) {
	// Handle special operators (crosses_above, crosses_below)
	if (rule.operatorStr == "crosses_above") {
		return crossesAbove(rule.indicator, rule.value, index);
	}

	if (rule.operatorStr == "crosses_below") {
		return crossesBelow(rule.indicator, rule.value, index);
	}

	// Standard comparison operators
	double indicatorValue = getIndicatorValue(rule.indicator, index);

	if (std::isnan(indicatorValue)) {
		LOG_DEBUG("Indicator value is NaN: " + rule.indicator);
		return false;
	}

	// If compareWith is specified, compare with another indicator
	double compareValue = rule.value;
	if (!rule.compareWith.empty()) {
		compareValue = getIndicatorValue(rule.compareWith, index);
		if (std::isnan(compareValue)) {
			LOG_DEBUG("Compare indicator value is NaN: " + rule.compareWith);
			return false;
		}
	}

	bool result = compareValues(indicatorValue, rule.operatorStr, compareValue);

	LOG_DEBUG("Rule: " + rule.indicator + " " + rule.operatorStr + " " +
	          std::to_string(compareValue) + " = " +
	          std::to_string(indicatorValue) + " " + rule.operatorStr + " " +
	          std::to_string(compareValue) + " = " + (result ? "TRUE" : "FALSE"));

	return result;
}

// Evaluate conditions (AND/OR logic)
bool SignalGenerator::evaluateConditions(const TradingConditions& conditions, size_t index) {
	if (conditions.rules.empty()) {
		return false;
	}

	bool result = false;

	if (conditions.logic == "AND") {
		result = true; // Start with true for AND
		for (const auto& rule : conditions.rules) {
			if (!evaluateRule(rule, index)) {
				result = false;
				break; // Short-circuit
			}
		}
	} else if (conditions.logic == "OR") {
		result = false; // Start with false for OR
		for (const auto& rule : conditions.rules) {
			if (evaluateRule(rule, index)) {
				result = true;
				break; // Short-circuit
			}
		}
	} else {
		LOG_WARNING("Unknown logic operator: " + conditions.logic);
		return false;
	}

	return result;
}

// Check if entry conditions are met
bool SignalGenerator::checkEntryConditions(const std::vector<Candle>& candles) {
	if (!calculateIndicators(candles)) {
		return false;
	}

	// Evaluate conditions at last candle
	size_t lastIndex = candles.size() - 1;
	return evaluateConditions(recipe.entryConditions, lastIndex);
}

// Check if exit conditions are met
bool SignalGenerator::checkExitConditions(const std::vector<Candle>& candles) {
	if (!calculateIndicators(candles)) {
		return false;
	}

	// Evaluate conditions at last candle
	size_t lastIndex = candles.size() - 1;
	return evaluateConditions(recipe.exitConditions, lastIndex);
}

// Generate signal based on latest candle data
Signal SignalGenerator::generateSignal(const std::vector<Candle>& candles) {
	Signal signal;
	signal.type = SignalType::NONE;
	signal.symbol = recipe.market.symbol;
	signal.price = 0.0;
	signal.timestamp = 0;
	signal.reason = "";

	if (candles.empty()) {
		lastError = "No candles provided";
		return signal;
	}

	// Get latest candle data
	const Candle& lastCandle = candles.back();
	signal.price = lastCandle.close;
	signal.timestamp = lastCandle.timestamp;

	// Calculate indicators
	if (!calculateIndicators(candles)) {
		signal.reason = "Failed to calculate indicators";
		return signal;
	}

	// Check entry conditions (BUY signal)
	if (checkEntryConditions(candles)) {
		signal.type = SignalType::BUY;
		signal.reason = "Entry conditions met";
		LOG_INFO("BUY signal generated for " + signal.symbol + " at " + std::to_string(signal.price));
		return signal;
	}

	// Check exit conditions (SELL signal)
	if (checkExitConditions(candles)) {
		signal.type = SignalType::SELL;
		signal.reason = "Exit conditions met";
		LOG_INFO("SELL signal generated for " + signal.symbol + " at " + std::to_string(signal.price));
		return signal;
	}

	// No signal
	signal.reason = "No conditions met";
	return signal;
}

// Pre-calculate all indicators for entire dataset (optimization for backtesting)
bool SignalGenerator::precalculateIndicators(const std::vector<Candle>& candles) {
	return calculateIndicators(candles);
}

// Generate signal at specific index (assumes indicators are pre-calculated)
Signal SignalGenerator::generateSignalAt(size_t index, const std::vector<Candle>& candles) {
	Signal signal;
	signal.type = SignalType::NONE;
	signal.symbol = recipe.market.symbol;
	signal.price = 0.0;
	signal.timestamp = 0;
	signal.reason = "";

	if (index >= candles.size()) {
		lastError = "Index out of range";
		return signal;
	}

	// Get candle data at index
	const Candle& candle = candles[index];
	signal.price = candle.close;
	signal.timestamp = candle.timestamp;

	// Check entry conditions (BUY signal)
	if (evaluateConditions(recipe.entryConditions, index)) {
		signal.type = SignalType::BUY;
		signal.reason = "Entry conditions met";
		return signal;
	}

	// Check exit conditions (SELL signal)
	if (evaluateConditions(recipe.exitConditions, index)) {
		signal.type = SignalType::SELL;
		signal.reason = "Exit conditions met";
		return signal;
	}

	// No signal
	signal.reason = "No conditions met";
	return signal;
}

// Check entry conditions at specific index (assumes indicators are pre-calculated)
bool SignalGenerator::checkEntryConditionsAt(size_t index) {
	return evaluateConditions(recipe.entryConditions, index);
}

// Check exit conditions at specific index (assumes indicators are pre-calculated)
bool SignalGenerator::checkExitConditionsAt(size_t index) {
	return evaluateConditions(recipe.exitConditions, index);
}

} // namespace Emiglio
