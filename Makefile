# Emiglio Makefile
# Build solo su Haiku OS
# Non compilare su WSL/Linux - le API Haiku non sono disponibili

NAME = Emiglio
TYPE = APP
APP_MIME_SIG = application/x-vnd.Emiglio

SRCS = \
	src/core/Application.cpp \
	src/core/RiskManager.cpp \
	src/data/DataStorage.cpp \
	src/data/BFSStorage.cpp \
	src/exchange/BinanceAPI.cpp \
	src/exchange/BinanceWebSocket.cpp \
	src/exchange/WebSocketClient.cpp \
	src/strategy/RecipeLoader.cpp \
	src/strategy/Indicators.cpp \
	src/strategy/SignalGenerator.cpp \
	src/backtest/BacktestSimulator.cpp \
	src/backtest/PerformanceAnalyzer.cpp \
	src/backtest/Portfolio.cpp \
	src/paper/PaperPortfolio.cpp \
	src/ui/MainWindow.cpp \
	src/ui/DashboardView.cpp \
	src/ui/ChartsView.cpp \
	src/ui/TradesView.cpp \
	src/ui/RecipeEditorView.cpp \
	src/ui/BacktestView.cpp \
	src/ui/SettingsView.cpp \
	src/ui/LiveTradingView.cpp \
	src/utils/JsonParser.cpp \
	src/utils/Logger.cpp \
	src/utils/Config.cpp

RDEFS = Emiglio.rdef

LIBS = \
	be \
	bnetapi \
	netservices2 \
	network \
	translation \
	tracker \
	columnlistview \
	sqlite3 \
	ssl \
	crypto \
	stdc++

SYSTEM_INCLUDE_PATHS = \
	/boot/system/develop/headers/private/shared \
	/boot/system/develop/headers/private/netservices2

LOCAL_INCLUDE_PATHS = \
	src/core \
	src/data \
	src/exchange \
	src/strategy \
	src/ai \
	src/backtest \
	src/ui \
	src/utils \
	external/rapidjson/include \
	external/websocketpp

OPTIMIZE := FULL
LOCALES = en it

DEFINES = \
	_BUILDING_EMIGLIO=1

WARNINGS = ALL

# Compiler flags
COMPILER_FLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter -Wno-deprecated-declarations -Wno-class-memaccess

# Linker flags
LINKER_FLAGS =

## Include Haiku generic Makefile
include $(BUILDHOME)/etc/makefile-engine

# Test targets (build separately)
.PHONY: tests benchmarks clean-tests

tests:
	@echo "Building test suite..."
	@$(MAKE) -C src/tests all

benchmarks:
	@echo "Building benchmarks..."
	@$(MAKE) -C src/tests benchmarks

clean-tests:
	@$(MAKE) -C src/tests clean
