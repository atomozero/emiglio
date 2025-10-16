# Utils Makefile for Emiglio

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I.. -I../../external/rapidjson/include

OBJS = Logger.o Config.o JsonParser.o Crypto.o

all: $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

.PHONY: all clean
