# Emiglio Test Suite

This directory contains unit tests for Emiglio components.

## Test Files

### 1. **test_websocket.cpp** - WebSocket Client Tests
Tests the native WebSocket implementation:
- Connection establishment (wss:// URLs)
- SSL/TLS handshake
- Message reception and parsing
- Callback system
- Error handling
- Multiple connection handling
- Performance with rapid messages

**Run:**
```bash
make -f Makefile.new websocket
```

### 2. **test_indicators.cpp** - Technical Indicators Tests
Tests all technical indicators:
- SMA (Simple Moving Average)
- EMA (Exponential Moving Average)
- RSI (Relative Strength Index)
- MACD (Moving Average Convergence Divergence)
- Bollinger Bands
- Standard Deviation
- ATR (Average True Range)
- Stochastic Oscillator
- Volume indicators (OBV)
- Edge cases and performance

**Run:**
```bash
make -f Makefile.new indicators
```

### 3. **test_recipe_loader.cpp** - Recipe Loader Tests
Tests JSON recipe parsing and validation:
- Simple recipe loading
- Multiple indicators
- Complex entry/exit conditions
- Invalid JSON handling
- Missing fields
- Directory scanning
- Recipe validation
- Performance with many recipes

**Run:**
```bash
make -f Makefile.new recipe
```

## Building Tests

### Prerequisites

Ensure you're on Haiku OS with development tools installed:
```bash
pkgman install gcc make sqlite openssl
```

### Build All Tests

```bash
cd src/tests
make -f Makefile.new all
```

### Run All Tests

```bash
make -f Makefile.new run
```

This will:
1. Build all test executables
2. Run WebSocket tests
3. Run Indicator tests
4. Run RecipeLoader tests
5. Display results

## Test Output

Each test shows:
- ✓ Test name when passed
- ✗ Assertion details when failed
- Additional info for performance tests

Example:
```
=== WebSocketClient Tests ===

Running websocket_creation...
✓ websocket_creation passed
Running url_parsing...
✓ url_parsing passed
Running connect_disconnect...
✓ connect_disconnect passed

=== All WebSocket tests passed! ===
```

## Network-Dependent Tests

Some WebSocket tests require:
- Internet connection
- Access to Binance WebSocket API (stream.binance.com:9443)

These tests will be skipped if connectivity is unavailable.

## Test Coverage

### WebSocketClient Tests
- ✅ Object creation and initialization
- ✅ URL parsing and validation
- ✅ Callback registration
- ✅ Real connection to Binance
- ✅ Message reception
- ✅ Error handling
- ✅ Disconnect handling
- ✅ Performance metrics

### Indicator Tests
- ✅ SMA calculation accuracy
- ✅ EMA calculation and smoothing
- ✅ RSI oversold/overbought detection
- ✅ MACD components and histogram
- ✅ Bollinger Bands symmetry
- ✅ Standard deviation correctness
- ✅ ATR volatility measurement
- ✅ Stochastic oscillator range
- ✅ Volume indicators
- ✅ Edge cases (empty data, invalid periods)
- ✅ Performance with large datasets (10k points)

### RecipeLoader Tests
- ✅ Simple recipe parsing
- ✅ Multiple indicators
- ✅ Complex conditions
- ✅ Invalid JSON handling
- ✅ Missing required fields
- ✅ File not found errors
- ✅ Directory scanning
- ✅ Recipe validation
- ✅ Trailing stops
- ✅ Max positions
- ✅ Comments in JSON
- ✅ Performance with 100 recipes

## Debugging Tests

### Enable Verbose Output

Modify test files to add debug prints:
```cpp
std::cout << "Debug: variable = " << variable << std::endl;
```

### Run Single Test

To run just one test function, comment out others in `main()` or use:
```bash
./test_websocket  # Run specific test binary
```

### Check Test Logs

WebSocket and network tests may produce logs:
```bash
tail -f /tmp/emilio*.log
```

## Adding New Tests

### 1. Create Test File

```cpp
#include "../component/YourComponent.h"
#include <iostream>
#include <cassert>

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    std::cout << "Running " #name "..." << std::endl; \
    test_##name(); \
    std::cout << "✓ " #name " passed" << std::endl; \
} while(0)

#define ASSERT_TRUE(expr) /* ... */

TEST(your_test_name) {
    // Your test code
    ASSERT_TRUE(condition);
}

int main() {
    RUN_TEST(your_test_name);
    return 0;
}
```

### 2. Add to Makefile

Edit `Makefile.new`:
```makefile
NEW_TESTS = test_websocket test_indicators test_recipe_loader test_your_component

test_your_component: test_your_component.o $(DEPS)
	$(CXX) -o $@ $^ $(LDFLAGS)
```

### 3. Run Test

```bash
make -f Makefile.new test_your_component
./test_your_component
```

## Continuous Integration

For automated testing, run:
```bash
make -f Makefile.new run && echo "All tests passed" || echo "Tests failed"
```

Exit code 0 = all tests passed
Exit code 1 = at least one test failed

## Performance Benchmarks

Some tests include performance measurements:

**Indicators (10k data points):**
- Target: < 100ms for all indicators
- Actual: ~20-50ms on modern hardware

**WebSocket (5 seconds):**
- Expected: 10-100 messages depending on market activity
- Latency: 50-150ms per message

**RecipeLoader (100 recipes):**
- Target: < 1000ms
- Actual: ~100-300ms

## Known Issues

1. **WebSocket tests may timeout** on slow connections
2. **Binance API rate limiting** may affect rapid tests
3. **Floating-point precision** may cause minor variations in indicator tests

## Troubleshooting

### Test Compilation Errors

```bash
# Check dependencies
ls -la ../exchange/WebSocketClient.o
ls -la ../strategy/Indicators.o

# Rebuild dependencies
make -C ../exchange WebSocketClient.o
make -C ../strategy Indicators.o
```

### Test Execution Errors

```bash
# Check library paths
ldd ./test_websocket

# Install missing libraries
pkgman install libssl libcrypto
```

### Network Test Failures

```bash
# Test connectivity
ping stream.binance.com
curl https://stream.binance.com:9443/ws/btcusdt@trade
```

## Contributing

When adding new features:
1. Write tests FIRST (TDD approach)
2. Ensure tests pass before committing
3. Update this README with new test descriptions
4. Add performance benchmarks where applicable

## License

These tests are part of Emiglio and follow the same license.
