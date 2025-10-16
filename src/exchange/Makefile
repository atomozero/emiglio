# Exchange API Makefile

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I.. -I../../external/rapidjson/include -I/boot/system/develop/headers/private/netservices

OBJS = BinanceAPI.o

.PHONY: all clean

all: $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)
