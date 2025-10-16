# Test Suite Makefile for Emiglio
# Build tests on Haiku OS only

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I.. -I../../external/rapidjson/include -I/boot/system/develop/headers/private/netservices2
LDFLAGS = -lsqlite3 -lpthread -lssl -lcrypto

# Test executables
TESTS = TestLogger TestConfig TestDataStorage TestBFSvsSQLite TestBinanceAPI TestIndicators TestRecipeLoader TestSignalGenerator TestBacktest

# Benchmarks
BENCHMARKS = BenchmarkPhase3 BenchmarkPhase4

# Common objects
COMMON_OBJS = \
	TestFramework.o \
	../utils/Logger.o \
	../utils/Config.o \
	../utils/JsonParser.o \
	../data/DataStorage.o

.PHONY: all clean run benchmarks

all: $(TESTS)

benchmarks: $(BENCHMARKS)

TestLogger: TestLogger.o TestFramework.o ../utils/Logger.o
	$(CXX) -o $@ $^ $(LDFLAGS)

TestConfig: TestConfig.o TestFramework.o ../utils/Logger.o ../utils/Config.o ../utils/JsonParser.o
	$(CXX) -o $@ $^ $(LDFLAGS)

TestDataStorage: TestDataStorage.o TestFramework.o ../utils/Logger.o ../data/DataStorage.o
	$(CXX) -o $@ $^ $(LDFLAGS)

TestBFSvsSQLite: TestBFSvsSQLite.o TestFramework.o ../utils/Logger.o ../data/DataStorage.o ../data/BFSStorage.o
	$(CXX) -o $@ $^ $(LDFLAGS) -lbe

TestBinanceAPI: TestBinanceAPI.o TestFramework.o ../utils/Logger.o ../utils/JsonParser.o ../exchange/BinanceAPI.o
	$(CXX) -o $@ $^ $(LDFLAGS) -lnetservices2 -lbnetapi -lnetwork -lbe

TestIndicators: TestIndicators.o TestFramework.o ../utils/Logger.o ../strategy/Indicators.o
	$(CXX) -o $@ $^ $(LDFLAGS)

TestRecipeLoader: TestRecipeLoader.o TestFramework.o ../utils/Logger.o ../utils/JsonParser.o ../strategy/RecipeLoader.o
	$(CXX) -o $@ $^ $(LDFLAGS)

TestSignalGenerator: TestSignalGenerator.o TestFramework.o ../utils/Logger.o ../utils/JsonParser.o ../strategy/Indicators.o ../strategy/RecipeLoader.o ../strategy/SignalGenerator.o
	$(CXX) -o $@ $^ $(LDFLAGS)

BenchmarkPhase3: BenchmarkPhase3.o TestFramework.o ../utils/Logger.o ../utils/JsonParser.o ../strategy/Indicators.o ../strategy/RecipeLoader.o ../strategy/SignalGenerator.o
	$(CXX) -o $@ $^ $(LDFLAGS)

BenchmarkPhase4: BenchmarkPhase4.o TestFramework.o ../utils/Logger.o ../utils/JsonParser.o ../strategy/Indicators.o ../strategy/RecipeLoader.o ../strategy/SignalGenerator.o ../backtest/Portfolio.o ../backtest/BacktestSimulator.o ../backtest/PerformanceAnalyzer.o ../data/DataStorage.o
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

../utils/%.o: ../utils/%.cpp
	$(MAKE) -C ../utils $*.o

../data/%.o: ../data/%.cpp
	$(MAKE) -C ../data $*.o

../exchange/%.o: ../exchange/%.cpp
	$(MAKE) -C ../exchange $*.o

../strategy/%.o: ../strategy/%.cpp
	$(MAKE) -C ../strategy $*.o

../backtest/%.o: ../backtest/%.cpp
	$(MAKE) -C ../backtest $*.o

TestBacktest: TestBacktest.o TestFramework.o ../utils/Logger.o ../utils/JsonParser.o ../strategy/Indicators.o ../strategy/RecipeLoader.o ../strategy/SignalGenerator.o ../backtest/Portfolio.o ../backtest/BacktestSimulator.o ../backtest/PerformanceAnalyzer.o ../data/DataStorage.o
	$(CXX) -o $@ $^ $(LDFLAGS)

run: all
	@echo "Running all tests..."
	@echo ""
	./TestLogger
	@echo ""
	./TestConfig
	@echo ""
	./TestDataStorage

compare: TestBFSvsSQLite
	@echo "Running BFS vs SQLite comparison..."
	@echo ""
	./TestBFSvsSQLite

binance: TestBinanceAPI
	@echo "Running Binance API tests..."
	@echo ""
	./TestBinanceAPI

benchmarks: all
	@echo "Running benchmarks..."
	./TestLogger
	./TestConfig
	./TestDataStorage

clean:
	rm -f $(TESTS) $(BENCHMARKS) *.o
	rm -f /tmp/emilio_*.log /tmp/emilio_*.db /tmp/emilio_*.json /tmp/benchmark_*.json
