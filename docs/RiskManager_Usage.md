# RiskManager - Usage Guide

## Overview

Il `RiskManager` è il componente fondamentale per la gestione del rischio nel trading live di Emiglio. Protegge il capitale attraverso:

- Position sizing automatico
- Stop-loss e take-profit
- Limiti di perdita giornaliera
- Controllo numero massimo posizioni
- Trailing stop
- Emergency controls

## Quick Start

```cpp
#include "RiskManager.h"
#include "RecipeLoader.h"

using namespace Emiglio::Core;

// 1. Create RiskManager
RiskManager riskMgr;

// 2. Load recipe configuration
RecipeLoader loader;
Recipe recipe;
loader.loadFromFile("recipes/rsi_scalping_btc.json", recipe);

// 3. Configure RiskManager
riskMgr.configure(recipe.risk);
riskMgr.setCapital(1000.0);  // $1000 USDT

// 4. Validate before opening position
double btcPrice = 67500.0;
double quantity = 0.01;  // 0.01 BTC

if (riskMgr.canOpenPosition("BTCUSDT", btcPrice, quantity)) {
    // Safe to open position

    // Create position
    Position pos;
    pos.id = "pos_001";
    pos.symbol = "BTCUSDT";
    pos.entryPrice = btcPrice;
    pos.quantity = quantity;
    pos.entryTime = time(nullptr);

    // Calculate stop-loss and take-profit
    pos.stopLossPrice = riskMgr.calculateStopLossPrice(btcPrice, true);  // true = long
    pos.takeProfitPrice = riskMgr.calculateTakeProfitPrice(btcPrice, true);

    // Enable trailing stop
    pos.trailingStopEnabled = true;
    pos.trailingStopPercent = 1.5;  // 1.5%
    pos.highestPrice = btcPrice;

    // Add to RiskManager
    riskMgr.addPosition(pos);

    // Execute order on exchange...
}
```

## Position Sizing

```cpp
// Automatic position sizing based on capital
double totalCapital = 1000.0;
double currentPrice = 67500.0;

double positionValue = riskMgr.calculatePositionSize(currentPrice, totalCapital);
// Returns: ~100.0 (10% of capital by default)

double quantity = riskMgr.calculateQuantity(positionValue, currentPrice);
// Returns: 0.00148 BTC
```

## Stop-Loss and Take-Profit

```cpp
// Configure risk parameters
RiskConfig config;
config.stopLossPercent = 2.0;      // 2% stop-loss
config.takeProfitPercent = 5.0;    // 5% take-profit
config.maxDailyLossPercent = 5.0;  // Max 5% loss per day
config.maxOpenPositions = 3;       // Max 3 concurrent positions

riskMgr.configure(config);

// Calculate SL/TP for a long position
double entryPrice = 67500.0;
double slPrice = riskMgr.calculateStopLossPrice(entryPrice, true);
// Returns: 66150.0 (67500 - 2%)

double tpPrice = riskMgr.calculateTakeProfitPrice(entryPrice, true);
// Returns: 70875.0 (67500 + 5%)
```

## Real-Time Monitoring

```cpp
// In your main trading loop (called every tick)
void onPriceUpdate(const std::string& symbol, double currentPrice) {
    // Update all positions with current price
    auto positions = riskMgr.getOpenPositions();

    for (auto& pos : positions) {
        if (pos.symbol == symbol) {
            // Update position with current price
            riskMgr.updatePosition(pos.id, currentPrice);

            // Check if should close
            auto trigger = riskMgr.shouldClosePosition(pos, currentPrice);

            if (trigger.triggered) {
                // Close position!
                Logger::Warning("Closing position %s: %s (exit=%.2f)",
                    pos.id.c_str(), trigger.reason.c_str(), trigger.exitPrice);

                // Execute sell order on exchange...
                // exchangeAPI->sellMarket(symbol, pos.quantity);

                // Calculate final P&L
                double pnl = (trigger.exitPrice - pos.entryPrice) * pos.quantity;
                bool isWinner = pnl > 0;

                // Record trade in daily stats
                riskMgr.recordTrade(pnl, isWinner);

                // Remove from RiskManager
                riskMgr.removePosition(pos.id);
            }
        }
    }
}
```

## Daily Loss Limit

```cpp
// Check before opening new positions
if (riskMgr.isWithinDailyLossLimit()) {
    // Safe to trade
} else {
    Logger::Warning("Daily loss limit reached - no new positions allowed");
    // Stop trading for today
}

// Get daily statistics
DailyStats stats = riskMgr.getDailyStats();
printf("Today's Stats:\n");
printf("  Realized P&L: %.2f (%.2f%%)\n", stats.realizedPnL,
    riskMgr.getDailyPnLPercent());
printf("  Trades: %d (Win: %d, Loss: %d)\n",
    stats.tradesExecuted, stats.winningTrades, stats.losingTrades);
printf("  Max Drawdown: %.2f\n", stats.maxDrawdown);

// Manually reset stats (normally auto-resets at midnight)
riskMgr.resetDailyStats();
```

