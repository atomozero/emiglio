# Emiglio - Cryptocurrency Trading Bot

🤖 Native Haiku OS trading bot with advanced backtesting, real-time charting, and AI-powered analysis.

![Status](https://img.shields.io/badge/status-production--ready-green)
![Platform](https://img.shields.io/badge/platform-Haiku%20OS-blue)
![Language](https://img.shields.io/badge/language-C%2B%2B17-orange)

## ✨ Features

- **📊 Advanced Backtesting** - Test strategies with realistic commission, slippage, and latency simulation
- **📈 Real-Time Charts** - Candlestick charts with technical indicators and interactive controls
- **🎯 Recipe System** - JSON-based strategy definitions (no coding required)
- **⚡ High Performance** - 64x faster than baseline with optimized algorithms
- **💾 Data Management** - Automatic download and caching of historical data from Binance
- **🎨 Native UI** - Beautiful Haiku OS interface with BeAPI integration
- **📉 10 Technical Indicators** - SMA, EMA, RSI, MACD, Bollinger Bands, and more
- **🔍 Performance Analytics** - Comprehensive metrics including Sharpe ratio, max drawdown, win rate

## 🚀 Quick Start

### Prerequisites

Haiku OS with the following packages:
```bash
pkgman install sqlite openssl curl_devel
```

### Build & Run

```bash
# Clone or navigate to project
cd /boot/home/Emiglio

# Build the application
make -f MakefileUI

# Run
./objects.x86_64-cc13-release/Emiglio
```

### First Steps

1. **Download Data**: Open the "Trades/Chart" tab, select a currency pair (e.g., BTC + USDT), and data will download automatically
2. **View Charts**: Explore real-time candlestick charts with indicators
3. **Run Backtest**: Go to "Backtest" tab, select a recipe, configure parameters, and click "Run Backtest"
4. **Analyze Results**: View detailed performance metrics, trade history, and equity curves

## 📱 Interface Overview

### Dashboard
- Portfolio overview with P&L tracking
- System statistics (recipes, data, backtests)
- Auto-refresh every 5 seconds

### Trades/Chart
- Interactive candlestick charts with zoom and pan
- Technical indicators (EMA, Bollinger Bands, RSI)
- Volume bars and crosshair tooltips
- Automatic data download from Binance
- Recent trades list

### Backtest
- Strategy selection and configuration
- Date range, capital, and risk parameters
- Real-time execution with progress bar
- Detailed results with 8+ performance metrics
- Export functionality (text/JSON)

### Recipe Editor
- Load and validate existing strategies
- Visual inspection of indicators and conditions
- Delete and manage recipes

### Settings
- System configuration (coming soon)

## 🎯 Key Performance Metrics

| Metric | Value |
|--------|-------|
| Backtest Speed (10k candles) | 849ms (64x faster) |
| Memory Usage | ~50MB |
| Test Coverage | 95.5% |
| Technical Indicators | 10 implemented |

## 📊 Supported Data

- **Exchanges**: Binance (others coming soon)
- **Timeframes**: 1m, 5m, 15m, 1h, 4h, 1d
- **Auto-download**: Last 30 days on first request
- **Incremental updates**: Smart gap detection

## 🔧 Architecture

Built with modern C++17 and native Haiku APIs:
- **UI Layer**: BeAPI (BWindow, BView, BLayout)
- **Data Layer**: SQLite with optimized queries
- **Network**: NetServices2 for HTTP/HTTPS
- **Threading**: Asynchronous operations with BMessenger
- **JSON**: RapidJSON for strategy parsing

## 📚 Documentation

- **[Quick Start Guide](docs/user/QUICK_START.md)** - Get up and running in 5 minutes
- **[Features](docs/project/FEATURES.md)** - Complete feature catalog
- **[Architecture](docs/developer/ARCHITECTURE.md)** - Technical design overview
- **[Performance](docs/developer/PERFORMANCE.md)** - Optimization details and benchmarks
- **[Changelog](docs/project/CHANGELOG.md)** - Development history
- **[Roadmap](docs/project/ROADMAP.md)** - Future plans

Full documentation index: [docs/INDEX.md](docs/INDEX.md)

## 🛠️ Development

### Build System
```bash
# Main application
make -f MakefileUI

# Tests and benchmarks
cd src/tests && make

# Utility scripts
cd scripts && make
```

### Project Structure
```
Emiglio/
├── src/              # Source code
│   ├── core/         # Application entry point
│   ├── ui/           # User interface (BeAPI)
│   ├── backtest/     # Backtesting engine
│   ├── strategy/     # Strategy system
│   ├── data/         # Data management
│   ├── exchange/     # Exchange APIs
│   └── utils/        # Utilities
├── recipes/          # Strategy definitions
├── data/             # SQLite database
├── docs/             # Documentation
└── external/         # Dependencies (RapidJSON)
```

## 🤝 Contributing

This is a personal/educational project. Feel free to use it as inspiration for your own trading bots!

### Recipe Format

Strategies are defined in JSON format. Example:
```json
{
  "name": "Simple RSI Strategy",
  "market": {
    "exchange": "binance",
    "symbol": "BTC/USDT",
    "timeframe": "1h"
  },
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

See `recipes/` folder for more examples.

## ⚠️ Disclaimer

This software is for educational and backtesting purposes only. Cryptocurrency trading involves substantial risk. Never trade with money you cannot afford to lose. The authors are not responsible for any financial losses.

## 📄 License

Personal/Educational Project - See CLAUDE.md for full technical specification

## 🙏 Acknowledgments

- Haiku OS community for excellent documentation
- RapidJSON for fast JSON parsing
- Binance for public API access

---

**Built with ❤️ for Haiku OS**

*For technical details and complete project specification, see [CLAUDE.md](CLAUDE.md)*
