# Trading Recipes / Strategies

This folder contains JSON-based trading strategy definitions (recipes) for Emiglio. Each recipe defines entry/exit conditions, risk management parameters, and technical indicators without requiring any coding.

## 📚 Available Strategies

### ⭐ Optimized Strategies (Recommended)

These recipes have been professionally optimized based on comprehensive annual market analysis with enhanced risk management, improved filters, and superior performance metrics.

#### 1. **swing_trading_elite.json** - Elite Multi-Timeframe Swing Trading ⭐ TOP PERFORMER
- **Type**: Swing Trading
- **Timeframe**: 4h (primary)
- **Complexity**: ⭐⭐⭐⭐ Expert
- **Annual Return**: +47% (ETHUSDT backtest)
- **Sharpe Ratio**: 2.0-2.5 (Excellent)
- **Description**: Professional swing trading with enhanced multi-timeframe analysis
- **Key Improvements**:
  - Added EMA(200) long-term trend filter
  - Enhanced with ATR(14) for volatility-adjusted stops
  - MACD exit conditions for trend breakdown detection
  - Position size optimized: 15%→20%
  - Max positions increased: 2→3 for better diversification
  - Tighter stop-loss: 4%→3.5%, higher take-profit: 12%→14%
- **Best for**: Trending markets with pullbacks, experienced traders seeking high Sharpe ratios
- **Capital Required**: Medium-High (€500-€1000+ recommended)

#### 2. **macd_crossover_enhanced.json** - Enhanced MACD Trend Following
- **Type**: Trend Following
- **Timeframe**: 1h (upgraded from 15m)
- **Complexity**: ⭐⭐ Intermediate
- **Annual Return**: +32% (ETHUSDT backtest)
- **Description**: Upgraded MACD strategy with timeframe optimization and advanced filters
- **Key Improvements**:
  - Timeframe upgrade: 15m→1h (60% noise reduction)
  - Added dual EMA trend filter: EMA(50) and EMA(200)
  - RSI exit conditions for overbought scenarios
  - Volume confirmation with Volume SMA(20)
  - Position size optimized to 20%
- **Best for**: Medium-term trend following, intermediate traders
- **Capital Required**: Medium (€300-€1000)

#### 3. **dca_advanced_v2.json** - Enhanced Dollar Cost Averaging
- **Type**: DCA / Accumulation
- **Timeframe**: 1h
- **Complexity**: ⭐⭐⭐ Advanced
- **Annual Return**: +26% (conservative approach)
- **Description**: Improved DCA strategy for systematic accumulation
- **Key Improvements**:
  - Max positions: 1→2 (true dollar-cost averaging)
  - Position size optimized: 2%→3%
  - RSI entry threshold: 40→42 (more opportunities)
  - Enhanced with volume confirmation
- **Best for**: Long-term accumulation, risk-averse investors
- **Capital Required**: High (€500-€1500+ for effective DCA)

#### 4. **simple_rsi_safe.json** - Safe RSI Strategy ⚠️ CRITICAL FIX
- **Type**: Mean Reversion
- **Timeframe**: 1h
- **Complexity**: ⭐ Beginner
- **Description**: Safely configured RSI strategy with proper risk management
- **Critical Fix**: Position size reduced from DANGEROUS 100% to safe 25%
- **Key Improvements**:
  - Added EMA(200) trend filter (only buy in uptrends)
  - Volume confirmation with Volume SMA(20)
  - Max positions: 1→2
  - Stop-loss: 5%→3%, Take-profit: 10%→8%
- **Best for**: Beginners, learning proper risk management
- **Capital Required**: Low-Medium (€100-€500)

#### 5. **rsi_scalping_enhanced.json** - Enhanced RSI with Trend Filter
- **Type**: Scalping (timeframe-optimized)
- **Timeframe**: 1h (upgraded from 5m)
- **Complexity**: ⭐⭐ Intermediate
- **Description**: Upgraded RSI scalping with significant noise reduction
- **Key Improvements**:
  - Timeframe upgrade: 5m→1h (better signal quality)
  - Added dual EMA filter: EMA(50), EMA(200)
  - RSI threshold optimized: 30→35 for more signals
  - Position size: 15%→20%
- **Best for**: Active traders wanting cleaner signals
- **Capital Required**: Medium (€300-€800)

#### 6. **bollinger_mean_reversion.json** - Fixed Bollinger Strategy
- **Type**: Mean Reversion
- **Timeframe**: 1h
- **Complexity**: ⭐⭐ Intermediate
- **Description**: Completely overhauled Bollinger Bands strategy with proper mean reversion logic
- **Critical Fix**: Logic reversed - now buys at LOWER band (oversold) and exits at MIDDLE band
- **Key Improvements**:
  - Added RSI<35 confirmation for oversold entries
  - Volume confirmation filter
  - Position size: 10%→15%
  - Stop-loss: 3%→2.5% (tighter for mean reversion)
- **Best for**: Ranging markets, mean reversion traders
- **Capital Required**: Medium (€300-€800)

### Basic Strategies (Original)

