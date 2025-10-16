# Backtest Module Makefile
# Compiles backtest components

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I.. -I../../external/rapidjson/include

.PHONY: all clean

all: Portfolio.o BacktestSimulator.o PerformanceAnalyzer.o

Portfolio.o: Portfolio.cpp Portfolio.h Trade.h
	$(CXX) $(CXXFLAGS) -c Portfolio.cpp -o Portfolio.o

BacktestSimulator.o: BacktestSimulator.cpp BacktestSimulator.h Portfolio.h BacktestResult.h Trade.h
	$(CXX) $(CXXFLAGS) -c BacktestSimulator.cpp -o BacktestSimulator.o

PerformanceAnalyzer.o: PerformanceAnalyzer.cpp PerformanceAnalyzer.h BacktestResult.h Trade.h
	$(CXX) $(CXXFLAGS) -c PerformanceAnalyzer.cpp -o PerformanceAnalyzer.o

clean:
	rm -f *.o
