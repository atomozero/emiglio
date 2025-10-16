#ifndef INDICATORS_H
#define INDICATORS_H

#include <vector>
#include <string>
#include "../data/DataStorage.h"

namespace Emiglio {

// Technical Indicators for trading strategies
class Indicators {
public:
	// Simple Moving Average (SMA)
	// Returns average of last 'period' values
	static std::vector<double> sma(const std::vector<double>& data, int period);

	// Exponential Moving Average (EMA)
	// Gives more weight to recent prices
	// Formula: EMA = (Price - Previous EMA) × multiplier + Previous EMA
	// where multiplier = 2 / (period + 1)
	static std::vector<double> ema(const std::vector<double>& data, int period);

	// Relative Strength Index (RSI)
	// Momentum oscillator (0-100) measuring speed and magnitude of price changes
	// RSI > 70: overbought, RSI < 30: oversold
	static std::vector<double> rsi(const std::vector<double>& data, int period = 14);

	// Moving Average Convergence Divergence (MACD)
	// Trend-following momentum indicator
	// Returns: {macd_line, signal_line, histogram}
	struct MACDResult {
		std::vector<double> macdLine;   // MACD line (fast EMA - slow EMA)
		std::vector<double> signalLine; // Signal line (EMA of MACD line)
		std::vector<double> histogram;  // MACD histogram (macd - signal)
	};
	static MACDResult macd(const std::vector<double>& data,
	                       int fastPeriod = 12,
	                       int slowPeriod = 26,
	                       int signalPeriod = 9);

	// Bollinger Bands
	// Volatility indicator with upper/middle/lower bands
	// Middle = SMA, Upper = SMA + (stddev × multiplier), Lower = SMA - (stddev × multiplier)
	struct BollingerBandsResult {
		std::vector<double> upper;  // Upper band
		std::vector<double> middle; // Middle band (SMA)
		std::vector<double> lower;  // Lower band
	};
	static BollingerBandsResult bollingerBands(const std::vector<double>& data,
	                                             int period = 20,
	                                             double multiplier = 2.0);

	// Average True Range (ATR)
	// Volatility indicator
	static std::vector<double> atr(const std::vector<Candle>& candles, int period = 14);

	// Stochastic Oscillator
	// Momentum indicator comparing closing price to price range over time (0-100)
	// %K > 80: overbought, %K < 20: oversold
	struct StochasticResult {
		std::vector<double> k; // %K line (fast)
		std::vector<double> d; // %D line (slow, SMA of %K)
	};
	static StochasticResult stochastic(const std::vector<Candle>& candles,
	                                    int kPeriod = 14,
	                                    int dPeriod = 3);

	// On-Balance Volume (OBV)
	// Volume indicator measuring buying/selling pressure
	static std::vector<double> obv(const std::vector<Candle>& candles);

	// Average Directional Index (ADX)
	// Trend strength indicator (0-100)
	// ADX > 25: strong trend, ADX < 20: weak trend
	static std::vector<double> adx(const std::vector<Candle>& candles, int period = 14);

	// Commodity Channel Index (CCI)
	// Momentum oscillator identifying cyclical trends
	// CCI > 100: overbought, CCI < -100: oversold
	static std::vector<double> cci(const std::vector<Candle>& candles, int period = 20);

	// Helper: Extract closing prices from candles
	static std::vector<double> getClosePrices(const std::vector<Candle>& candles);

	// Helper: Extract high prices from candles
	static std::vector<double> getHighPrices(const std::vector<Candle>& candles);

	// Helper: Extract low prices from candles
	static std::vector<double> getLowPrices(const std::vector<Candle>& candles);

	// Helper: Extract volumes from candles
	static std::vector<double> getVolumes(const std::vector<Candle>& candles);

	// Helper: Standard deviation
	static double stddev(const std::vector<double>& data, int start, int period);

	// Helper: Mean/average
	static double mean(const std::vector<double>& data, int start, int period);

	// Convenience methods that return single values (last value from calculation)
	static double calculateSMA(const std::vector<double>& data, int period);
	static double calculateEMA(const std::vector<double>& data, int period);
	static double calculateRSI(const std::vector<double>& data, int period = 14);
	static double calculateStdDev(const std::vector<double>& data);
	static double calculateATR(const std::vector<double>& highs,
	                           const std::vector<double>& lows,
	                           const std::vector<double>& closes,
	                           int period = 14);
	static double calculateStochastic(const std::vector<double>& highs,
	                                  const std::vector<double>& lows,
	                                  const std::vector<double>& closes,
	                                  int period = 14);
	static double calculateOBV(const std::vector<double>& closes,
	                           const std::vector<double>& volumes);

	struct MACDValue {
		double macdLine;
		double signalLine;
		double histogram;
	};
	static MACDValue calculateMACD(const std::vector<double>& data,
	                               int fastPeriod = 12,
	                               int slowPeriod = 26,
	                               int signalPeriod = 9);

	struct BollingerBandsValue {
		double upper;
		double middle;
		double lower;
	};
	static BollingerBandsValue calculateBollingerBands(const std::vector<double>& data,
	                                                    int period = 20,
	                                                    double stdDevMultiplier = 2.0);
};

} // namespace Emiglio

#endif // INDICATORS_H
