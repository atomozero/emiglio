# EMIGLIO - ANALISI COMPARATIVA RICETTE

Generated: 2025-10-18
Period analizzato: Ultimo anno (365 giorni)  
Simboli: ETHUSDT, EURUSDT, ETHEUR  
Timeframes: 1h, 1d

## RICETTE DISPONIBILI

### 1. Simple RSI Strategy
**File:** simple_rsi.json  
**Descrizione:** Buy when RSI < 30, sell when RSI > 70  
**Simbolo originale:** BTCUSDT | **Timeframe:** 1h  
**Capitale iniziale:** $10,000  
**Position Size:** 100% (⚠️ MOLTO AGGRESSIVO)  
**Stop Loss:** 5% | **Take Profit:** 10%  
**Max Daily Loss:** 10%  
**Max Positions:** 1  
**Indicatori:** RSI(14)  
**Entry:** RSI < 30  
**Exit:** RSI > 70  

**Caratteristiche:**
- ✅ Strategia semplice e chiara
- ⚠️ Position size al 100% è estremamente rischioso
- ✅ Stop loss ben definito
- ❌ Nessun filtro di trend - può entrare contro il trend principale

**Timeframe consigliati:** 1h, 4h  
**Mercati consigliati:** Laterali/ranging  

---

### 2. RSI Scalping Bitcoin (5m)
**File:** rsi_scalping_btc.json  
**Descrizione:** Simple RSI-based scalping strategy for BTC/USDT on 5-minute timeframe  
**Simbolo originale:** BTCUSDT | **Timeframe:** 5m  
**Capitale iniziale:** $1,000  
**Position Size:** 10%  
**Stop Loss:** 2% | **Take Profit:** 5%  
**Max Daily Loss:** 5%  
**Max Positions:** 1  
**Indicatori:** RSI(14)  
**Entry:** RSI < 30  
**Exit:** RSI > 70  

**Caratteristiche:**
- ✅ Position size conservativo (10%)
- ✅ Stop loss stretto per scalping
- ⚠️ Timeframe 5m richiede molta attenzione e commissioni basse
- ❌ Per scalping l'ultimo anno su 5m richiede moltissimi dati

**Timeframe consigliati:** 5m, 15m  
**Mercati consigliati:** Alta liquidità, volatilità moderata  

---

### 3. RSI Scalping Bitcoin con EMA
**File:** example_rsi_scalping.json  
**Descrizione:** Strategia scalping su BTC/USDT basata su RSI oversold/overbought con conferma volume  
**Simbolo originale:** BTC/USDT | **Timeframe:** 5m  
**Capitale iniziale:** $1,000  
**Position Size:** 10%  
**Stop Loss:** 2% | **Take Profit:** 5%  
**Max Daily Loss:** 5%  
**Max Positions:** 3  
**Indicatori:** RSI(14), EMA(50), Volume SMA(20)  
**Entry:** RSI < 30 AND close > EMA50  
**Exit:** RSI > 70  

**Caratteristiche:**
- ✅ Filtro trend con EMA50 - entra solo sopra media mobile
- ✅ Conferma volume (indicatore presente)
- ✅ Può gestire fino a 3 posizioni
- ✅ Migliore della versione base grazie al filtro trend

**Timeframe consigliati:** 5m, 15m, 1h (con adattamento)  
**Mercati consigliati:** Trending UP  

---

### 4. DCA Advanced ETH/USDT
**File:** dca_advanced_eth.json  
**Descrizione:** Advanced Dollar Cost Averaging strategy that buys on dips with dynamic position sizing  
**Simbolo originale:** ETH/USDT | **Timeframe:** 1h  
**Capitale iniziale:** $10,000  
**Position Size:** 2% (molto conservativo)  
**Stop Loss:** 25% (molto largo) | **Take Profit:** 20%  
**Max Daily Loss:** 5%  
**Max Positions:** 1  
**Indicatori:** RSI(14), EMA(50), EMA(200), MACD(12,26,9)  
**Entry:** RSI < 40 AND close > EMA200  
**Exit:** RSI > 75  

**Caratteristiche:**
- ✅ Ottimo filtro di trend lungo periodo (EMA200)
- ✅ Position size minimo (2%) permette molti acquisti
- ✅ Ideale per DCA in bull market
- ⚠️ Stop loss 25% molto largo - sopporta grandi drawdown
- ✅ Exit conservativo (RSI > 75 invece di 70)

