#include "../src/exchange/BinanceAPI.h"
#include "../src/data/DataStorage.h"
#include "../src/utils/Logger.h"
#include <Application.h>
#include <iostream>
#include <chrono>
#include <iomanip>

using namespace Emiglio;
using namespace std::chrono;

void printSeparator() {
	std::cout << std::string(80, '=') << std::endl;
}

bool testDataStorage(const std::string& dbPath) {
	printSeparator();
	std::cout << "TEST 1: DataStorage" << std::endl;
	printSeparator();

	auto start = high_resolution_clock::now();

	DataStorage storage;
	if (!storage.init(dbPath)) {
		std::cout << "[FAIL] Failed to initialize DataStorage" << std::endl;
		return false;
	}
	std::cout << "[OK] DataStorage initialized" << std::endl;

	// Test candle count
	int btcCount = storage.getCandleCount("binance", "BTCUSDT", "1h");
	int ethCount = storage.getCandleCount("binance", "ETHUSDT", "4h");
	std::cout << "[OK] BTCUSDT 1h candles: " << btcCount << std::endl;
	std::cout << "[OK] ETHUSDT 4h candles: " << ethCount << std::endl;

	// Test candle retrieval
	time_t endTime = std::time(nullptr);
	time_t startTime = endTime - (7 * 24 * 3600); // Last 7 days

	auto candles = storage.getCandles("binance", "BTCUSDT", "1h", startTime, endTime);
	std::cout << "[OK] Retrieved " << candles.size() << " BTCUSDT candles" << std::endl;

	if (!candles.empty()) {
		std::cout << "    First: $" << std::fixed << std::setprecision(2)
		          << candles.front().close << " (timestamp: " << candles.front().timestamp << ")" << std::endl;
		std::cout << "    Last:  $" << candles.back().close << " (timestamp: " << candles.back().timestamp << ")" << std::endl;
		std::cout << "    Average price: $" << std::fixed << std::setprecision(2);

		double avgPrice = 0.0;
		for (const auto& candle : candles) {
			avgPrice += candle.close;
		}
		avgPrice /= candles.size();
		std::cout << avgPrice << std::endl;
	}

	// Test data integrity
	bool dataOK = true;
	for (const auto& candle : candles) {
		if (candle.high < candle.low || candle.high < candle.open ||
		    candle.high < candle.close || candle.low > candle.open ||
		    candle.low > candle.close || candle.volume < 0) {
			dataOK = false;
			std::cout << "[FAIL] Data integrity check failed!" << std::endl;
			break;
		}
	}
	if (dataOK) {
		std::cout << "[OK] Data integrity check passed" << std::endl;
	}

	auto end = high_resolution_clock::now();
	auto duration = duration_cast<milliseconds>(end - start);
	std::cout << "[BENCHMARK] DataStorage test: " << duration.count() << " ms" << std::endl;

	return btcCount > 0 && dataOK;
}

bool testBinanceAPI() {
	printSeparator();
	std::cout << "TEST 2: BinanceAPI (curl-based implementation)" << std::endl;
	printSeparator();

	auto start = high_resolution_clock::now();

	BinanceAPI api;
	if (!api.init("", "")) {
		std::cout << "[FAIL] Failed to initialize BinanceAPI" << std::endl;
		return false;
	}
	std::cout << "[OK] BinanceAPI initialized" << std::endl;

	// Test ping
	auto pingStart = high_resolution_clock::now();
	if (!api.ping()) {
		std::cout << "[FAIL] Ping failed" << std::endl;
		return false;
	}
	auto pingEnd = high_resolution_clock::now();
	auto pingDuration = duration_cast<milliseconds>(pingEnd - pingStart);
	std::cout << "[OK] Ping successful (" << pingDuration.count() << " ms)" << std::endl;

	// Test small data fetch
	time_t endTime = std::time(nullptr);
	time_t startTime = endTime - (6 * 3600); // Last 6 hours

	auto fetchStart = high_resolution_clock::now();
	auto candles = api.getCandles("BTCUSDT", "1h", startTime, endTime, 10);
	auto fetchEnd = high_resolution_clock::now();
	auto fetchDuration = duration_cast<milliseconds>(fetchEnd - fetchStart);

	std::cout << "[OK] Fetched " << candles.size() << " candles (" << fetchDuration.count() << " ms)" << std::endl;

	if (!candles.empty()) {
		std::cout << "    Latest price: $" << std::fixed << std::setprecision(2)
		          << candles.back().close << std::endl;
		std::cout << "    Latest volume: " << std::fixed << std::setprecision(4)
		          << candles.back().volume << " BTC" << std::endl;
		std::cout << "    High: $" << candles.back().high << std::endl;
		std::cout << "    Low:  $" << candles.back().low << std::endl;
	}

	// Test multiple symbols
	std::cout << "\n[INFO] Testing multiple symbols..." << std::endl;
	std::vector<std::string> symbols = {"ETHUSDT", "BNBUSDT", "ADAUSDT"};
	int successCount = 0;

	auto multiStart = high_resolution_clock::now();
	for (const auto& symbol : symbols) {
		auto symCandles = api.getCandles(symbol, "1h", startTime, endTime, 5);
		if (!symCandles.empty()) {
			std::cout << "    " << symbol << ": $" << std::fixed << std::setprecision(2)
			          << symCandles.back().close << std::endl;
			successCount++;
		}
	}
	auto multiEnd = high_resolution_clock::now();
	auto multiDuration = duration_cast<milliseconds>(multiEnd - multiStart);

	std::cout << "[OK] Multi-symbol test: " << successCount << "/" << symbols.size()
	          << " successful (" << multiDuration.count() << " ms)" << std::endl;

	auto end = high_resolution_clock::now();
	auto duration = duration_cast<milliseconds>(end - start);
	std::cout << "[BENCHMARK] BinanceAPI total: " << duration.count() << " ms" << std::endl;

	return candles.size() > 0 && successCount >= 2;
}

