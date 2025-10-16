# Emiglio - Trading Bot Nativo per Haiku OS

## Analisi dei Trading Bot Esistenti

### 1. Freqtrade (Python, Open Source)
**Punti di Forza:**
- Architettura modulare con strategie personalizzabili via Python
- Sistema di backtesting robusto con dati storici
- Supporto multi-exchange (Binance, Kraken, Coinbase, etc.)
- Sistema di plugin per indicatori tecnici (RSI, MACD, Bollinger Bands)
- Dry-run mode per testing senza rischi
- Risk management integrato (stop-loss, take-profit, trailing stop)
- Ottimizzazione iperparametri con Hyperopt

**Punti di Debolezza:**
- Richiede Python runtime (pesante)
- Configurazione complessa per principianti
- Performance limitata per HFT (High Frequency Trading)

### 2. Gekko (Node.js, Deprecato ma Influente)
**Punti di Forza:**
- Interfaccia web intuitiva e user-friendly
- Sistema di strategie basato su file JavaScript modulari
- Buon sistema di visualizzazione grafici e performance
- Backtesting integrato con UI

**Punti di Debolezza:**
- Non più mantenuto (ultimo aggiornamento 2018)
- Performance limitate
- Mancanza di funzionalità moderne (DeFi, DEX)

### 3. Zenbot (Node.js)
**Punti di Forza:**
- Algoritmi genetici per ottimizzazione strategie automatica
- Supporto per trading ad alta frequenza
- Architettura a plugin estensibile
- CLI semplice da usare

**Punti di Debolezza:**
- Documentazione limitata
- Community piccola
- Configurazione non intuitiva

### 4. Jesse (Python)
**Punti di Forza:**
- Focus su backtesting accurato con dati tick-by-tick
- Ottimizzazione strategie con algoritmi genetici avanzati
- Live trading con risk management professionale
- Framework moderno e ben documentato
- Supporto per multiple timeframes

**Punti di Debolezza:**
- Versione pro a pagamento per funzionalità avanzate
- Richiede conoscenza Python avanzata
- Setup complesso

### 5. 3Commas / Cryptohopper (Commercial SaaS)
**Punti di Forza:**
- Interfaccia user-friendly senza coding
- Trading bot pre-configurati
- Social trading (copia strategie altri trader)
- Integrazione con TradingView
- App mobile

**Punti di Debolezza:**
- Costi mensili elevati ($30-100+/mese)
- Dati su server terzi (privacy/security)
- Limitata personalizzazione
- Dipendenza da servizio cloud

---

## Analisi delle API Native di Haiku OS

### Application Kit (libapp.so)
**BApplication**
- Entry point per applicazioni Haiku native
- Gestione messaggi di sistema
- Event loop principale

**BLooper / BHandler**
- Threading asincrono per operazioni I/O
- Message passing tra thread
- Ideale per gestire richieste API senza bloccare UI

**BMessage**
- Sistema di messaging inter-thread/inter-process
- Serializzazione dati
- Perfetto per comunicazione tra componenti

### Interface Kit (libinterface.so)
**BWindow / BView**
- Finestre native Haiku con look & feel consistente
- Layout management moderno (BLayout, BGroupLayout, etc.)

**BListView / BColumnListView**
- Visualizzazione liste trades, ordini, portafoglio
- Ordinamento e filtering integrati

**BMenuBar / BToolBar**
- Menu nativi Haiku
- Toolbar con azioni rapide

**BStatusBar / BTextControl**
- Visualizzazione progress (download dati storici)
- Input campi per configurazione

### Network Kit (libnetwork.so)
**BHttpRequest / BUrlRequest**
- HTTP/HTTPS requests per API REST
- Supporto JSON (parsing manuale o tramite lib esterne)
- SSL/TLS integrato

**BNetworkAddress / BSocket**
- WebSocket per stream real-time prezzi
- Connessioni persistenti agli exchange

### Storage Kit (libstorage.so)
**BFile / BDirectory / BPath**
- Storage locale ricette di trading (JSON/YAML)
- Cache dati storici per backtesting
- Log operazioni

**BQuery / BNode**
- Ricerca file con attributi estesi
- Metadata per organizzazione ricette

### Support Kit (libsupport.so)
**BString**
- Manipolazione stringhe UTF-8
- Parsing response API

**BList / BObjectList**
- Collezioni dati native
- Gestione liste ordini, trades, candele

---

## Architettura Emiglio

### Componenti Principali