**Timeframe consigliati:** 1h, 4h, 1d  
**Mercati consigliati:** Bull market, accumulo lungo termine  

---

### 5. Grid Trading BTC/USDT
**File:** grid_trading_btc.json  
**Descrizione:** Grid trading strategy that places multiple buy and sell orders at different price levels  
**Simbolo originale:** BTC/USDT | **Timeframe:** 15m  
**Capitale iniziale:** $10,000  
**Position Size:** 5%  
**Stop Loss:** 5% | **Take Profit:** 3% (piccolo per molti trade)  
**Max Daily Loss:** 10%  
**Max Positions:** 10 (⭐ GRID)  
**Indicatori:** SMA(50), ATR(14), Bollinger Bands(20, 2)  
**Entry:** close > BB_lower AND close < BB_upper  
**Exit:** close > BB_upper  

**Caratteristiche:**
- ✅ Progettato per mercati laterali
- ✅ 10 posizioni simultanee = vera grid
- ✅ Take profit piccolo (3%) per catturare oscillazioni
- ⚠️ Entry condition confusa (sempre tra le bande)
- ❌ In trending market potrebbe soffrire

**Timeframe consigliati:** 15m, 1h  
**Mercati consigliati:** Ranging/Laterali, bassa volatilità  

---

### 6. Swing Trading Multi-Timeframe BTC
**File:** swing_trading_multi_tf.json  
**Descrizione:** Swing trading strategy using multiple timeframes for confirmation  
**Simbolo originale:** BTC/USDT | **Timeframe:** 4h  
**Capitale iniziale:** $10,000  
**Position Size:** 15%  
**Stop Loss:** 4% | **Take Profit:** 12%  
**Max Daily Loss:** 6%  
**Max Positions:** 2  
**Indicatori:** EMA(21), EMA(50), RSI(14), MACD(12,26,9), Bollinger(20,2)  
**Entry:** close > EMA50 AND EMA21 > EMA50 AND RSI > 45 AND RSI < 70  
**Exit:** RSI > 75 OR close < EMA21  

**Caratteristiche:**
- ✅ Eccellente: doppio filtro EMA per trend
- ✅ RSI tra 45-70 evita oversold e overbought
- ✅ Exit multipli: profit (RSI>75) o stop dinamico (EMA21)
- ✅ Risk/Reward 1:3 (4% stop, 12% profit)
- ⭐ QUESTA È UNA DELLE MIGLIORI RICETTE

**Timeframe consigliati:** 4h, 1d  
**Mercati consigliati:** Trending UP, swing trading  

---

### 7. Bollinger Bands Breakout
**File:** bollinger_breakout.json  
**Descrizione:** Bollinger Bands breakout strategy. Buy when price crosses above upper band  
**Simbolo originale:** BTCUSDT | **Timeframe:** 1h  
**Capitale iniziale:** $10,000  
**Position Size:** 20%  
**Stop Loss:** 2.5% | **Take Profit:** 6%  
**Max Daily Loss:** 8%  
**Max Positions:** 1  
**Indicatori:** Bollinger Bands(20, 2)  
**Entry:** close > BB_upper (breakout al rialzo)  
**Exit:** close < BB_lower  

**Caratteristiche:**
- ⚠️ Strategia controintuitiva: compra sui massimi
- ❌ In mercato laterale genererà molti falsi breakout
- ⚠️ Exit richiede breakout opposto (perdite potenzialmente grandi)
- ✅ Può funzionare in forte trending market con momentum

**Timeframe consigliati:** 1h, 4h (solo in trend forti)  
**Mercati consigliati:** Forte momentum, breakout confermati  

---

### 8. MACD Crossover Ethereum
**File:** macd_crossover_eth.json  
**Descrizione:** MACD crossover strategy for ETH/USDT. Enter when MACD crosses above signal line  
**Simbolo originale:** ETHUSDT | **Timeframe:** 15m  
**Capitale iniziale:** $5,000  
**Position Size:** 15%  
**Stop Loss:** 3% | **Take Profit:** 8%  
**Max Daily Loss:** 10%  
**Max Positions:** 2  
**Indicatori:** MACD(12,26,9)  
**Entry:** MACD crosses_above signal  
**Exit:** MACD crosses_below signal  

