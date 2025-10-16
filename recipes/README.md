# Trading Recipes / Strategies

This folder contains JSON-based trading strategy definitions (recipes) for Emiglio. Each recipe defines entry/exit conditions, risk management parameters, and technical indicators without requiring any coding.

## 📚 Available Strategies

### Basic Strategies

#### 1. **simple_rsi.json** - Simple RSI Oversold/Overbought
- **Type**: Scalping
- **Timeframe**: 15m
- **Complexity**: ⭐ Beginner
- **Description**: Basic RSI strategy that buys when oversold (<30) and sells when overbought (>70)
- **Best for**: Learning the basics, low-volatility markets

#### 2. **rsi_scalping_btc.json** - RSI Scalping Bitcoin
- **Type**: Scalping
- **Timeframe**: 5m
- **Complexity**: ⭐⭐ Beginner-Intermediate
- **Description**: Fast scalping on BTC using RSI with volume confirmation
- **Best for**: Active traders, high-frequency trading

#### 3. **bollinger_breakout.json** - Bollinger Bands Breakout
- **Type**: Breakout
- **Timeframe**: 1h
- **Complexity**: ⭐⭐ Intermediate
- **Description**: Trades breakouts from Bollinger Bands with volume confirmation
- **Best for**: Volatile markets, trending conditions

#### 4. **macd_crossover_eth.json** - MACD Crossover Ethereum
- **Type**: Trend Following
- **Timeframe**: 4h
- **Complexity**: ⭐⭐ Intermediate
- **Description**: Trend-following strategy using MACD crossovers on ETH
- **Best for**: Medium-term trades, trending markets

### Advanced Strategies

#### 5. **grid_trading_btc.json** - Grid Trading Bitcoin
- **Type**: Grid Trading
- **Timeframe**: 15m
- **Complexity**: ⭐⭐⭐ Advanced
- **Description**: Places multiple buy/sell orders at different price levels to profit from volatility
- **Features**:
  - 10 grid levels with 1.5% spacing
  - Bollinger Bands for range definition
  - ATR for volatility measurement
  - Auto-exits on breakouts
- **Best for**: Ranging/sideways markets, high volatility
- **Capital Required**: Higher (multiple positions)

#### 6. **dca_advanced_eth.json** - Advanced Dollar Cost Averaging ETH
- **Type**: DCA / Accumulation
- **Timeframe**: 1h
- **Complexity**: ⭐⭐⭐ Advanced
- **Description**: Sophisticated DCA that buys more aggressively on larger dips
- **Features**:
  - Dynamic position sizing (1.5x to 4x multiplier)
  - Multiple entry strategies based on dip severity
  - RSI, MACD, and EMA confirmation
  - LIFO exit strategy
- **Best for**: Long-term accumulation, bull market corrections
- **Capital Required**: High (reserve for multiple entries)

#### 7. **swing_trading_multi_tf.json** - Multi-Timeframe Swing Trading BTC
- **Type**: Swing Trading
- **Timeframe**: 4h (primary), 1d (higher), 1h (lower)
- **Complexity**: ⭐⭐⭐⭐ Expert
- **Description**: Professional swing trading using multiple timeframe analysis
- **Features**:
  - Daily trend confirmation (21/50 EMA)
  - 4h intermediate trend and entry signals
  - 1h precise entry timing
  - Partial profit-taking (30/30/40)
  - 3:1 risk-reward ratio
- **Best for**: Trending markets with pullbacks, patient traders
- **Hold Time**: 3-14 days per swing

## 🎯 Strategy Selection Guide

### By Market Condition

| Market Type | Recommended Strategies |
|-------------|----------------------|
| **Trending Up** | macd_crossover_eth, swing_trading_multi_tf, dca_advanced_eth |
| **Trending Down** | (Avoid or use short positions - not yet implemented) |
| **Ranging/Sideways** | grid_trading_btc, bollinger_breakout |
| **High Volatility** | rsi_scalping_btc, grid_trading_btc |
| **Low Volatility** | simple_rsi, swing_trading_multi_tf |

### By Trading Style

