# Strategy Module Makefile

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I.. -I../../external/rapidjson/include

OBJS = Indicators.o RecipeLoader.o SignalGenerator.o

.PHONY: all clean

all: $(OBJS)

%.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)
