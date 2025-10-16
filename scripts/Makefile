# Scripts Makefile
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I.. -I../external/rapidjson/include
LDFLAGS = -lstdc++ -lnetservices2 -lbnetapi -lbe -lssl -lcrypto -lsqlite3

all: generate_test_data import_binance_data test_components

generate_test_data: generate_test_data.o ../src/data/DataStorage.o ../src/utils/Logger.o
	$(CXX) -o $@ $^ -lsqlite3

generate_test_data.o: generate_test_data.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

import_binance_data: import_binance_data.o ../src/exchange/BinanceAPI.o ../src/utils/JsonParser.o ../src/data/DataStorage.o ../src/utils/Logger.o
	$(CXX) -o $@ $^ $(LDFLAGS)

import_binance_data.o: import_binance_data.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test_components: test_components.o ../src/exchange/BinanceAPI.o ../src/utils/JsonParser.o ../src/data/DataStorage.o ../src/utils/Logger.o
	$(CXX) -o $@ $^ $(LDFLAGS)

test_components.o: test_components.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

../src/exchange/BinanceAPI.o:
	$(MAKE) -C ../src/exchange BinanceAPI.o

../src/utils/JsonParser.o:
	$(MAKE) -C ../src/utils JsonParser.o

../src/data/DataStorage.o:
	$(MAKE) -C ../src/data DataStorage.o

../src/utils/Logger.o:
	$(MAKE) -C ../src/utils Logger.o

clean:
	rm -f generate_test_data import_binance_data test_components *.o

.PHONY: all clean