| Style | Recommended Strategies |
|-------|----------------------|
| **Scalping** (minutes-hours) | rsi_scalping_btc, simple_rsi |
| **Day Trading** (hours-day) | bollinger_breakout, macd_crossover_eth |
| **Swing Trading** (days-weeks) | swing_trading_multi_tf |
| **Position/Long-term** (weeks-months) | dca_advanced_eth |
| **Grid/Range Trading** | grid_trading_btc |

### By Experience Level

- **Beginner**: simple_rsi, rsi_scalping_btc
- **Intermediate**: bollinger_breakout, macd_crossover_eth
- **Advanced**: grid_trading_btc, dca_advanced_eth
- **Expert**: swing_trading_multi_tf

## 📝 Recipe Format

All recipes follow this JSON structure:

```json
{
  "name": "Strategy Name",
  "version": "1.0",
  "description": "What the strategy does",
  "market": {
    "exchange": "binance",
    "symbol": "BTC/USDT",
    "timeframe": "1h"
  },
  "capital": {
    "initial": 10000,
    "position_size_percent": 10
  },
  "risk_management": {
    "stop_loss_percent": 2.0,
    "take_profit_percent": 5.0
  },
  "indicators": [
    {"name": "rsi", "period": 14}
  ],
  "entry_conditions": {
    "logic": "AND",
    "rules": [...]
  },
  "exit_conditions": {
    "logic": "OR",
    "rules": [...]
  }
}
```

## 🔧 Customizing Recipes

### Common Parameters to Adjust

1. **Position Size** (`position_size_percent`)
   - Smaller (1-5%): Conservative, less risk
   - Medium (5-15%): Moderate risk
   - Larger (15-30%): Aggressive, higher risk

2. **Stop Loss** (`stop_loss_percent`)
   - Tight (1-2%): Scalping, quick exits
   - Medium (2-5%): Day trading
   - Wide (5-10%): Swing trading

3. **Indicator Periods**
   - Shorter periods (9-14): More responsive, more signals
   - Longer periods (20-50): More stable, fewer signals

4. **RSI Thresholds**
   - Conservative: <25 oversold, >75 overbought
   - Standard: <30 oversold, >70 overbought
   - Aggressive: <35 oversold, >65 overbought

### Testing Your Changes

1. **Load recipe** in Recipe Editor tab
2. **Validate** the JSON syntax
3. **Backtest** on historical data
4. **Analyze** performance metrics
5. **Paper trade** before live trading

## ⚠️ Important Notes

### Risk Management

- Always use stop losses
- Never risk more than 1-2% of capital per trade
- Test strategies thoroughly in backtesting
- Start with paper trading
- Monitor positions actively

### Market Conditions

- No single strategy works in all market conditions
- Adjust parameters based on volatility
- Be aware of major news events
- Consider overall crypto market sentiment

### Backtesting Caveats

- Past performance doesn't guarantee future results
- Backtests may not account for all market conditions
- Slippage and commission impact real performance
- Liquidity varies in live markets

## 📊 Performance Metrics to Monitor

When backtesting strategies, focus on:

- **Sharpe Ratio** (>1.5 is good)
- **Max Drawdown** (<20% preferred)
- **Win Rate** (>50% for trend following)
- **Profit Factor** (>1.5 is profitable)
- **Total Trades** (enough for statistical significance)

## 🚀 Getting Started

1. **Choose a strategy** based on your style and market conditions
2. **Review the recipe** in a text editor or Recipe Editor
3. **Backtest** on at least 3-6 months of data
4. **Optimize** parameters if needed
5. **Paper trade** for 1-2 weeks
6. **Go live** with small position sizes
7. **Scale up** as you gain confidence

## 📚 Learning Resources

- [Quick Start Guide](../docs/user/QUICK_START.md)
- [Recipe System Documentation](../CLAUDE.md#sistema-di-ricette-recipe-system)
- [Technical Indicators Guide](../docs/developer/ARCHITECTURE.md)
- [Backtesting Best Practices](../docs/project/FEATURES.md)

## 🤝 Contributing Recipes

Have a profitable strategy? Consider sharing!

1. Create a new JSON file following the format
2. Add comprehensive comments and description
3. Include backtest results
4. Document ideal market conditions
5. Add entry to this README

## 📄 License

These recipes are part of Emiglio and are provided for educational purposes. Use at your own risk. Trading cryptocurrencies involves substantial risk of loss.

---

**Happy Trading! Remember: Always test before trading with real money.** 🎯
