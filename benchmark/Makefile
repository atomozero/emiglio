# Emiglio Benchmark Suite Makefile

CXX = g++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -I../src -I../external/rapidjson/include
LDFLAGS = -lsqlite3 -lssl -lcrypto -lstdc++ -lpthread

TARGET = benchmark_suite
SRCS = benchmark_suite.cpp \
       ../src/strategy/Indicators.cpp \
       ../src/backtest/Portfolio.cpp \
       ../src/backtest/BacktestSimulator.cpp \
       ../src/backtest/PerformanceAnalyzer.cpp \
       ../src/data/DataStorage.cpp \
       ../src/exchange/BinanceAPI.cpp \
       ../src/exchange/BinanceWebSocket.cpp \
       ../src/utils/Logger.cpp \
       ../src/utils/JsonParser.cpp \
       ../src/strategy/RecipeLoader.cpp \
       ../src/strategy/SignalGenerator.cpp

OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
