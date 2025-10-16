# Fase 6: UI Improvements + Live Trading

## Status: 🚧 IN PROGRESS

**Obiettivo**: Completare l'interfaccia utente e implementare il sistema di trading live su Binance.

**Data Inizio**: 2025-10-13
**Priorità**: Alta
**Complessità**: Media-Alta

---

## Parte 1: UI Improvements

### 1.1 Dashboard Enhancements

**File**: `src/ui/DashboardView.cpp` (già implementato parzialmente)

**Migliorie da implementare**:

#### A. Portfolio Live Tracking
- [ ] **Live Portfolio State**
  - Integrare con sistema di trading live
  - Mostrare posizioni aperte correnti
  - Aggiornamento real-time di cash e invested
  - Calcolo P&L da trades reali

#### B. Recent Activity Feed
- [ ] **Database Storage per Backtest Results**
  - Creare tabella `backtest_results` in SQLite
  - Schema: id, recipe_name, symbol, timeframe, timestamp, total_return, sharpe, max_drawdown, total_trades
  - Salvare risultati automaticamente dopo ogni backtest

- [ ] **Display Recent Backtests**
  - Query ultimi 10 backtest results
  - Formato: `[Recipe] Symbol Timeframe | Return: X% | Sharpe: Y | Date`
  - Click per visualizzare dettagli completi
  - Color coding (verde per return positivo, rosso per negativo)

#### C. System Health Indicators
- [ ] **Connection Status**
  - Indicatore connessione a Binance API (verde/rosso)
  - Ultimo ping timestamp
  - Rate limit remaining

- [ ] **Data Freshness**
  - Ultimo update dei dati market
  - Oldest candle timestamp
  - Newest candle timestamp

**Tempo stimato**: 3-4 ore

---

### 1.2 Recipe Editor - Visual Builder

**File**: `src/ui/RecipeEditorView.cpp` (già implementato parzialmente)

**Migliorie da implementare**:

#### A. Indicator Management
- [ ] **Add Indicator Dialog**
  - Popup window con dropdown indicator type
  - Fields configurabili (period, multiplier, etc)
  - Preview formula
  - OK/Cancel buttons

- [ ] **Edit Indicator**
  - Double-click su indicator in lista
  - Apre dialog con valori attuali
  - Modifica e salva

- [ ] **Remove Indicator**
  - Selezione + tasto Remove
  - Conferma con BAlert

#### B. Condition Builder
- [ ] **Add Entry/Exit Condition Dialog**
  - Dropdown indicatore (dalla lista indicators configurati)
  - Dropdown operatore (>, <, >=, <=, ==, crosses_above, crosses_below)
  - Input valore threshold
  - Opzione: compare with another indicator
  - Logic selector (AND/OR) per multiple conditions

- [ ] **Visual Condition Display**
  - Mostra formula leggibile: "RSI < 30"
  - Mostra logic: "AND" / "OR" tra conditions
  - Color coding per tipo (entry = blue, exit = orange)

#### C. Form Validation
- [ ] **Real-time Validation**
  - Name: non vuoto, no duplicati
  - Symbol: formato valido (XXX/YYY o XXXYYY)
  - Stop-loss/Take-profit: > 0, < 100
  - Position size: > 0, <= 100
  - Indicators: almeno 1 se usato in conditions
  - Conditions: almeno 1 entry e 1 exit

- [ ] **Visual Feedback**
  - Border rosso per campi invalidi
  - Status label con messaggio errore
  - Save button disabilitato se invalid

#### D. Template System
- [ ] **Recipe Templates**
  - Folder `recipes/templates/` con recipes predefinite
  - Menu "New from Template"
  - Templates: Simple RSI, MACD Cross, Bollinger Bands, EMA Cross
  - Carica template e permette editing

**Tempo stimato**: 6-8 ore

---

### 1.3 Charts View (Nuovo)

**File**: `src/ui/ChartsView.h/.cpp` (da creare)

**Features da implementare**:

#### A. Candlestick Chart
- [ ] **BView Custom Drawing**
  - Override Draw() per disegnare candles
  - X-axis: tempo (timestamp formattato)
  - Y-axis: prezzo (auto-scaling)
  - Green candles (close > open), Red candles (close < open)
  - Wicks (high/low lines)

- [ ] **Zoom e Pan**
  - Mouse wheel per zoom in/out
  - Click+drag per pan orizzontale
  - Reset view button

#### B. Indicator Overlay
- [ ] **Overlay Indicators su Chart**
  - SMA/EMA: linea colorata sopra candles
  - Bollinger Bands: 3 linee (upper, middle, lower)
  - Select indicators da menu dropdown
  - Toggle visibility con checkbox