```
Emiglio/
├── src/
│   ├── core/
│   │   ├── Application.cpp         # BApplication main
│   │   ├── TradingEngine.cpp       # Motore principale trading
│   │   ├── OrderManager.cpp        # Gestione ordini (buy/sell/cancel)
│   │   ├── Portfolio.cpp           # Gestione portafoglio e bilanci
│   │   └── RiskManager.cpp         # Stop-loss, position sizing, risk
│   │
│   ├── data/
│   │   ├── MarketDataFetcher.cpp   # Download dati da exchange
│   │   ├── DataStorage.cpp         # SQLite storage candele/trades
│   │   ├── HistoricalData.cpp      # Gestione dati storici
│   │   └── RealtimeStream.cpp      # WebSocket stream prezzi live
│   │
│   ├── exchange/
│   │   ├── ExchangeAPI.cpp         # Interfaccia base exchange
│   │   ├── BinanceAPI.cpp          # Implementazione Binance
│   │   ├── CoinbaseAPI.cpp         # Implementazione Coinbase
│   │   ├── KrakenAPI.cpp           # Implementazione Kraken
│   │   └── GeminiAPI.cpp           # Implementazione Gemini
│   │
│   ├── strategy/
│   │   ├── Strategy.cpp            # Base class per strategie
│   │   ├── RecipeLoader.cpp        # Parser ricette JSON/YAML
│   │   ├── Indicators.cpp          # RSI, MACD, EMA, Bollinger, etc.
│   │   └── SignalGenerator.cpp     # Generazione segnali buy/sell
│   │
│   ├── ai/
│   │   ├── AIAnalyzer.cpp          # Interfaccia AI services
│   │   ├── GeminiClient.cpp        # Google Gemini API
│   │   ├── OpenAIClient.cpp        # ChatGPT API
│   │   ├── SentimentAnalysis.cpp   # Analisi sentiment news/social
│   │   └── PricePredictor.cpp      # Predizioni AI-based
│   │
│   ├── backtest/
│   │   ├── BacktestEngine.cpp      # Motore backtesting
│   │   ├── Simulator.cpp           # Simulazione ordini/fills
│   │   ├── PerformanceAnalyzer.cpp # Metriche (Sharpe, drawdown, etc.)
│   │   └── ReportGenerator.cpp     # Report HTML/PDF risultati
│   │
│   ├── ui/
│   │   ├── MainWindow.cpp          # Finestra principale
│   │   ├── DashboardView.cpp       # Dashboard overview
│   │   ├── ChartView.cpp           # Grafici candele + indicatori
│   │   ├── TradesView.cpp          # Lista trades eseguiti
│   │   ├── OrdersView.cpp          # Ordini attivi/storici
│   │   ├── RecipeEditorView.cpp    # Editor ricette strategie
│   │   ├── BacktestView.cpp        # UI backtesting
│   │   └── SettingsView.cpp        # Configurazione bot
│   │
│   └── utils/
│       ├── JsonParser.cpp          # Parsing JSON (RapidJSON/nlohmann)
│       ├── Logger.cpp              # Logging operazioni
│       ├── Config.cpp              # Configurazione globale
│       └── Crypto.cpp              # HMAC-SHA256 per autenticazione API
│
├── recipes/
│   ├── scalping_rsi.json           # Strategia scalping basata su RSI
│   ├── trend_following_ema.json    # Trend following con EMA crossover
│   ├── mean_reversion_bb.json      # Mean reversion con Bollinger Bands
│   ├── ai_sentiment_btc.json       # AI sentiment analysis Bitcoin
│   └── dca_strategy.json           # Dollar Cost Averaging
│
├── data/
│   ├── historical/                 # Dati storici candele (SQLite)
│   ├── trades.db                   # Database trades eseguiti
│   └── cache/                      # Cache API responses
│
├── config/
│   ├── exchanges.json              # Configurazione API keys exchanges
│   ├── ai_services.json            # API keys Gemini/OpenAI
│   └── settings.json               # Settings generali bot
│
├── Makefile                        # Build nativo Haiku (no CMake)
└── README.md                       # Documentazione
```

### Pattern Architetturali

**1. Strategy Pattern per Trading Strategies**
- Interfaccia `IStrategy` con metodi `analyze()`, `shouldBuy()`, `shouldSell()`
- Implementazioni concrete per ogni strategia
- Caricamento dinamico da ricette JSON

**2. Observer Pattern per Eventi**
- Eventi: `OnPriceUpdate`, `OnOrderFilled`, `OnNewCandle`
- Observers: UI components, logging, alerting

**3. Factory Pattern per Exchange APIs**
- `ExchangeFactory::create("binance")` ritorna `BinanceAPI*`
- Interfaccia unificata per tutti gli exchange

**4. Repository Pattern per Data Access**
- `HistoricalDataRepository` per accesso dati storici
- `TradeRepository` per storico operazioni
- Astrazione SQLite

---

