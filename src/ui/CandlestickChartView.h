#ifndef EMIGLIO_CANDLESTICKCHARTVIEW_H
#define EMIGLIO_CANDLESTICKCHARTVIEW_H

#include <View.h>
#include "../data/DataStorage.h"
#include <vector>
#include <map>
#include <string>

namespace Emiglio {
namespace UI {

// Custom view for rendering candlestick chart
class CandlestickChartView : public BView {
public:
	CandlestickChartView();
	virtual ~CandlestickChartView();

	virtual void Draw(BRect updateRect) override;
	virtual void MouseDown(BPoint where) override;
	virtual void MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage) override;
	virtual void MouseUp(BPoint where) override;
	virtual void FrameResized(float newWidth, float newHeight) override;

	// Data management
	void SetCandles(const std::vector<Candle>& candles);
	void SetIndicatorData(const std::string& name, const std::vector<double>& data);
	void ClearIndicators();

	// View controls
	void ZoomIn();
	void ZoomOut();
	void PanLeft();
	void PanRight();
	void ResetView();

	// Get candles for volume calculation
	const std::vector<Candle>& GetCandles() const { return candles; }

private:
	void DrawGrid(BRect bounds);
	void DrawCandles(BRect bounds);
	void DrawIndicators(BRect bounds);
	void DrawAxes(BRect bounds);
	void DrawVolumeBars(BRect bounds);
	void DrawCrosshair(BRect bounds);
	void DrawTooltip(BRect bounds);
	void DrawLegend(BRect bounds);
	void CalculateScale();

	// Data
	std::vector<Candle> candles;
	std::map<std::string, std::vector<double>> indicators;

	// View state
	int32 startIndex;       // First candle to display
	int32 visibleCandles;   // Number of candles visible
	double minPrice;        // Min price in view
	double maxPrice;        // Max price in view
	float candleWidth;      // Width of each candle in pixels

	// Interaction state
	bool isDragging;
	BPoint lastMousePos;
	BPoint currentMousePos;  // For crosshair
	bool showCrosshair;
};

} // namespace UI
} // namespace Emiglio

#endif // EMIGLIO_CANDLESTICKCHARTVIEW_H
