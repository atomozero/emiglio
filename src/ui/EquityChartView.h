#ifndef EMIGLIO_EQUITY_CHART_VIEW_H
#define EMIGLIO_EQUITY_CHART_VIEW_H

#include <View.h>
#include "../backtest/BacktestResult.h"
#include "../backtest/Trade.h"
#include <vector>

namespace Emiglio {
namespace UI {

// Custom view for rendering equity curve chart
class EquityChartView : public BView {
public:
	EquityChartView();
	virtual ~EquityChartView();

	virtual void Draw(BRect updateRect) override;
	virtual void MouseDown(BPoint where) override;
	virtual void MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage) override;
	virtual void MouseUp(BPoint where) override;
	virtual void FrameResized(float newWidth, float newHeight) override;

	// Data management
	void SetEquityCurve(const std::vector<Backtest::EquityPoint>& curve);
	void SetTrades(const std::vector<Backtest::Trade>& tradeList);
	void SetInitialCapital(double capital);
	void SetSelectedTradeIndex(int32 index);
	void Clear();

	// Get data
	const std::vector<Backtest::EquityPoint>& GetEquityCurve() const { return equityCurve; }

private:
	void DrawGrid(BRect bounds);
	void DrawEquityLine(BRect bounds);
	void DrawTradeMarkers(BRect bounds);
	void DrawAxes(BRect bounds);
	void DrawLegend(BRect bounds);
	void CalculateScale();

	// Data
	std::vector<Backtest::EquityPoint> equityCurve;
	std::vector<Backtest::Trade> trades;
	double initialCapital;

	// View state
	double minEquity;        // Min equity in view
	double maxEquity;        // Max equity in view
	time_t startTime;        // First timestamp
	time_t endTime;          // Last timestamp

	// Interaction state
	bool isDragging;
	BPoint lastMousePos;
	BPoint currentMousePos;  // For crosshair (future)
	bool showCrosshair;      // For future use
	int32 selectedTradeIndex; // Index of selected trade to highlight
};

} // namespace UI
} // namespace Emiglio

#endif // EMIGLIO_EQUITY_CHART_VIEW_H
