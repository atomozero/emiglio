#include "../src/exchange/BinanceAPI.h"
#include "../src/data/DataStorage.h"
#include "../src/utils/Logger.h"
#include <Application.h>
#include <iostream>
#include <ctime>
#include <OS.h>

using namespace Emiglio;

bool DownloadData(BinanceAPI& api, DataStorage& storage,
                  const std::string& symbol, const std::string& interval,
                  time_t startTime, time_t endTime) {
	LOG_INFO("Downloading " + symbol + " data from Binance (" + interval + ")");
	LOG_INFO("  Period: " + std::to_string(startTime) + " to " + std::to_string(endTime));

	// Binance limits to 1000 candles per request
	const int LIMIT = 1000;
	time_t currentStart = startTime;
	int totalCandles = 0;

	// Calculate interval duration in seconds
	int intervalSeconds = 3600; // Default 1h
	if (interval == "1m") intervalSeconds = 60;
	else if (interval == "5m") intervalSeconds = 300;
	else if (interval == "15m") intervalSeconds = 900;
	else if (interval == "1h") intervalSeconds = 3600;
	else if (interval == "4h") intervalSeconds = 14400;
	else if (interval == "1d") intervalSeconds = 86400;

	while (currentStart < endTime) {
		LOG_INFO("Fetching batch starting at " + std::to_string(currentStart));

		// Calculate batch end time
		time_t batchEnd = currentStart + (LIMIT * intervalSeconds);
		if (batchEnd > endTime) batchEnd = endTime;

		// Fetch data using BinanceAPI
		std::vector<Candle> candles = api.getCandles(symbol, interval,
		                                               currentStart, batchEnd, LIMIT);

		if (candles.empty()) {
			LOG_WARNING("No more data available");
			break;
		}

		LOG_INFO("Received " + std::to_string(candles.size()) + " candles");

		// Set exchange and timeframe (BinanceAPI doesn't set these)
		for (auto& candle : candles) {
			candle.exchange = "binance";
			candle.timeframe = interval;
		}

		// Insert into database
		if (!storage.insertCandles(candles)) {
			LOG_ERROR("Failed to insert candles into database");
			return false;
		}

		totalCandles += candles.size();

		// Update start time for next batch
		currentStart = candles.back().timestamp + intervalSeconds;

		// Check if we got less than requested (end of available data)
		if (candles.size() < static_cast<size_t>(LIMIT)) {
			LOG_INFO("Reached end of available data");
			break;
		}

		// Rate limiting - sleep 100ms between requests
		snooze(100000); // 100ms in microseconds
	}

	LOG_INFO("Downloaded total of " + std::to_string(totalCandles) + " candles");

	return totalCandles > 0;
}

int main(int argc, char* argv[]) {
	// Initialize logger to stdout
	Logger::getInstance().setLogLevel(LogLevel::INFO);

	// Parse arguments
	std::string symbol = "BTCUSDT";
	std::string interval = "1h";
	int days = 30; // Default 30 days

	if (argc > 1) symbol = argv[1];
	if (argc > 2) interval = argv[2];
	if (argc > 3) days = std::stoi(argv[3]);

#ifdef __HAIKU__
	// Create BApplication instance (required for NetServices2)
	// But don't call Run() - we'll execute synchronously
	BApplication app("application/x-vnd.Emiglio-DataImporter");
#endif

	LOG_INFO("=== Binance Data Importer ===");
	LOG_INFO("Configuration:");
	LOG_INFO("  Symbol: " + symbol);
	LOG_INFO("  Interval: " + interval);
	LOG_INFO("  Days: " + std::to_string(days));

	// Calculate time range
	time_t endTime = std::time(nullptr);
	time_t startTime = endTime - (days * 24 * 3600);

	// Initialize BinanceAPI
	LOG_INFO("Initializing Binance API...");
	BinanceAPI api;
	if (!api.init("", "")) {
		LOG_ERROR("Failed to initialize Binance API");
		return 1;
	}

	// Test connection
	LOG_INFO("Testing connection...");
	if (!api.ping()) {
		LOG_ERROR("Failed to connect to Binance API");
		return 1;
	}
	LOG_INFO("Connection OK");

	// Initialize database
	DataStorage storage;
	if (!storage.init("/boot/home/Emiglio/data/emilio.db")) {
		LOG_ERROR("Failed to initialize database");
		return 1;
	}

	// Clear existing data
	LOG_INFO("Clearing existing data for " + symbol);
	storage.clearCandles("binance", symbol, interval);

	// Download data
	if (!DownloadData(api, storage, symbol, interval, startTime, endTime)) {
		LOG_ERROR("Failed to download data");
		return 1;
	}

	// Verify
	int count = storage.getCandleCount("binance", symbol, interval);
	LOG_INFO("Database now contains " + std::to_string(count) + " candles for " + symbol);

	// Get sample
	auto candles = storage.getCandles("binance", symbol, interval, startTime, endTime);
	if (!candles.empty()) {
		LOG_INFO("Sample data:");
		LOG_INFO("  First: " + std::to_string(candles.front().close) +
		         " at " + std::to_string(candles.front().timestamp));
		LOG_INFO("  Last: " + std::to_string(candles.back().close) +
		         " at " + std::to_string(candles.back().timestamp));
	}

	LOG_INFO("=== Import complete! ===");
	LOG_INFO("You can now run backtests with real Binance data.");

	return 0;
}
