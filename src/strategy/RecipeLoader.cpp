#include "RecipeLoader.h"
#include "../utils/JsonParser.h"
#include "../utils/Logger.h"
#include <fstream>
#include <sstream>

namespace Emiglio {

RecipeLoader::RecipeLoader() {
}

RecipeLoader::~RecipeLoader() {
}

std::string RecipeLoader::getLastError() const {
	return lastError;
}

// Load recipe from JSON file
bool RecipeLoader::loadFromFile(const std::string& filename, Recipe& recipe) {
	// Read file contents
	std::ifstream file(filename);
	if (!file.is_open()) {
		lastError = "Failed to open file: " + filename;
		LOG_ERROR(lastError);
		return false;
	}

	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string jsonStr = buffer.str();
	file.close();

	return loadFromString(jsonStr, recipe);
}

// Load recipe from JSON string
bool RecipeLoader::loadFromString(const std::string& jsonStr, Recipe& recipe) {
	JsonParser parser;

	if (!parser.parse(jsonStr)) {
		lastError = "Failed to parse JSON";
		LOG_ERROR(lastError);
		return false;
	}

	// Parse basic info
	recipe.name = parser.getString("name", "");
	recipe.description = parser.getString("description", "");

	if (recipe.name.empty()) {
		lastError = "Recipe name is required";
		LOG_ERROR(lastError);
		return false;
	}

	// Parse market config
	recipe.market.exchange = parser.getString("market.exchange", "");
	recipe.market.symbol = parser.getString("market.symbol", "");
	recipe.market.timeframe = parser.getString("market.timeframe", "");

	if (recipe.market.exchange.empty() || recipe.market.symbol.empty() || recipe.market.timeframe.empty()) {
		lastError = "Market configuration incomplete (exchange, symbol, timeframe required)";
		LOG_ERROR(lastError);
		return false;
	}

	// Parse capital config
	recipe.capital.initial = parser.getDouble("capital.initial", 0.0);
	recipe.capital.positionSizePercent = parser.getDouble("capital.position_size_percent", 10.0);

	if (recipe.capital.initial <= 0) {
		lastError = "Initial capital must be > 0";
		LOG_ERROR(lastError);
		return false;
	}

	// Parse risk config
	recipe.risk.stopLossPercent = parser.getDouble("risk_management.stop_loss_percent", 0.0);
	recipe.risk.takeProfitPercent = parser.getDouble("risk_management.take_profit_percent", 0.0);
	recipe.risk.maxDailyLossPercent = parser.getDouble("risk_management.max_daily_loss_percent", 5.0);
	recipe.risk.maxOpenPositions = parser.getInt("risk_management.max_open_positions", 1);

	// Parse indicators array
	size_t indicatorCount = parser.getArraySize("indicators");
	LOG_INFO("Found " + std::to_string(indicatorCount) + " indicators");

	for (size_t i = 0; i < indicatorCount; i++) {
		IndicatorConfig indicator;
		indicator.name = parser.getArrayObjectString("indicators", i, "name", "");
		indicator.period = static_cast<int>(parser.getArrayObjectInt64("indicators", i, "period", 14));

		// Parse additional parameters (e.g., oversold, overbought for RSI)
		// For now, we'll support common parameters as direct fields
		double oversold = parser.getArrayObjectDouble("indicators", i, "oversold", 0.0);
		double overbought = parser.getArrayObjectDouble("indicators", i, "overbought", 0.0);
		double multiplier = parser.getArrayObjectDouble("indicators", i, "multiplier", 0.0);

		if (oversold > 0) indicator.params["oversold"] = oversold;
		if (overbought > 0) indicator.params["overbought"] = overbought;
		if (multiplier > 0) indicator.params["multiplier"] = multiplier;

		if (!indicator.name.empty()) {
			recipe.indicators.push_back(indicator);
			LOG_DEBUG("Added indicator: " + indicator.name + " (period=" + std::to_string(indicator.period) + ")");
		}
	}

	// Parse entry conditions
	recipe.entryConditions.logic = parser.getString("entry_conditions.logic", "AND");

	size_t entryRuleCount = parser.getArraySize("entry_conditions.rules");
	LOG_INFO("Found " + std::to_string(entryRuleCount) + " entry rules");

	for (size_t i = 0; i < entryRuleCount; i++) {
		TradingRule rule;
		rule.indicator = parser.getArrayObjectString("entry_conditions.rules", i, "indicator", "");
		rule.operatorStr = parser.getArrayObjectString("entry_conditions.rules", i, "operator", "");
		rule.value = parser.getArrayObjectDouble("entry_conditions.rules", i, "value", 0.0);
		rule.compareWith = parser.getArrayObjectString("entry_conditions.rules", i, "compare_with", "");

		if (!rule.indicator.empty() && !rule.operatorStr.empty()) {
			recipe.entryConditions.rules.push_back(rule);
			LOG_DEBUG("Entry rule: " + rule.indicator + " " + rule.operatorStr + " " + std::to_string(rule.value));
		}
	}

	// Parse exit conditions
	recipe.exitConditions.logic = parser.getString("exit_conditions.logic", "OR");

	size_t exitRuleCount = parser.getArraySize("exit_conditions.rules");
	LOG_INFO("Found " + std::to_string(exitRuleCount) + " exit rules");

	for (size_t i = 0; i < exitRuleCount; i++) {
		TradingRule rule;
		rule.indicator = parser.getArrayObjectString("exit_conditions.rules", i, "indicator", "");
		rule.operatorStr = parser.getArrayObjectString("exit_conditions.rules", i, "operator", "");
		rule.value = parser.getArrayObjectDouble("exit_conditions.rules", i, "value", 0.0);
		rule.compareWith = parser.getArrayObjectString("exit_conditions.rules", i, "compare_with", "");

		if (!rule.indicator.empty() && !rule.operatorStr.empty()) {
			recipe.exitConditions.rules.push_back(rule);
			LOG_DEBUG("Exit rule: " + rule.indicator + " " + rule.operatorStr + " " + std::to_string(rule.value));
		}
	}

	// Validation
	if (recipe.entryConditions.rules.empty()) {
		lastError = "No entry conditions defined";
		LOG_WARNING(lastError);
	}

	if (recipe.exitConditions.rules.empty()) {
		lastError = "No exit conditions defined";
		LOG_WARNING(lastError);
	}

	LOG_INFO("Successfully loaded recipe: " + recipe.name);
	return true;
}

// Save recipe to JSON file
bool RecipeLoader::saveToFile(const std::string& filename, const Recipe& recipe) {
	std::ofstream file(filename);
	if (!file.is_open()) {
		lastError = "Failed to open file for writing: " + filename;
		LOG_ERROR(lastError);
		return false;
	}

	// Build JSON string manually (since we don't have a JSON writer yet)
	file << "{\n";
	file << "  \"name\": \"" << recipe.name << "\",\n";
	file << "  \"description\": \"" << recipe.description << "\",\n";
	file << "  \"market\": {\n";
	file << "    \"exchange\": \"" << recipe.market.exchange << "\",\n";
	file << "    \"symbol\": \"" << recipe.market.symbol << "\",\n";
	file << "    \"timeframe\": \"" << recipe.market.timeframe << "\"\n";
	file << "  },\n";
	file << "  \"capital\": {\n";
	file << "    \"initial\": " << recipe.capital.initial << ",\n";
	file << "    \"position_size_percent\": " << recipe.capital.positionSizePercent << "\n";
	file << "  },\n";
	file << "  \"risk_management\": {\n";
	file << "    \"stop_loss_percent\": " << recipe.risk.stopLossPercent << ",\n";
	file << "    \"take_profit_percent\": " << recipe.risk.takeProfitPercent << ",\n";
	file << "    \"max_daily_loss_percent\": " << recipe.risk.maxDailyLossPercent << ",\n";
	file << "    \"max_open_positions\": " << recipe.risk.maxOpenPositions << "\n";
	file << "  },\n";
	file << "  \"indicators\": [\n";

	for (size_t i = 0; i < recipe.indicators.size(); i++) {
		const auto& ind = recipe.indicators[i];
		file << "    {\n";
		file << "      \"name\": \"" << ind.name << "\",\n";
		file << "      \"period\": " << ind.period;

		// Add params if any
		for (const auto& [key, value] : ind.params) {
			file << ",\n      \"" << key << "\": " << value;
		}

		file << "\n    }";
		if (i < recipe.indicators.size() - 1) file << ",";
		file << "\n";
	}

	file << "  ],\n";
	file << "  \"entry_conditions\": {\n";
	file << "    \"logic\": \"" << recipe.entryConditions.logic << "\",\n";
	file << "    \"rules\": [\n";

	for (size_t i = 0; i < recipe.entryConditions.rules.size(); i++) {
		const auto& rule = recipe.entryConditions.rules[i];
		file << "      {\n";
		file << "        \"indicator\": \"" << rule.indicator << "\",\n";
		file << "        \"operator\": \"" << rule.operatorStr << "\",\n";
		file << "        \"value\": " << rule.value << "\n";
		file << "      }";
		if (i < recipe.entryConditions.rules.size() - 1) file << ",";
		file << "\n";
	}

	file << "    ]\n";
	file << "  },\n";
	file << "  \"exit_conditions\": {\n";
	file << "    \"logic\": \"" << recipe.exitConditions.logic << "\",\n";
	file << "    \"rules\": [\n";

	for (size_t i = 0; i < recipe.exitConditions.rules.size(); i++) {
		const auto& rule = recipe.exitConditions.rules[i];
		file << "      {\n";
		file << "        \"indicator\": \"" << rule.indicator << "\",\n";
		file << "        \"operator\": \"" << rule.operatorStr << "\",\n";
		file << "        \"value\": " << rule.value << "\n";
		file << "      }";
		if (i < recipe.exitConditions.rules.size() - 1) file << ",";
		file << "\n";
	}

	file << "    ]\n";
	file << "  }\n";
	file << "}\n";

	file.close();

	LOG_INFO("Recipe saved to: " + filename);
	return true;
}

} // namespace Emiglio
