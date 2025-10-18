#include "DataSyncManager.h"
#include "Logger.h"
#include "Config.h"
#include "../data/DataStorage.h"
#include "../exchange/BinanceAPI.h"

#include <ctime>
#include <algorithm>
#include <unistd.h>

namespace Emiglio {

DataSyncManager::DataSyncManager()
	: progressCallback(nullptr)
{
}

DataSyncManager::~DataSyncManager() {
}

DataSyncManager& DataSyncManager::getInstance() {
	static DataSyncManager instance;
	return instance;
}

void DataSyncManager::setProgressCallback(
	std::function<void(int, int, const std::string&)> callback) {
	progressCallback = callback;
}

std::vector<std::string> DataSyncManager::getSymbolsNeedingSync() {
	// Get list of popular trading pairs
	std::vector<std::string> symbols = {
		"BTCUSDT", "ETHUSDT", "BNBUSDT", "SOLUSDT", "XRPUSDT",
		"ADAUSDT", "DOGEUSDT", "MATICUSDT", "DOTUSDT", "AVAXUSDT"
	};

	// Add user's preferred quote currency pairs
	Config& config = Config::getInstance();
	std::string preferredQuote = config.getPreferredQuote();

	if (preferredQuote != "USDT") {
		std::vector<std::string> bases = {"BTC", "ETH", "BNB", "SOL", "XRP"};
		for (const auto& base : bases) {
			std::string symbol = base + preferredQuote;
			// Only add if not already in list
			if (std::find(symbols.begin(), symbols.end(), symbol) == symbols.end()) {
				symbols.push_back(symbol);
			}
		}
	}

	return symbols;
}

int64_t DataSyncManager::getLastTimestamp(const std::string& exchange,
                                           const std::string& symbol,
                                           const std::string& timeframe) {
	DataStorage storage;
	if (!storage.init("/boot/home/Emiglio/data/emilio.db")) {
		LOG_ERROR("Failed to initialize storage for timestamp check");
		return 0;
	}

	// Get the most recent candle by requesting from 30 days ago to now
	time_t now = time(nullptr);
	time_t thirtyDaysAgo = now - (30 * 24 * 60 * 60);

	std::vector<Candle> candles = storage.getCandles(exchange, symbol, timeframe,
	                                                   thirtyDaysAgo, now);
	if (candles.empty()) {
		// No data exists, start from 30 days ago
		return thirtyDaysAgo;
	}

	// Return the timestamp of the latest candle + 1 hour to continue from next candle
	return candles.back().timestamp + 3600;  // timestamp is in seconds, add 1 hour
}

bool DataSyncManager::downloadRange(const std::string& exchange,
                                     const std::string& symbol,
                                     const std::string& timeframe,
                                     int64_t startTime,
                                     int64_t endTime) {
	if (exchange != "binance") {
		LOG_WARNING("Only Binance exchange is currently supported for sync");
		return false;
	}

	BinanceAPI api;
	if (!api.init("", "")) {
		LOG_ERROR("Failed to initialize Binance API for sync");
		return false;
	}

	DataStorage storage;
	if (!storage.init("/boot/home/Emiglio/data/emilio.db")) {
		LOG_ERROR("Failed to initialize storage for sync");
		return false;
	}

	// Download in chunks (max 1000 candles per request)
	int64_t chunkSize = 1000;
	int64_t timeframeMs = 3600000;  // 1h default

	// Convert timeframe to milliseconds
	if (timeframe == "1m") timeframeMs = 60000;
	else if (timeframe == "5m") timeframeMs = 300000;
	else if (timeframe == "15m") timeframeMs = 900000;
	else if (timeframe == "1h") timeframeMs = 3600000;
	else if (timeframe == "4h") timeframeMs = 14400000;
	else if (timeframe == "1d") timeframeMs = 86400000;

	int64_t currentStart = startTime;
	int totalDownloaded = 0;

	while (currentStart < endTime) {
		int64_t currentEnd = std::min(currentStart + (chunkSize * timeframeMs), endTime);

		LOG_INFO("Downloading " + symbol + " " + timeframe + " from " +
		         std::to_string(currentStart) + " to " + std::to_string(currentEnd));

		std::vector<Candle> candles = api.getCandles(
			symbol, timeframe, currentStart, currentEnd, chunkSize);

		if (candles.empty()) {
			LOG_WARNING("No candles received for " + symbol + " " + timeframe);
			break;
		}

		// Store candles using insertCandles
		if (!storage.insertCandles(candles)) {
			LOG_WARNING("Failed to store some candles for " + symbol + " " + timeframe);
		}

		totalDownloaded += candles.size();

		if (progressCallback) {
			progressCallback(totalDownloaded, -1, symbol + " " + timeframe);
		}

		// Move to next chunk
		currentStart = candles.back().timestamp + 1;  // timestamp is in seconds

		// Small delay to avoid rate limiting
		usleep(100000);  // 100ms
	}

	LOG_INFO("Downloaded " + std::to_string(totalDownloaded) + " candles for " +
	         symbol + " " + timeframe);

	return totalDownloaded > 0;
}

bool DataSyncManager::syncSymbol(const std::string& exchange,
                                  const std::string& symbol,
                                  const std::string& timeframe) {
	LOG_INFO("Syncing " + symbol + " " + timeframe + " from " + exchange);

	int64_t lastTimestamp = getLastTimestamp(exchange, symbol, timeframe);
	int64_t currentTime = time(nullptr);  // Current time in seconds

	if (lastTimestamp >= currentTime - 3600) {  // If less than 1 hour old
		LOG_INFO(symbol + " " + timeframe + " is already up to date");
		return true;
	}

	return downloadRange(exchange, symbol, timeframe, lastTimestamp, currentTime);
}

bool DataSyncManager::syncAllData() {
	LOG_INFO("Starting full data sync...");

	std::vector<std::string> symbols = getSymbolsNeedingSync();
	std::vector<std::string> timeframes = {"1h", "4h", "1d"};

	int totalSymbols = symbols.size() * timeframes.size();
	int completed = 0;

	for (const auto& symbol : symbols) {
		for (const auto& timeframe : timeframes) {
			completed++;

			if (progressCallback) {
				progressCallback(completed, totalSymbols,
				                "Syncing " + symbol + " " + timeframe);
			}

			if (!syncSymbol("binance", symbol, timeframe)) {
				LOG_WARNING("Failed to sync " + symbol + " " + timeframe);
			}

			// Small delay between symbols
			usleep(200000);  // 200ms
		}
	}

	LOG_INFO("Data sync completed: " + std::to_string(completed) + " symbol/timeframe pairs processed");
	return true;
}

} // namespace Emiglio