## Portfolio Metrics

```cpp
// Real-time portfolio status
double exposure = riskMgr.getTotalExposure();
printf("Total Exposure: %.2f USDT\n", exposure);

double available = riskMgr.getAvailableCapital();
printf("Available Capital: %.2f USDT\n", available);

double usedPercent = riskMgr.getUsedCapitalPercent();
printf("Capital Used: %.2f%%\n", usedPercent);

double unrealizedPnL = riskMgr.getTotalUnrealizedPnL();
printf("Unrealized P&L: %.2f USDT\n", unrealizedPnL);

// Risk metrics
double maxPosValue = riskMgr.getMaxPositionValue();
printf("Max Position Size: %.2f USDT\n", maxPosValue);

double remainingLoss = riskMgr.getRemainingDailyLoss();
printf("Remaining Daily Loss Budget: %.2f USDT\n", remainingLoss);
```

## Trailing Stop Example

```cpp
Position pos;
pos.id = "pos_btc_001";
pos.symbol = "BTCUSDT";
pos.entryPrice = 67500.0;
pos.quantity = 0.01;
pos.stopLossPrice = 66150.0;  // Initial SL at -2%
pos.trailingStopEnabled = true;
pos.trailingStopPercent = 1.5;  // Trail at 1.5%
pos.highestPrice = 67500.0;

riskMgr.addPosition(pos);

// Price moves up to 70000
riskMgr.updatePosition(pos.id, 70000.0);
// Trailing stop now at: 70000 - 1.5% = 68950

// Price moves up to 72000
riskMgr.updatePosition(pos.id, 72000.0);
// Trailing stop now at: 72000 - 1.5% = 70920

// Price drops to 70500
auto trigger = riskMgr.shouldClosePosition(pos, 70500.0);
// trigger.triggered = true
// trigger.reason = "trailing-stop"
// trigger.exitPrice = 70920

// Result: Locked in profit even though price dropped!
```

## Emergency Controls

```cpp
// Disable trading (no new positions)
riskMgr.disableTrading();

// Check if trading is enabled
if (riskMgr.isTradingEnabled()) {
    // Open new positions
}

// Re-enable trading
riskMgr.enableTrading();

// Emergency stop-loss (close ALL positions on limit)
riskMgr.setEmergencyStopLoss(true);

// Full reset
riskMgr.reset();  // Clears positions, resets stats, enables trading
```

## Integration with TradingEngine

```cpp
class TradingEngine {
private:
    RiskManager* riskMgr;
    ExchangeAPI* exchange;

public:
    void onBuySignal(const std::string& symbol, double price) {
        // 1. Calculate position size
        double capital = riskMgr->getTotalCapital();
        double posValue = riskMgr->calculatePositionSize(price, capital);
        double quantity = riskMgr->calculateQuantity(posValue, price);

        // 2. Validate risk
        if (!riskMgr->canOpenPosition(symbol, price, quantity)) {
            Logger::Warning("Risk check failed - skipping buy signal");
            return;
        }

        // 3. Execute order
        Order* order = exchange->buyMarket(symbol, quantity);

        if (order->status == OrderStatus::FILLED) {
            // 4. Create position
            Position pos;
            pos.id = generatePositionId();
            pos.symbol = symbol;
            pos.entryPrice = order->fillPrice;
            pos.quantity = order->fillQuantity;
            pos.entryTime = time(nullptr);
            pos.stopLossPrice = riskMgr->calculateStopLossPrice(order->fillPrice, true);
            pos.takeProfitPrice = riskMgr->calculateTakeProfitPrice(order->fillPrice, true);

            // 5. Add to RiskManager
            riskMgr->addPosition(pos);

            Logger::Info("Position opened: %s, qty=%.8f, entry=%.2f, SL=%.2f, TP=%.2f",
                pos.id.c_str(), pos.quantity, pos.entryPrice,
                pos.stopLossPrice, pos.takeProfitPrice);
        }
    }

    void onPriceTick(const std::string& symbol, double price) {
        auto positions = riskMgr->getOpenPositions();

        for (auto& pos : positions) {
            if (pos.symbol == symbol) {
                auto trigger = riskMgr->shouldClosePosition(pos, price);

                if (trigger.triggered) {
                    closePosition(pos, trigger);
                }
            }
        }
    }

    void closePosition(const Position& pos, const RiskManager::TriggerResult& trigger) {
        // Execute sell order
        Order* order = exchange->sellMarket(pos.symbol, pos.quantity);

        if (order->status == OrderStatus::FILLED) {
            // Calculate P&L
            double pnl = (order->fillPrice - pos.entryPrice) * pos.quantity;
            bool isWinner = pnl > 0;

            // Update statistics
            riskMgr->recordTrade(pnl, isWinner);
            riskMgr->removePosition(pos.id);

            Logger::Info("Position closed: %s, reason=%s, pnl=%.2f",
                pos.id.c_str(), trigger.reason.c_str(), pnl);
        }
    }
};
```

