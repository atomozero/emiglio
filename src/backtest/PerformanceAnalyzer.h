#ifndef EMIGLIO_BACKTEST_PERFORMANCE_ANALYZER_H
#define EMIGLIO_BACKTEST_PERFORMANCE_ANALYZER_H

#include "BacktestResult.h"
#include <string>
#include <vector>

namespace Emiglio {
namespace Backtest {

// Performance analyzer for backtest results
class PerformanceAnalyzer {
public:
	PerformanceAnalyzer();
	~PerformanceAnalyzer();

	// Analyze result and calculate all metrics
	void analyze(BacktestResult& result);

	// Generate reports
	std::string generateTextReport(const BacktestResult& result) const;
	std::string generateJSONReport(const BacktestResult& result) const;

private:
	// Metric calculations
	double calculateTotalReturn(const BacktestResult& result) const;
	double calculateAnnualizedReturn(const BacktestResult& result) const;
	double calculateSharpeRatio(const BacktestResult& result) const;
	double calculateSortinoRatio(const BacktestResult& result) const;
	double calculateMaxDrawdown(const BacktestResult& result, int& maxDrawdownDays) const;
	double calculateWinRate(const BacktestResult& result) const;
	double calculateProfitFactor(const BacktestResult& result) const;
	double calculateExpectancy(const BacktestResult& result) const;
	double calculateAverageWin(const BacktestResult& result) const;
	double calculateAverageLoss(const BacktestResult& result) const;

	// Helper functions
	std::vector<double> calculateReturns(const std::vector<EquityPoint>& equityCurve) const;
	double calculateStandardDeviation(const std::vector<double>& data) const;
	double calculateDownsideDeviation(const std::vector<double>& returns) const;
};

} // namespace Backtest
} // namespace Emiglio

#endif // EMIGLIO_BACKTEST_PERFORMANCE_ANALYZER_H