## Sistema di Ricette (Recipe System)

### Concetto
Le ricette sono file JSON/YAML che definiscono strategie di trading in modo dichiarativo, senza codice. L'utente può:
- Creare nuove ricette via UI editor
- Modificare parametri esistenti
- Condividere ricette con la community
- Testare ricette in backtesting prima del live trading

### Struttura Ricetta JSON

```json
{
  "name": "RSI Scalping Bitcoin",
  "version": "1.0",
  "description": "Strategia scalping su BTC/USDT basata su RSI oversold/overbought",
  "author": "Your Name",
  "created": "2025-10-12",

  "market": {
    "exchange": "binance",
    "symbol": "BTC/USDT",
    "timeframe": "5m"
  },

  "capital": {
    "initial": 1000,
    "position_size_percent": 10,
    "max_open_positions": 3
  },

  "risk_management": {
    "stop_loss_percent": 2.0,
    "take_profit_percent": 5.0,
    "trailing_stop": true,
    "trailing_stop_percent": 1.5,
    "max_daily_loss_percent": 5.0
  },

  "indicators": [
    {
      "name": "rsi",
      "period": 14,
      "overbought": 70,
      "oversold": 30
    },
    {
      "name": "ema",
      "periods": [9, 21, 50]
    },
    {
      "name": "volume_sma",
      "period": 20
    }
  ],

  "entry_conditions": {
    "logic": "AND",
    "rules": [
      {
        "indicator": "rsi",
        "operator": "<",
        "value": 30,
        "description": "RSI in oversold"
      },
      {
        "indicator": "close",
        "operator": ">",
        "reference": "ema_50",
        "description": "Prezzo sopra EMA 50 (trend rialzista)"
      },
      {
        "indicator": "volume",
        "operator": ">",
        "reference": "volume_sma",
        "multiplier": 1.5,
        "description": "Volume superiore a media x1.5"
      }
    ]
  },

  "exit_conditions": {
    "logic": "OR",
    "rules": [
      {
        "indicator": "rsi",
        "operator": ">",
        "value": 70,
        "description": "RSI in overbought (prendi profitto)"
      },
      {
        "type": "stop_loss",
        "description": "Stop loss colpito"
      },
      {
        "type": "take_profit",
        "description": "Take profit colpito"
      }
    ]
  },

  "ai_assistance": {
    "enabled": true,
    "provider": "gemini",
    "features": [
      {
        "type": "sentiment_analysis",
        "sources": ["news", "twitter"],
        "symbols": ["BTC"],
        "influence_weight": 0.2
      },
      {
        "type": "price_prediction",
        "horizon": "1h",
        "confidence_threshold": 0.7,
        "influence_weight": 0.15
      },
      {
        "type": "anomaly_detection",
        "alert_on_anomaly": true
      }
    ]
  },

  "notifications": {
    "on_entry": true,
    "on_exit": true,
    "on_error": true,
    "channels": ["ui", "log"]
  },

  "backtest_config": {
    "start_date": "2024-01-01",
    "end_date": "2025-10-01",
    "commission": 0.001,
    "slippage": 0.0005
  }
}
```

### Esempi di Ricette

**1. Trend Following con EMA Crossover**
```json
{
  "name": "EMA Crossover Trend Following",
  "entry_conditions": {
    "logic": "AND",
    "rules": [
      {
        "type": "crossover",
        "fast": "ema_9",
        "slow": "ema_21",
        "direction": "up"
      },
      {
        "indicator": "close",
        "operator": ">",
        "reference": "ema_50"
      }
    ]
  }
}
```

**2. Mean Reversion Bollinger Bands**
```json
{
  "name": "Bollinger Bands Mean Reversion",
  "indicators": [
    {
      "name": "bollinger_bands",
      "period": 20,
      "std_dev": 2
    }
  ],
  "entry_conditions": {
    "rules": [
      {
        "indicator": "close",
        "operator": "<",
        "reference": "bb_lower",
        "description": "Prezzo sotto banda inferiore"
      }
    ]
  },
  "exit_conditions": {
    "rules": [
      {
        "indicator": "close",
        "operator": ">",
        "reference": "bb_middle",
        "description": "Ritorno alla media"
      }
    ]
  }
}
```

**3. AI-Enhanced Sentiment Strategy**
```json
{
  "name": "AI Sentiment + Technical",
  "ai_assistance": {
    "enabled": true,
    "provider": "chatgpt",
    "features": [
      {
        "type": "sentiment_analysis",
        "sources": ["reddit", "twitter", "news"],
        "symbols": ["BTC", "ETH"],
        "sentiment_threshold": 0.6,
        "influence_weight": 0.4
      }
    ]
  },
  "entry_conditions": {
    "logic": "AND",
    "rules": [
      {
        "type": "ai_sentiment",
        "operator": ">",
        "value": 0.6,
        "description": "Sentiment positivo"
      },
      {
        "indicator": "rsi",
        "operator": "<",
        "value": 50
      }
    ]
  }
}
```

