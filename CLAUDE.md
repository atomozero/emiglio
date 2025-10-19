# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

---

## Project Overview

**Emiglio** is an educational cryptocurrency trading system built natively for Haiku OS. It's designed exclusively for learning backtesting, strategy development, and financial software architecture—NOT for real trading.

- **Platform**: Haiku OS (native BeAPI application)
- **Language**: C++17
- **UI**: BeAPI (BApplication, BWindow, BView)
- **Data**: SQLite with optimized queries
- **Network**: NetServices2 for HTTP/HTTPS, custom WebSocket client
- **JSON**: RapidJSON (header-only)

---

## Build Commands

### Main Application

```bash
# Build the UI application
make -f MakefileUI

# Clean build
make -f MakefileUI clean

# Run the application
./objects.x86_64-cc13-release/Emiglio
```

### Tests

```bash
# Build all tests
cd src/tests && make

# Run specific test suites
./TestIndicators      # Technical indicators
./TestRecipeLoader    # Strategy JSON parsing
./TestBacktest        # Backtesting engine
./TestBinanceAPI      # Exchange API integration

# Run all tests via helper script
/boot/home/Emiglio/scripts/run_all_tests.sh
```

### Dependencies

Required packages on Haiku OS:
```bash
pkgman install sqlite openssl curl_devel
```

---

## Architecture Overview

### Layer Structure

```
UI Layer (BeAPI)
    └─> MainWindow → DashboardView, BacktestView, TradesView, ChartsView,
                     LiveTradingView, RecipeEditorView, SettingsView

Strategy Engine
    └─> RecipeLoader (JSON parsing) → SignalGenerator → Indicators

Backtest Engine
    └─> BacktestSimulator → Portfolio → PerformanceAnalyzer → BacktestResult

Data Layer
    └─> DataStorage (SQLite) ← BinanceAPI (REST)

Real-time Layer
    └─> BinanceWebSocket → WebSocketClient → Live price/trade updates
```

### Core Components

**1. Recipe System** (`src/strategy/`)
- Strategies defined as JSON files in `recipes/` directory
- `RecipeLoader` parses JSON into `Recipe` structs
- Contains indicators, entry/exit conditions, risk parameters
- 13+ pre-built strategies (7 original + 6 optimized)

**2. Indicators** (`src/strategy/Indicators.cpp`)
- 10 technical indicators: SMA, EMA, RSI, MACD, Bollinger Bands, ATR, Stochastic, OBV, StdDev, Volume SMA
- Optimized for performance (64x faster than baseline)
- All calculations use `std::vector<double>` inputs/outputs

**3. Backtest Engine** (`src/backtest/`)
- `BacktestSimulator`: Main simulation loop over historical candles
- `Portfolio`: Tracks positions, capital, equity
- `PerformanceAnalyzer`: Calculates Sharpe ratio, max drawdown, win rate, etc.
- Realistic simulation: commission (0.1%), slippage (0.05%), stop-loss, take-profit

**4. Data Storage** (`src/data/DataStorage.cpp`)
- SQLite database for OHLCV candles
- Auto-download from Binance API (last 30 days on first request)
- Incremental updates with gap detection
- Indexed queries for fast backtesting