## Best Practices

### 1. Always Validate Before Trading
```cpp
// GOOD
if (riskMgr.canOpenPosition(symbol, price, qty)) {
    // Place order
}

// BAD - Never skip risk checks!
exchange->buyMarket(symbol, qty);  // Risky!
```

### 2. Update Positions Regularly
```cpp
// Update on every price tick (WebSocket)
void onWebSocketTick(const Ticker& ticker) {
    for (auto& pos : riskMgr.getOpenPositions()) {
        if (pos.symbol == ticker.symbol) {
            riskMgr.updatePosition(pos.id, ticker.price);
        }
    }
}
```

### 3. Monitor Daily Stats
```cpp
// Log stats every hour
void logDailyProgress() {
    DailyStats stats = riskMgr.getDailyStats();

    Logger::Info("Daily Progress: PnL=%.2f%%, Trades=%d, WinRate=%.1f%%",
        riskMgr.getDailyPnLPercent(),
        stats.tradesExecuted,
        stats.tradesExecuted > 0 ?
            (stats.winningTrades * 100.0 / stats.tradesExecuted) : 0.0);

    // Alert if approaching daily limit
    if (stats.realizedPnL < 0) {
        double lossPercent = std::abs(stats.realizedPnL / stats.startingCapital) * 100.0;
        if (lossPercent > config.maxDailyLossPercent * 0.8) {
            Logger::Warning("Approaching daily loss limit: %.2f%% (limit: %.2f%%)",
                lossPercent, config.maxDailyLossPercent);
        }
    }
}
```

### 4. Use Trailing Stops for Trending Markets
```cpp
// Enable trailing stops in recipe
{
  "risk_management": {
    "stop_loss_percent": 2.0,
    "take_profit_percent": 5.0,
    "trailing_stop": true,
    "trailing_stop_percent": 1.5
  }
}
```

### 5. Test in Paper Mode First
```cpp
// Before live trading
riskMgr.configure(recipe.risk);
riskMgr.setCapital(1000.0);  // Virtual capital

// Run for 24-48h in paper mode
// Validate metrics match backtesting
// Only then switch to live
```

## Thread Safety

**IMPORTANTE**: Il RiskManager **NON è thread-safe**. Se usi multi-threading:

```cpp
#include <Locker.h>

class TradingEngine {
private:
    RiskManager riskMgr;
    BLocker riskLock;

public:
    bool canOpenPosition(const std::string& symbol, double price, double qty) {
        BAutolock lock(riskLock);
        return riskMgr.canOpenPosition(symbol, price, qty);
    }

    void updatePosition(const std::string& id, double price) {
        BAutolock lock(riskLock);
        riskMgr.updatePosition(id, price);
    }
};
```

## Logging

Il RiskManager logga automaticamente:
- Configurazione iniziale
- Posizioni aperte/chiuse
- Stop-loss/take-profit triggered
- Daily stats reset
- Trading enable/disable
- Risk limit violations

```
[INFO] RiskManager configured: SL=2.00%, TP=5.00%, MaxDailyLoss=5.00%, MaxPos=3
[INFO] Position added: pos_001, symbol=BTCUSDT, qty=0.01000000, entry=67500.00
[WARNING] Stop-loss triggered for pos_001: price=66100.00, SL=66150.00
[INFO] Position removed: pos_001
[INFO] Trade recorded: PnL=-13.50, Winner=0, DailyPnL=-13.50
```

## Troubleshooting

### "Cannot open position: max positions reached"
```cpp
// Increase max positions in recipe
config.maxOpenPositions = 5;
riskMgr.configure(config);
```

### "Cannot open position: daily loss limit reached"
```cpp
// Check daily stats
DailyStats stats = riskMgr.getDailyStats();
printf("Daily loss: %.2f (limit: %.2f)\n",
    std::abs(stats.realizedPnL),
    riskMgr.getRiskConfig().maxDailyLossPercent);

// Options:
// 1. Wait until midnight (auto-reset)
// 2. Manually reset (risky!)
riskMgr.resetDailyStats();
// 3. Adjust limit
config.maxDailyLossPercent = 10.0;
```

### Trailing stop not updating
```cpp
// Make sure to call updatePosition() on every price tick
void onTick(double price) {
    riskMgr.updatePosition(positionId, price);  // Required!

    // Check position
    Position* pos = riskMgr.getPosition(positionId);
    printf("Highest: %.2f, Current SL: %.2f\n",
        pos->highestPrice, pos->stopLossPrice);
}
```

## Next Steps

- Implementa `OrderManager.cpp` per esecuzione ordini
- Implementa `Portfolio.cpp` per tracking capitale real-time
- Integra RiskManager nel `TradingEngine.cpp`
- Test paper trading per 48h
- Gradual rollout live con capitale minimo
