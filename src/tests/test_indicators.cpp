#include "../strategy/Indicators.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>
#include <chrono>

using namespace Emiglio;

// Test macros
#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    std::cout << "Running " #name "..." << std::endl; \
    test_##name(); \
    std::cout << "✓ " #name " passed" << std::endl; \
} while(0)

#define ASSERT_TRUE(expr) do { \
    if (!(expr)) { \
        std::cerr << "✗ Assertion failed: " #expr << " at line " << __LINE__ << std::endl; \
        exit(1); \
    } \
} while(0)

#define ASSERT_FALSE(expr) ASSERT_TRUE(!(expr))
#define ASSERT_NEAR(a, b, epsilon) ASSERT_TRUE(std::abs((a) - (b)) < (epsilon))

// Helper to create sample price data
std::vector<double> createSamplePrices() {
    // Sample prices for testing (20 values)
    return {
        100.0, 101.0, 102.0, 101.5, 100.5,  // Rising then falling
        99.0, 98.5, 99.5, 100.0, 101.0,     // Recovery
        102.0, 103.0, 104.0, 103.5, 103.0,  // Rising
        102.5, 102.0, 101.5, 101.0, 100.5   // Falling
    };
}

// Test: SMA calculation
TEST(sma_calculation) {
    Indicators indicators;
    std::vector<double> prices = {10.0, 20.0, 30.0, 40.0, 50.0};

    // SMA(5) of [10, 20, 30, 40, 50] = 30.0
    double sma = indicators.calculateSMA(prices, 5);
    ASSERT_NEAR(sma, 30.0, 0.001);

    // SMA(3) of last 3 values [30, 40, 50] = 40.0
    sma = indicators.calculateSMA(prices, 3);
    ASSERT_NEAR(sma, 40.0, 0.001);

    // Period larger than data
    sma = indicators.calculateSMA(prices, 10);
    ASSERT_TRUE(std::isnan(sma)); // Should return NaN or handle gracefully
}

// Test: EMA calculation
TEST(ema_calculation) {
    Indicators indicators;
    std::vector<double> prices = {10.0, 11.0, 12.0, 11.0, 10.0, 11.0, 12.0, 13.0};

    // EMA should be smoother than SMA
    double ema = indicators.calculateEMA(prices, 5);
    ASSERT_TRUE(ema > 10.0 && ema < 13.0); // Should be in reasonable range

    // EMA with period 1 should equal last price
    ema = indicators.calculateEMA(prices, 1);
    ASSERT_NEAR(ema, 13.0, 0.001);
}

// Test: RSI calculation
TEST(rsi_calculation) {
    Indicators indicators;

    // Rising prices should give high RSI
    std::vector<double> risingPrices = {100, 102, 104, 106, 108, 110, 112, 114, 116, 118, 120};
    double rsi = indicators.calculateRSI(risingPrices, 10);
    ASSERT_TRUE(rsi > 70.0); // Should be overbought

    // Falling prices should give low RSI
    std::vector<double> fallingPrices = {120, 118, 116, 114, 112, 110, 108, 106, 104, 102, 100};
    rsi = indicators.calculateRSI(fallingPrices, 10);
    ASSERT_TRUE(rsi < 30.0); // Should be oversold

    // Sideways prices should give RSI around 50
    std::vector<double> sidewaysPrices = {100, 101, 100, 101, 100, 101, 100, 101, 100, 101, 100};
    rsi = indicators.calculateRSI(sidewaysPrices, 10);
    ASSERT_TRUE(rsi > 40.0 && rsi < 60.0); // Should be neutral
}

// Test: MACD calculation
TEST(macd_calculation) {
    Indicators indicators;
    std::vector<double> prices = createSamplePrices();

    Indicators::MACDValue macd = indicators.calculateMACD(prices, 12, 26, 9);

    // MACD components should be reasonable
    ASSERT_FALSE(std::isnan(macd.macdLine));
    ASSERT_FALSE(std::isnan(macd.signalLine));
    ASSERT_FALSE(std::isnan(macd.histogram));

    // Histogram should equal macdLine - signalLine
    ASSERT_NEAR(macd.histogram, macd.macdLine - macd.signalLine, 0.001);
}

// Test: Bollinger Bands calculation
TEST(bollinger_bands_calculation) {
    Indicators indicators;
    std::vector<double> prices = createSamplePrices();

    Indicators::BollingerBandsValue bb = indicators.calculateBollingerBands(prices, 20, 2.0);

    // Middle band should equal SMA
    double sma = indicators.calculateSMA(prices, 20);
    ASSERT_NEAR(bb.middle, sma, 0.001);

    // Upper band should be above middle
    ASSERT_TRUE(bb.upper > bb.middle);

    // Lower band should be below middle
    ASSERT_TRUE(bb.lower < bb.middle);

    // Bands should be symmetric around middle (approximately)
    double upperDistance = bb.upper - bb.middle;
    double lowerDistance = bb.middle - bb.lower;
    ASSERT_NEAR(upperDistance, lowerDistance, 0.001);
}

// Test: Standard Deviation calculation
TEST(standard_deviation) {
    Indicators indicators;

    // Known values: [2, 4, 4, 4, 5, 5, 7, 9]
    // Mean = 5, Variance = 4, StdDev = 2
    std::vector<double> values = {2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0};
    double stdDev = indicators.calculateStdDev(values);
    ASSERT_NEAR(stdDev, 2.0, 0.01);

    // Constant values should have zero std dev
    std::vector<double> constant = {10.0, 10.0, 10.0, 10.0};
    stdDev = indicators.calculateStdDev(constant);
    ASSERT_NEAR(stdDev, 0.0, 0.001);
}