- [ ] **Sub-chart per Oscillatori**
  - RSI chart sotto main chart
  - MACD chart sotto main chart
  - Auto-scaling separato
  - Horizontal lines per thresholds (RSI: 30/70)

#### C. Trade Markers
- [ ] **Visualizza Entry/Exit Points**
  - Triangolo verde ▲ per buy entry
  - Triangolo rosso ▼ per sell exit
  - Tooltip con dettagli (prezzo, timestamp, reason)
  - Filter per backtest vs live trades

#### D. Symbol Selector
- [ ] **Symbol Dropdown**
  - Lista simboli disponibili nel database
  - Search field per filtrare
  - Favorite symbols (star button)

- [ ] **Timeframe Selector**
  - Buttons: 1m, 5m, 15m, 1h, 4h, 1d
  - Reload chart data on change

**Tempo stimato**: 10-12 ore (feature complessa)

**Alternative**: Considerare integrazione di libreria charting esterna se disponibile per Haiku (meno probabile).

---

## Parte 2: Live Trading System

### 2.1 Architecture Overview

```
User (UI)
  ↓
TradingEngine
  ↓
OrderManager → BinanceAPI → Binance Exchange
  ↓                ↓
Portfolio      WebSocket (real-time updates)
  ↓
Database (trade history)
```

---

### 2.2 Core Components

#### A. Trading Engine

**File**: `src/trading/TradingEngine.h/.cpp` (da creare)

**Responsabilità**:
- Esegue strategie su dati real-time
- Genera segnali di trading (buy/sell)
- Invia ordini a OrderManager
- Gestisce portfolio state
- Risk management (stop-loss, take-profit)

**API**:
```cpp
class TradingEngine {
public:
    // Configuration
    bool init(const Recipe& strategy, double initialCapital);
    void setMode(TradingMode mode); // PAPER, LIVE

    // Lifecycle
    void start();
    void stop();
    void pause();

    // Real-time data feed
    void onCandle(const Candle& candle);
    void onTick(const Ticker& tick); // For stop-loss monitoring

    // Strategy execution
    void evaluateStrategy();
    void executeSignal(TradeSignal signal);

    // State
    Portfolio& getPortfolio();
    std::vector<Trade> getActiveTrades();
    TradingStats getStats();

private:
    Recipe recipe;
    SignalGenerator signalGen;
    OrderManager orderManager;
    Portfolio portfolio;
    TradingMode mode; // PAPER or LIVE
};
```

---

#### B. Order Manager

**File**: `src/trading/OrderManager.h/.cpp` (da creare)

**Responsabilità**:
- Interfaccia tra TradingEngine e BinanceAPI
- Gestisce ordini: create, cancel, query status
- Order queue per retry logic
- Error handling e logging

**API**:
```cpp
class OrderManager {
public:
    bool init(BinanceAPI* api);

    // Order operations
    Order createMarketOrder(const std::string& symbol, OrderSide side, double quantity);
    Order createLimitOrder(const std::string& symbol, OrderSide side, double price, double quantity);
    Order createStopLossOrder(const std::string& symbol, double stopPrice, double quantity);
    bool cancelOrder(const std::string& orderId);

    // Order status
    Order getOrder(const std::string& orderId);
    std::vector<Order> getOpenOrders(const std::string& symbol);

    // Balance management
    std::vector<Balance> getBalances();
    double getAvailableBalance(const std::string& asset);

private:
    BinanceAPI* api;
    std::map<std::string, Order> orderCache;
    std::queue<Order> retryQueue;
};
```

---

#### C. Paper Trading Mode

**File**: `src/trading/PaperTrading.h/.cpp` (da creare)

**Responsabilità**:
- Simula trading senza soldi reali
- Usa prezzi real-time da Binance
- Applica commissioni realistiche
- Portfolio virtuale

**Features**:
- Stesso OrderManager API
- Ordini "eseguiti" istantaneamente a prezzo corrente + slippage simulato
- Salva trades in database separato (`paper_trades` table)
- Permette testing strategie senza rischio

**Tempo stimato Paper Trading**: 4-5 ore

---

#### D. Live Trading UI View

**File**: `src/ui/LiveTradingView.h/.cpp` (da creare)

**Layout**:

