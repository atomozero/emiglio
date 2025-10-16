// Emiglio Trading Bot - Benchmark Suite
// Measures performance of critical functions

#include "../src/strategy/Indicators.h"
#include "../src/backtest/BacktestSimulator.h"
#include "../src/backtest/Portfolio.h"
#include "../src/data/DataStorage.h"
#include "../src/exchange/BinanceAPI.h"
#include "../src/exchange/BinanceWebSocket.h"
#include "../src/utils/Logger.h"

#include <iostream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <cmath>

using namespace Emiglio;
using namespace std::chrono;

// Benchmark timer helper
class BenchmarkTimer {
public:
	BenchmarkTimer(const std::string& name) : name(name) {
		start = high_resolution_clock::now();
	}

	~BenchmarkTimer() {
		auto end = high_resolution_clock::now();
		auto duration = duration_cast<microseconds>(end - start);

		std::cout << std::setw(40) << std::left << name
		          << std::setw(15) << std::right << duration.count() << " μs" << std::endl;
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
	std::cout << std::setw(40) << std::left << "Operation"
	          << std::setw(15) << std::right << "Time" << std::endl;
	std::cout << std::string(55, '-') << std::endl;

	// Generate test data
	std::vector<Candle> candles = generateTestCandles(10000);
	std::vector<double> closes = Indicators::getClosePrices(candles);

	// Benchmark SMA
	{
		BenchmarkTimer timer("SMA(20) - 10k candles");
		auto result = Indicators::sma(closes, 20);
	}

	{
		BenchmarkTimer timer("SMA(200) - 10k candles");
		auto result = Indicators::sma(closes, 200);
	}

	// Benchmark EMA
	{
		BenchmarkTimer timer("EMA(20) - 10k candles");
		auto result = Indicators::ema(closes, 20);
	}

	{
		BenchmarkTimer timer("EMA(200) - 10k candles");
		auto result = Indicators::ema(closes, 200);
	}

	// Benchmark RSI
	{
		BenchmarkTimer timer("RSI(14) - 10k candles");
		auto result = Indicators::rsi(closes, 14);
	}

	// Benchmark MACD
	{
		BenchmarkTimer timer("MACD(12,26,9) - 10k candles");
		auto result = Indicators::macd(closes, 12, 26, 9);
	}

	// Benchmark Bollinger Bands
	{
		BenchmarkTimer timer("Bollinger(20,2) - 10k candles");
		auto result = Indicators::bollingerBands(closes, 20, 2.0);
	}

	// Benchmark ATR
	{
		BenchmarkTimer timer("ATR(14) - 10k candles");
		auto result = Indicators::atr(candles, 14);
	}

	// Benchmark Stochastic
	{
		BenchmarkTimer timer("Stochastic(14,3,3) - 10k candles");
		auto result = Indicators::stochastic(candles, 14, 3, 3);
	}
}

// Benchmark: Database operations
void benchmarkDatabase() {
	std::cout << "\n=== Database Benchmarks ===" << std::endl;
	std::cout << std::setw(40) << std::left << "Operation"
	          << std::setw(15) << std::right << "Time" << std::endl;
	std::cout << std::string(55, '-') << std::endl;

	DataStorage storage;
	if (!storage.init("/tmp/benchmark_test.db")) {
		std::cout << "Failed to init database" << std::endl;
		return;
	}

	std::vector<Candle> candles = generateTestCandles(1000);

	// Benchmark: Insert candles
	{
		BenchmarkTimer timer("Insert 1000 candles");
		storage.insertCandles(candles);
	}

	// Benchmark: Query candles
	{
		BenchmarkTimer timer("Query 1000 candles");
		auto result = storage.getCandles("binance", "BTCUSDT", "1h", 0, std::time(nullptr));
	}

	// Benchmark: Count candles
	{
		BenchmarkTimer timer("Count candles");
		int count = storage.getCandleCount("binance", "BTCUSDT", "1h");
	}

	// Cleanup
	std::remove("/tmp/benchmark_test.db");
}

// Benchmark: Portfolio operations
void benchmarkPortfolio() {
	std::cout << "\n=== Portfolio Benchmarks ===" << std::endl;
	std::cout << std::setw(40) << std::left << "Operation"
	          << std::setw(15) << std::right << "Time" << std::endl;
	std::cout << std::string(55, '-') << std::endl;

	Portfolio portfolio(10000.0); // $10k initial

	// Benchmark: Buy order
	{
		BenchmarkTimer timer("Execute buy order");
		portfolio.buy("BTCUSDT", 0.1, 50000.0, std::time(nullptr));
	}

	// Benchmark: Sell order
	{
		BenchmarkTimer timer("Execute sell order");
		portfolio.sell("BTCUSDT", 0.1, 51000.0, std::time(nullptr));
	}

	// Benchmark: Get position
	{
		BenchmarkTimer timer("Get position");
		auto pos = portfolio.getPosition("BTCUSDT");
	}

	// Benchmark: Calculate total value (100 positions)
	Portfolio bigPortfolio(100000.0);
	for (int i = 0; i < 100; i++) {
		bigPortfolio.buy("SYM" + std::to_string(i), 1.0, 100.0, std::time(nullptr));
	}

	{
		BenchmarkTimer timer("Calculate value (100 positions)");
		double value = bigPortfolio.getTotalValue();
	}
}

// Benchmark: Backtest simulation
void benchmarkBacktest() {
	std::cout << "\n=== Backtest Benchmarks ===" << std::endl;
	std::cout << std::setw(40) << std::left << "Operation"
	          << std::setw(15) << std::right << "Time" << std::endl;
	std::cout << std::string(55, '-') << std::endl;

	// Generate test data
	std::vector<Candle> candles = generateTestCandles(1000);

	Backtest::BacktestConfig config;
	config.initialCapital = 10000.0;
	config.symbol = "BTCUSDT";
	config.timeframe = "1h";
	config.startTime = candles.front().timestamp;
	config.endTime = candles.back().timestamp;
	config.stopLoss = 0.02;
	config.takeProfit = 0.05;

	Backtest::BacktestSimulator simulator;

	// Benchmark: Simple RSI strategy (1000 candles)
	{
		BenchmarkTimer timer("Backtest RSI (1000 candles)");

		// Simple RSI strategy
		for (size_t i = 20; i < candles.size(); i++) {
			std::vector<Candle> window(candles.begin(), candles.begin() + i);
			std::vector<double> closes = Indicators::getClosePrices(window);
			std::vector<double> rsi = Indicators::rsi(closes, 14);

			if (!rsi.empty() && rsi.back() < 30) {
				// Buy signal
			} else if (!rsi.empty() && rsi.back() > 70) {
				// Sell signal
			}
		}
	}
}

// Benchmark: WebSocket message processing
void benchmarkWebSocket() {
	std::cout << "\n=== WebSocket Benchmarks ===" << std::endl;
	std::cout << std::setw(40) << std::left << "Operation"
	          << std::setw(15) << std::right << "Time" << std::endl;
	std::cout << std::string(55, '-') << std::endl;

	BinanceWebSocket ws;

	// Benchmark: Connect
	{
		BenchmarkTimer timer("WebSocket connect");
		ws.connect();
	}

	// Benchmark: Subscribe to ticker
	{
		BenchmarkTimer timer("Subscribe to ticker");
		ws.subscribeTicker("BTCUSDT", [](const TickerUpdate& update) {
			// Callback
		});
	}

	// Benchmark: Subscribe to trades
	{
		BenchmarkTimer timer("Subscribe to trades");
		ws.subscribeTrades("BTCUSDT", [](const TradeUpdate& update) {
			// Callback
		});
	}

	// Let it run for a bit to test throughput
	std::cout << "\nRunning WebSocket for 3 seconds to measure throughput..." << std::endl;
	int tickerCount = 0;
	int tradeCount = 0;

	ws.subscribeTicker("ETHUSDT", [&tickerCount](const TickerUpdate& update) {
		tickerCount++;
	});

	ws.subscribeTrades("ETHUSDT", [&tradeCount](const TradeUpdate& update) {
		tradeCount++;
	});

	auto start = high_resolution_clock::now();
	while (duration_cast<seconds>(high_resolution_clock::now() - start).count() < 3) {
		// Let it run
		std::this_thread::sleep_for(milliseconds(100));
	}

	std::cout << "Received " << tickerCount << " ticker updates in 3s ("
	          << (tickerCount / 3.0) << " updates/sec)" << std::endl;
	std::cout << "Received " << tradeCount << " trade updates in 3s ("
	          << (tradeCount / 3.0) << " updates/sec)" << std::endl;

	ws.disconnect();
}

// Main benchmark runner
int main(int argc, char* argv[]) {
	std::cout << "\n";
	std::cout << "╔════════════════════════════════════════════════════════╗" << std::endl;
	std::cout << "║         EMIGLIO TRADING BOT - BENCHMARK SUITE          ║" << std::endl;
	std::cout << "╚════════════════════════════════════════════════════════╝" << std::endl;

	// Initialize random seed
	srand(time(nullptr));

	// Run all benchmarks
	benchmarkIndicators();
	benchmarkDatabase();
	benchmarkPortfolio();
	benchmarkBacktest();
	benchmarkWebSocket();

	std::cout << "\n";
	std::cout << "╔════════════════════════════════════════════════════════╗" << std::endl;
	std::cout << "║              BENCHMARKS COMPLETED                      ║" << std::endl;
	std::cout << "╚════════════════════════════════════════════════════════╝" << std::endl;
	std::cout << "\n";

	return 0;
}