// Test: ATR (Average True Range) calculation
TEST(atr_calculation) {
    Indicators indicators;

    std::vector<double> highs = {105, 106, 107, 106, 105, 106, 108, 109};
    std::vector<double> lows = {95, 96, 97, 96, 95, 96, 98, 99};
    std::vector<double> closes = {100, 101, 102, 101, 100, 101, 103, 104};

    double atr = indicators.calculateATR(highs, lows, closes, 7);

    // ATR should be positive
    ASSERT_TRUE(atr > 0.0);

    // ATR should be reasonable (less than max range)
    ASSERT_TRUE(atr < 20.0);
}

// Test: Stochastic Oscillator
TEST(stochastic_oscillator) {
    Indicators indicators;

    std::vector<double> highs = {110, 111, 112, 111, 110, 111, 113, 114};
    std::vector<double> lows = {100, 101, 102, 101, 100, 101, 103, 104};
    std::vector<double> closes = {105, 106, 107, 106, 105, 106, 108, 109};

    double stochK = indicators.calculateStochastic(highs, lows, closes, 5);

    // Stochastic should be between 0 and 100
    ASSERT_TRUE(stochK >= 0.0 && stochK <= 100.0);
}

// Test: Volume indicators
TEST(volume_indicators) {
    Indicators indicators;

    std::vector<double> volumes = {1000, 1200, 1500, 1300, 1100, 1400, 1600, 1800};

    // Volume SMA
    double volSMA = indicators.calculateSMA(volumes, 5);
    ASSERT_TRUE(volSMA > 0.0);

    // OBV (On Balance Volume) - requires prices and volumes
    std::vector<double> prices = {100, 102, 104, 103, 101, 103, 105, 107};
    double obv = indicators.calculateOBV(prices, volumes);
    ASSERT_TRUE(obv != 0.0); // OBV should accumulate
}

// Test: Moving Average Convergence
TEST(ma_convergence) {
    Indicators indicators;
    std::vector<double> prices = createSamplePrices();

    double fastMA = indicators.calculateEMA(prices, 9);
    double slowMA = indicators.calculateEMA(prices, 21);

    // Fast MA should be more responsive
    ASSERT_FALSE(std::isnan(fastMA));
    ASSERT_FALSE(std::isnan(slowMA));

    // In trending market, MAs should converge/diverge predictably
    // (This is more of a sanity check)
    ASSERT_TRUE(std::abs(fastMA - slowMA) < 10.0);
}

// Test: Edge cases
TEST(edge_cases) {
    Indicators indicators;

    // Empty vector
    std::vector<double> empty;
    double result = indicators.calculateSMA(empty, 5);
    ASSERT_TRUE(std::isnan(result)); // Should handle gracefully

    // Single value
    std::vector<double> single = {100.0};
    result = indicators.calculateSMA(single, 1);
    ASSERT_NEAR(result, 100.0, 0.001);

    // Period = 0
    std::vector<double> prices = {100, 101, 102};
    result = indicators.calculateSMA(prices, 0);
    ASSERT_TRUE(std::isnan(result)); // Invalid period

    // Negative prices (should still calculate)
    std::vector<double> negative = {-10.0, -5.0, 0.0, 5.0, 10.0};
    result = indicators.calculateSMA(negative, 5);
    ASSERT_NEAR(result, 0.0, 0.001);
}

// Performance test: Large dataset
TEST(performance_large_dataset) {
    Indicators indicators;

    // Create large dataset
    std::vector<double> largePrices;
    for (int i = 0; i < 10000; i++) {
        largePrices.push_back(100.0 + std::sin(i * 0.1) * 10.0);
    }

    // Time the calculation
    auto start = std::chrono::high_resolution_clock::now();

    double sma = indicators.calculateSMA(largePrices, 50);
    double ema = indicators.calculateEMA(largePrices, 50);
    double rsi = indicators.calculateRSI(largePrices, 14);
    Indicators::BollingerBandsValue bb = indicators.calculateBollingerBands(largePrices, 20, 2.0);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "  Calculated 4 indicators on 10k data points in " << duration.count() << "ms" << std::endl;

    // Should complete reasonably fast (< 100ms)
    ASSERT_TRUE(duration.count() < 100);

    // Results should be valid
    ASSERT_FALSE(std::isnan(sma));
    ASSERT_FALSE(std::isnan(ema));
    ASSERT_FALSE(std::isnan(rsi));
}

int main() {
    std::cout << "=== Technical Indicators Tests ===" << std::endl << std::endl;

    RUN_TEST(sma_calculation);
    RUN_TEST(ema_calculation);
    RUN_TEST(rsi_calculation);
    RUN_TEST(macd_calculation);
    RUN_TEST(bollinger_bands_calculation);
    RUN_TEST(standard_deviation);
    RUN_TEST(atr_calculation);
    RUN_TEST(stochastic_oscillator);
    RUN_TEST(volume_indicators);
    RUN_TEST(ma_convergence);
    RUN_TEST(edge_cases);
    RUN_TEST(performance_large_dataset);

    std::cout << "\n=== All indicator tests passed! ===" << std::endl;
    return 0;
}