---

## Integrazione AI Services (Gemini / ChatGPT)

### Funzionalità AI

**1. Sentiment Analysis**
- Analisi news (CoinDesk, CoinTelegraph, Bloomberg Crypto)
- Social media sentiment (Twitter/X, Reddit r/cryptocurrency)
- Fear & Greed Index
- Output: score -1.0 (very bearish) to +1.0 (very bullish)

**2. Price Prediction**
- Input: dati storici OHLCV + indicatori tecnici
- Model: Gemini Pro o GPT-4 con prompt engineering
- Output: predizione prezzo + confidence score
- Timeframes: 15m, 1h, 4h, 1d

**3. Anomaly Detection**
- Rilevamento pattern anomali nei prezzi/volumi
- Alert su movimenti sospetti (pump & dump)
- Suggerimento revisione strategia

**4. Strategy Optimization**
- AI analizza performance backtest
- Suggerisce modifiche parametri (es. RSI period, stop-loss %)
- Genera varianti ricetta da testare

### Architettura Chiamate AI

```cpp
// Interfaccia base
class IAIService {
public:
    virtual AIResponse analyzeSentiment(const string& symbol,
                                       const vector<string>& sources) = 0;
    virtual AIResponse predictPrice(const string& symbol,
                                   const MarketData& data,
                                   const string& timeframe) = 0;
    virtual AIResponse detectAnomalies(const MarketData& data) = 0;
    virtual AIResponse optimizeStrategy(const BacktestResults& results) = 0;
};

// Implementazione Gemini
class GeminiClient : public IAIService {
private:
    BString apiKey;
    BHttpRequest* httpClient;

    BString buildPrompt(const string& type, const json& data);
    AIResponse parseResponse(const json& response);

public:
    AIResponse analyzeSentiment(...) override;
    // ...
};
```

### Prompt Engineering per Trading

**Sentiment Analysis Prompt**
```
Analyze the following cryptocurrency market data and news articles
for Bitcoin (BTC):

Recent News Headlines:
- [news1]
- [news2]
- [news3]

Social Media Trends:
- Twitter mentions: [count]
- Reddit posts sentiment: [summary]

Current Price Action:
- Price: $[price]
- 24h Change: [percent]%
- Volume: $[volume]

Provide:
1. Overall sentiment score (-1.0 to +1.0)
2. Key factors influencing sentiment
3. Confidence level (0-1)
4. Timeframe relevance (short/medium/long term)

Return response in JSON format.
```

**Price Prediction Prompt**
```
You are a cryptocurrency trading analyst. Analyze this Bitcoin (BTC/USDT)
data and predict the price movement for the next 1 hour:

Technical Indicators:
- RSI(14): [value]
- MACD: [value]
- EMA(9): [value], EMA(21): [value], EMA(50): [value]
- Bollinger Bands: Upper [value], Middle [value], Lower [value]
- Volume: [value] (24h avg: [value])

Recent Price Action (last 24h):
- High: $[high]
- Low: $[low]
- Current: $[current]
- Trend: [uptrend/downtrend/sideways]

Market Context:
- Bitcoin Dominance: [percent]%
- Fear & Greed Index: [value]

Provide:
1. Price prediction for next 1h (range: min-max)
2. Confidence score (0-1)
3. Key supporting factors
4. Risk factors

Return JSON format.
```

### Rate Limiting & Caching
- Cache AI responses per 15-30 minuti (evitare costi eccessivi)
- Rate limiting: max 10-20 requests/minuto
- Fallback a logica tecnica pura se AI non disponibile
- Budget giornaliero configurabile per API calls

---

## Sistema di Backtesting e Simulazione Offline

### Obiettivi
1. **Test strategie su dati storici** prima del live trading
2. **Simulazione realistica** con commissioni, slippage, latenza
3. **Ottimizzazione parametri** (hyperparameter tuning)
4. **Analisi performance** con metriche professionali

### Pipeline Backtesting

```
1. Download Dati Storici
   └─> Exchange API → SQLite cache

2. Load Ricetta Strategia
   └─> JSON parser → Strategy object

3. Simulazione
   ├─> Itera candele storiche
   ├─> Calcola indicatori
   ├─> Evalua entry/exit conditions
   ├─> Simula ordini (fill, commissioni, slippage)
   └─> Aggiorna portfolio virtuale

4. Analisi Risultati
   ├─> Calcola metriche (ROI, Sharpe, drawdown)
   ├─> Genera equity curve
   ├─> Lista trades
   └─> Report HTML/PDF

5. Ottimizzazione (opzionale)
   ├─> Grid search parametri
   ├─> Genetic algorithm
   └─> Identifica best params
```

