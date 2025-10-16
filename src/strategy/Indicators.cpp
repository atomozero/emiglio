#include "Indicators.h"
#include <cmath>
#include <algorithm>
#include <numeric>

namespace Emiglio {

// Helper: Extract closing prices
std::vector<double> Indicators::getClosePrices(const std::vector<Candle>& candles) {
	std::vector<double> closes;
	closes.reserve(candles.size());
	for (const auto& candle : candles) {
		closes.push_back(candle.close);
	}
	return closes;
}

// Helper: Extract high prices
std::vector<double> Indicators::getHighPrices(const std::vector<Candle>& candles) {
	std::vector<double> highs;
	highs.reserve(candles.size());
	for (const auto& candle : candles) {
		highs.push_back(candle.high);
	}
	return highs;
}

// Helper: Extract low prices
std::vector<double> Indicators::getLowPrices(const std::vector<Candle>& candles) {
	std::vector<double> lows;
	lows.reserve(candles.size());
	for (const auto& candle : candles) {
		lows.push_back(candle.low);
	}
	return lows;
}

// Helper: Extract volumes
std::vector<double> Indicators::getVolumes(const std::vector<Candle>& candles) {
	std::vector<double> volumes;
	volumes.reserve(candles.size());
	for (const auto& candle : candles) {
		volumes.push_back(candle.volume);
	}
	return volumes;
}

// Helper: Calculate mean
double Indicators::mean(const std::vector<double>& data, int start, int period) {
	if (start + period > static_cast<int>(data.size())) return 0.0;

	double sum = 0.0;
	for (int i = start; i < start + period; i++) {
		sum += data[i];
	}
	return sum / period;
}

// Helper: Calculate standard deviation
double Indicators::stddev(const std::vector<double>& data, int start, int period) {
	if (start + period > static_cast<int>(data.size())) return 0.0;

	double avg = mean(data, start, period);
	double sum = 0.0;

	for (int i = start; i < start + period; i++) {
		double diff = data[i] - avg;
		sum += diff * diff;
	}

	return std::sqrt(sum / period);
}

// Simple Moving Average (SMA) - Optimized with sliding window
std::vector<double> Indicators::sma(const std::vector<double>& data, int period) {
	std::vector<double> result;

	if (data.size() < static_cast<size_t>(period)) {
		return result; // Not enough data
	}

	// Fill initial values with NaN
	for (int i = 0; i < period - 1; i++) {
		result.push_back(NAN);
	}

	// Calculate first SMA (initial window sum), skip NaN values if present
	double sum = 0.0;
	int validCount = 0;
	for (int i = 0; i < period; i++) {
		if (!std::isnan(data[i])) {
			sum += data[i];
			validCount++;
		}
	}

	// If not enough valid data, return NaN
	if (validCount < period) {
		result.push_back(NAN);
	} else {
		result.push_back(sum / period);
	}

	// Calculate subsequent SMAs using sliding window
	// Remove oldest value, add newest value: O(1) per iteration
	for (size_t i = period; i < data.size(); i++) {
		// Update valid count and sum for sliding window
		if (!std::isnan(data[i - period])) {
			sum -= data[i - period];
			validCount--;
		}
		if (!std::isnan(data[i])) {
			sum += data[i];
			validCount++;
		}

		// If not enough valid data, return NaN
		if (validCount < period) {
			result.push_back(NAN);
		} else {
			result.push_back(sum / validCount);
		}
	}

	return result;
}

// Exponential Moving Average (EMA)
std::vector<double> Indicators::ema(const std::vector<double>& data, int period) {
	std::vector<double> result;

	if (data.size() < static_cast<size_t>(period)) {
		return result; // Not enough data
	}

	double multiplier = 2.0 / (period + 1);

	// Fill initial values with NaN
	for (int i = 0; i < period - 1; i++) {
		result.push_back(NAN);
	}

	// First EMA is SMA
	double sum = 0.0;
	for (int i = 0; i < period; i++) {
		sum += data[i];
	}
	double ema_val = sum / period;
	result.push_back(ema_val);

	// Calculate subsequent EMAs
	for (size_t i = period; i < data.size(); i++) {
		ema_val = (data[i] - ema_val) * multiplier + ema_val;
		result.push_back(ema_val);
	}

	return result;
}

// Relative Strength Index (RSI)
std::vector<double> Indicators::rsi(const std::vector<double>& data, int period) {
	std::vector<double> result;

	if (data.size() < static_cast<size_t>(period + 1)) {
		return result; // Not enough data
	}

	// Fill initial values with NaN
	for (int i = 0; i < period; i++) {
		result.push_back(NAN);
	}

	// Calculate initial average gain and loss
	double avgGain = 0.0;
	double avgLoss = 0.0;

	for (int i = 1; i <= period; i++) {
		double change = data[i] - data[i - 1];
		if (change > 0) {
			avgGain += change;
		} else {
			avgLoss += std::abs(change);
		}
	}

	avgGain /= period;
	avgLoss /= period;

	// Calculate first RSI
	double rsi_val;
	if (avgLoss == 0) {
		rsi_val = (avgGain == 0) ? 50.0 : 100.0; // No losses = RSI 100, no movement = RSI 50
	} else {
		double rs = avgGain / avgLoss;
		rsi_val = 100.0 - (100.0 / (1.0 + rs));
	}
	result.push_back(rsi_val);

	// Calculate subsequent RSI values using smoothed averages
	for (size_t i = period + 1; i < data.size(); i++) {
		double change = data[i] - data[i - 1];
		double gain = (change > 0) ? change : 0;
		double loss = (change < 0) ? std::abs(change) : 0;

		avgGain = ((avgGain * (period - 1)) + gain) / period;
		avgLoss = ((avgLoss * (period - 1)) + loss) / period;

		if (avgLoss == 0) {
			rsi_val = (avgGain == 0) ? 50.0 : 100.0; // No losses = RSI 100, no movement = RSI 50
		} else {
			double rs = avgGain / avgLoss;
			rsi_val = 100.0 - (100.0 / (1.0 + rs));
		}
		result.push_back(rsi_val);
	}

	return result;
}

// MACD (Moving Average Convergence Divergence)
Indicators::MACDResult Indicators::macd(const std::vector<double>& data,
                                         int fastPeriod,
                                         int slowPeriod,
                                         int signalPeriod) {
	MACDResult result;

	if (data.size() < static_cast<size_t>(slowPeriod)) {
		return result; // Not enough data
	}

	// Calculate fast and slow EMAs
	std::vector<double> fastEMA = ema(data, fastPeriod);
	std::vector<double> slowEMA = ema(data, slowPeriod);

	// Calculate MACD line (fast - slow)
	result.macdLine.resize(data.size(), NAN);
	for (size_t i = slowPeriod - 1; i < data.size(); i++) {
		result.macdLine[i] = fastEMA[i] - slowEMA[i];
	}

	// Calculate signal line (EMA of MACD line)
	// Extract only valid MACD values (skip NaN at beginning)
	std::vector<double> validMACD;
	size_t macdStartIdx = slowPeriod - 1;
	for (size_t i = macdStartIdx; i < result.macdLine.size(); i++) {
		validMACD.push_back(result.macdLine[i]);
	}

	// Calculate EMA on valid MACD values
	std::vector<double> signalEMA = ema(validMACD, signalPeriod);

	// Map signal EMA back to original indices
	result.signalLine.resize(data.size(), NAN);
	for (size_t i = 0; i < signalEMA.size(); i++) {
		result.signalLine[macdStartIdx + i] = signalEMA[i];
	}

	// Calculate histogram (MACD - signal)
	result.histogram.resize(data.size(), NAN);
	for (size_t i = 0; i < data.size(); i++) {
		if (!std::isnan(result.macdLine[i]) && !std::isnan(result.signalLine[i])) {
			result.histogram[i] = result.macdLine[i] - result.signalLine[i];
		}
	}

	return result;
}

// Bollinger Bands - Optimized (avoid recalculating mean)
Indicators::BollingerBandsResult Indicators::bollingerBands(const std::vector<double>& data,
                                                              int period,
                                                              double multiplier) {
	BollingerBandsResult result;

	if (data.size() < static_cast<size_t>(period)) {
		return result; // Not enough data
	}

	// Calculate middle band (SMA)
	result.middle = sma(data, period);

	// Calculate upper and lower bands
	result.upper.resize(data.size(), NAN);
	result.lower.resize(data.size(), NAN);

	for (size_t i = period - 1; i < data.size(); i++) {
		// Use already calculated SMA instead of recalculating mean
		double sma_val = result.middle[i];

		// Fixed: Use forward indexing for clarity (was: i - j)
		// Calculate standard deviation manually (without calling mean again)
		double sum = 0.0;
		for (size_t j = i - period + 1; j <= i; j++) {
			double diff = data[j] - sma_val;  // ✅ Clear forward indexing
			sum += diff * diff;
		}
		double sd = std::sqrt(sum / period);

		result.upper[i] = sma_val + (sd * multiplier);
		result.lower[i] = sma_val - (sd * multiplier);
	}

	return result;
}

// Average True Range (ATR)
std::vector<double> Indicators::atr(const std::vector<Candle>& candles, int period) {
	std::vector<double> result;

	if (candles.size() < static_cast<size_t>(period + 1)) {
		return result; // Not enough data
	}

	// Calculate True Range for each candle
	std::vector<double> trueRanges;
	trueRanges.push_back(NAN); // First candle has no previous close

	for (size_t i = 1; i < candles.size(); i++) {
		double tr1 = candles[i].high - candles[i].low;
		double tr2 = std::abs(candles[i].high - candles[i - 1].close);
		double tr3 = std::abs(candles[i].low - candles[i - 1].close);

		double tr = std::max({tr1, tr2, tr3});
		trueRanges.push_back(tr);
	}

	// Calculate ATR (SMA of True Range)
	return sma(trueRanges, period);
}

// Stochastic Oscillator
Indicators::StochasticResult Indicators::stochastic(const std::vector<Candle>& candles,
                                                     int kPeriod,
                                                     int dPeriod) {
	StochasticResult result;

	if (candles.size() < static_cast<size_t>(kPeriod)) {
		return result; // Not enough data
	}

	// Calculate %K line
	result.k.resize(candles.size(), NAN);

	for (size_t i = kPeriod - 1; i < candles.size(); i++) {
		// Find highest high and lowest low in period
		double highestHigh = candles[i - kPeriod + 1].high;
		double lowestLow = candles[i - kPeriod + 1].low;

		for (size_t j = i - kPeriod + 1; j <= i; j++) {
			highestHigh = std::max(highestHigh, candles[j].high);
			lowestLow = std::min(lowestLow, candles[j].low);
		}

		double currentClose = candles[i].close;
		if (highestHigh != lowestLow) {
			result.k[i] = ((currentClose - lowestLow) / (highestHigh - lowestLow)) * 100.0;
		} else {
			result.k[i] = 50.0; // Neutral when no range
		}
	}

	// Calculate %D line (SMA of %K)
	result.d = sma(result.k, dPeriod);

	return result;
}

// On-Balance Volume (OBV)
std::vector<double> Indicators::obv(const std::vector<Candle>& candles) {
	std::vector<double> result;

	if (candles.empty()) {
		return result;
	}

	double obv_val = 0.0;
	result.push_back(obv_val); // First candle

	for (size_t i = 1; i < candles.size(); i++) {
		if (candles[i].close > candles[i - 1].close) {
			obv_val += candles[i].volume;
		} else if (candles[i].close < candles[i - 1].close) {
			obv_val -= candles[i].volume;
		}
		// If close == previous close, OBV unchanged

		result.push_back(obv_val);
	}

	return result;
}

// Average Directional Index (ADX) - Optimized with sliding window
std::vector<double> Indicators::adx(const std::vector<Candle>& candles, int period) {
	std::vector<double> result;

	if (candles.size() < static_cast<size_t>(period * 2)) {
		return result; // Not enough data
	}

	// Calculate +DM, -DM, and TR
	std::vector<double> plusDM, minusDM, tr;

	plusDM.push_back(0);
	minusDM.push_back(0);
	tr.push_back(candles[0].high - candles[0].low);

	for (size_t i = 1; i < candles.size(); i++) {
		double highDiff = candles[i].high - candles[i - 1].high;
		double lowDiff = candles[i - 1].low - candles[i].low;

		double plusDM_val = (highDiff > lowDiff && highDiff > 0) ? highDiff : 0;
		double minusDM_val = (lowDiff > highDiff && lowDiff > 0) ? lowDiff : 0;

		plusDM.push_back(plusDM_val);
		minusDM.push_back(minusDM_val);

		double tr1 = candles[i].high - candles[i].low;
		double tr2 = std::abs(candles[i].high - candles[i - 1].close);
		double tr3 = std::abs(candles[i].low - candles[i - 1].close);
		tr.push_back(std::max({tr1, tr2, tr3}));
	}

	// Calculate smoothed +DI and -DI using sliding window
	std::vector<double> plusDI, minusDI;

	// Fill initial NaN values
	for (int i = 0; i < period - 1; i++) {
		plusDI.push_back(NAN);
		minusDI.push_back(NAN);
		result.push_back(NAN);
	}

	// Calculate initial sums for first window
	double sumPlusDM = 0, sumMinusDM = 0, sumTR = 0;
	for (int j = 0; j < period; j++) {
		sumPlusDM += plusDM[j];
		sumMinusDM += minusDM[j];
		sumTR += tr[j];
	}

	// First DI and ADX calculation
	double plusDI_val = (sumTR != 0) ? (sumPlusDM / sumTR) * 100 : 0;
	double minusDI_val = (sumTR != 0) ? (sumMinusDM / sumTR) * 100 : 0;

	plusDI.push_back(plusDI_val);
	minusDI.push_back(minusDI_val);

	double diSum = plusDI_val + minusDI_val;
	double diDiff = std::abs(plusDI_val - minusDI_val);
	double dx = (diSum != 0) ? (diDiff / diSum) * 100 : 0;
	result.push_back(dx);

	// Subsequent DI and ADX calculations using sliding window
	for (size_t i = period; i < candles.size(); i++) {
		// Sliding window: remove oldest, add newest
		sumPlusDM = sumPlusDM - plusDM[i - period] + plusDM[i];
		sumMinusDM = sumMinusDM - minusDM[i - period] + minusDM[i];
		sumTR = sumTR - tr[i - period] + tr[i];

		plusDI_val = (sumTR != 0) ? (sumPlusDM / sumTR) * 100 : 0;
		minusDI_val = (sumTR != 0) ? (sumMinusDM / sumTR) * 100 : 0;

		plusDI.push_back(plusDI_val);
		minusDI.push_back(minusDI_val);

		// Calculate DX
		diSum = plusDI_val + minusDI_val;
		diDiff = std::abs(plusDI_val - minusDI_val);
		dx = (diSum != 0) ? (diDiff / diSum) * 100 : 0;

		// Smoothed ADX
		double prevADX = result.back();
		double adx_val = ((prevADX * (period - 1)) + dx) / period;
		result.push_back(adx_val);
	}

	return result;
}

// Commodity Channel Index (CCI) - Optimized with sliding window for SMA
std::vector<double> Indicators::cci(const std::vector<Candle>& candles, int period) {
	std::vector<double> result;

	if (candles.size() < static_cast<size_t>(period)) {
		return result; // Not enough data
	}

	// Calculate typical price (TP) for each candle
	std::vector<double> typicalPrices;
	typicalPrices.reserve(candles.size());
	for (const auto& candle : candles) {
		double tp = (candle.high + candle.low + candle.close) / 3.0;
		typicalPrices.push_back(tp);
	}

	// Fill initial NaN values
	for (int i = 0; i < period - 1; i++) {
		result.push_back(NAN);
	}

	// Calculate first SMA of typical prices (initial window)
	double sumTP = 0.0;
	for (int j = 0; j < period; j++) {
		sumTP += typicalPrices[j];
	}
	double smaTP = sumTP / period;

	// Calculate first CCI
	double meanDev = 0.0;
	for (int j = 0; j < period; j++) {
		meanDev += std::abs(typicalPrices[j] - smaTP);
	}
	meanDev /= period;

	double cci_val = (meanDev != 0) ? (typicalPrices[period - 1] - smaTP) / (0.015 * meanDev) : 0;
	result.push_back(cci_val);

	// Calculate subsequent CCIs using sliding window for SMA
	for (size_t i = period; i < candles.size(); i++) {
		// Sliding window for SMA: remove oldest, add newest
		sumTP = sumTP - typicalPrices[i - period] + typicalPrices[i];
		smaTP = sumTP / period;

		// Fixed: Use forward indexing for clarity (was: i - j)
		// Calculate mean deviation
		meanDev = 0.0;
		for (size_t j = i - period + 1; j <= i; j++) {
			meanDev += std::abs(typicalPrices[j] - smaTP);  // ✅ Clear forward indexing
		}
		meanDev /= period;

		// Calculate CCI
		cci_val = (meanDev != 0) ? (typicalPrices[i] - smaTP) / (0.015 * meanDev) : 0;
		result.push_back(cci_val);
	}

	return result;
}

// Convenience methods that return single values (last value from calculation)

double Indicators::calculateSMA(const std::vector<double>& data, int period) {
	if (data.size() < static_cast<size_t>(period)) {
		return NAN;
	}

	std::vector<double> result = sma(data, period);
	return result.empty() ? NAN : result.back();
}

double Indicators::calculateEMA(const std::vector<double>& data, int period) {
	if (data.size() < static_cast<size_t>(period)) {
		return NAN;
	}

	std::vector<double> result = ema(data, period);
	return result.empty() ? NAN : result.back();
}

double Indicators::calculateRSI(const std::vector<double>& data, int period) {
	if (data.size() < static_cast<size_t>(period + 1)) {
		return NAN;
	}

	std::vector<double> result = rsi(data, period);
	return result.empty() ? NAN : result.back();
}

double Indicators::calculateStdDev(const std::vector<double>& data) {
	if (data.empty()) {
		return NAN;
	}

	// Calculate mean
	double sum = 0.0;
	for (double val : data) {
		sum += val;
	}
	double mean = sum / data.size();

	// Calculate variance
	double variance = 0.0;
	for (double val : data) {
		double diff = val - mean;
		variance += diff * diff;
	}
	variance /= data.size();

	return std::sqrt(variance);
}

double Indicators::calculateATR(const std::vector<double>& highs,
                                const std::vector<double>& lows,
                                const std::vector<double>& closes,
                                int period) {
	if (highs.size() != lows.size() || highs.size() != closes.size()) {
		return NAN;
	}

	if (highs.size() < static_cast<size_t>(period + 1)) {
		return NAN;
	}

	// Convert to candles
	std::vector<Candle> candles;
	for (size_t i = 0; i < highs.size(); i++) {
		Candle c;
		c.high = highs[i];
		c.low = lows[i];
		c.close = closes[i];
		c.open = closes[i]; // Not used in ATR calculation
		c.volume = 0;
		c.timestamp = 0;
		candles.push_back(c);
	}

	std::vector<double> result = atr(candles, period);
	return result.empty() ? NAN : result.back();
}

double Indicators::calculateStochastic(const std::vector<double>& highs,
                                       const std::vector<double>& lows,
                                       const std::vector<double>& closes,
                                       int period) {
	if (highs.size() != lows.size() || highs.size() != closes.size()) {
		return NAN;
	}

	if (highs.size() < static_cast<size_t>(period)) {
		return NAN;
	}

	// Convert to candles
	std::vector<Candle> candles;
	for (size_t i = 0; i < highs.size(); i++) {
		Candle c;
		c.high = highs[i];
		c.low = lows[i];
		c.close = closes[i];
		c.open = closes[i]; // Not used in Stochastic calculation
		c.volume = 0;
		c.timestamp = 0;
		candles.push_back(c);
	}

	StochasticResult result = stochastic(candles, period, 3);
	return result.k.empty() ? NAN : result.k.back();
}

double Indicators::calculateOBV(const std::vector<double>& closes,
                                const std::vector<double>& volumes) {
	if (closes.size() != volumes.size() || closes.empty()) {
		return NAN;
	}

	// Convert to candles
	std::vector<Candle> candles;
	for (size_t i = 0; i < closes.size(); i++) {
		Candle c;
		c.close = closes[i];
		c.volume = volumes[i];
		c.high = closes[i];
		c.low = closes[i];
		c.open = closes[i];
		c.timestamp = 0;
		candles.push_back(c);
	}

	std::vector<double> result = obv(candles);
	return result.empty() ? 0.0 : result.back();
}

Indicators::MACDValue Indicators::calculateMACD(const std::vector<double>& data,
                                                int fastPeriod,
                                                int slowPeriod,
                                                int signalPeriod) {
	MACDValue singleResult;
	singleResult.macdLine = NAN;
	singleResult.signalLine = NAN;
	singleResult.histogram = NAN;

	if (data.size() < static_cast<size_t>(slowPeriod)) {
		return singleResult;
	}

	// Use the vector-based macd function
	auto vectorResult = macd(data, fastPeriod, slowPeriod, signalPeriod);

	if (!vectorResult.macdLine.empty() && !vectorResult.signalLine.empty() && !vectorResult.histogram.empty()) {
		singleResult.macdLine = vectorResult.macdLine.back();
		singleResult.signalLine = vectorResult.signalLine.back();
		singleResult.histogram = vectorResult.histogram.back();
	}

	return singleResult;
}

Indicators::BollingerBandsValue Indicators::calculateBollingerBands(const std::vector<double>& data,
                                                                     int period,
                                                                     double stdDevMultiplier) {
	BollingerBandsValue singleResult;
	singleResult.upper = NAN;
	singleResult.middle = NAN;
	singleResult.lower = NAN;

	if (data.size() < static_cast<size_t>(period)) {
		return singleResult;
	}

	// Use the vector-based bollingerBands function
	auto vectorResult = bollingerBands(data, period, stdDevMultiplier);

	if (!vectorResult.upper.empty() && !vectorResult.middle.empty() && !vectorResult.lower.empty()) {
		singleResult.upper = vectorResult.upper.back();
		singleResult.middle = vectorResult.middle.back();
		singleResult.lower = vectorResult.lower.back();
	}

	return singleResult;
}

} // namespace Emiglio