```
┌─────────────────────────────────────────────────────┐
│  Live Trading                                       │
├─────────────────────────────────────────────────────┤
│  Mode: [O] Paper Trading  [ ] Live Trading         │
│  ⚠️  WARNING: Live mode uses real money!           │
├─────────────────────────────────────────────────────┤
│  Strategy Configuration:                            │
│    Recipe: [Simple RSI        ▼]                   │
│    Symbol: [BTCUSDT]                                │
│    Capital: [$1000.00]                              │
│    Position Size: [10%]                             │
│                                                     │
│  [Start Trading] [Stop Trading]                    │
├─────────────────────────────────────────────────────┤
│  Active Positions:                                  │
│  ┌───────────────────────────────────────────────┐ │
│  │ BTCUSDT | LONG | 0.01 BTC | Entry: $50,000   │ │
│  │ P&L: +$250 (+5.0%) | Stop: $47,500           │ │
│  │ [Close Position]                               │ │
│  └───────────────────────────────────────────────┘ │
├─────────────────────────────────────────────────────┤
│  Recent Orders:                                     │
│  ┌───────────────────────────────────────────────┐ │
│  │ BUY  0.01 BTCUSDT @ $50,000 - FILLED          │ │
│  │ SELL 0.01 BTCUSDT @ $52,500 - FILLED          │ │
│  └───────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────┘
```

**Features**:
- [ ] Mode selector (Paper/Live) con warning
- [ ] Recipe selector
- [ ] Symbol e capital input
- [ ] Start/Stop buttons
- [ ] Active positions list con P&L real-time
- [ ] Recent orders log
- [ ] Connection status indicator

**Tempo stimato**: 5-6 ore

---

### 2.3 Implementation Plan - Live Trading

#### Phase 1: Order Management (Priorità Alta)
1. [ ] Implementare `OrderManager` base
2. [ ] Integrare con `BinanceAPI` esistente
3. [ ] Implementare `createMarketOrder()`
4. [ ] Implementare `getBalances()`
5. [ ] Test con ordini reali su Binance Testnet (se disponibile)

**Tempo**: 6-8 ore

#### Phase 2: Paper Trading (Priorità Alta)
1. [ ] Implementare `PaperTrading` engine
2. [ ] Virtual portfolio management
3. [ ] Order simulation con slippage
4. [ ] Database storage per paper trades
5. [ ] Testing completo

**Tempo**: 4-5 ore

#### Phase 3: Trading Engine (Priorità Alta)
1. [ ] Implementare `TradingEngine` base
2. [ ] Integrazione con `SignalGenerator`
3. [ ] Real-time candle processing
4. [ ] Stop-loss/Take-profit monitoring
5. [ ] Risk management checks

**Tempo**: 8-10 ore

#### Phase 4: Live Trading UI (Priorità Media)
1. [ ] Creare `LiveTradingView`
2. [ ] Mode selector (Paper/Live)
3. [ ] Active positions display
4. [ ] Order history display
5. [ ] Start/Stop controls

**Tempo**: 5-6 ore

#### Phase 5: Real-time Data Feed (Priorità Media)
1. [ ] WebSocket client per Binance
2. [ ] Candle stream subscription
3. [ ] Ticker stream per stop-loss
4. [ ] Reconnection logic
5. [ ] Feed data to TradingEngine

**Tempo**: 8-10 ore (complesso)

---

### 2.4 Safety Features (CRITICAL)

#### A. Pre-Trade Checks
- [ ] **Balance Verification**
  - Check sufficient funds before order
  - Reserve funds for open orders
  - Prevent over-leveraging

- [ ] **Order Size Limits**
  - Min order size (Binance: $10 USD equivalent)
  - Max order size (configurabile, es. 10% portfolio)
  - Max open positions (default: 3)

- [ ] **Price Sanity Checks**
  - Price deviation alert (>5% from last price)
  - Confirm dialog for large orders
  - Kill switch per emergenze

#### B. Error Handling
- [ ] **Network Errors**
  - Retry logic con exponential backoff
  - Offline mode detection
  - Alert user se disconnesso

- [ ] **API Errors**
  - Parse Binance error codes
  - Insufficient funds → log + alert
  - Rate limit exceeded → pause + retry
  - Invalid order → log + skip

- [ ] **Order Failures**
  - Track failed orders
  - Manual review queue
  - Alert notification

#### C. Logging & Audit Trail
- [ ] **Comprehensive Logging**
  - Ogni ordine: timestamp, symbol, side, quantity, price, status
  - Signal generation: indicator values, conditions met
  - Errors: full stack trace, context

- [ ] **Database Audit Table**
  - `trading_audit` table
  - Schema: id, timestamp, event_type, details (JSON), order_id, strategy_name

---

### 2.5 Risk Management

#### A. Stop-Loss Implementation
- [ ] **Trailing Stop-Loss**
  - Move stop-loss as price moves favorably
  - Trail by fixed % or ATR

