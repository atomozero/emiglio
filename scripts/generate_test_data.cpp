#include "../src/data/DataStorage.h"
#include "../src/utils/Logger.h"
#include <cmath>
#include <iostream>

using namespace Emiglio;

// Generate synthetic OHLCV data with realistic price movement
std::vector<Candle> generateSyntheticData(int numCandles, double startPrice) {
	std::vector<Candle> candles;

	time_t baseTime = 1609459200; // 2021-01-01 00:00:00
	double price = startPrice;

	for (int i = 0; i < numCandles; i++) {
		Candle c;
		c.exchange = "binance";
		c.symbol = "BTCUSDT";
		c.timeframe = "1h";
		c.timestamp = baseTime + (i * 3600); // 1 hour intervals

		// Generate price movement with:
		// - Trend (sine wave)
		// - Volatility (random noise)
		// - Mean reversion
		double trendFactor = std::sin(i * 0.02) * 500.0; // Long-term trend
		double volatility = (rand() % 1000 - 500) * 0.5; // Random noise
		double meanReversion = (startPrice - price) * 0.01; // Pull back to mean

		double priceChange = trendFactor + volatility + meanReversion;
		price += priceChange;

		// Ensure price stays positive
		if (price < startPrice * 0.5) price = startPrice * 0.5;
		if (price > startPrice * 2.0) price = startPrice * 2.0;

		// Generate OHLC from price
		double volatilityPercent = 0.005; // 0.5% intrabar volatility
		c.open = price;
		c.high = price * (1.0 + volatilityPercent + (rand() % 100) * 0.0001);
		c.low = price * (1.0 - volatilityPercent - (rand() % 100) * 0.0001);
		c.close = price + (rand() % 200 - 100) * 0.5;
		c.volume = 10000.0 + (rand() % 50000);

		candles.push_back(c);
	}

	return candles;
}

int main() {
	// Initialize logger
	Logger::getInstance().setLogLevel(LogLevel::INFO);

	LOG_INFO("=== Emiglio Test Data Generator ===");

	// Initialize database
	DataStorage storage;
	if (!storage.init("/boot/home/Emiglio/data/emilio.db")) {
		LOG_ERROR("Failed to initialize database");
		return 1;
	}

	// Generate data
	LOG_INFO("Generating synthetic BTCUSDT data...");

	int numCandles = 2000; // ~83 days of hourly data
	double startPrice = 50000.0; // Start at $50k

	std::vector<Candle> candles = generateSyntheticData(numCandles, startPrice);

	LOG_INFO("Generated " + std::to_string(candles.size()) + " candles");

	// Insert into database
	LOG_INFO("Inserting into database...");

	if (!storage.insertCandles(candles)) {
		LOG_ERROR("Failed to insert candles");
		return 1;
	}

	LOG_INFO("Successfully inserted " + std::to_string(candles.size()) + " candles");

	// Verify
	int count = storage.getCandleCount("binance", "BTCUSDT", "1h");
	LOG_INFO("Database now contains " + std::to_string(count) + " BTCUSDT candles");

	// Get first and last candles
	time_t startTime = candles.front().timestamp;
	time_t endTime = candles.back().timestamp;

	auto retrieved = storage.getCandles("binance", "BTCUSDT", "1h", startTime, endTime);

	if (!retrieved.empty()) {
		LOG_INFO("Verification successful:");
		LOG_INFO("  First candle: " + std::to_string(retrieved.front().close) +
		         " at timestamp " + std::to_string(retrieved.front().timestamp));
		LOG_INFO("  Last candle: " + std::to_string(retrieved.back().close) +
		         " at timestamp " + std::to_string(retrieved.back().timestamp));
		LOG_INFO("  Price range: $" + std::to_string(retrieved.front().close) +
		         " - $" + std::to_string(retrieved.back().close));
	}

	LOG_INFO("=== Test data generation complete! ===");
	LOG_INFO("You can now run backtests from the UI using this data.");

	return 0;
}