bool testDataImportPerformance() {
	printSeparator();
	std::cout << "TEST 3: Data Import Performance" << std::endl;
	printSeparator();

	auto start = high_resolution_clock::now();

	BinanceAPI api;
	if (!api.init("", "")) {
		std::cout << "[FAIL] Failed to initialize API" << std::endl;
		return false;
	}

	DataStorage storage;
	if (!storage.init("/boot/home/Emiglio/data/emilio.db")) {
		std::cout << "[FAIL] Failed to initialize storage" << std::endl;
		return false;
	}

	// Test importing 1 day of 15m data (96 candles)
	time_t endTime = std::time(nullptr);
	time_t startTime = endTime - (24 * 3600);

	std::cout << "[INFO] Importing 1 day of 15m BTCUSDT data..." << std::endl;

	auto fetchStart = high_resolution_clock::now();
	auto candles = api.getCandles("BTCUSDT", "15m", startTime, endTime, 100);
	auto fetchEnd = high_resolution_clock::now();
	auto fetchDuration = duration_cast<milliseconds>(fetchEnd - fetchStart);

	std::cout << "[OK] Fetched " << candles.size() << " candles in "
	          << fetchDuration.count() << " ms" << std::endl;
	std::cout << "    Fetch rate: " << std::fixed << std::setprecision(1)
	          << (candles.size() * 1000.0 / fetchDuration.count()) << " candles/sec" << std::endl;

	if (candles.empty()) {
		std::cout << "[FAIL] No candles fetched" << std::endl;
		return false;
	}

	// Test database insertion
	// Clear existing test data first
	storage.clearCandles("binance", "BTCUSDT", "15m");

	// Set exchange and timeframe
	for (auto& candle : candles) {
		candle.exchange = "binance";
		candle.timeframe = "15m";
	}

	auto insertStart = high_resolution_clock::now();
	bool inserted = storage.insertCandles(candles);
	auto insertEnd = high_resolution_clock::now();
	auto insertDuration = duration_cast<milliseconds>(insertEnd - insertStart);

	if (!inserted) {
		std::cout << "[FAIL] Failed to insert candles" << std::endl;
		return false;
	}

	std::cout << "[OK] Inserted " << candles.size() << " candles in "
	          << insertDuration.count() << " ms" << std::endl;
	std::cout << "    Insert rate: " << std::fixed << std::setprecision(1)
	          << (candles.size() * 1000.0 / insertDuration.count()) << " candles/sec" << std::endl;

	// Verify data
	int count = storage.getCandleCount("binance", "BTCUSDT", "15m");
	std::cout << "[OK] Verified " << count << " candles in database" << std::endl;

	auto end = high_resolution_clock::now();
	auto duration = duration_cast<milliseconds>(end - start);
	std::cout << "[BENCHMARK] Total import test: " << duration.count() << " ms" << std::endl;

	return count == static_cast<int>(candles.size());
}