- [ ] **Time-Based Stop**
  - Close position after N hours if no profit

- [ ] **Multi-Level Stops**
  - Partial close at first stop (50%)
  - Full close at second stop

#### B. Position Sizing
- [ ] **Fixed Percentage**
  - Trade size = portfolio * positionSizePercent

- [ ] **Kelly Criterion** (avanzato)
  - Optimal size based on win rate e avg win/loss

- [ ] **Volatility-Based**
  - Reduce size in high volatility

#### C. Portfolio Limits
- [ ] **Max Drawdown Limit**
  - Stop trading if drawdown > X%
  - Resume when drawdown recovers

- [ ] **Daily Loss Limit**
  - Max $ loss per day
  - Pause trading for rest of day if hit

- [ ] **Max Concurrent Positions**
  - Default: 3 positions
  - Diversification enforcement

---

## Testing Strategy

### UI Testing
- [ ] Manual testing ogni view
- [ ] Screenshot documentation
- [ ] User feedback loop

### Live Trading Testing
- [ ] **Unit Tests**
  - OrderManager logic
  - Portfolio calculations
  - Risk checks

- [ ] **Integration Tests**
  - OrderManager + BinanceAPI
  - TradingEngine + SignalGenerator
  - Database storage

- [ ] **Paper Trading Test**
  - Run per 1 settimana
  - Monitor all trades
  - Verify P&L calculations
  - Check stop-loss execution

- [ ] **Live Trading Test (CAUTIOUS)**
  - Start con $50-100 capital
  - Test 1 strategy only
  - Monitor 24/7 for 3 giorni
  - Gradual capital increase

---

## Security Considerations

### API Key Management
- [ ] **Secure Storage**
  - Store API keys in separate encrypted file
  - Use Haiku Keystore se disponibile
  - NEVER hardcode in source

- [ ] **API Key Permissions**
  - Read-only keys per market data
  - Trade-enabled keys ONLY per live trading
  - Enable IP whitelist su Binance
  - Disable withdrawals

### Code Review
- [ ] Review OrderManager code for vulnerabilities
- [ ] Validate all user inputs
- [ ] Sanitize symbols before API calls
- [ ] Check for injection vulnerabilities

---

## Deliverables

### Fase 6A: UI Improvements
- [x] Dashboard con portfolio live tracking
- [x] Recipe Editor visual builder completo
- [ ] Charts View con candlestick chart e indicators
- [ ] Backtest results storage in database

### Fase 6B: Live Trading
- [ ] OrderManager implementato e testato
- [ ] Paper Trading mode funzionante
- [ ] TradingEngine base implementato
- [ ] LiveTradingView UI completa
- [ ] Safety features e risk management
- [ ] WebSocket real-time data feed

---

## Timeline Estimate

| Task | Hours | Priority |
|------|-------|----------|
| Dashboard improvements | 3-4 | High |
| Recipe Editor visual builder | 6-8 | High |
| Charts View | 10-12 | Medium |
| OrderManager | 6-8 | **CRITICAL** |
| Paper Trading | 4-5 | **CRITICAL** |
| TradingEngine | 8-10 | High |
| LiveTradingView | 5-6 | High |
| WebSocket client | 8-10 | Medium |
| Testing & Debugging | 10-15 | **CRITICAL** |

**Total**: 60-78 hours (~2 settimane di lavoro full-time)

---

## Success Criteria

### UI
- [x] Dashboard shows real portfolio stats
- [ ] Recipe Editor can create/edit/save recipes visually
- [ ] Charts View displays candlesticks e indicators
- [ ] All views responsive e bug-free

### Live Trading
- [ ] Paper trading works for 1 week without errors
- [ ] Live trading executes orders correctly on Binance
- [ ] Stop-loss triggers correctly
- [ ] Portfolio P&L matches Binance account
- [ ] No security vulnerabilities
- [ ] Comprehensive audit logging

---

## Next Immediate Steps

1. **Completare Dashboard** (3-4 ore)
   - Implementare database storage per backtest results
   - Display recent backtests nella UI

2. **OrderManager Base** (6-8 ore)
   - Implementare createMarketOrder()
   - Implementare getBalances()
   - Test su Binance

3. **Paper Trading Engine** (4-5 ore)
   - Virtual portfolio
   - Order simulation
   - Testing completo

4. **LiveTradingView UI** (5-6 ore)
   - Basic layout
   - Mode selector
   - Start/Stop controls

**Priorità iniziale**: OrderManager + Paper Trading → permette testing strategie senza rischio!

---

**Status**: 🚧 **IN PROGRESS**
**Next Phase**: Implementation Sprint 1 - OrderManager + Paper Trading
