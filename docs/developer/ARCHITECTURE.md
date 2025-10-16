# Emiglio Architecture

Technical architecture and design documentation for Emiglio trading bot.

## 🏗️ System Overview

Emiglio is a native Haiku OS application built with C++17, using BeAPI for the user interface, SQLite for data storage, and NetServices2 for network operations.

### High-Level Architecture

```
┌─────────────────────────────────────────────────────┐
│                  User Interface (BeAPI)              │
│   MainWindow → DashboardView, BacktestView, etc.   │
└────────────────┬────────────────────────────────────┘
                 │
┌────────────────▼────────────────────────────────────┐
│              Application Layer                       │
│     Strategy Engine, Backtest Engine, Analytics     │
└────────────────┬────────────────────────────────────┘
                 │
┌────────────────▼────────────────────────────────────┐
│               Data Layer                             │
│    DataStorage (SQLite), Exchange APIs (Network)    │
└──────────────────────────────────────────────────────┘
```

### Core Components

1. **UI Layer**: Native BeAPI views and windows
2. **Strategy Engine**: Recipe parsing, indicators, signal generation
3. **Backtest Engine**: Portfolio simulation, performance analysis
4. **Data Management**: SQLite storage, data fetching
5. **Exchange Integration**: Binance API, WebSocket (future)
6. **Utilities**: Logging, configuration, JSON parsing

---

## 📁 Project Structure

```
Emiglio/
├── src/
│   ├── core/                    # Application core
│   │   ├── Application.cpp     # BApplication entry point
│   │   ├── TradingEngine.cpp   # Main trading logic (future)
│   │   ├── OrderManager.cpp    # Order management (future)
│   │   ├── Portfolio.cpp       # Portfolio tracking
│   │   └── RiskManager.cpp     # Risk management (future)
│   │
│   ├── ui/                      # User Interface (BeAPI)
│   │   ├── MainWindow.*        # Main application window
│   │   ├── DashboardView.*     # Portfolio overview
│   │   ├── BacktestView.*      # Backtesting interface
│   │   ├── TradesView.*        # Trade history + charts
│   │   ├── RecipeEditorView.*  # Strategy editor
│   │   ├── LiveTradingView.*   # Live trading (future)
│   │   ├── ChartsView.*        # Advanced charting
│   │   └── SettingsView.*      # Application settings
│   │
│   ├── backtest/                # Backtesting Engine
│   │   ├── Trade.h             # Trade data structure
│   │   ├── BacktestResult.h    # Results data structure
│   │   ├── Portfolio.*         # Virtual portfolio
│   │   ├── BacktestSimulator.* # Core simulation engine
│   │   └── PerformanceAnalyzer.* # Metrics calculation
│   │
│   ├── strategy/                # Strategy System
│   │   ├── Strategy.*          # Base strategy class
│   │   ├── Indicators.*        # Technical indicators
│   │   ├── RecipeLoader.*      # JSON strategy parser
│   │   └── SignalGenerator.*   # Buy/sell signal logic
│   │
│   ├── data/                    # Data Management
│   │   ├── DataStorage.*       # SQLite interface
│   │   ├── BFSStorage.*        # BeFS attribute storage (experimental)
│   │   ├── HistoricalData.*    # Historical data management
│   │   └── RealtimeStream.*    # Real-time data (future)
│   │
│   ├── exchange/                # Exchange Integration
│   │   ├── ExchangeAPI.h       # Base exchange interface
│   │   ├── BinanceAPI.*        # Binance implementation
│   │   ├── BinanceWebSocket.*  # Binance WebSocket
│   │   ├── CoinbaseAPI.*       # Coinbase (placeholder)
│   │   ├── KrakenAPI.*         # Kraken (placeholder)
│   │   └── GeminiAPI.*         # Gemini (placeholder)
│   │
│   ├── paper/                   # Paper Trading
│   │   └── PaperPortfolio.*    # Virtual trading portfolio
│   │
│   ├── utils/                   # Utilities
│   │   ├── Logger.*            # Logging system
│   │   ├── JsonParser.*        # JSON parsing
│   │   ├── Config.*            # Configuration
│   │   └── Crypto.*            # HMAC, signing
│   │
│   └── tests/                   # Test Suite
│       ├── TestIndicators.cpp
│       ├── TestSignalGenerator.cpp
│       ├── TestBacktest.cpp
│       ├── BenchmarkPhase3.cpp
│       └── BenchmarkPhase4.cpp
│
├── recipes/                     # Strategy Definitions
│   ├── simple_rsi.json
│   ├── ema_crossover.json
│   └── example_*.json
│
├── data/                        # Database Files
│   └── emiglio.db              # SQLite database
│
├── docs/                        # Documentation
│   ├── INDEX.md
│   ├── user/
│   ├── developer/
│   ├── project/
│   └── archive/
│
├── external/                    # External Dependencies
│   └── rapidjson/              # JSON library
│
├── scripts/                     # Utility Scripts
│   ├── import_binance_data.cpp
│   └── generate_test_data.cpp
│
├── Makefile                     # Main build file
├── MakefileUI                   # UI application build
├── Emiglio.rdef                # Resource definitions
└── CLAUDE.md                    # Technical specification
```