bool testSystemIntegration() {
	printSeparator();
	std::cout << "TEST 4: System Integration" << std::endl;
	printSeparator();

	auto start = high_resolution_clock::now();

	std::cout << "[INFO] Testing full workflow: API -> DB -> Retrieval" << std::endl;

	// 1. Fetch from API
	BinanceAPI api;
	if (!api.init("", "")) {
		std::cout << "[FAIL] API init failed" << std::endl;
		return false;
	}

	time_t endTime = std::time(nullptr);
	time_t startTime = endTime - (12 * 3600); // 12 hours

	auto candles = api.getCandles("ETHUSDT", "1h", startTime, endTime, 12);
	std::cout << "[OK] Step 1: Fetched " << candles.size() << " candles from API" << std::endl;

	if (candles.empty()) {
		std::cout << "[FAIL] No data from API" << std::endl;
		return false;
	}

	// 2. Store in database
	DataStorage storage;
	if (!storage.init("/boot/home/Emiglio/data/emilio.db")) {
		std::cout << "[FAIL] Storage init failed" << std::endl;
		return false;
	}

	storage.clearCandles("binance", "ETHUSDT", "1h");

	for (auto& candle : candles) {
		candle.exchange = "binance";
		candle.timeframe = "1h";
	}

	if (!storage.insertCandles(candles)) {
		std::cout << "[FAIL] Insert failed" << std::endl;
		return false;
	}
	std::cout << "[OK] Step 2: Stored candles in database" << std::endl;

	// 3. Retrieve from database
	auto retrieved = storage.getCandles("binance", "ETHUSDT", "1h", startTime, endTime);
	std::cout << "[OK] Step 3: Retrieved " << retrieved.size() << " candles from database" << std::endl;

	// 4. Verify data integrity
	if (retrieved.size() != candles.size()) {
		std::cout << "[FAIL] Size mismatch: " << candles.size() << " vs " << retrieved.size() << std::endl;
		return false;
	}

	bool dataMatch = true;
	for (size_t i = 0; i < candles.size(); i++) {
		if (std::abs(candles[i].close - retrieved[i].close) > 0.01) {
			std::cout << "[FAIL] Data mismatch at index " << i << std::endl;
			dataMatch = false;
			break;
		}
	}

	if (dataMatch) {
		std::cout << "[OK] Step 4: Data integrity verified" << std::endl;
	}

	auto end = high_resolution_clock::now();
	auto duration = duration_cast<milliseconds>(end - start);
	std::cout << "[BENCHMARK] Integration test: " << duration.count() << " ms" << std::endl;

	return dataMatch;
}

int main() {
	// Initialize logger (minimal output)
	Logger::getInstance().setLogLevel(LogLevel::ERROR);

#ifdef __HAIKU__
	// Create BApplication instance for NetServices
	BApplication app("application/x-vnd.Emiglio-ComponentTest");
#endif

	std::cout << "\n";
	printSeparator();
	std::cout << "EMIGLIO TRADING SYSTEM - COMPONENT TEST & BENCHMARK SUITE" << std::endl;
	std::cout << "Version: Phase 5 (curl-based BinanceAPI)" << std::endl;
	printSeparator();
	std::cout << "\n";

	auto globalStart = high_resolution_clock::now();

	int passed = 0;
	int total = 4;

	// Run tests
	if (testDataStorage("/boot/home/Emiglio/data/emilio.db")) passed++;
	std::cout << "\n";

	if (testBinanceAPI()) passed++;
	std::cout << "\n";

	if (testDataImportPerformance()) passed++;
	std::cout << "\n";

	if (testSystemIntegration()) passed++;
	std::cout << "\n";

	// Summary
	auto globalEnd = high_resolution_clock::now();
	auto globalDuration = duration_cast<milliseconds>(globalEnd - globalStart);

	printSeparator();
	std::cout << "TEST SUMMARY" << std::endl;
	printSeparator();
	std::cout << "Tests passed: " << passed << "/" << total << std::endl;
	std::cout << "Total execution time: " << std::fixed << std::setprecision(2)
	          << (globalDuration.count() / 1000.0) << " seconds" << std::endl;

	if (passed == total) {
		std::cout << "\n" << std::string(80, '=') << std::endl;
		std::cout << "[SUCCESS] All component tests passed!" << std::endl;
		std::cout << "System is ready for production use." << std::endl;
		std::cout << std::string(80, '=') << std::endl;
	} else {
		std::cout << "\n[FAILURE] " << (total - passed) << " test(s) failed" << std::endl;
	}

	return (passed == total) ? 0 : 1;
}
