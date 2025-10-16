# Phase 2 - Testing Checklist

## ✅ Already Tested
- [x] HTTP connection (ping)
- [x] Server time sync
- [x] getTicker() single symbol
- [x] getAllTickers() batch request
- [x] Cache functionality
- [x] Rate limiter
- [x] JSON parsing (objects, arrays, nested arrays)
- [x] Exchange info metadata

## ⏳ Need Testing

### 1. Historical Data (Candles)
- [ ] getCandles() with real time range
- [ ] Parse OHLCV data correctly
- [ ] Handle different timeframes (1m, 5m, 1h, 1d)
- [ ] Verify timestamp conversion (ms → seconds)
- [ ] Test limit parameter

### 2. Order Book
- [ ] getOrderBook() parsing
- [ ] Parse bids array (price, quantity)
- [ ] Parse asks array (price, quantity)
- [ ] Test different depth limits (5, 10, 100)
- [ ] Verify price sorting

### 3. Recent Trades
- [ ] getRecentTrades() parsing
- [ ] Parse trade objects from array
- [ ] Extract price, quantity, timestamp
- [ ] Test limit parameter

### 4. Authenticated Endpoints (CRITICAL)
- [ ] HMAC-SHA256 signature generation
- [ ] getBalances() with test account
- [ ] Verify signature format
- [ ] Test timestamp synchronization
- [ ] Handle API key errors

### 5. Error Handling
- [ ] Invalid symbol handling
- [ ] Network timeout
- [ ] Rate limit exceeded (429 error)
- [ ] Invalid API keys (401 error)
- [ ] Malformed JSON response
- [ ] Empty responses

### 6. Edge Cases
- [ ] Very large time ranges (candles)
- [ ] Symbols with special characters
- [ ] Concurrent requests
- [ ] Cache expiration timing
- [ ] Rate limiter boundary conditions

## 🎯 Priority Order

1. **HIGH**: getCandles() - Critical for backtesting
2. **HIGH**: Authenticated endpoints - Required for trading
3. **MEDIUM**: Order book - Needed for order placement
4. **MEDIUM**: Error handling - Production readiness
5. **LOW**: Recent trades - Nice to have
6. **LOW**: Edge cases - Can be tested in production