---

## 🔧 Core Components Detail

### 1. Application Core

**File**: `src/core/Application.cpp`

```cpp
class EmilioApp : public BApplication {
public:
    EmilioApp();
    void ReadyToRun() override;
    void MessageReceived(BMessage* msg) override;

private:
    MainWindow* fMainWindow;
    DataStorage* fDataStorage;
    Logger* fLogger;
};
```

**Responsibilities**:
- Initialize BeAPI application
- Create main window
- Set up data storage and logging
- Handle application-level messages

---

### 2. User Interface Layer

#### MainWindow

**File**: `src/ui/MainWindow.cpp`

```cpp
class MainWindow : public BWindow {
public:
    MainWindow(DataStorage* storage);

private:
    BTabView* fTabView;
    DashboardView* fDashboardView;
    BacktestView* fBacktestView;
    TradesView* fTradesView;
    RecipeEditorView* fRecipeEditorView;
    SettingsView* fSettingsView;
};
```

**Features**:
- Tab-based navigation
- Menu bar with actions
- Persistent window position/size (future)

#### DashboardView

**Responsibilities**:
- Display portfolio overview (P&L, positions)
- Show system statistics (recipes, data, backtests)
- List recent activity
- Auto-refresh every 5 seconds

**UI Elements**:
- `BStringView` for labels
- `BListView` for recent backtests
- `BButton` for quick actions

#### BacktestView

**Responsibilities**:
- Recipe selection and configuration
- Parameter input (capital, commission, etc.)
- Run backtest with progress bar
- Display results and metrics
- Export functionality

**UI Elements**:
- `BMenuField` for recipe selection
- `BTextControl` for parameter input
- `BButton` for actions
- `BStatusBar` for progress
- `BListView` for trade list
- `BStringView` for metrics

#### ChartsView (TradesView)

**Responsibilities**:
- Display candlestick charts
- Technical indicator overlays
- Interactive controls (zoom, pan, crosshair)
- Volume bars
- Trade markers (future)

**Custom Drawing**:
```cpp
void ChartsView::Draw(BRect updateRect) {
    // Draw candlesticks
    for (auto& candle : visibleCandles) {
        if (candle.close > candle.open)
            SetHighColor(0, 255, 0);  // Green
        else
            SetHighColor(255, 0, 0);  // Red

        DrawCandle(candle);
    }

    // Draw indicators
    DrawIndicatorLine(emaValues, blue);
    DrawBollingerBands(bbUpper, bbMiddle, bbLower);
}
```

**Mouse Handling**:
```cpp
void ChartsView::MouseDown(BPoint where) {
    // Pan or zoom based on mouse button
}

void ChartsView::MouseMoved(BPoint where, uint32 code, const BMessage* drag) {
    // Update crosshair position
    UpdateTooltip(where);
}
```

---

### 3. Backtest Engine

#### BacktestSimulator

**File**: `src/backtest/BacktestSimulator.cpp`