### Metriche di Performance

**Metriche Base**
- **Total Return**: rendimento totale %
- **Annual Return**: rendimento annualizzato
- **Total Trades**: numero trade eseguiti
- **Winning Trades**: trade in profitto
- **Losing Trades**: trade in perdita
- **Win Rate**: % trade vincenti
- **Average Win/Loss**: media profit/loss per trade

**Metriche Avanzate**
- **Sharpe Ratio**: return/risk ratio (target > 1.5)
- **Sortino Ratio**: come Sharpe ma considera solo downside risk
- **Max Drawdown**: massima perdita picco-valle %
- **Recovery Time**: tempo per recuperare da max drawdown
- **Profit Factor**: gross profit / gross loss (target > 1.5)
- **Calmar Ratio**: annual return / max drawdown
- **Expectancy**: guadagno atteso per trade

**Metriche Risk**
- **VaR (Value at Risk)**: perdita massima attesa con X% confidenza
- **CVaR (Conditional VaR)**: perdita media oltre VaR
- **Beta**: correlazione con mercato (BTC)
- **Ulcer Index**: stress da drawdown

### Simulazione Realistica

**Commissioni**
- Maker fee: 0.1% (Binance standard)
- Taker fee: 0.1%
- Configurabile per exchange

**Slippage**
- Modello: prezzo esecuzione = prezzo segnale ± (slippage% + volatilità)
- Slippage base: 0.05-0.1%
- Aumenta con volatilità e volume basso
- Configurabile per symbol

**Latenza**
- Delay tra segnale e esecuzione: 100-500ms
- Simulato con timestamp offset
- Importante per strategie scalping

**Liquidità**
- Ordini grandi potrebbero non essere riempiti completamente
- Partial fills simulation
- Order book depth consideration (avanzato)

### Storage Dati Storici

**SQLite Schema**
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

CREATE TABLE trades (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    strategy_name TEXT NOT NULL,
    backtest_id TEXT,
    timestamp INTEGER NOT NULL,
    symbol TEXT NOT NULL,
    side TEXT NOT NULL, -- 'buy' or 'sell'
    price REAL NOT NULL,
    quantity REAL NOT NULL,
    commission REAL NOT NULL,
    pnl REAL,
    portfolio_value REAL
);

CREATE TABLE backtest_results (
    id TEXT PRIMARY KEY,
    recipe_name TEXT NOT NULL,
    start_date INTEGER NOT NULL,
    end_date INTEGER NOT NULL,
    initial_capital REAL NOT NULL,
    final_capital REAL NOT NULL,
    total_return REAL NOT NULL,
    sharpe_ratio REAL,
    max_drawdown REAL,
    win_rate REAL,
    total_trades INTEGER,
    created_at INTEGER NOT NULL,
    config TEXT -- JSON della ricetta usata
);
```

### Download Dati Storici

```cpp
class MarketDataFetcher {
public:
    // Download candele storiche da exchange
    status_t fetchHistoricalData(
        const string& exchange,
        const string& symbol,
        const string& timeframe,
        time_t startDate,
        time_t endDate,
        ProgressCallback callback
    );

    // Update dati (solo nuove candele)
    status_t updateData(
        const string& exchange,
        const string& symbol,
        const string& timeframe
    );

private:
    DataStorage* storage;
    ExchangeAPI* api;
};
```

### Ottimizzazione Parametri

**Grid Search**
```cpp
// Esempio: ottimizza RSI period e thresholds
vector<BacktestResult> gridSearch = {
    for (int rsiPeriod = 10; rsiPeriod <= 20; rsiPeriod += 2) {
        for (int oversold = 20; oversold <= 35; oversold += 5) {
            for (int overbought = 65; overbought <= 80; overbought += 5) {
                Recipe recipe = loadRecipe("rsi_strategy.json");
                recipe.setParam("rsi.period", rsiPeriod);
                recipe.setParam("rsi.oversold", oversold);
                recipe.setParam("rsi.overbought", overbought);

                BacktestResult result = backtestEngine.run(recipe);
                results.push_back(result);
            }
        }
    }
};

