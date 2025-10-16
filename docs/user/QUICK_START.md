# Emiglio Quick Start Guide

Get up and running with Emiglio in just a few minutes!

## ⚡ 5-Minute Setup

### Step 1: Prerequisites

Make sure you have Haiku OS with these packages installed:

```bash
pkgman install sqlite openssl curl_devel
```

### Step 2: Build

```bash
cd /boot/home/Emiglio
make -f MakefileUI
```

This will compile the application. It should take about 30 seconds.

### Step 3: Launch

```bash
./objects.x86_64-cc13-release/Emiglio
```

The Emiglio window will open with 5 tabs: Dashboard, Backtest, Trades/Chart, Recipe Editor, and Settings.

## 🎯 Your First Backtest

Let's run your first backtest to see how a trading strategy performs!

### 1. Open Backtest Tab

Click on the **"Backtest"** tab at the top of the window.

### 2. Select a Strategy

In the "Recipe Selection" dropdown, choose **"Simple RSI Strategy"**.

This is a basic strategy that:
- Buys when RSI drops below 30 (oversold)
- Sells when RSI rises above 70 (overbought)

### 3. Configure Parameters

The default parameters are good for testing:
- **Symbol**: BTCUSDT
- **Initial Capital**: $10,000
- **Commission**: 0.1%
- **Slippage**: 0.05%

You can adjust these if you want to experiment.

### 4. Run the Backtest

Click the **"Run Backtest"** button.

You'll see:
- A progress bar showing the simulation progress
- After completion (should take less than 1 second), results appear below

### 5. Review Results

The results panel shows:
- **Total Return**: How much profit/loss was made
- **Sharpe Ratio**: Risk-adjusted return (higher is better)
- **Max Drawdown**: Largest loss from peak
- **Win Rate**: Percentage of profitable trades
- **Total Trades**: Number of trades executed

Below that, you'll see a list of all trades with entry/exit details.

### 6. Export Results (Optional)

Click **"Export Report"** to save results as a text file or JSON for further analysis.

## 📊 Viewing Charts

### 1. Open Trades/Chart Tab

Click on the **"Trades/Chart"** tab.

### 2. Select a Currency Pair

- **Base Currency**: Choose from dropdown (e.g., BTC, ETH, SOL)
- **Quote Currency**: Choose from dropdown (e.g., USDT, EUR, BTC)

### 3. Select Timeframe

Choose a timeframe from the dropdown:
- 1m, 5m, 15m, 1h, 4h, or 1d

### 4. Load/Update Data

Click **"Load/Update Data"**.

If data doesn't exist, Emiglio will automatically download the last 30 days of historical data from Binance. This may take a few seconds.

Progress is shown in a progress bar.

### 5. Explore the Chart

Once loaded, you'll see:
- **Candlestick chart**: Green for up candles, red for down
- **Volume bars**: At the bottom of the chart
- **Indicators**:
  - EMA(20) - Blue line
  - Bollinger Bands - Gray bands
  - RSI - Oscillator (coming soon in overlay)

**Interactive Controls:**
- **Mouse Wheel**: Zoom in/out
- **Right-Click + Drag**: Pan left/right
- **Hover**: Crosshair with price/time info

### 6. Recent Trades

Below the chart, you'll see a list of recent trades (if any have been executed by paper trading or backtests).

## 🎨 Exploring Recipes

### 1. Open Recipe Editor

Click on the **"Recipe Editor"** tab.

### 2. Load a Recipe

- Click on any recipe in the list on the left
- The details appear on the right side

### 3. Inspect the Strategy

You can see:
- **Market**: Which exchange, symbol, and timeframe
- **Risk Parameters**: Stop-loss, take-profit, position size
- **Indicators**: Which technical indicators are used
- **Entry Conditions**: When to open a trade
- **Exit Conditions**: When to close a trade

### 4. Validate

Click **"Validate Recipe"** to check if the strategy is correctly configured.

### 5. Delete (Optional)

If you want to remove a strategy, select it and click **"Delete Recipe"**.

## 🔄 Next Steps

Now that you've run your first backtest and explored the interface, here's what you can do:

### Create Custom Strategies

Edit the JSON files in `/boot/home/Emiglio/recipes/` to create your own strategies.

Example recipe structure:
```json
{
  "name": "My Strategy",
  "market": {
    "exchange": "binance",
    "symbol": "ETH/USDT",
    "timeframe": "1h"
  },
  "entry_conditions": {
    "logic": "AND",
    "rules": [
      {"indicator": "sma_20", "operator": ">", "reference": "sma_50"}
    ]
  }
}
```

See existing recipes for full examples.

### Download More Data

Use the **Trades/Chart** tab to download data for different currency pairs and timeframes. Emiglio will cache the data for fast access later.

### Optimize Strategies

Try different parameters in backtests:
- Change commission rates
- Adjust stop-loss/take-profit levels
- Test different timeframes
- Compare multiple symbols

### Monitor Dashboard

The **Dashboard** tab shows:
- Portfolio overview
- System statistics
- Recent activity

Auto-refreshes every 5 seconds!

## 💡 Tips & Tricks

### Performance

- **Fast Backtests**: Emiglio is optimized to backtest 10,000 candles in under 1 second
- **Data Caching**: Downloaded data is stored locally, no need to re-download
- **Smart Updates**: When you reload data, only missing candles are downloaded

### Strategies

- **Start Simple**: Begin with basic RSI or SMA strategies
- **Test Thoroughly**: Run backtests on different time periods
- **Understand Metrics**: Learn what Sharpe ratio, max drawdown, and win rate mean
- **Avoid Overfitting**: Don't optimize too much on historical data

### Data Management

- **30-Day Auto-Download**: First load gets the last 30 days automatically
- **Incremental Updates**: Subsequent loads only fetch new data
- **Multiple Pairs**: Download data for multiple symbols to compare

## ❓ Common Questions

**Q: Why does the first data load take time?**
A: Emiglio downloads 30 days of historical data from Binance. After that, updates are fast.

**Q: Can I test on longer time periods?**
A: Yes! Generate more test data or adjust the backtest date range in the Backtest tab.

**Q: How do I know if a strategy is good?**
A: Look for:
- Positive total return
- Sharpe ratio > 1.0
- Win rate > 50%
- Reasonable max drawdown (< 20%)

**Q: Can I use this for live trading?**
A: Currently, Emiglio is for backtesting only. Live trading features are planned for future releases.

**Q: What if I get an error?**
A: Check the console output for error messages. Common issues:
- Network connectivity (for data downloads)
- Invalid recipe JSON format
- Insufficient data for indicators

## 🚀 You're Ready!

You now know the basics of Emiglio. Explore, experiment, and happy backtesting!

For more detailed information, see:
- **User Guide** - Complete documentation (coming soon)
- **[Features](../project/FEATURES.md)** - Full feature list
- **[Architecture](../developer/ARCHITECTURE.md)** - How it works

---

**Need Help?** Check the [Documentation Index](../INDEX.md) for more resources.

**Last Updated**: 2025-10-14