```cpp
class BacktestSimulator {
public:
    BacktestResult run(
        Recipe recipe,
        vector<Candle> candles,
        double initialCapital,
        double commission,
        double slippage
    );

private:
    // Pre-calculate all indicators (O(n))
    IndicatorValues preCalculateIndicators(
        const Recipe& recipe,
        const vector<Candle>& candles
    );

    // Main backtest loop (O(n))
    void simulateTrading(
        const Recipe& recipe,
        const vector<Candle>& candles,
        const IndicatorValues& indicators,
        Portfolio& portfolio
    );
};
```

**Algorithm**:
1. **Pre-calculation Phase** (O(n)):
   - Calculate all indicators once
   - Store results in vectors for O(1) lookup

2. **Simulation Phase** (O(n)):
   ```cpp
   for (int i = 0; i < candles.size(); i++) {
       // Lookup pre-calculated values
       double rsi = indicators.rsi[i];
       double sma = indicators.sma[i];

       // Evaluate entry conditions
       if (!hasPosition && evaluateEntry(indicators, i)) {
           portfolio.openPosition(candles[i]);
       }

       // Evaluate exit conditions
       if (hasPosition && evaluateExit(indicators, i)) {
           portfolio.closePosition(candles[i]);
       }

       // Check stop-loss / take-profit
       portfolio.updateStops(candles[i]);
   }
   ```

3. **Analysis Phase** (O(n)):
   - Calculate performance metrics
   - Generate equity curve
   - Compute drawdown

#### Portfolio

**File**: `src/backtest/Portfolio.cpp`

```cpp
class Portfolio {
public:
    void openPosition(const Candle& candle, double quantity);
    void closePosition(const Candle& candle);
    void updateEquity(double currentPrice);

    double getCash() const;
    double getEquity() const;
    double getTotalValue() const;

private:
    double fCash;
    double fInitialCash;
    Position fCurrentPosition;
    vector<Trade> fTrades;
    vector<double> fEquityCurve;
};
```

**Features**:
- Cash management
- Position tracking
- Commission and slippage calculation
- Equity curve tracking

#### PerformanceAnalyzer

**File**: `src/backtest/PerformanceAnalyzer.cpp`

```cpp
class PerformanceAnalyzer {
public:
    static PerformanceMetrics analyze(
        const vector<Trade>& trades,
        const vector<double>& equityCurve,
        double initialCapital
    );

private:
    static double calculateSharpeRatio(...);
    static double calculateMaxDrawdown(...);
    static double calculateSortinoRatio(...);
    // ... more metrics
};
```

**Metrics Calculated**:
- Total return, annualized return
- Sharpe ratio, Sortino ratio
- Max drawdown, recovery time
- Win rate, profit factor
- Average win, average loss
- Expectancy

---

### 4. Strategy System

#### Recipe Structure

**JSON Format**:
```json
{
  "name": "Strategy Name",
  "market": {
    "exchange": "binance",
    "symbol": "BTC/USDT",
    "timeframe": "1h"
  },
  "capital": {
    "initial": 10000,
    "position_size_percent": 100
  },
  "risk_management": {
    "stop_loss_percent": 5.0,
    "take_profit_percent": 10.0
  },
  "indicators": [
    {"name": "rsi", "period": 14},
    {"name": "sma", "period": 20}
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

#### RecipeLoader

**File**: `src/strategy/RecipeLoader.cpp`

```cpp
class RecipeLoader {
public:
    static Recipe load(const string& filepath);
    static vector<Recipe> loadAllFromDirectory(const string& dir);
    static bool validate(const Recipe& recipe);

private:
    static void parseIndicators(const json& j, Recipe& recipe);
    static void parseConditions(const json& j, Recipe& recipe);
};
```

#### Indicators

**File**: `src/strategy/Indicators.cpp`

**Available Indicators**:
1. **SMA** (Simple Moving Average)
2. **EMA** (Exponential Moving Average)
3. **RSI** (Relative Strength Index)
4. **MACD** (Moving Average Convergence Divergence)
5. **Bollinger Bands**
6. **ATR** (Average True Range)
7. **ADX** (Average Directional Index)
8. **CCI** (Commodity Channel Index)
9. **Stochastic Oscillator**
10. **OBV** (On-Balance Volume)

**Interface**:
```cpp
class Indicators {
public:
    static vector<double> calculateSMA(
        const vector<Candle>& candles,
        int period
    );

