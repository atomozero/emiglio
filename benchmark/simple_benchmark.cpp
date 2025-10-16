// Emiglio Trading Bot - Simple Benchmark Suite
// Measures performance of critical indicator and database functions

#include "../src/strategy/Indicators.h"
#include "../src/data/DataStorage.h"
#include "../src/utils/Logger.h"

#include <iostream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>

using namespace Emiglio;
using namespace std::chrono;

// Benchmark timer helper
class BenchmarkTimer {
public:
	BenchmarkTimer(const std::string& name) : name(name) {
		start = high_resolution_clock::now();
	}

	double elapsed() {
		auto end = high_resolution_clock::now();
		auto duration = duration_cast<microseconds>(end - start);
		return duration.count();
	}

	void print() {
		std::cout << std::setw(45) << std::left << name
		          << std::setw(12) << std::right << std::fixed << std::setprecision(0)
		          << elapsed() << " μs" << std::endl;
	}

private:
	std::string name;
	high_resolution_clock::time_point start;
};

// Generate synthetic candle data for testing
std::vector<Candle> generateTestCandles(int count, double basePrice = 50000.0) {
	std::vector<Candle> candles;
	candles.reserve(count);

	time_t timestamp = std::time(nullptr) - (count * 3600);

	for (int i = 0; i < count; i++) {
		Candle c;
		c.timestamp = timestamp + (i * 3600);
		c.open = basePrice + (rand() % 1000 - 500);
		c.high = c.open + (rand() % 500);
		c.low = c.open - (rand() % 500);
		c.close = c.open + (rand() % 200 - 100);
		c.volume = 1000000.0 + (rand() % 5000000);
		c.symbol = "BTCUSDT";
		c.exchange = "binance";
		c.timeframe = "1h";

		candles.push_back(c);
	}

	return candles;
}

// Benchmark: Indicator calculations
void benchmarkIndicators() {
	std::cout << "\n=== Indicator Benchmarks ===" << std::endl;
	std::cout << std::setw(45) << std::left << "Operation"
	          << std::setw(12) << std::right << "Time" << std::endl;
	std::cout << std::string(57, '-') << std::endl;

	// Test with different data sizes
	for (int size : {1000, 5000, 10000}) {
		std::vector<Candle> candles = generateTestCandles(size);
		std::vector<double> closes = Indicators::getClosePrices(candles);

		std::string sizeStr = std::to_string(size) + " candles";

		// Benchmark SMA
		{
			BenchmarkTimer timer("  SMA(20) - " + sizeStr);
			auto result = Indicators::sma(closes, 20);
			timer.print();
		}

		// Benchmark EMA
		{
			BenchmarkTimer timer("  EMA(20) - " + sizeStr);
			auto result = Indicators::ema(closes, 20);
			timer.print();
		}

		// Benchmark RSI
		{
			BenchmarkTimer timer("  RSI(14) - " + sizeStr);
			auto result = Indicators::rsi(closes, 14);
			timer.print();
		}

		// Benchmark MACD
		{
			BenchmarkTimer timer("  MACD(12,26,9) - " + sizeStr);
			auto result = Indicators::macd(closes, 12, 26, 9);
			timer.print();
		}

		// Benchmark Bollinger Bands
		{
			BenchmarkTimer timer("  Bollinger(20,2) - " + sizeStr);
			auto result = Indicators::bollingerBands(closes, 20, 2.0);
			timer.print();
		}

		std::cout << std::endl;
	}
}

// Benchmark: Database operations
void benchmarkDatabase() {
	std::cout << "\n=== Database Benchmarks ===" << std::endl;
	std::cout << std::setw(45) << std::left << "Operation"
	          << std::setw(12) << std::right << "Time" << std::endl;
	std::cout << std::string(57, '-') << std::endl;

	DataStorage storage;
	if (!storage.init("/tmp/benchmark_test.db")) {
		std::cout << "Failed to init database" << std::endl;
		return;
	}

	// Test with different batch sizes
	for (int size : {100, 500, 1000, 5000}) {
		std::vector<Candle> candles = generateTestCandles(size);
		std::string sizeStr = std::to_string(size) + " candles";

		// Benchmark: Insert candles
		{
			BenchmarkTimer timer("  Insert " + sizeStr);
			storage.insertCandles(candles);
			timer.print();
		}

		// Benchmark: Query candles
		{
			BenchmarkTimer timer("  Query " + sizeStr);
			auto result = storage.getCandles("binance", "BTCUSDT", "1h", 0, std::time(nullptr));
			timer.print();
		}

		// Clear for next test
		// (No delete method, so we'll just keep adding)
	}

	// Benchmark: Count candles
	{
		BenchmarkTimer timer("  Count all candles");
		int count = storage.getCandleCount("binance", "BTCUSDT", "1h");
		timer.print();
		std::cout << "  Total candles in DB: " << count << std::endl;
	}

	// Cleanup
	std::remove("/tmp/benchmark_test.db");
}

// Calculate throughput metrics
void calculateThroughput() {
	std::cout << "\n=== Throughput Analysis ===" << std::endl;
	std::cout << std::string(57, '-') << std::endl;

	// Test indicator throughput
	{
		int candleCount = 10000;
		std::vector<Candle> candles = generateTestCandles(candleCount);
		std::vector<double> closes = Indicators::getClosePrices(candles);

		BenchmarkTimer timer("Processing 10k candles");
		auto sma20 = Indicators::sma(closes, 20);
		auto ema20 = Indicators::ema(closes, 20);
		auto rsi14 = Indicators::rsi(closes, 14);
		auto macd = Indicators::macd(closes, 12, 26, 9);
		auto bb = Indicators::bollingerBands(closes, 20, 2.0);

		double totalTime = timer.elapsed();
		double candlesPerSecond = (candleCount * 5) / (totalTime / 1000000.0); // 5 indicators

		std::cout << "  Total time for 5 indicators: "
		          << std::fixed << std::setprecision(2) << (totalTime / 1000.0) << " ms" << std::endl;
		std::cout << "  Throughput: "
		          << std::fixed << std::setprecision(0) << candlesPerSecond << " candles/sec" << std::endl;
	}
}

// Main benchmark runner
int main() {
	std::cout << "\n";
	std::cout << "╔═══════════════════════════════════════════════════════════╗" << std::endl;
	std::cout << "║       EMIGLIO TRADING BOT - BENCHMARK SUITE                ║" << std::endl;
	std::cout << "╚═══════════════════════════════════════════════════════════╝" << std::endl;

	// Initialize random seed
	srand(time(nullptr));

	// Run all benchmarks
	benchmarkIndicators();
	benchmarkDatabase();
	calculateThroughput();

	std::cout << "\n";
	std::cout << "╔═══════════════════════════════════════════════════════════╗" << std::endl;
	std::cout << "║              BENCHMARKS COMPLETED                         ║" << std::endl;
	std::cout << "╚═══════════════════════════════════════════════════════════╝" << std::endl;
	std::cout << "\n";

	return 0;
}
