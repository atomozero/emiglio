# Emiglio Roadmap

This document outlines the future development plans for Emiglio.

## ✅ Completed (Version 1.0)

### Core Features
- [x] SQLite data storage
- [x] 10+ technical indicators
- [x] Recipe-based strategy system
- [x] Full backtesting engine
- [x] Native Haiku UI (5 tabs)
- [x] Binance data integration
- [x] Real-time charting
- [x] Performance analytics
- [x] 64x optimization speedup

See [FEATURES.md](FEATURES.md) for complete list.

---

## 🚧 In Progress

### Data & Integration
- [ ] **Fix Binance Data Import Runtime**
  - Debug NetServices2 BHttpSession::Execute() blocking issue
  - Implement retry logic with exponential backoff
  - Add timeout configuration
  - Status: Tool compiles, runtime debugging needed

### UI Enhancements
- [ ] **Recipe Editor Save Functionality**
  - Implement full JSON serialization
  - Create "Add Indicator" dialog
  - Create "Add Condition" dialog
  - Input validation and parsing
  - Status: Load/validate/delete working, save pending

- [ ] **Trade History Persistence**
  - Implement `DataStorage::insertTrade()`
  - Implement `DataStorage::getTrades()`
  - Create trades database schema
  - Integrate with TradesView
  - Status: UI ready, database integration TODO

- [ ] **Dashboard Backtest History**
  - Implement `DataStorage::insertBacktestResult()`
  - Implement `DataStorage::getAllBacktestResults()`
  - Create backtest_results schema
  - Display in Dashboard
  - Status: UI ready, storage logic TODO

---

## 🎯 Short Term (Next 1-2 Months)

### Priority 1: Polish Existing Features

**Data Management**
- [ ] CSV import for historical data
- [ ] Export backtest results to CSV
- [ ] Data validation and cleaning
- [ ] Database maintenance tools (vacuum, repair)

**Recipe System**
- [ ] Visual recipe builder (drag-and-drop)
- [ ] Recipe templates library
- [ ] Import/export recipes
- [ ] Recipe versioning system

**UI/UX Improvements**
- [ ] Keyboard shortcuts for common actions
- [ ] Tooltips and help text
- [ ] Dark mode / theme support
- [ ] Preferences persistence
- [ ] Window size/position memory

### Priority 2: Enhanced Analytics

**Performance Metrics**
- [ ] Walk-forward analysis
- [ ] Monte Carlo simulation
- [ ] Equity curve charting
- [ ] Drawdown visualization
- [ ] Monthly/yearly returns heatmap

**Trade Analysis**
- [ ] Trade duration analysis
- [ ] Win/loss distribution
- [ ] Entry/exit timing analysis
- [ ] Comparison between strategies

**Reporting**
- [ ] HTML report generation
- [ ] PDF export
- [ ] Email reports (optional)
- [ ] Shareable report links

---

## 📅 Medium Term (3-6 Months)

### Advanced Features

**Strategy Optimization**
- [ ] Grid search parameter optimization
- [ ] Genetic algorithm optimization
- [ ] Walk-forward optimization
- [ ] Multi-objective optimization
- [ ] Parameter sensitivity analysis

**Paper Trading**
- [ ] Real-time paper trading mode
- [ ] Simulated order execution
- [ ] Performance tracking
- [ ] Comparison with backtests
- [ ] Paper trading leaderboard

**Multi-Asset Support**
- [ ] Portfolio-level backtesting
- [ ] Cross-asset strategies
- [ ] Correlation analysis
- [ ] Asset allocation
- [ ] Rebalancing logic

**Advanced Charting**
- [ ] Multiple indicator overlays
- [ ] Custom indicator drawing
- [ ] Pattern recognition highlights
- [ ] Volume profile
- [ ] Order flow visualization

**Additional Exchanges**
- [ ] Coinbase Pro integration
- [ ] Kraken integration
- [ ] Gemini integration
- [ ] Exchange comparison tools

---

## 🚀 Long Term (6-12 Months)

### Live Trading (Carefully!)

**Foundation**
- [ ] WebSocket real-time data streaming
- [ ] Order execution system
- [ ] Position management
- [ ] Risk management module
- [ ] Emergency stop system

**Safety Features**
- [ ] Max position size limits
- [ ] Daily loss limits
- [ ] Mandatory stop-losses
- [ ] Order confirmation dialogs
- [ ] Trade journaling
- [ ] Audit logging

**Monitoring**
- [ ] Real-time P&L tracking
- [ ] Open position monitoring
- [ ] Order status tracking
- [ ] Performance dashboard
- [ ] Alert system (email/push)