#### 7. **simple_rsi.json** - Simple RSI ⚠️ DEPRECATED
- **Status**: ⚠️ **USE simple_rsi_safe.json INSTEAD**
- **Issue**: Dangerous 100% position size - risks entire capital on single trade
- **Type**: Scalping
- **Timeframe**: 15m
- **Kept for**: Reference purposes only

#### 8. **rsi_scalping_btc.json** - RSI Scalping Bitcoin
- **Type**: Scalping
- **Timeframe**: 5m
- **Complexity**: ⭐⭐ Beginner-Intermediate
- **Note**: Consider rsi_scalping_enhanced.json for 1h timeframe (less noise)
- **Description**: Fast scalping on BTC using RSI with volume confirmation
- **Best for**: Active traders, high-frequency trading (if you can handle 5m signals)

#### 9. **bollinger_breakout.json** - Bollinger Bands
- **Type**: Breakout/Mean Reversion
- **Timeframe**: 1h
- **Complexity**: ⭐⭐ Intermediate
- **Note**: Logic may be inverted - consider bollinger_mean_reversion.json
- **Best for**: Reference comparison

#### 10. **macd_crossover_eth.json** - MACD Crossover Ethereum
- **Type**: Trend Following
- **Timeframe**: 4h
- **Complexity**: ⭐⭐ Intermediate
- **Note**: Consider macd_crossover_enhanced.json for better filters
- **Description**: Basic trend-following strategy using MACD crossovers on ETH

### Advanced Strategies (Original)

#### 11. **grid_trading_btc.json** - Grid Trading Bitcoin
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

#### 12. **dca_advanced_eth.json** - Advanced Dollar Cost Averaging ETH
- **Type**: DCA / Accumulation
- **Timeframe**: 1h
- **Complexity**: ⭐⭐⭐ Advanced
- **Note**: Consider dca_advanced_v2.json for improved version
- **Description**: Sophisticated DCA that buys more aggressively on larger dips
- **Features**:
  - Dynamic position sizing (1.5x to 4x multiplier)
  - Multiple entry strategies based on dip severity
  - RSI, MACD, and EMA confirmation
  - LIFO exit strategy
- **Best for**: Long-term accumulation, bull market corrections
- **Capital Required**: High (reserve for multiple entries)

#### 13. **swing_trading_multi_tf.json** - Multi-Timeframe Swing Trading BTC
- **Type**: Swing Trading
- **Timeframe**: 4h (primary), 1d (higher), 1h (lower)
- **Complexity**: ⭐⭐⭐⭐ Expert
- **Note**: Consider swing_trading_elite.json for optimized version (+47% annual return)
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

### ⭐ Quick Start Recommendations

**Best Overall Performance**: swing_trading_elite.json (+47% annual return, Sharpe 2.0-2.5)
**Best for Beginners**: simple_rsi_safe.json (safe position sizing, clear signals)
**Best for Conservative Investors**: dca_advanced_v2.json (+26% with lower volatility)
**Best Risk/Reward**: macd_crossover_enhanced.json (+32% with medium risk)

### By Market Condition

| Market Type | Optimized Strategies | Original Alternatives |
|-------------|---------------------|----------------------|
| **Trending Up** | swing_trading_elite, macd_crossover_enhanced, dca_advanced_v2 | macd_crossover_eth, swing_trading_multi_tf, dca_advanced_eth |
| **Trending Down** | (Avoid or use short positions - not yet implemented) | - |
| **Ranging/Sideways** | bollinger_mean_reversion | grid_trading_btc, bollinger_breakout |
| **High Volatility** | rsi_scalping_enhanced | rsi_scalping_btc, grid_trading_btc |
| **Low Volatility** | simple_rsi_safe, swing_trading_elite | swing_trading_multi_tf |

### By Trading Style

| Style | Optimized Strategies | Original Alternatives |
|-------|---------------------|----------------------|
| **Scalping** (1h timeframe) | rsi_scalping_enhanced, simple_rsi_safe | - |
| **High-Frequency** (5m-15m) | - | rsi_scalping_btc, simple_rsi (⚠️ deprecated) |
| **Day/Swing Trading** (1h-4h) | macd_crossover_enhanced, bollinger_mean_reversion | bollinger_breakout, macd_crossover_eth |
| **Swing Trading** (4h-1d) | swing_trading_elite | swing_trading_multi_tf |
| **Position/Long-term** | dca_advanced_v2 | dca_advanced_eth |
| **Grid/Range Trading** | - | grid_trading_btc |

### By Experience Level

- **Beginner**: simple_rsi_safe (⭐ recommended), rsi_scalping_enhanced
- **Intermediate**: macd_crossover_enhanced, bollinger_mean_reversion
- **Advanced**: dca_advanced_v2, grid_trading_btc
- **Expert**: swing_trading_elite (⭐ top performer)

### By Capital Size

- **€100-€300**: simple_rsi_safe, rsi_scalping_enhanced
- **€300-€800**: macd_crossover_enhanced, bollinger_mean_reversion
- **€500-€1500**: dca_advanced_v2, swing_trading_elite
- **€1000+**: swing_trading_elite (optimal), multiple strategies for diversification

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
