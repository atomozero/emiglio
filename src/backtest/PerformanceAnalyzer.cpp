#include "PerformanceAnalyzer.h"
#include "../utils/Logger.h"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace Emiglio {
namespace Backtest {

PerformanceAnalyzer::PerformanceAnalyzer() {
}

PerformanceAnalyzer::~PerformanceAnalyzer() {
}

void PerformanceAnalyzer::analyze(BacktestResult& result) {
	LOG_INFO("Analyzing backtest performance for: " + result.recipeName);

	// Calculate all metrics
	result.totalReturn = calculateTotalReturn(result);
	result.totalReturnPercent = result.totalReturn;
	result.annualizedReturn = calculateAnnualizedReturn(result);

	int maxDrawdownDays = 0;
	result.maxDrawdown = calculateMaxDrawdown(result, maxDrawdownDays);
	result.maxDrawdownPercent = result.maxDrawdown;

	result.sharpeRatio = calculateSharpeRatio(result);
	result.sortinoRatio = calculateSortinoRatio(result);
	result.winRate = calculateWinRate(result);
	result.profitFactor = calculateProfitFactor(result);
	result.expectancy = calculateExpectancy(result);
	result.averageWin = calculateAverageWin(result);
	result.averageLoss = calculateAverageLoss(result);

	LOG_INFO("Performance analysis completed");
}

double PerformanceAnalyzer::calculateTotalReturn(const BacktestResult& result) const {
	if (result.initialCapital == 0.0) return 0.0;
	return ((result.finalEquity - result.initialCapital) / result.initialCapital) * 100.0;
}

double PerformanceAnalyzer::calculateAnnualizedReturn(const BacktestResult& result) const {
	if (result.initialCapital == 0.0 || result.startTime == 0 || result.endTime == 0) {
		return 0.0;
	}

	// Calculate time period in years
	double totalSeconds = difftime(result.endTime, result.startTime);
	double years = totalSeconds / (365.25 * 24 * 60 * 60);

	if (years <= 0.0) return 0.0;

	// Annualized return = (finalEquity / initialCapital)^(1/years) - 1
	double totalReturnFactor = result.finalEquity / result.initialCapital;
	double annualizedReturn = (std::pow(totalReturnFactor, 1.0 / years) - 1.0) * 100.0;

	return annualizedReturn;
}

double PerformanceAnalyzer::calculateSharpeRatio(const BacktestResult& result) const {
	if (result.equityCurve.size() < 2) return 0.0;

	// Calculate period returns
	std::vector<double> returns = calculateReturns(result.equityCurve);

	if (returns.empty()) return 0.0;

	// Calculate average return
	double avgReturn = 0.0;
	for (double r : returns) {
		avgReturn += r;
	}
	avgReturn /= returns.size();

	// Calculate standard deviation
	double stdDev = calculateStandardDeviation(returns);

	if (stdDev == 0.0) return 0.0;

	// Sharpe ratio = (average return - risk free rate) / standard deviation
	// Assuming risk-free rate = 0 for simplicity
	double sharpe = avgReturn / stdDev;

	// Annualize Sharpe ratio (assuming daily data, multiply by sqrt(252))
	// For simplicity, we'll return the raw Sharpe
	return sharpe;
}

double PerformanceAnalyzer::calculateSortinoRatio(const BacktestResult& result) const {
	if (result.equityCurve.size() < 2) return 0.0;

	// Calculate period returns
	std::vector<double> returns = calculateReturns(result.equityCurve);

	if (returns.empty()) return 0.0;

	// Calculate average return
	double avgReturn = 0.0;
	for (double r : returns) {
		avgReturn += r;
	}
	avgReturn /= returns.size();

	// Calculate downside deviation (only negative returns)
	double downsideDev = calculateDownsideDeviation(returns);

	if (downsideDev == 0.0) return 0.0;

	// Sortino ratio = (average return - risk free rate) / downside deviation
	double sortino = avgReturn / downsideDev;

	return sortino;
}

double PerformanceAnalyzer::calculateMaxDrawdown(const BacktestResult& result,
                                                  int& maxDrawdownDays) const {
	if (result.equityCurve.empty()) return 0.0;

	double maxDrawdown = 0.0;
	double peak = result.initialCapital;
	maxDrawdownDays = 0;
	int currentDrawdownDays = 0;

	for (const auto& point : result.equityCurve) {
		// Update peak
		if (point.equity > peak) {
			peak = point.equity;
			currentDrawdownDays = 0;
		} else {
			currentDrawdownDays++;
		}

		// Calculate drawdown
		double drawdown = 0.0;
		if (peak > 0.0) {
			drawdown = ((peak - point.equity) / peak) * 100.0;
		}

		// Update max drawdown
		if (drawdown > maxDrawdown) {
			maxDrawdown = drawdown;
			maxDrawdownDays = currentDrawdownDays;
		}
	}

	return maxDrawdown;
}

double PerformanceAnalyzer::calculateWinRate(const BacktestResult& result) const {
	if (result.totalTrades == 0) return 0.0;
	return (static_cast<double>(result.winningTrades) / result.totalTrades) * 100.0;
}

double PerformanceAnalyzer::calculateProfitFactor(const BacktestResult& result) const {
	double totalWins = 0.0;
	double totalLosses = 0.0;

	for (const auto& trade : result.trades) {
		if (trade.pnl > 0.0) {
			totalWins += trade.pnl;
		} else if (trade.pnl < 0.0) {
			totalLosses += std::abs(trade.pnl);
		}
	}

	if (totalLosses == 0.0) {
		return totalWins > 0.0 ? 999.99 : 0.0;  // Infinite profit factor
	}

	return totalWins / totalLosses;
}

double PerformanceAnalyzer::calculateExpectancy(const BacktestResult& result) const {
	if (result.totalTrades == 0) return 0.0;

	double totalPnL = 0.0;
	for (const auto& trade : result.trades) {
		totalPnL += trade.pnl;
	}

	return totalPnL / result.totalTrades;
}

double PerformanceAnalyzer::calculateAverageWin(const BacktestResult& result) const {
	if (result.winningTrades == 0) return 0.0;

	double totalWins = 0.0;
	for (const auto& trade : result.trades) {
		if (trade.pnl > 0.0) {
			totalWins += trade.pnl;
		}
	}

	return totalWins / result.winningTrades;
}

double PerformanceAnalyzer::calculateAverageLoss(const BacktestResult& result) const {
	if (result.losingTrades == 0) return 0.0;

	double totalLosses = 0.0;
	for (const auto& trade : result.trades) {
		if (trade.pnl < 0.0) {
			totalLosses += trade.pnl;  // Already negative
		}
	}

	return totalLosses / result.losingTrades;
}

std::vector<double> PerformanceAnalyzer::calculateReturns(
	const std::vector<EquityPoint>& equityCurve) const {

	std::vector<double> returns;

	if (equityCurve.size() < 2) return returns;

	for (size_t i = 1; i < equityCurve.size(); i++) {
		double prevEquity = equityCurve[i - 1].equity;
		double currEquity = equityCurve[i].equity;

		if (prevEquity > 0.0) {
			double ret = (currEquity - prevEquity) / prevEquity;
			returns.push_back(ret);
		}
	}

	return returns;
}

double PerformanceAnalyzer::calculateStandardDeviation(const std::vector<double>& data) const {
	if (data.empty()) return 0.0;

	// Calculate mean
	double mean = 0.0;
	for (double value : data) {
		mean += value;
	}
	mean /= data.size();

	// Calculate variance
	double variance = 0.0;
	for (double value : data) {
		double diff = value - mean;
		variance += diff * diff;
	}
	variance /= data.size();

	return std::sqrt(variance);
}

double PerformanceAnalyzer::calculateDownsideDeviation(const std::vector<double>& returns) const {
	if (returns.empty()) return 0.0;

	// Calculate mean
	double mean = 0.0;
	for (double r : returns) {
		mean += r;
	}
	mean /= returns.size();

	// Calculate downside variance (only for returns below mean)
	double downsideVariance = 0.0;
	int count = 0;
	for (double r : returns) {
		if (r < mean) {
			double diff = r - mean;
			downsideVariance += diff * diff;
			count++;
		}
	}

	if (count == 0) return 0.0;

	downsideVariance /= count;

	return std::sqrt(downsideVariance);
}

std::string PerformanceAnalyzer::generateTextReport(const BacktestResult& result) const {
	std::ostringstream report;

	report << std::fixed << std::setprecision(2);

	report << "===================================\n";
	report << "BACKTEST PERFORMANCE REPORT\n";
	report << "===================================\n\n";

	report << "Strategy: " << result.recipeName << "\n";
	report << "Symbol: " << result.symbol << "\n";
	report << "Period: " << result.totalCandles << " candles\n\n";

	report << "--- Capital ---\n";
	report << "Initial Capital: $" << result.initialCapital << "\n";
	report << "Final Equity: $" << result.finalEquity << "\n";
	report << "Peak Equity: $" << result.peakEquity << "\n\n";

	report << "--- Returns ---\n";
	report << "Total Return: $" << (result.finalEquity - result.initialCapital)
	       << " (" << result.totalReturnPercent << "%)\n";
	report << "Annualized Return: " << result.annualizedReturn << "%\n\n";

	report << "--- Risk Metrics ---\n";
	report << "Max Drawdown: " << result.maxDrawdownPercent << "%\n";
	report << "Sharpe Ratio: " << std::setprecision(3) << result.sharpeRatio << "\n";
	report << "Sortino Ratio: " << result.sortinoRatio << "\n\n";

	report << std::setprecision(2);
	report << "--- Trading Stats ---\n";
	report << "Total Trades: " << result.totalTrades << "\n";
	report << "Winning Trades: " << result.winningTrades << "\n";
	report << "Losing Trades: " << result.losingTrades << "\n";
	report << "Win Rate: " << result.winRate << "%\n";
	report << "Profit Factor: " << std::setprecision(3) << result.profitFactor << "\n";
	report << "Expectancy: $" << std::setprecision(2) << result.expectancy << "\n";
	report << "Average Win: $" << result.averageWin << "\n";
	report << "Average Loss: $" << result.averageLoss << "\n\n";

	report << "--- Costs ---\n";
	report << "Total Commission: $" << result.totalCommission << "\n";
	report << "Total Slippage: $" << result.totalSlippage << "\n\n";

	report << "===================================\n";

	return report.str();
}

std::string PerformanceAnalyzer::generateJSONReport(const BacktestResult& result) const {
	std::ostringstream json;

	json << std::fixed << std::setprecision(2);

	// Split symbol into base and quote (e.g., BTCUSDT -> BTC + USDT)
	std::string base = "";
	std::string quote = "";
	std::string symbol = result.symbol;

	// Common quote currencies
	std::vector<std::string> quoteCoins = {"USDT", "USDC", "BUSD", "USD", "EUR", "BTC", "ETH", "BNB"};

	for (const auto& quoteCoin : quoteCoins) {
		if (symbol.length() > quoteCoin.length() &&
		    symbol.substr(symbol.length() - quoteCoin.length()) == quoteCoin) {
			quote = quoteCoin;
			base = symbol.substr(0, symbol.length() - quoteCoin.length());
			break;
		}
	}

	// Fallback if no match found
	if (base.empty()) {
		base = symbol;
		quote = "UNKNOWN";
	}

	json << "{\n";
	json << "  \"strategy\": \"" << result.recipeName << "\",\n";
	json << "  \"symbol\": {\n";
	json << "    \"full\": \"" << result.symbol << "\",\n";
	json << "    \"base\": \"" << base << "\",\n";
	json << "    \"quote\": \"" << quote << "\"\n";
	json << "  },\n";

	// Period information
	json << "  \"period\": {\n";
	json << "    \"startTime\": " << result.startTime << ",\n";
	json << "    \"endTime\": " << result.endTime << ",\n";
	json << "    \"durationDays\": " << ((result.endTime - result.startTime) / 86400.0) << ",\n";
	json << "    \"totalCandles\": " << result.totalCandles << "\n";
	json << "  },\n";

	json << "  \"capital\": {\n";
	json << "    \"initial\": " << result.initialCapital << ",\n";
	json << "    \"final\": " << result.finalEquity << ",\n";
	json << "    \"peak\": " << result.peakEquity << ",\n";
	json << "    \"netProfit\": " << (result.finalEquity - result.initialCapital) << "\n";
	json << "  },\n";

	json << "  \"returns\": {\n";
	json << "    \"totalReturn\": " << result.totalReturnPercent << ",\n";
	json << "    \"annualizedReturn\": " << result.annualizedReturn << "\n";
	json << "  },\n";

	json << "  \"risk\": {\n";
	json << "    \"maxDrawdown\": " << result.maxDrawdownPercent << ",\n";
	json << "    \"maxDrawdownAmount\": " << (result.peakEquity - (result.peakEquity * (1.0 - result.maxDrawdownPercent / 100.0))) << ",\n";
	json << "    \"sharpeRatio\": " << std::setprecision(3) << result.sharpeRatio << ",\n";
	json << "    \"sortinoRatio\": " << result.sortinoRatio << "\n";
	json << "  },\n";

	json << "  \"trading\": {\n";
	json << "    \"totalTrades\": " << result.totalTrades << ",\n";
	json << "    \"winningTrades\": " << result.winningTrades << ",\n";
	json << "    \"losingTrades\": " << result.losingTrades << ",\n";
	json << "    \"winRate\": " << std::setprecision(2) << result.winRate << ",\n";
	json << "    \"profitFactor\": " << std::setprecision(3) << result.profitFactor << ",\n";
	json << "    \"expectancy\": " << std::setprecision(2) << result.expectancy << ",\n";
	json << "    \"averageWin\": " << result.averageWin << ",\n";
	json << "    \"averageLoss\": " << result.averageLoss << "\n";
	json << "  },\n";

	json << "  \"costs\": {\n";
	json << "    \"totalCommission\": " << result.totalCommission << ",\n";
	json << "    \"totalSlippage\": " << result.totalSlippage << ",\n";
	json << "    \"totalCosts\": " << (result.totalCommission + result.totalSlippage) << "\n";
	json << "  },\n";

	// Calculate best/worst trade and streaks
	double bestTrade = 0.0;
	double worstTrade = 0.0;
	int currentWinStreak = 0;
	int currentLossStreak = 0;
	int longestWinStreak = 0;
	int longestLossStreak = 0;
	double totalWinAmount = 0.0;
	double totalLossAmount = 0.0;

	for (const auto& trade : result.trades) {
		if (trade.status == TradeStatus::CLOSED) {
			// Best/worst trade
			if (trade.pnl > bestTrade) bestTrade = trade.pnl;
			if (trade.pnl < worstTrade) worstTrade = trade.pnl;

			// Streaks
			if (trade.pnl > 0) {
				currentWinStreak++;
				currentLossStreak = 0;
				totalWinAmount += trade.pnl;
				if (currentWinStreak > longestWinStreak) {
					longestWinStreak = currentWinStreak;
				}
			} else {
				currentLossStreak++;
				currentWinStreak = 0;
				totalLossAmount += trade.pnl;
				if (currentLossStreak > longestLossStreak) {
					longestLossStreak = currentLossStreak;
				}
			}
		}
	}

	json << "  \"performance\": {\n";
	json << "    \"bestTrade\": " << bestTrade << ",\n";
	json << "    \"worstTrade\": " << worstTrade << ",\n";
	json << "    \"totalWinAmount\": " << totalWinAmount << ",\n";
	json << "    \"totalLossAmount\": " << totalLossAmount << ",\n";
	json << "    \"longestWinStreak\": " << longestWinStreak << ",\n";
	json << "    \"longestLossStreak\": " << longestLossStreak << "\n";
	json << "  },\n";

	// Trade list with all details
	json << "  \"trades\": [\n";
	bool firstTrade = true;
	for (const auto& trade : result.trades) {
		if (trade.status == TradeStatus::CLOSED) {
			if (!firstTrade) json << ",\n";
			firstTrade = false;

			json << "    {\n";
			json << "      \"id\": \"" << trade.id << "\",\n";
			json << "      \"entryTime\": " << trade.entryTime << ",\n";
			json << "      \"exitTime\": " << trade.exitTime << ",\n";
			json << "      \"entryPrice\": " << trade.entryPrice << ",\n";
			json << "      \"exitPrice\": " << trade.exitPrice << ",\n";
			json << "      \"quantity\": " << std::setprecision(6) << trade.quantity << ",\n";
			json << "      \"pnl\": " << std::setprecision(2) << trade.pnl << ",\n";
			json << "      \"pnlPercent\": " << (trade.pnl / (trade.entryPrice * trade.quantity) * 100.0) << ",\n";
			json << "      \"exitReason\": \"" << trade.exitReason << "\"\n";
			json << "    }";
		}
	}
	json << "\n  ]\n";

	json << "}\n";

	return json.str();
}

} // namespace Backtest
} // namespace Emiglio