**5. Exchange Integration** (`src/exchange/`)
- `BinanceAPI`: REST API for historical data, account info, orders
- `BinanceWebSocket`: Real-time streaming (ticker, trades, klines)
- `WebSocketClient`: Low-level WebSocket implementation (wss://)

**6. UI Components** (`src/ui/`)
- `MainWindow`: Tab-based interface (Dashboard, Charts, Backtest, Live Trading, etc.)
- `BacktestView`: Run backtests, configure parameters, view results
- `ChartsView`: Candlestick charts with indicators, interactive zoom/pan
- `LiveTradingView`: Real-time WebSocket connection, paper trading
- `DashboardView`: Portfolio overview, Binance account integration

---

## Recipe System

### Recipe Structure

Recipes are JSON files defining trading strategies declaratively (no coding required). Example:

```json
{
  "name": "Simple RSI Strategy",
  "market": {
    "exchange": "binance",
    "symbol": "BTC/USDT",
    "timeframe": "1h"
  },
  "capital": {
    "initial": 1000,
    "position_size_percent": 25
  },
  "risk_management": {
    "stop_loss_percent": 2.0,
    "take_profit_percent": 5.0,
    "max_open_positions": 1
  },
  "indicators": [
    {"name": "rsi", "period": 14}
  ],
  "entry_conditions": {
    "logic": "AND",
    "rules": [
      {"indicator": "rsi", "operator": "<", "value": 30}
    ]
  },
  "exit_conditions": {
    "logic": "OR",
    "rules": [
      {"indicator": "rsi", "operator": ">", "value": 70}
    ]
  }
}
```

### Recipe Loading Flow

1. `RecipeLoader::loadFromFile()` reads JSON from `recipes/` directory
2. Parses into `Recipe` struct (market, capital, risk, indicators, conditions)
3. `SignalGenerator` evaluates conditions against live/historical data
4. `BacktestSimulator` executes strategy on historical candles

### Key Recipe Fields

- **indicators**: List of technical indicators to calculate (RSI, MACD, SMA, EMA, etc.)
- **entry_conditions**: Rules to trigger BUY signal (logic: "AND"/"OR")
- **exit_conditions**: Rules to trigger SELL signal
- **risk_management**: Stop-loss %, take-profit %, max positions
- **capital**: Initial capital, position sizing (% of portfolio per trade)

---

## Data Flow

### Historical Data Download

1. User selects symbol/timeframe in ChartsView or BacktestView
2. `DataStorage::getCandles()` checks SQLite cache
3. If missing, `BinanceAPI::fetchHistoricalData()` downloads from Binance
4. Data stored in SQLite `candles` table with indexes
5. Incremental updates via `DataStorage::updateCandles()`

### Backtesting Flow

1. Load recipe: `RecipeLoader::loadFromFile()`
2. Fetch historical candles: `DataStorage::getCandles()`
3. Initialize `BacktestSimulator` with recipe + config (commission, slippage)
4. Loop through candles:
   - Calculate indicators for current candle
   - Evaluate entry/exit conditions via `SignalGenerator`
   - Execute trades in `Portfolio` (buy/sell with commission/slippage)
   - Check stop-loss/take-profit
   - Update equity curve
5. Analyze results: `PerformanceAnalyzer::analyze()`
6. Display metrics in `BacktestView` UI

### Real-Time WebSocket Flow

1. User clicks "Connect" in LiveTradingView
2. `BinanceWebSocket::connect()` establishes wss:// connection
3. Subscribe to streams: ticker, trades, klines
4. `WebSocketClient` receives messages in background thread
5. Callbacks fire in main thread via `processMessages()`
6. UI updates with live prices, trades, positions

---

## Testing

### Test Organization

- **Unit Tests**: `src/tests/Test*.cpp` (11 test suites)
- **Benchmarks**: `src/tests/Benchmark*.cpp`
- **Helper Scripts**: `scripts/run_all_tests.sh`, `scripts/compare_all_recipes.sh`

### Running Tests

```bash
# Run all tests
cd src/tests
make
./TestLogger
./TestConfig
./TestDataStorage
./TestIndicators
./TestRecipeLoader
./TestSignalGenerator
./TestBacktest

# Or use helper script
/boot/home/Emiglio/scripts/run_all_tests.sh
```

### Test Coverage

- **TestIndicators**: All 10 technical indicators, edge cases, performance (10k candles)
- **TestRecipeLoader**: JSON parsing, validation, error handling
- **TestSignalGenerator**: Entry/exit condition evaluation, complex rules
- **TestBacktest**: Full backtest simulation, portfolio tracking, metrics
- **TestBinanceAPI**: REST API calls, authentication, data fetching
- **TestDataStorage**: SQLite operations, caching, incremental updates

---

## WebSocket Implementation

### BinanceWebSocket API

```cpp
BinanceWebSocket ws;

// Connect
ws.connect();

// Subscribe to ticker (24hr stats)
ws.subscribeTicker("btcusdt", [](const TickerUpdate& update) {
    LOG_INFO("Price: " + std::to_string(update.lastPrice));
});

// Subscribe to trades (individual trades)
ws.subscribeTrades("btcusdt", [](const TradeUpdate& trade) {
    LOG_INFO("Trade: " + std::to_string(trade.price) + " x " + std::to_string(trade.quantity));
});

// Subscribe to klines (candlesticks)
ws.subscribeKlines("btcusdt", "1m", [](const KlineUpdate& kline) {
    LOG_INFO("Candle: O=" + std::to_string(kline.open) + " C=" + std::to_string(kline.close));
});

// Process messages in main thread (call periodically, e.g., via BMessageRunner)
ws.processMessages();

// Disconnect
ws.disconnect();
```

### WebSocket Notes

- Uses custom `WebSocketClient` implementation (no external dependencies)
- Connects to `wss://stream.binance.com:9443/ws/<stream>`
- Runs in background thread, callbacks processed in main thread
- Auto-reconnect on disconnect (future enhancement)

---

## Performance Optimizations

### Indicator Calculation (Phase 3)
- **64x speedup** over baseline (849ms for 10k candles, was 54.9s)
- Pre-allocated vectors, single-pass algorithms
- Reduced redundant calculations in `SignalGenerator`

### SQLite Queries (Phase 1)
- Indexed `(exchange, symbol, timeframe, timestamp)` for fast lookups
- Prepared statements to avoid SQL injection
- Batch inserts for historical data downloads

### UI Responsiveness
- Backtest runs in background thread, UI updates via `BMessage`
- Progress bar shows completion % during simulation
- Charts use double-buffering for smooth rendering

---

## Common Development Tasks

### Adding a New Indicator

1. Add function signature to `src/strategy/Indicators.h`:
   ```cpp
   static std::vector<double> calculateMyIndicator(const std::vector<double>& prices, int period);
   ```

2. Implement in `src/strategy/Indicators.cpp`:
   ```cpp
   std::vector<double> Indicators::calculateMyIndicator(const std::vector<double>& prices, int period) {
       std::vector<double> result;
       // Your calculation logic
       return result;
   }
   ```

3. Add to `SignalGenerator::calculateIndicator()` switch statement

4. Add test in `src/tests/TestIndicators.cpp`

### Adding a New Recipe

1. Create JSON file in `recipes/my_strategy.json`
2. Define indicators, entry/exit conditions, risk parameters
3. Test with backtest: Load recipe in BacktestView, run simulation
4. Validate in UI via RecipeEditorView

### Adding a New Exchange

1. Create `src/exchange/MyExchangeAPI.h` and `.cpp`
2. Inherit from `ExchangeAPI` base class (if exists) or match `BinanceAPI` interface
3. Implement: `fetchHistoricalData()`, `getAccountInfo()`, `placeLimitOrder()`, etc.
4. Add authentication (HMAC-SHA256 or API key)
5. Update `DataStorage` to support new exchange identifier

---

## Coding Conventions

### Namespaces
- Main namespace: `Emiglio`
- Sub-namespaces: `Emiglio::UI`, `Emiglio::Backtest`, `Emiglio::Strategy`

### Error Handling
- Use `getLastError()` pattern for functions that can fail
- Log errors via `Logger::getInstance()` (LOG_ERROR, LOG_WARNING, LOG_INFO)
- Return `false` on failure, `true` on success for boolean methods

### Memory Management
- BeAPI objects: Use `new` for BView/BWindow (Haiku manages lifecycle)
- STL containers: Use smart pointers (`std::unique_ptr`, `std::shared_ptr`) when ownership is complex
- No raw `delete` unless required by BeAPI

### Logging
```cpp
#include "../utils/Logger.h"

LOG_INFO("Operation succeeded");
LOG_WARNING("Potential issue detected");
LOG_ERROR("Operation failed: " + errorMsg);
```

---

## Important Files

### Configuration
- **recipes/**: Trading strategy JSON files (13+ strategies)
- **data/**: SQLite database (`candles.db`, `trades.db`)
- **Emiglio.rdef**: Application resource definition (icon, version, signature)

### Key Headers
- **src/strategy/RecipeLoader.h**: Recipe struct definitions, JSON parsing
- **src/backtest/BacktestResult.h**: Backtest results, performance metrics
- **src/data/DataStorage.h**: SQLite interface, candle data
- **src/exchange/BinanceAPI.h**: REST API, authentication, data fetching
- **src/exchange/BinanceWebSocket.h**: WebSocket streams (ticker, trades, klines)

### Documentation
- **docs/INDEX.md**: Documentation index
- **docs/developer/ARCHITECTURE.md**: Detailed architecture (100+ lines)
- **docs/developer/PERFORMANCE.md**: Optimization details
- **docs/WEBSOCKET.md**: WebSocket implementation guide
- **docs/user/QUICK_START.md**: User guide
- **README.md**: Project overview, features, disclaimer

---

## Security & Risk Warnings

### API Keys
- Store Binance API keys securely via `CredentialManager` (encrypted storage)
- Never commit API keys to git
- Use read-only API permissions (no withdrawal)

### Educational Purpose
- This software is **NOT** for real trading with real money
- Designed exclusively for educational and learning purposes
- Backtested results do NOT guarantee future performance
- See README.md disclaimer for full legal warning

### Risk Management
- Recipes include stop-loss, take-profit, position sizing limits
- `RiskManager` enforces max daily loss, max open positions
- Paper trading mode for testing strategies without real funds

---

## Haiku OS Specifics

### BeAPI Usage

**BApplication**: Entry point in `src/core/Application.cpp`
```cpp
class EmiglioApplication : public BApplication {
    virtual void ReadyToRun() {
        mainWindow = new UI::MainWindow();
        mainWindow->Show();
    }
};
```

**BWindow**: Main window in `src/ui/MainWindow.cpp`
```cpp
MainWindow::MainWindow()
    : BWindow(BRect(100, 100, 900, 700), "Emiglio", B_TITLED_WINDOW, B_QUIT_ON_WINDOW_CLOSE)
{
    // Add views, layouts
}
```

**BMessage**: For async communication (backtest progress updates)
```cpp
void BacktestView::MessageReceived(BMessage* msg) {
    switch (msg->what) {
        case MSG_BACKTEST_PROGRESS:
            // Update progress bar
            break;
    }
}
```

### Build System
- Uses Haiku's `makefile-engine` (included via `include $(BUILDHOME)/etc/makefile-engine`)
- No CMake, no autotools—pure Haiku Makefile
- Compiler: `g++` with `-std=c++17`

### Libraries
- **be**: BeAPI core (BApplication, BWindow, BView)
- **columnlistview**: BColumnListView (for trade tables)
- **netservices2**: HTTP/HTTPS via NetServices2
- **sqlite3**: SQLite database
- **ssl/crypto**: OpenSSL for HTTPS, HMAC-SHA256

---

## Known Limitations

1. **Haiku-only**: Cannot compile on Linux/WSL (BeAPI not available)
2. **Single exchange**: Currently only Binance supported (Coinbase, Kraken placeholders)
3. **No AI integration**: Gemini/ChatGPT analysis planned but not implemented
4. **Limited order types**: Market orders only in paper trading (limit orders future)
5. **No multi-timeframe**: Strategies use single timeframe (planned enhancement)

---

## Future Enhancements (Roadmap)

- Multi-exchange support (Coinbase, Kraken, Binance.US)
- AI-powered sentiment analysis (Gemini API)
- Advanced order types (limit, stop-limit, trailing stop)
- Multi-timeframe strategies
- Portfolio optimization (genetic algorithms)
- Mobile companion app (Haiku mobile, if available)
- Real-time alerts (notifications, email, SMS)

See `docs/project/ROADMAP.md` for detailed roadmap.

---

## Contact & Support

- **Issues**: File bugs/features at project GitHub (if applicable)
- **Haiku Community**: https://discuss.haiku-os.org/
- **BeBook Reference**: https://www.haiku-os.org/docs/bebook/

---

## Summary

Emiglio is a **native Haiku OS trading bot** for educational purposes. It combines:
- **Recipe-based strategies** (JSON, no coding required)
- **Robust backtesting** (realistic simulation, professional metrics)
- **Real-time streaming** (WebSocket integration)
- **Native UI** (BeAPI, consistent with Haiku look & feel)
- **High performance** (C++17, optimized algorithms, 64x faster)

**Key architectural patterns**:
- Strategy Pattern (recipes)
- Observer Pattern (WebSocket callbacks)
- Repository Pattern (DataStorage)
- MVC (UI views, data models, controllers)

**Development workflow**:
1. Edit code (can use WSL for git/editing, but compile on Haiku)
2. Build: `make -f MakefileUI`
3. Test: `cd src/tests && make && ./TestXXX`
4. Run: `./objects.x86_64-cc13-release/Emiglio`
5. Iterate

**Remember**: This is an **educational project**. Never use for real trading without proper financial education and professional guidance.