### AI Integration

**AI-Powered Analysis**
- [ ] Gemini API integration
- [ ] ChatGPT API integration
- [ ] Sentiment analysis (news/social)
- [ ] Price prediction models
- [ ] Anomaly detection
- [ ] Strategy optimization suggestions

**Machine Learning**
- [ ] Pattern recognition
- [ ] Trend prediction
- [ ] Volatility forecasting
- [ ] Risk modeling
- [ ] Adaptive strategies

### Advanced Features

**Collaboration**
- [ ] Strategy sharing platform
- [ ] Community recipes marketplace
- [ ] Performance leaderboards
- [ ] Social trading features

**Cloud Integration** (Optional)
- [ ] Cloud backup
- [ ] Multi-device sync
- [ ] Web interface (view-only)
- [ ] Mobile companion app

---

## 🔬 Research & Exploration

### Experimental Features

**Advanced Strategies**
- [ ] Market-making strategies
- [ ] Arbitrage detection
- [ ] DeFi integration
- [ ] Options strategies
- [ ] Futures trading

**Technical Innovations**
- [ ] Tick-by-tick data support
- [ ] Order book analysis
- [ ] High-frequency trading (HFT)
- [ ] Custom indicator SDK
- [ ] Plugin system

**Alternative Approaches**
- [ ] Reinforcement learning strategies
- [ ] Neural network integration
- [ ] Quantum-inspired algorithms
- [ ] Blockchain integration

---

## 📊 Success Metrics

### v1.0 → v2.0 Goals

**Performance**
- Backtest 100k candles in < 5 seconds
- Real-time chart updates at 60 FPS
- < 100ms UI response time

**Features**
- 20+ technical indicators
- 10+ strategy templates
- 3+ exchange integrations
- Full paper trading mode

**Quality**
- 98%+ test coverage
- Zero crashes in production
- < 5 reported bugs per release
- 4.5+ user satisfaction (if shared)

**Community** (if open-sourced)
- 100+ stars on GitHub
- 10+ contributors
- 50+ strategies shared
- Active forums/discord

---

## 🎓 Learning Objectives

As this is also an educational project, each phase aims to teach:

**Short Term**
- Advanced C++ patterns
- Haiku OS internals
- Real-time data handling
- Complex UI layouts

**Medium Term**
- Optimization algorithms
- Financial mathematics
- Statistical analysis
- Data visualization

**Long Term**
- Live trading systems
- AI/ML integration
- Risk management
- Production deployment

---

## 💡 Community Feedback

Want to influence the roadmap? Here's how:

### Feature Requests
1. Check if it's already listed above
2. Consider if it fits Emiglio's goals
3. Open a discussion about implementation
4. Volunteer to help build it!

### Priority Adjustments
If you think something should be prioritized differently:
- Explain the use case
- Describe the impact
- Suggest alternative approaches

---

## 🚫 Out of Scope

To maintain focus, these are explicitly **not** planned:

- ❌ Support for non-crypto assets (stocks, forex, etc.)
- ❌ Social media platform (for traders to chat)
- ❌ Signal broadcasting services
- ❌ Copy-trading automation
- ❌ Portfolio management for others
- ❌ Paid subscription model
- ❌ Windows/macOS versions (Haiku only)
- ❌ Mobile app (except companion view)

---

## 📈 Version Timeline

**Estimated release schedule:**

- **v1.1** (Current + short-term) - Q1 2026
- **v1.5** (+ medium-term core) - Q2 2026
- **v2.0** (+ paper trading) - Q3 2026
- **v2.5** (+ advanced features) - Q4 2026
- **v3.0** (+ live trading) - 2027+

*Note: These are aspirational timelines. Actual progress depends on available time and priorities.*

---

## 🤝 How to Contribute

Interested in helping build these features?

1. **Check the roadmap** - Find something you're interested in
2. **Review [ARCHITECTURE.md](../developer/ARCHITECTURE.md)** - Understand the codebase
3. **Start small** - Pick a well-defined task
4. **Communicate** - Discuss your approach first
5. **Code** - Write clean, tested code
6. **Document** - Update relevant docs
7. **Test** - Ensure everything works
8. **Share** - Submit for review

---

## 🔄 Roadmap Updates

This roadmap is a living document. It will be updated:
- After each major release
- When priorities change
- Based on user feedback
- As new technologies emerge

**Last Updated**: 2025-10-14
**Version**: 1.0

---

**Questions about the roadmap?** See [Documentation Index](../INDEX.md) for more information.

**Want to suggest changes?** Open a discussion about your ideas!