// Ordina per Sharpe Ratio
sort(results, [](a, b) { return a.sharpeRatio > b.sharpeRatio; });
BacktestResult best = results[0];
```

**Genetic Algorithm** (avanzato)
- Popolazione ricette con parametri randomici
- Fitness function: Sharpe Ratio o Profit Factor
- Crossover e mutation parametri
- Evoluzione per 50-100 generazioni
- Output: ricetta ottimizzata

### Report Backtesting

**Formato HTML con:**
- Summary metriche principali
- Equity curve (grafico portfolio value nel tempo)
- Drawdown chart
- Lista trade (tabella con date, symbol, side, price, pnl)
- Distribuzione profit/loss (istogramma)
- Monthly returns heatmap
- Configurazione strategia usata

**Export PDF** per condivisione/archiviazione

---

## Build System per Haiku OS

### Makefile Nativo (NO WSL)

```makefile
# Emiglio Makefile
# Build solo su Haiku OS

NAME = Emiglio
TYPE = APP
APP_MIME_SIG = application/x-vnd.Emiglio

SRCS = \
    src/core/Application.cpp \
    src/core/TradingEngine.cpp \
    src/core/OrderManager.cpp \
    src/core/Portfolio.cpp \
    src/core/RiskManager.cpp \
    src/data/MarketDataFetcher.cpp \
    src/data/DataStorage.cpp \
    src/data/HistoricalData.cpp \
    src/data/RealtimeStream.cpp \
    src/exchange/ExchangeAPI.cpp \
    src/exchange/BinanceAPI.cpp \
    src/exchange/CoinbaseAPI.cpp \
    src/strategy/Strategy.cpp \
    src/strategy/RecipeLoader.cpp \
    src/strategy/Indicators.cpp \
    src/strategy/SignalGenerator.cpp \
    src/ai/AIAnalyzer.cpp \
    src/ai/GeminiClient.cpp \
    src/ai/OpenAIClient.cpp \
    src/backtest/BacktestEngine.cpp \
    src/backtest/Simulator.cpp \
    src/backtest/PerformanceAnalyzer.cpp \
    src/ui/MainWindow.cpp \
    src/ui/DashboardView.cpp \
    src/ui/ChartView.cpp \
    src/ui/RecipeEditorView.cpp \
    src/ui/BacktestView.cpp \
    src/utils/JsonParser.cpp \
    src/utils/Logger.cpp \
    src/utils/Config.cpp \
    src/utils/Crypto.cpp

RDEFS = HaikuCryptoTrader.rdef

LIBS = \
    be \
    network \
    translation \
    tracker \
    sqlite3 \
    ssl \
    crypto

SYSTEM_INCLUDE_PATHS = \
    /boot/system/develop/headers/private/shared

LOCAL_INCLUDE_PATHS = \
    src/core \
    src/data \
    src/exchange \
    src/strategy \
    src/ai \
    src/backtest \
    src/ui \
    src/utils \
    external/rapidjson/include \
    external/websocketpp

OPTIMIZE := FULL
LOCALES = en it

DEFINES = \
    _BUILDING_EMIGLIO=1

WARNINGS = ALL

# Compiler flags
COMPILER_FLAGS = -std=c++17 -Wall -Wextra

# Linker flags
LINKER_FLAGS =

## Include Haiku generic Makefile
include $(BUILDHOME)/etc/makefile-engine
```

### Dipendenze Esterne

**1. RapidJSON** (header-only)
- JSON parsing ad alte performance
- No build required, solo include

**2. WebSocket++** (header-only)
- WebSocket client per stream real-time
- Basato su Boost.Asio o standalone asio

**3. SQLite3**
- Disponibile su Haiku via pkgman: `pkgman install sqlite`

**4. OpenSSL**
- Per HTTPS e HMAC-SHA256 (auth API)
- Disponibile su Haiku: `pkgman install openssl`

**5. libcurl** (opzionale)
- Alternativa a BHttpRequest per HTTP requests
- `pkgman install curl`

### Setup Progetto su Haiku

```bash
# 1. Installa dipendenze
pkgman install sqlite openssl curl_devel

# 2. Clone o copia progetto
cd /boot/home/projects
# (copia da WSL o git clone)

# 3. Download dipendenze header-only
cd Emiglio
mkdir -p external
cd external

# RapidJSON
git clone https://github.com/Tencent/rapidjson.git

# WebSocket++
git clone https://github.com/zaphoyd/websocketpp.git

# 4. Build
cd ..
make

# 5. Run
./Emiglio
```

### Resource Definition (.rdef)

```cpp
// Emiglio.rdef
resource app_signature "application/x-vnd.Emiglio";

resource app_flags B_MULTIPLE_LAUNCH | B_ARGV_ONLY;

resource app_version {
    major  = 1,
    middle = 0,
    minor  = 0,

    variety = B_APPV_ALPHA,
    internal = 0,

    short_info = "Emiglio",
    long_info  = "Native cryptocurrency trading bot for Haiku OS"
};