    static vector<double> calculateRSI(
        const vector<Candle>& candles,
        int period
    );

    // ... more indicators
};
```

**Optimization Techniques**:
- Sliding windows for rolling calculations
- Reuse of intermediate results
- Single-pass algorithms where possible

#### SignalGenerator

**File**: `src/strategy/SignalGenerator.cpp`

```cpp
class SignalGenerator {
public:
    static bool evaluateConditions(
        const Conditions& conditions,
        const IndicatorValues& indicators,
        int currentIndex
    );

private:
    static bool evaluateRule(
        const Rule& rule,
        const IndicatorValues& indicators,
        int index
    );

    static bool checkOperator(
        double left,
        const string& op,
        double right
    );
};
```

---

### 5. Data Management

#### DataStorage

**File**: `src/data/DataStorage.cpp`

```cpp
class DataStorage {
public:
    DataStorage(const string& dbPath);

    // Candle operations
    status_t insertCandle(const Candle& candle);
    vector<Candle> getCandles(
        const string& exchange,
        const string& symbol,
        const string& timeframe,
        time_t startTime = 0,
        time_t endTime = 0
    );

    // Future: Trade operations
    status_t insertTrade(const Trade& trade);
    vector<Trade> getTrades(...);

    // Future: Backtest results
    status_t insertBacktestResult(const BacktestResult& result);
    vector<BacktestResult> getAllBacktestResults();

private:
    sqlite3* fDatabase;
    BLocker fLock;  // Thread-safe access
};
```

**Database Schema**:
```sql
CREATE TABLE candles (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    exchange TEXT NOT NULL,
    symbol TEXT NOT NULL,
    timeframe TEXT NOT NULL,
    timestamp INTEGER NOT NULL,
    open REAL NOT NULL,
    high REAL NOT NULL,
    low REAL NOT NULL,
    close REAL NOT NULL,
    volume REAL NOT NULL,
    UNIQUE(exchange, symbol, timeframe, timestamp)
);

CREATE INDEX idx_candles_lookup
ON candles(exchange, symbol, timeframe, timestamp);
```

---

### 6. Exchange Integration

#### BinanceAPI

**File**: `src/exchange/BinanceAPI.cpp`

```cpp
class BinanceAPI : public ExchangeAPI {
public:
    BinanceAPI(const string& apiKey = "",
               const string& apiSecret = "");

    // Public endpoints
    Ticker getTicker(const string& symbol) override;
    vector<Candle> getCandles(
        const string& symbol,
        const string& timeframe,
        time_t startTime = 0,
        time_t endTime = 0,
        int limit = 500
    ) override;

    // Private endpoints (require API keys)
    vector<Balance> getBalances() override;
    Order createOrder(
        const string& symbol,
        const string& side,
        double quantity,
        double price = 0
    ) override;

private:
    BHttpSession* fSession;
    string fApiKey;
    string fApiSecret;

    string signRequest(const string& queryString);
    BString makeRequest(
        const string& endpoint,
        const string& params = "",
        bool signed = false
    );
};
```

**Features**:
- Rate limiting (1200 req/min)
- Ticker caching (1 second)
- HMAC-SHA256 signing for authenticated requests
- Error handling and retry logic

---

## 🔄 Data Flow

### Backtest Execution Flow

```
1. User clicks "Run Backtest"
   ↓
2. BacktestView collects parameters
   ↓
3. Load recipe from RecipeLoader
   ↓
4. Fetch candles from DataStorage
   ↓
5. BacktestSimulator.run()
   ├─> Pre-calculate indicators (Indicators class)
   ├─> Simulate trading (Portfolio class)
   └─> Analyze performance (PerformanceAnalyzer)
   ↓
6. Return BacktestResult
   ↓
7. BacktestView displays results
   ↓
8. User exports or runs another test
```

### Data Download Flow

```
1. User selects currency pair and timeframe
   ↓
2. TradesView checks DataStorage for existing data
   ↓
