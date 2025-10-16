#ifndef RECIPELOADER_H
#define RECIPELOADER_H

#include <string>
#include <vector>
#include <map>

namespace Emiglio {

// Trading rule for entry/exit conditions
struct TradingRule {
	std::string indicator;     // e.g., "rsi", "macd", "sma"
	std::string operatorStr;   // e.g., ">", "<", ">=", "<=", "==", "crosses_above", "crosses_below"
	double value;              // Threshold value
	std::string compareWith;   // Optional: compare with another indicator instead of fixed value
};

// Entry/Exit conditions
struct TradingConditions {
	std::string logic;              // "AND" or "OR"
	std::vector<TradingRule> rules; // List of rules
};

// Indicator configuration
struct IndicatorConfig {
	std::string name;               // e.g., "rsi", "sma", "ema", "macd", "bollinger"
	int period;                     // Period for calculation (e.g., 14 for RSI)
	std::map<std::string, double> params; // Additional parameters (e.g., {"overbought": 70, "oversold": 30})
};

// Market configuration
struct MarketConfig {
	std::string exchange;     // e.g., "binance"
	std::string symbol;       // e.g., "BTC/USDT"
	std::string timeframe;    // e.g., "5m", "1h"
};

// Capital management
struct CapitalConfig {
	double initial;                  // Initial capital in quote currency (e.g., USDT)
	double positionSizePercent;      // Position size as % of capital (e.g., 10 = 10%)
};

// Risk management
struct RiskConfig {
	double stopLossPercent;          // Stop loss as % (e.g., 2.0 = 2%)
	double takeProfitPercent;        // Take profit as % (e.g., 5.0 = 5%)
	double maxDailyLossPercent;      // Max daily loss limit (e.g., 5.0 = 5%)
	int maxOpenPositions;            // Max concurrent open positions (default 1)
};

// Complete trading strategy recipe
struct Recipe {
	std::string name;                    // Strategy name
	std::string description;             // Optional description

	MarketConfig market;                 // Market configuration
	CapitalConfig capital;               // Capital settings
	RiskConfig risk;                     // Risk management

	std::vector<IndicatorConfig> indicators;  // Indicators to calculate
	TradingConditions entryConditions;        // Entry signal conditions
	TradingConditions exitConditions;         // Exit signal conditions
};

// Recipe loader - loads trading strategies from JSON files
class RecipeLoader {
public:
	RecipeLoader();
	~RecipeLoader();

	// Load recipe from JSON file
	bool loadFromFile(const std::string& filename, Recipe& recipe);

	// Load recipe from JSON string
	bool loadFromString(const std::string& jsonStr, Recipe& recipe);

	// Save recipe to JSON file
	bool saveToFile(const std::string& filename, const Recipe& recipe);

	// Get last error message
	std::string getLastError() const;

private:
	std::string lastError;

	// Helper: Parse indicator config from JSON
	bool parseIndicator(const void* jsonValue, IndicatorConfig& config);

	// Helper: Parse trading rule from JSON
	bool parseRule(const void* jsonValue, TradingRule& rule);

	// Helper: Parse trading conditions from JSON
	bool parseConditions(const void* jsonValue, TradingConditions& conditions);

	// Helper: Parse market config from JSON
	bool parseMarket(const void* jsonValue, MarketConfig& config);

	// Helper: Parse capital config from JSON
	bool parseCapital(const void* jsonValue, CapitalConfig& config);

	// Helper: Parse risk config from JSON
	bool parseRisk(const void* jsonValue, RiskConfig& config);
};

} // namespace Emiglio

#endif // RECIPELOADER_H
