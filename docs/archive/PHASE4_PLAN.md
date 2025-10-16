# Phase 4: Backtesting Engine - Implementation Plan

## 🎯 Objectives

Build a realistic backtesting simulator that:
1. ✅ Simulates trading with historical data
2. ✅ Tracks positions, orders, portfolio value
3. ✅ Applies realistic trading costs (commissions, slippage)
4. ✅ Generates performance metrics (Sharpe, Max Drawdown, Win Rate, etc.)
5. ✅ Supports walk-forward testing
6. ✅ Exports detailed trade logs and equity curves

---

## 📦 Components

### 1. **Trade.h** - Trade representation
```cpp
struct Trade {
    std::string id;              // Unique trade ID
    std::string symbol;          // Trading pair
    TradeType type;              // LONG or SHORT
    TradeStatus status;          // OPEN, CLOSED, CANCELLED

    double entryPrice;           // Entry price
    double exitPrice;            // Exit price (0 if still open)
    double quantity;             // Position size

    time_t entryTime;            // Entry timestamp
    time_t exitTime;             // Exit timestamp (0 if still open)

    double commission;           // Total commission paid
    double slippage;             // Slippage cost
    double pnl;                  // Profit/Loss (realized)
    double pnlPercent;           // P&L percentage

    std::string entryReason;     // Why entered (signal reason)
    std::string exitReason;      // Why exited (stop-loss, take-profit, signal, etc.)
};

enum class TradeType { LONG, SHORT };
enum class TradeStatus { OPEN, CLOSED, CANCELLED };
```

### 2. **Portfolio.h/.cpp** - Portfolio manager
```cpp
class Portfolio {
public:
    Portfolio(double initialCapital);

    // Position management
    bool openPosition(const Trade& trade);
    bool closePosition(const std::string& tradeId, double exitPrice, const std::string& reason);

    // Queries
    double getEquity() const;                    // Current equity
    double getCash() const;                      // Available cash
    double getPositionValue() const;             // Value of open positions
    std::vector<Trade> getOpenTrades() const;
    std::vector<Trade> getClosedTrades() const;

    // Risk management
    bool canOpenPosition(double size) const;
    double getMaxPositionSize() const;

private:
    double initialCapital;
    double cash;
    std::vector<Trade> openTrades;
    std::vector<Trade> closedTrades;
    int nextTradeId;
};
```

### 3. **BacktestSimulator.h/.cpp** - Main simulator
```cpp
class BacktestSimulator {
public:
    BacktestSimulator(const Recipe& recipe, const BacktestConfig& config);

    // Run backtest
    BacktestResult run(const std::vector<Candle>& candles);

    // Configuration
    void setCommission(double percent);          // e.g., 0.1% = 0.001
    void setSlippage(double percent);            // e.g., 0.05% = 0.0005
    void setInitialCapital(double capital);

private:
    Recipe recipe;
    BacktestConfig config;
    SignalGenerator signalGen;
    Portfolio portfolio;

    // Helpers
    void processCandle(const Candle& candle);
    void checkStopLoss(const Candle& candle);
    void checkTakeProfit(const Candle& candle);
    void updateEquityCurve(const Candle& candle);
};

struct BacktestConfig {
    double initialCapital;
    double commissionPercent;    // 0.001 = 0.1%
    double slippagePercent;      // 0.0005 = 0.05%
    bool useStopLoss;
    bool useTakeProfit;
    int maxOpenPositions;
};
```

### 4. **PerformanceAnalyzer.h/.cpp** - Metrics calculator
```cpp
class PerformanceAnalyzer {
public:
    PerformanceAnalyzer(const BacktestResult& result);

    // Performance metrics
    double getTotalReturn() const;               // Total return %
    double getAnnualizedReturn() const;          // Annualized return %
    double getSharpeRatio() const;               // Sharpe ratio
    double getSortinoRatio() const;              // Sortino ratio (downside deviation)
    double getMaxDrawdown() const;               // Max drawdown %
    double getMaxDrawdownDuration() const;       // Days in drawdown

    // Trade statistics
    int getTotalTrades() const;
    int getWinningTrades() const;
    int getLosingTrades() const;
    double getWinRate() const;                   // Win rate %
    double getAverageWin() const;
    double getAverageLoss() const;
    double getProfitFactor() const;              // Total wins / Total losses
    double getExpectancy() const;                // Average P&L per trade

    // Risk metrics
    double getMaxConsecutiveLosses() const;
    double getMaxConsecutiveWins() const;
    double getValueAtRisk(double confidence) const;  // VaR

    // Generate report
    std::string generateReport() const;
    void exportToJSON(const std::string& filename) const;

private:
    BacktestResult result;

    // Calculations
    std::vector<double> calculateReturns() const;
    double calculateStdDev(const std::vector<double>& returns) const;
};
```

### 5. **BacktestResult.h** - Result structure
```cpp
struct BacktestResult {
    // Metadata
    std::string recipeName;
    std::string symbol;
    time_t startTime;
    time_t endTime;
    int totalCandles;

    // Capital
    double initialCapital;
    double finalEquity;
    double peakEquity;

    // Trades
    std::vector<Trade> trades;
    int totalTrades;
    int winningTrades;
    int losingTrades;

    // Equity curve
    std::vector<EquityPoint> equityCurve;

    // Costs
    double totalCommission;
    double totalSlippage;

    // Performance (calculated by PerformanceAnalyzer)
    double totalReturn;
    double sharpeRatio;
    double maxDrawdown;
    double winRate;
    // ... more metrics
};

struct EquityPoint {
    time_t timestamp;
    double equity;
    double cash;
    double positionValue;
};
```