resource vector_icon {
    // Icona SVG (trading chart icon)
    $"6E636966080500020106023C55B83A20A4BF28113D98BB49F3364A392C00"
    // ... (vector icon data)
};
```

---

## Roadmap Implementazione

### Phase 1: Core Infrastructure (2-3 settimane)
- [ ] Setup progetto e Makefile
- [ ] BApplication skeleton
- [ ] JSON parser (RapidJSON integration)
- [ ] Config loader
- [ ] Logger system
- [ ] SQLite data storage
- [ ] Basic UI (MainWindow, placeholder views)

### Phase 2: Exchange Integration (2 settimane)
- [ ] ExchangeAPI base class
- [ ] BinanceAPI implementation (REST)
- [ ] HTTP client con BHttpRequest
- [ ] HMAC-SHA256 authentication
- [ ] Market data fetching (OHLCV)
- [ ] Order placement (buy/sell)
- [ ] Portfolio sync

### Phase 3: Strategy Engine (2 settimane)
- [ ] Technical indicators (RSI, EMA, MACD, Bollinger)
- [ ] Recipe loader e parser
- [ ] Strategy evaluation engine
- [ ] Signal generator
- [ ] Entry/exit conditions logic

### Phase 4: Backtesting (2 settimane)
- [ ] Historical data downloader
- [ ] Backtest engine
- [ ] Trade simulator
- [ ] Performance analyzer
- [ ] Report generator (HTML)
- [ ] UI for backtesting

### Phase 5: Live Trading (1-2 settimane)
- [ ] Order manager
- [ ] Risk manager (stop-loss, position sizing)
- [ ] Real-time data stream (WebSocket)
- [ ] Trading engine loop
- [ ] Portfolio tracker
- [ ] Notifications

### Phase 6: AI Integration (2 settimane)
- [ ] Gemini API client
- [ ] ChatGPT API client
- [ ] Sentiment analysis
- [ ] Price prediction
- [ ] Anomaly detection
- [ ] Strategy optimization suggestions

### Phase 7: Advanced UI (2 settimane)
- [ ] Real-time charts (candlesticks + indicators)
- [ ] Dashboard con portfolio value
- [ ] Trades list view
- [ ] Orders management
- [ ] Recipe editor (JSON)
- [ ] Settings panel

### Phase 8: Testing & Polish (1-2 settimane)
- [ ] Unit tests
- [ ] Integration tests
- [ ] Bug fixing
- [ ] Performance optimization
- [ ] Documentation
- [ ] Example recipes

**Total: ~12-15 settimane (3-4 mesi)**

---

## Considerazioni di Sicurezza

### API Keys Storage
- **NO hardcode** in codice sorgente
- Storage in `~/config/settings/Emiglio/exchanges.json`
- Permissions: `chmod 600` (solo owner read/write)
- Encryption con AES-256 (password master opzionale)
- API keys con permessi minimi (no withdrawal)

### Risk Management
- **Position limits**: max % portfolio per trade
- **Daily loss limit**: stop trading se perdita giornaliera > X%
- **Max open positions**: limite numero trade simultanei
- **Whitelist symbols**: trade solo su coppie approvate
- **Dry-run mode**: test senza soldi reali

### Network Security
- **HTTPS only** per API calls
- **Certificate validation** (no self-signed)
- **Rate limiting** rispettato (evitare ban)
- **API errors handling** (retry con exponential backoff)

### Code Security
- **Input validation** per tutti i parametri utente
- **SQL injection prevention** (prepared statements)
- **Memory safety** (no buffer overflow)
- **Logging sensitive data**: censurare API keys nei log

---

## Vantaggi Emiglio vs Competitori

### 1. Native Performance
- **C++ puro**: velocità superiore vs Python/Node.js
- **Low latency**: critico per scalping
- **Low resource usage**: Haiku è leggero, bot anche

### 2. Sistema Ricette Intuitivo
- **No coding required**: chiunque può creare strategie
- **Visual editor**: UI per editing ricette
- **Condivisibilità**: import/export ricette
- **Versioning**: storico modifiche ricette

### 3. AI Integration Nativa
- **Gemini/ChatGPT** per analisi avanzate
- **Sentiment analysis** da news/social
- **Adaptive strategies**: AI suggerisce ottimizzazioni
- **Predictive analytics**: forecasting prezzi

### 4. Backtesting Robusto
- **Simulazione realistica**: commissioni, slippage, latenza
- **Metriche professionali**: Sharpe, Sortino, drawdown
- **Ottimizzazione automatica**: grid search, genetic algorithm
- **Report dettagliati**: HTML/PDF

### 5. Privacy & Control
- **Self-hosted**: nessun cloud, dati locali
- **Open source** (potenziale): community-driven
- **No subscription fees**: gratis (escluse API exchange/AI)

### 6. Haiku OS Integration
- **Look & feel nativo**: consistent con OS
- **Lightweight**: gira su hardware modesto
- **Stability**: Haiku è stabile e deterministico

---

## Note Finali

### Sviluppo su WSL
**ATTENZIONE**: Emiglio è progettato per **Haiku OS nativo** e utilizza API specifiche (BApplication, BWindow, etc.) che **NON sono disponibili su Linux/WSL**.

**IMPORTANTE**: Non compilare su WSL. Il codice verrà scritto e preparato su WSL, ma la compilazione avverrà **solo su Haiku OS**.

Per sviluppare:
1. **Opzione consigliata**: Sviluppa su WSL (editing, git), testa e compila su Haiku OS (VM o bare metal)
2. **Workflow suggerito**:
   - Scrivi codice su WSL (editor, git, Claude Code)
   - Sincronizza file su Haiku via shared folder o git
   - Compila e testa su Haiku nativo
   - Itera

### Prossimi Passi
1. Setup ambiente Haiku OS
2. Iniziare con Phase 1 (core infrastructure)
3. Testare singolarmente ogni componente
4. Iterare su feedback backtesting
5. Paper trading prima di live trading

### Community & Support
- **Haiku Forum**: https://discuss.haiku-os.org/
- **Haiku Development Docs**: https://www.haiku-os.org/docs/
- **BeBook (API Reference)**: https://www.haiku-os.org/docs/bebook/

---

## Esempio Workflow Utente Tipico

1. **Setup Iniziale**
   - Installa Emiglio
   - Configura API keys exchange (Binance)
   - Configura API key Gemini (opzionale)

2. **Download Dati Storici**
   - Menu: Data → Download Historical Data
   - Seleziona: Binance, BTC/USDT, 5m timeframe
   - Range: ultimi 6 mesi
   - Download in background (progress bar)

3. **Crea Ricetta**
   - Menu: Strategy → New Recipe
   - Template: "RSI Scalping"
   - Modifica parametri nel JSON editor:
     - RSI period: 14
     - Oversold: 30
     - Overbought: 70
     - Stop loss: 2%
   - Enable AI sentiment analysis
   - Save: "my_rsi_btc.json"

4. **Backtesting**
   - Menu: Backtest → Run Backtest
   - Select recipe: "my_rsi_btc.json"
   - Date range: last 3 months
   - Initial capital: $1000
   - Run → Attesa 10-30 secondi
   - Risultati:
     - Total return: +15.3%
     - Sharpe: 1.8
     - Max drawdown: -8.2%
     - Win rate: 58%
   - Review trades list
   - Export report HTML

5. **Ottimizzazione (opzionale)**
   - Menu: Backtest → Optimize Parameters
   - Select params to optimize: RSI period, oversold/overbought
   - Run grid search (2-5 minuti)
   - Best params found: RSI 12, oversold 28, overbought 72
   - Update recipe con nuovi parametri

6. **Paper Trading**
   - Menu: Trading → Start Paper Trading
   - Select recipe: "my_rsi_btc.json"
   - Capital: $1000 (virtuale)
   - Start → Bot gira in dry-run mode
   - Monitora dashboard per 1-2 giorni
   - Verifica performance reale

7. **Live Trading**
   - Soddisfatto di paper trading
   - Menu: Trading → Start Live Trading
   - **Conferma**: "Are you sure? Real money at risk!"
   - Select recipe + capital allocation
   - Start → Bot esegue trade reali
   - Monitora attivamente
   - Stop quando necessario

8. **AI-Enhanced Analysis**
   - Durante live trading
   - AI panel mostra:
     - Sentiment score: 0.65 (bullish)
     - Price prediction 1h: $67,500-$68,200
     - Confidence: 72%
   - AI suggerisce: "Consider reducing position size, high volatility detected"
   - Utente modifica recipe o stop manuale

---

## Filosofia del Nome "Emiglio"

Il nome **Emiglio** è stato scelto per dare un tocco personale e umano al trading bot, in contrasto con i nomi tecnici tipici del settore. Emiglio rappresenta:

- **Affidabilità**: Come un assistente fidato che lavora per te 24/7
- **Intelligenza**: Un bot che impara e si adatta grazie all'AI
- **Semplicità**: Un nome facile da ricordare e pronunciare
- **Personalità**: Non è solo un software, è il tuo trading partner

---

**Fine Documento**

*Questo documento rappresenta la visione completa del progetto Emiglio. L'implementazione richiederà iterazioni e aggiustamenti basati su test reali e feedback. Ricorda: il codice viene sviluppato su WSL ma compilato SOLO su Haiku OS.*