**Caratteristiche:**
- ✅ Strategia classica e affidabile
- ⚠️ MACD può dare molti falsi segnali su 15m
- ⚠️ Richiede "crosses_above" operator - verificare implementazione
- ✅ Risk/Reward 1:2.67 decente

**Timeframe consigliati:** 1h, 4h, 1d (meglio timeframe alti)  
**Mercati consigliati:** Trending market  

---

## ANALISI COMPARATIVA PRELIMINARE

### Classificazione per Tipo di Mercato

**TRENDING MARKETS (rialzista):**
1. ⭐ Swing Trading Multi-TF - MIGLIORE OVERALL
2. ⭐ DCA Advanced ETH - ottimo per accumulo
3. ✅ RSI Scalping con EMA - filtro trend presente
4. ✅ MACD Crossover ETH - su timeframe alti

**RANGING MARKETS (laterale):**
1. ⭐ Grid Trading BTC - progettato per questo
2. ✅ Simple RSI - funziona in range
3. ⚠️ RSI Scalping - su timeframe bassi

**HIGH VOLATILITY / BREAKOUT:**
1. ⚠️ Bollinger Breakout - rischiosa, solo con forte momentum

### Classificazione per Timeframe

**SCALPING (5m-15m):**
- RSI Scalping BTC (5m)
- RSI Scalping con EMA (5m)
- MACD Crossover (15m) ⚠️ preferibile su tf alti
- Grid Trading (15m)

**INTRADAY (1h-4h):**
- ⭐ Swing Trading Multi-TF (4h)
- Simple RSI (1h)
- DCA Advanced (1h)
- Bollinger Breakout (1h)
- MACD Crossover (1h meglio di 15m)

**POSITION/SWING (1d):**
- ⭐ Swing Trading Multi-TF (1d)
- DCA Advanced (1d)
- MACD Crossover (1d)

### Classificazione per Livello di Rischio

**CONSERVATIVO:**
1. DCA Advanced ETH (2% position, 1 pos max, SL 25%)
2. RSI Scalping (10% position)

**MODERATO:**
1. ⭐ Swing Trading Multi-TF (15% position, SL 4%)
2. MACD Crossover (15% position, SL 3%)
3. Grid Trading (5% × 10 posizioni = 50% max)

**AGGRESSIVO:**
1. ⚠️ Simple RSI (100% position!) - MOLTO RISCHIOSO
2. Bollinger Breakout (20% position, exit distante)

---

## RACCOMANDAZIONI PER TIMEFRAME/SIMBOLI

### ETHUSDT 1h
**Top 3 ricette raccomandate:**
1. ⭐ Swing Trading Multi-TF (adattato a 1h)
2. ⭐ DCA Advanced ETH (nativo)
3. MACD Crossover

### ETHUSDT 1d  
**Top 3 ricette raccomandate:**
1. ⭐ Swing Trading Multi-TF (ottimo per daily)
2. ⭐ DCA Advanced ETH
3. MACD Crossover

### EURUSDT 1h
**Top 3 ricette raccomandate:**
1. ⭐ Swing Trading Multi-TF
2. Simple RSI (ma ridurre position size!)
3. Grid Trading (se mercato laterale)

### EURUSDT 1d
**Top 3 ricette raccomandate:**
1. ⭐ Swing Trading Multi-TF
2. DCA Advanced (adattato)
3. MACD Crossover

### ETHEUR 1h/1d
**Top 3 ricette raccomandate:**
1. ⭐ Swing Trading Multi-TF
2. ⭐ DCA Advanced ETH
3. Grid Trading (1h se ranging)

---

## PROSSIMI PASSI PER BACKTEST REALE

1. **Download dati storici:**
   - ETHUSDT: 1h e 1d (365 giorni)
   - EURUSDT: 1h e 1d (365 giorni)
   - ETHEUR: 1h e 1d (365 giorni)

2. **Eseguire backtest per ciascuna combinazione (48 test totali)**

3. **Metriche da raccogliere:**
   - Total Return %
   - Sharpe Ratio
   - Max Drawdown %
   - Win Rate %
   - Total Trades
   - Profit Factor
   - Average Trade Duration

4. **Analisi risultati:**
   - Ranking per performance assoluta
   - Ranking per risk-adjusted return
   - Best recipe per symbol/timeframe
   - Worst performers da scartare