3. If missing or outdated:
   ├─> Spawn background thread
   ├─> BinanceAPI.getCandles() with pagination
   ├─> Update progress bar via BMessenger
   └─> Insert candles into DataStorage
   ↓
4. Load candles from database
   ↓
5. ChartsView renders candlestick chart
   ↓
6. User interacts with chart (zoom, pan, etc.)
```

---

## 🧵 Threading Model

### UI Thread
- Handles all BeAPI UI operations
- Never blocks (async for long operations)
- Receives messages from worker threads

### Worker Threads
- Data downloading (BinanceAPI)
- Backtesting (BacktestSimulator)
- File I/O operations

### Communication
```cpp
// Worker thread
class DownloadThread : public BThread {
    void Run() {
        // Download data
        auto candles = api->getCandles(...);

        // Send progress updates
        BMessage msg(MSG_PROGRESS);
        msg.AddInt32("percent", progress);
        messenger.SendMessage(&msg);

        // Send completion
        BMessage doneMsg(MSG_DOWNLOAD_COMPLETE);
        doneMsg.AddPointer("candles", &candles);
        messenger.SendMessage(&doneMsg);
    }
};

// UI thread
void TradesView::MessageReceived(BMessage* msg) {
    switch (msg->what) {
        case MSG_PROGRESS:
            UpdateProgressBar(msg->GetInt32("percent"));
            break;
        case MSG_DOWNLOAD_COMPLETE:
            DisplayChart(msg->GetPointer("candles"));
            break;
    }
}
```

---

## 🔒 Error Handling

### Defensive Programming
```cpp
// Check preconditions
if (candles.empty()) {
    Logger::Error("No candles available for backtest");
    return BacktestResult::Invalid();
}

// Validate inputs
if (initialCapital <= 0) {
    Logger::Error("Invalid capital: {}", initialCapital);
    throw std::invalid_argument("Capital must be positive");
}

// Graceful degradation
try {
    auto result = simulator.run(recipe, candles);
    return result;
} catch (const std::exception& e) {
    Logger::Error("Backtest failed: {}", e.what());
    // Show error dialog to user
    BAlert* alert = new BAlert("Error", e.what(), "OK");
    alert->Go();
    return BacktestResult::Invalid();
}
```

---

## 📦 Build System

### Makefile Structure

**Main Application** (`MakefileUI`):
```makefile
NAME = Emiglio
TYPE = APP
APP_MIME_SIG = application/x-vnd.Emiglio

SRCS = src/core/Application.cpp \
       src/ui/MainWindow.cpp \
       ...

LIBS = be bnetapi sqlite3 ssl crypto

include $(BUILDHOME)/etc/makefile-engine
```

### Build Process
```bash
# Clean build
make -f MakefileUI clean
make -f MakefileUI

# Output
objects.x86_64-cc13-release/Emiglio
```

---

## 🧪 Testing Strategy

### Unit Tests
- Test individual components in isolation
- Mock dependencies
- Fast execution

### Integration Tests
- Test component interactions
- Use real database (test DB)
- Validate data flow

### Benchmark Tests
- Measure performance
- Detect regressions
- Compare optimizations

### Test Organization
```
src/tests/
├── TestIndicators.cpp      # Unit tests for indicators
├── TestSignalGenerator.cpp # Unit tests for signal logic
├── TestBacktest.cpp         # Integration tests for backtest
├── BenchmarkPhase3.cpp      # Indicator performance benchmarks
└── BenchmarkPhase4.cpp      # Backtest performance benchmarks
```

---

## 🔮 Future Architecture Enhancements

### Plugin System
- Load custom indicators at runtime
- User-contributed strategies
- Third-party exchange integrations

### Microservices (Optional)
- Separate data service
- Separate backtest workers
- API server for web/mobile

### Event-Driven Architecture
- Event bus for component communication
- Asynchronous strategy execution
- Real-time updates

---

## 📚 Additional Resources

- **[PERFORMANCE.md](PERFORMANCE.md)** - Performance optimization details
- **[CLAUDE.md](../../CLAUDE.md)** - Complete technical specification
- **[CHANGELOG.md](../project/CHANGELOG.md)** - Development history

---

**Last Updated**: 2025-10-14
**Version**: 1.0