---

## 🛠️ Implementation Steps

### Step 1: Core Structures ✅
- [ ] Create `Trade.h` with trade structure
- [ ] Create `Portfolio.h` and `Portfolio.cpp`
- [ ] Implement position tracking

### Step 2: Simulator ✅
- [ ] Create `BacktestSimulator.h` and `.cpp`
- [ ] Implement main simulation loop
- [ ] Add commission and slippage calculation
- [ ] Implement stop-loss and take-profit logic

### Step 3: Performance Analyzer ✅
- [ ] Create `PerformanceAnalyzer.h` and `.cpp`
- [ ] Implement all metrics calculations
- [ ] Add report generation

### Step 4: Testing ✅
- [ ] Create `TestBacktest.cpp`
- [ ] Test with simple buy-and-hold strategy
- [ ] Test with RSI strategy
- [ ] Validate metrics calculations

### Step 5: Integration ✅
- [ ] Create example backtest runner
- [ ] Test with real historical data
- [ ] Generate sample reports

---

## 📊 Key Features

### Realistic Simulation

**Commission Calculation:**
```cpp
double commission = orderValue * config.commissionPercent;
// Entry: pay commission
// Exit: pay commission again
```

**Slippage Simulation:**
```cpp
// Market orders: slippage always happens
double slippedPrice = price * (1 + config.slippagePercent); // Buy
double slippedPrice = price * (1 - config.slippagePercent); // Sell
```

**Stop-Loss/Take-Profit:**
```cpp
// Check on every candle if SL/TP hit
if (candle.low <= stopLossPrice) {
    closePosition(trade.id, stopLossPrice, "Stop-Loss");
}
if (candle.high >= takeProfitPrice) {
    closePosition(trade.id, takeProfitPrice, "Take-Profit");
}
```

### Performance Metrics

**Sharpe Ratio:**
```cpp
// (Average Return - Risk Free Rate) / Standard Deviation
double sharpe = (avgReturn - riskFreeRate) / stdDev;
```

**Max Drawdown:**
```cpp
// Maximum peak-to-trough decline
double maxDD = 0;
double peak = equityCurve[0];
for (auto point : equityCurve) {
    if (point.equity > peak) peak = point.equity;
    double dd = (peak - point.equity) / peak;
    if (dd > maxDD) maxDD = dd;
}
```

**Win Rate:**
```cpp
double winRate = (double)winningTrades / totalTrades * 100;
```

---

## 🧪 Testing Strategy

### Test 1: Buy-and-Hold
```cpp
// Simple strategy: buy at start, hold until end
// Expected: Linear equity curve, minimal trades
```

### Test 2: RSI Mean Reversion
```cpp
// Buy when RSI < 30, sell when RSI > 70
// Expected: Multiple trades, positive expectancy
```

### Test 3: Losing Strategy
```cpp
// Buy when RSI > 70 (overbought), sell when RSI < 30
// Expected: Negative returns, drawdown
```

### Test 4: Commission Impact
```cpp
// Compare 0% commission vs 0.1% commission
// Expected: 0.1% should reduce returns significantly
```

---

## 📈 Performance Targets

| Metric | Target |
|--------|--------|
| **Backtest 1000 candles** | < 100 ms |
| **Backtest 100k candles** | < 5 seconds |
| **Memory usage** | < 100 MB for 1M candles |
| **Metrics calculation** | < 10 ms |

---

## 🔄 Walk-Forward Testing (Future)

**Concept:**
1. Divide data into windows (e.g., 6 months train, 1 month test)
2. Optimize parameters on training window
3. Test on out-of-sample window
4. Roll forward and repeat

**Implementation:**
```cpp
struct WalkForwardConfig {
    int trainPeriodDays;   // e.g., 180
    int testPeriodDays;    // e.g., 30
    int stepDays;          // e.g., 30 (how much to roll forward)
};

std::vector<BacktestResult> runWalkForward(
    const std::vector<Candle>& candles,
    const Recipe& recipe,
    const WalkForwardConfig& config
);
```

---

## 📁 File Structure

```
src/backtest/
├── Trade.h                 # Trade structure
├── Portfolio.h             # Portfolio manager
├── Portfolio.cpp
├── BacktestSimulator.h     # Main simulator
├── BacktestSimulator.cpp
├── PerformanceAnalyzer.h   # Metrics calculator
├── PerformanceAnalyzer.cpp
├── BacktestResult.h        # Result structures
└── Makefile

src/tests/
├── TestBacktest.cpp        # Backtest tests
└── BenchmarkBacktest.cpp   # Performance benchmarks
```

---

## 🎯 Success Criteria

Phase 4 is complete when:

✅ Simulator runs realistic trades with costs
✅ Portfolio tracking works correctly
✅ All performance metrics calculated accurately
✅ Tests pass with 100% coverage
✅ Benchmarks meet performance targets
✅ Can backtest real strategies (RSI, MACD, etc.)
✅ Generate detailed reports (text + JSON)

---

## 🚀 Next Steps

After Phase 4:
- **Phase 5**: Live Trading (real-time execution)
- **Phase 6**: AI Integration (sentiment analysis)
- **Phase 7**: Native UI (visualization)

---

**Ready to implement? Let's start with Step 1: Core Structures!** 🔥
