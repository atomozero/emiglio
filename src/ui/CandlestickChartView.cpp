#include "CandlestickChartView.h"
#include <algorithm>
#include <cmath>

namespace Emiglio {
namespace UI {

CandlestickChartView::CandlestickChartView()
	: BView("CandlestickChart", B_WILL_DRAW | B_FRAME_EVENTS)
	, startIndex(0)
	, visibleCandles(100)
	, minPrice(0)
	, maxPrice(100)
	, candleWidth(8.0f)
	, isDragging(false)
	, currentMousePos(0, 0)
	, showCrosshair(false)
{
	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(255, 255, 255);  // White background
	SetHighColor(0, 0, 0);        // Black for drawing
	SetEventMask(B_POINTER_EVENTS, 0);  // Always track mouse
}

CandlestickChartView::~CandlestickChartView() {
}

void CandlestickChartView::SetCandles(const std::vector<Candle>& newCandles) {
	candles = newCandles;

	if (!candles.empty()) {
		// Show last N candles by default
		int32 totalCandles = static_cast<int32>(candles.size());
		startIndex = std::max(0, totalCandles - visibleCandles);
	}

	CalculateScale();
	Invalidate();
}

void CandlestickChartView::SetIndicatorData(const std::string& name,
                                             const std::vector<double>& data) {
	indicators[name] = data;
	Invalidate();
}

void CandlestickChartView::ClearIndicators() {
	indicators.clear();
	Invalidate();
}

void CandlestickChartView::CalculateScale() {
	if (candles.empty()) {
		minPrice = 0;
		maxPrice = 100;
		return;
	}

	// Calculate min/max price in visible range
	int32 endIndex = std::min(startIndex + visibleCandles,
	                          static_cast<int32>(candles.size()));

	minPrice = candles[startIndex].low;
	maxPrice = candles[startIndex].high;

	for (int32 i = startIndex; i < endIndex; i++) {
		if (candles[i].low < minPrice) minPrice = candles[i].low;
		if (candles[i].high > maxPrice) maxPrice = candles[i].high;
	}

	// Add 5% padding
	double padding = (maxPrice - minPrice) * 0.05;
	minPrice -= padding;
	maxPrice += padding;

	// Calculate candle width
	BRect bounds = Bounds();
	float chartWidth = bounds.Width() - 80;  // Leave space for price axis
	candleWidth = chartWidth / static_cast<float>(visibleCandles);
}

void CandlestickChartView::Draw(BRect updateRect) {
	BRect bounds = Bounds();

	// Fill background
	SetHighColor(255, 255, 255);
	FillRect(bounds);

	if (candles.empty()) {
		SetHighColor(0, 0, 0);
		BPoint center(bounds.Width() / 2, bounds.Height() / 2);
		DrawString("No data to display", center);
		return;
	}

	DrawGrid(bounds);
	DrawVolumeBars(bounds);
	DrawIndicators(bounds);
	DrawCandles(bounds);
	DrawAxes(bounds);
	DrawLegend(bounds);
	if (showCrosshair) {
		DrawCrosshair(bounds);
		DrawTooltip(bounds);
	}
}

void CandlestickChartView::DrawGrid(BRect bounds) {
	SetHighColor(230, 230, 230);  // Light gray

	// Horizontal grid lines (price levels)
	int numLines = 5;
	for (int i = 0; i <= numLines; i++) {
		float y = bounds.top + (bounds.Height() * i / numLines);
		StrokeLine(BPoint(bounds.left + 10, y), BPoint(bounds.right - 70, y));
	}

	// Vertical grid lines (time)
	int numVLines = 10;
	float chartWidth = bounds.Width() - 80;
	for (int i = 0; i <= numVLines; i++) {
		float x = bounds.left + 10 + (chartWidth * i / numVLines);
		StrokeLine(BPoint(x, bounds.top), BPoint(x, bounds.bottom - 30));
	}
}

void CandlestickChartView::DrawCandles(BRect bounds) {
	if (candles.empty()) return;

	int32 endIndex = std::min(startIndex + visibleCandles,
	                          static_cast<int32>(candles.size()));

	float chartHeight = bounds.Height() - 40;  // Leave space for time axis
	float priceRange = maxPrice - minPrice;

	if (priceRange <= 0) return;

	for (int32 i = startIndex; i < endIndex; i++) {
		const Candle& candle = candles[i];

		// Calculate positions
		float x = 10 + ((i - startIndex) * candleWidth) + (candleWidth / 2);
		float yHigh = 10 + chartHeight * (1.0 - (candle.high - minPrice) / priceRange);
		float yLow = 10 + chartHeight * (1.0 - (candle.low - minPrice) / priceRange);
		float yOpen = 10 + chartHeight * (1.0 - (candle.open - minPrice) / priceRange);
		float yClose = 10 + chartHeight * (1.0 - (candle.close - minPrice) / priceRange);

		// Determine candle color (green for bullish, red for bearish)
		bool bullish = candle.close > candle.open;
		if (bullish) {
			SetHighColor(0, 180, 0);  // Green
		} else {
			SetHighColor(220, 0, 0);  // Red
		}

		// Draw wick (high-low line)
		StrokeLine(BPoint(x, yHigh), BPoint(x, yLow));

		// Draw body (open-close rectangle)
		float bodyTop = std::min(yOpen, yClose);
		float bodyBottom = std::max(yOpen, yClose);
		float bodyHeight = bodyBottom - bodyTop;
		if (bodyHeight < 1) bodyHeight = 1;  // Minimum height

		float bodyWidth = candleWidth * 0.7f;
		if (bodyWidth < 2) bodyWidth = 2;

		BRect bodyRect(x - bodyWidth/2, bodyTop,
		               x + bodyWidth/2, bodyBottom);

		FillRect(bodyRect);

		// Draw border
		SetHighColor(0, 0, 0);
		StrokeRect(bodyRect);
	}
}

void CandlestickChartView::DrawIndicators(BRect bounds) {
	if (candles.empty() || indicators.empty()) return;

	float chartHeight = bounds.Height() - 40;
	float priceRange = maxPrice - minPrice;
	if (priceRange <= 0) return;

	// Draw each indicator
	for (const auto& indicator : indicators) {
		const std::string& name = indicator.first;
		const std::vector<double>& data = indicator.second;

		if (data.empty()) continue;

		// Set color based on indicator name
		if (name.find("EMA") != std::string::npos) {
			SetHighColor(0, 0, 255);  // Blue for EMA
		} else if (name.find("Upper") != std::string::npos) {
			SetHighColor(255, 140, 0);  // Orange for Bollinger upper
		} else if (name.find("Lower") != std::string::npos) {
			SetHighColor(255, 140, 0);  // Orange for Bollinger lower
		} else {
			SetHighColor(128, 0, 128);  // Purple for others
		}

		// Draw line connecting indicator values
		int32 endIndex = std::min(startIndex + visibleCandles,
		                          static_cast<int32>(data.size()));

		BPoint lastPoint;
		bool hasLastPoint = false;

		for (int32 i = startIndex; i < endIndex; i++) {
			if (i >= static_cast<int32>(data.size())) break;
			if (std::isnan(data[i]) || std::isinf(data[i])) continue;

			float x = 10 + ((i - startIndex) * candleWidth) + (candleWidth / 2);
			float y = 10 + chartHeight * (1.0 - (data[i] - minPrice) / priceRange);

			BPoint point(x, y);

			if (hasLastPoint) {
				StrokeLine(lastPoint, point);
			}

			lastPoint = point;
			hasLastPoint = true;
		}
	}
}

void CandlestickChartView::DrawAxes(BRect bounds) {
	SetHighColor(0, 0, 0);

	// Draw price labels on right side
	int numLabels = 5;
	float chartHeight = bounds.Height() - 40;

	BFont font(be_plain_font);
	font.SetSize(10);
	SetFont(&font);

	for (int i = 0; i <= numLabels; i++) {
		float y = 10 + (chartHeight * i / numLabels);
		double price = maxPrice - ((maxPrice - minPrice) * i / numLabels);

		char label[32];
		snprintf(label, sizeof(label), "%.2f", price);

		DrawString(label, BPoint(bounds.right - 65, y + 5));
	}

	// Draw time labels at bottom (show every Nth candle)
	if (!candles.empty()) {
		int32 endIndex = std::min(startIndex + visibleCandles,
		                          static_cast<int32>(candles.size()));
		int step = std::max(1, (endIndex - startIndex) / 10);

		for (int32 i = startIndex; i < endIndex; i += step) {
			float x = 10 + ((i - startIndex) * candleWidth);

			// Convert timestamp to simple index for now
			char label[32];
			snprintf(label, sizeof(label), "%d", i);

			DrawString(label, BPoint(x, bounds.bottom - 10));
		}
	}
}

void CandlestickChartView::MouseDown(BPoint where) {
	isDragging = true;
	lastMousePos = where;
	SetMouseEventMask(B_POINTER_EVENTS);
}

void CandlestickChartView::MouseMoved(BPoint where, uint32 transit,
                                       const BMessage* dragMessage) {
	// Always update crosshair position
	currentMousePos = where;
	showCrosshair = (transit == B_INSIDE_VIEW);

	if (isDragging) {
		// Pan based on mouse movement
		float dx = where.x - lastMousePos.x;
		int32 candleDelta = static_cast<int32>(-dx / candleWidth);

		if (candleDelta != 0) {
			int32 newStartIndex = startIndex + candleDelta;
			int32 maxStart = static_cast<int32>(candles.size()) - visibleCandles;

			startIndex = std::max(0, std::min(newStartIndex, maxStart));

			CalculateScale();
			Invalidate();

			lastMousePos = where;
		}
	} else {
		// Just update crosshair
		Invalidate();
	}
}

void CandlestickChartView::MouseUp(BPoint where) {
	isDragging = false;
}

void CandlestickChartView::FrameResized(float newWidth, float newHeight) {
	BView::FrameResized(newWidth, newHeight);
	CalculateScale();
	Invalidate();
}

void CandlestickChartView::ZoomIn() {
	visibleCandles = std::max(20, visibleCandles - 20);

	// Adjust start index to keep view centered
	int32 maxStart = static_cast<int32>(candles.size()) - visibleCandles;
	startIndex = std::max(0, std::min(startIndex, maxStart));

	CalculateScale();
	Invalidate();
}

void CandlestickChartView::ZoomOut() {
	visibleCandles = std::min(500, visibleCandles + 20);

	int32 maxStart = static_cast<int32>(candles.size()) - visibleCandles;
	startIndex = std::max(0, std::min(startIndex, maxStart));

	CalculateScale();
	Invalidate();
}

void CandlestickChartView::PanLeft() {
	startIndex = std::max(0, startIndex - 10);
	CalculateScale();
	Invalidate();
}

void CandlestickChartView::PanRight() {
	int32 maxStart = static_cast<int32>(candles.size()) - visibleCandles;
	startIndex = std::min(maxStart, startIndex + 10);
	CalculateScale();
	Invalidate();
}

void CandlestickChartView::ResetView() {
	if (!candles.empty()) {
		visibleCandles = 100;
		int32 totalCandles = static_cast<int32>(candles.size());
		startIndex = std::max(0, totalCandles - visibleCandles);
		CalculateScale();
		Invalidate();
	}
}

void CandlestickChartView::DrawVolumeBars(BRect bounds) {
	if (candles.empty()) return;

	int32 endIndex = std::min(startIndex + visibleCandles,
	                          static_cast<int32>(candles.size()));

	// Find max volume in visible range
	double maxVolume = 0;
	for (int32 i = startIndex; i < endIndex; i++) {
		if (candles[i].volume > maxVolume) maxVolume = candles[i].volume;
	}

	if (maxVolume <= 0) return;

	// Volume bars area (bottom 60px)
	float volumeHeight = 60;
	float volumeBottom = bounds.bottom - 30;

	// Draw volume bars
	for (int32 i = startIndex; i < endIndex; i++) {
		const Candle& candle = candles[i];
		float x = 10 + ((i - startIndex) * candleWidth);
		float barHeight = (candle.volume / maxVolume) * volumeHeight;
		float y = volumeBottom - barHeight;

		// Color based on candle direction
		bool bullish = candle.close > candle.open;
		if (bullish) {
			SetHighColor(0, 180, 0, 100);  // Semi-transparent green
		} else {
			SetHighColor(220, 0, 0, 100);  // Semi-transparent red
		}

		float barWidth = candleWidth * 0.8f;
		if (barWidth < 1) barWidth = 1;

		BRect barRect(x, y, x + barWidth, volumeBottom);
		FillRect(barRect);
	}
}

void CandlestickChartView::DrawCrosshair(BRect bounds) {
	if (!showCrosshair) return;

	// Draw semi-transparent crosshair lines
	SetHighColor(100, 100, 100, 150);
	SetDrawingMode(B_OP_ALPHA);

	// Vertical line
	float chartLeft = 10;
	float chartRight = bounds.right - 70;
	if (currentMousePos.x >= chartLeft && currentMousePos.x <= chartRight) {
		StrokeLine(BPoint(currentMousePos.x, 10),
		           BPoint(currentMousePos.x, bounds.bottom - 100));
	}

	// Horizontal line
	float chartTop = 10;
	float chartBottom = bounds.bottom - 100;
	if (currentMousePos.y >= chartTop && currentMousePos.y <= chartBottom) {
		StrokeLine(BPoint(chartLeft, currentMousePos.y),
		           BPoint(chartRight, currentMousePos.y));
	}

	SetDrawingMode(B_OP_COPY);
}

void CandlestickChartView::DrawTooltip(BRect bounds) {
	if (!showCrosshair || candles.empty()) return;

	// Find candle under mouse
	float chartLeft = 10;
	float mouseX = currentMousePos.x - chartLeft;
	int32 candleIndex = startIndex + static_cast<int32>(mouseX / candleWidth);

	if (candleIndex < 0 || candleIndex >= static_cast<int32>(candles.size())) return;

	const Candle& candle = candles[candleIndex];

	// Create tooltip text lines
	char line1[128], line2[128], line3[128];
	snprintf(line1, sizeof(line1), "Open:   %.2f  |  Close: %.2f", candle.open, candle.close);
	snprintf(line2, sizeof(line2), "High:   %.2f  |  Low:   %.2f", candle.high, candle.low);

	// Format volume with thousand separators
	if (candle.volume >= 1000000) {
		snprintf(line3, sizeof(line3), "Volume: %.2fM", candle.volume / 1000000.0);
	} else if (candle.volume >= 1000) {
		snprintf(line3, sizeof(line3), "Volume: %.2fK", candle.volume / 1000.0);
	} else {
		snprintf(line3, sizeof(line3), "Volume: %.0f", candle.volume);
	}

	// Draw tooltip box
	BFont font(be_plain_font);
	font.SetSize(10);
	SetFont(&font);

	// Calculate dimensions for multi-line tooltip
	float maxWidth = std::max({font.StringWidth(line1),
	                            font.StringWidth(line2),
	                            font.StringWidth(line3)});
	float lineHeight = 14;
	float padding = 8;
	float tooltipHeight = lineHeight * 3 + padding * 2;

	float tooltipX = currentMousePos.x + 15;
	float tooltipY = currentMousePos.y - 10;

	// Keep tooltip in bounds
	if (tooltipX + maxWidth + padding * 2 > bounds.right - 70) {
		tooltipX = currentMousePos.x - maxWidth - padding * 2 - 15;
	}
	if (tooltipY < 10) tooltipY = 10;
	if (tooltipY + tooltipHeight > bounds.bottom - 100) {
		tooltipY = bounds.bottom - 100 - tooltipHeight;
	}

	BRect tooltipRect(tooltipX, tooltipY,
	                  tooltipX + maxWidth + padding * 2,
	                  tooltipY + tooltipHeight);

	// Draw semi-transparent background
	SetHighColor(50, 50, 50, 240);
	SetDrawingMode(B_OP_ALPHA);
	FillRect(tooltipRect);

	// Draw border
	SetHighColor(200, 200, 200);
	SetDrawingMode(B_OP_COPY);
	StrokeRect(tooltipRect);

	// Draw text in white
	SetHighColor(255, 255, 255);
	DrawString(line1, BPoint(tooltipX + padding, tooltipY + padding + lineHeight));
	DrawString(line2, BPoint(tooltipX + padding, tooltipY + padding + lineHeight * 2));
	DrawString(line3, BPoint(tooltipX + padding, tooltipY + padding + lineHeight * 3));
}

void CandlestickChartView::DrawLegend(BRect bounds) {
	if (indicators.empty()) return;

	// Legend position (top-left)
	float legendX = 15;
	float legendY = 15;
	float lineHeight = 15;

	BFont font(be_plain_font);
	font.SetSize(10);
	SetFont(&font);

	int index = 0;
	for (const auto& indicator : indicators) {
		const std::string& name = indicator.first;

		// Set color matching indicator
		if (name.find("EMA") != std::string::npos) {
			SetHighColor(0, 0, 255);
		} else if (name.find("Upper") != std::string::npos ||
		           name.find("Lower") != std::string::npos) {
			SetHighColor(255, 140, 0);
		} else {
			SetHighColor(128, 0, 128);
		}

		// Draw color box
		BRect colorBox(legendX, legendY + index * lineHeight,
		               legendX + 10, legendY + index * lineHeight + 10);
		FillRect(colorBox);

		// Draw label
		SetHighColor(0, 0, 0);
		DrawString(name.c_str(), BPoint(legendX + 15, legendY + index * lineHeight + 10));

		index++;
	}
}

} // namespace UI
} // namespace Emiglio
