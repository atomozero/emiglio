#ifndef SIGNALGENERATOR_H
#define SIGNALGENERATOR_H

#include "RecipeLoader.h"
#include "Indicators.h"
#include "../data/DataStorage.h"
#include <string>
#include <vector>
#include <map>

namespace Emiglio {

// Trading signal type
enum class SignalType {
	NONE,      // No signal
	BUY,       // Enter long position
	SELL,      // Exit long position
	SHORT,     // Enter short position (not used in most spot markets)
	COVER      // Exit short position
};

// Trading signal
struct Signal {
	SignalType type;
	std::string symbol;
	double price;
	time_t timestamp;
	std::string reason;  // Why the signal was generated (for debugging)
};

// Signal generator - evaluates recipes and generates trading signals
class SignalGenerator {
public:
	SignalGenerator();
	~SignalGenerator();

	// Load recipe
	bool loadRecipe(const Recipe& recipe);

	// Generate signal based on latest candle data
	// Returns signal type (BUY, SELL, NONE)
	Signal generateSignal(const std::vector<Candle>& candles);

	// Pre-calculate all indicators for entire dataset (for backtesting optimization)
	bool precalculateIndicators(const std::vector<Candle>& candles);

	// Generate signal at specific index (assumes indicators are pre-calculated)
	Signal generateSignalAt(size_t index, const std::vector<Candle>& candles);

	// Check if entry conditions are met
	bool checkEntryConditions(const std::vector<Candle>& candles);

	// Check if exit conditions are met
	bool checkExitConditions(const std::vector<Candle>& candles);

	// Check entry/exit at specific index (assumes indicators are pre-calculated)
	bool checkEntryConditionsAt(size_t index);
	bool checkExitConditionsAt(size_t index);

	// Get current recipe
	const Recipe& getRecipe() const { return recipe; }

	// Get last error
	std::string getLastError() const { return lastError; }

private:
	Recipe recipe;
	std::string lastError;

	// Cache of calculated indicators
	std::map<std::string, std::vector<double>> indicatorCache;

	// Calculate all indicators for the recipe
	bool calculateIndicators(const std::vector<Candle>& candles);

	// Evaluate a single rule
	bool evaluateRule(const TradingRule& rule, size_t index);

	// Evaluate conditions (AND/OR logic)
	bool evaluateConditions(const TradingConditions& conditions, size_t index);

	// Compare two values based on operator
	bool compareValues(double left, const std::string& op, double right);

	// Get indicator value at specific index
	double getIndicatorValue(const std::string& indicatorName, size_t index);

	// Helper: Check if indicator crosses above threshold
	bool crossesAbove(const std::string& indicator, double threshold, size_t index);

	// Helper: Check if indicator crosses below threshold
	bool crossesBelow(const std::string& indicator, double threshold, size_t index);
};

} // namespace Emiglio

#endif // SIGNALGENERATOR_H
