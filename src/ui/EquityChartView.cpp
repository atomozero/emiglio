#include "EquityChartView.h"
#include "../utils/Logger.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace Emiglio {
namespace UI {

EquityChartView::EquityChartView()
	: BView("EquityChart", B_WILL_DRAW | B_FRAME_EVENTS)
	, initialCapital(0.0)
	, minEquity(0.0)
	, maxEquity(100.0)
	, startTime(0)
	, endTime(0)
	, isDragging(false)
	, currentMousePos(0, 0)
	, showCrosshair(false)
	, selectedTradeIndex(-1)
{
	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(250, 250, 252);  // Light blue-gray background
	SetHighColor(0, 0, 0);        // Black for drawing
	SetEventMask(B_POINTER_EVENTS, 0);  // Always track mouse
}

EquityChartView::~EquityChartView() {
}

void EquityChartView::SetEquityCurve(const std::vector<Backtest::EquityPoint>& curve) {
	equityCurve = curve;
	CalculateScale();
	Invalidate();
}

void EquityChartView::SetTrades(const std::vector<Backtest::Trade>& tradeList) {
	trades = tradeList;
	Invalidate();
}

void EquityChartView::SetInitialCapital(double capital) {
	initialCapital = capital;
	Invalidate();
}

void EquityChartView::SetSelectedTradeIndex(int32 index) {
	selectedTradeIndex = index;
	// Force complete invalidation and redraw
	Invalidate();
	Flush();
	Sync();
}

void EquityChartView::Clear() {
	equityCurve.clear();
	trades.clear();
	initialCapital = 0.0;
	minEquity = 0.0;
	maxEquity = 100.0;
	startTime = 0;
	endTime = 0;
	selectedTradeIndex = -1;
	Invalidate();
}

void EquityChartView::CalculateScale() {
	if (equityCurve.empty()) {
		minEquity = 0;
		maxEquity = 100;
		startTime = 0;
		endTime = 0;
		return;
	}

	// Find min/max equity
	minEquity = equityCurve[0].equity;
	maxEquity = equityCurve[0].equity;
	startTime = equityCurve[0].timestamp;
	endTime = equityCurve[0].timestamp;

	for (const auto& point : equityCurve) {
		if (point.equity < minEquity) minEquity = point.equity;
		if (point.equity > maxEquity) maxEquity = point.equity;
		if (point.timestamp < startTime) startTime = point.timestamp;
		if (point.timestamp > endTime) endTime = point.timestamp;
	}

	// Add 5% padding
	double padding = (maxEquity - minEquity) * 0.05;
	if (padding < 0.01) padding = maxEquity * 0.05;  // Minimum padding
	minEquity -= padding;
	maxEquity += padding;

	// Ensure initial capital is visible
	if (initialCapital > 0) {
		if (initialCapital < minEquity) minEquity = initialCapital - padding;
		if (initialCapital > maxEquity) maxEquity = initialCapital + padding;
	}
}

void EquityChartView::Draw(BRect updateRect) {
	BRect bounds = Bounds();

	// Fill background with gradient-like effect
	SetHighColor(250, 250, 252);
	FillRect(bounds);

	// Draw subtle border
	SetHighColor(220, 220, 225);
	StrokeRect(bounds);

	if (equityCurve.empty()) {
		SetHighColor(100, 100, 110);
		font_height fh;
		GetFontHeight(&fh);
		BPoint center(bounds.Width() / 2 - 100, bounds.Height() / 2);
		DrawString("No equity data - Run a backtest to see results", center);
		return;
	}

	DrawGrid(bounds);
	DrawEquityLine(bounds);
	DrawTradeMarkers(bounds);
	DrawAxes(bounds);
	DrawLegend(bounds);
}

void EquityChartView::DrawGrid(BRect bounds) {
	SetHighColor(235, 235, 240);  // Very light gray

	// Horizontal grid lines (equity levels)
	int numLines = 6;
	for (int i = 0; i <= numLines; i++) {
		float y = bounds.top + 30 + ((bounds.Height() - 70) * i / numLines);
		StrokeLine(BPoint(bounds.left + 60, y), BPoint(bounds.right - 15, y));
	}

	// Vertical grid lines (time) - less intrusive
	int numVLines = 8;
	float chartWidth = bounds.Width() - 75;
	for (int i = 0; i <= numVLines; i++) {
		float x = bounds.left + 60 + (chartWidth * i / numVLines);
		// Draw dashed vertical lines
		for (float y = bounds.top + 30; y < bounds.bottom - 40; y += 10) {
			StrokeLine(BPoint(x, y), BPoint(x, y + 5));
		}
	}
}

void EquityChartView::DrawEquityLine(BRect bounds) {
	if (equityCurve.size() < 2) return;

	float chartHeight = bounds.Height() - 70;  // Leave space for axes
	float chartWidth = bounds.Width() - 75;    // Leave space for price axis
	double equityRange = maxEquity - minEquity;
	time_t timeRange = endTime - startTime;

	if (equityRange <= 0 || timeRange <= 0) return;

	// Draw initial capital line (horizontal dashed line)
	if (initialCapital > 0) {
		float y = bounds.top + 30 + chartHeight * (1.0 - (initialCapital - minEquity) / equityRange);

		SetHighColor(180, 180, 185);  // Light gray
		SetPenSize(1.5);
		// Draw dashed line
		for (float x = bounds.left + 60; x < bounds.right - 15; x += 12) {
			StrokeLine(BPoint(x, y), BPoint(x + 6, y));
		}
		SetPenSize(1.0);
	}

	// Determine if profit or loss
	double finalEquity = equityCurve.back().equity;
	bool isProfit = finalEquity >= initialCapital;

	// Fill area under curve with subtle gradient effect
	if (equityCurve.size() > 1) {
		SetHighColor(isProfit ? 220 : 255, isProfit ? 245 : 235, isProfit ? 220 : 235, 100);  // Semi-transparent

		// Create polygon for filled area
		BPoint points[equityCurve.size() + 2];
		int pointCount = 0;

		// Bottom left
		float firstX = bounds.left + 60 + chartWidth * ((double)(equityCurve[0].timestamp - startTime) / timeRange);
		points[pointCount++] = BPoint(firstX, bounds.bottom - 40);

		// Equity curve points
		for (size_t i = 0; i < equityCurve.size(); i++) {
			const auto& point = equityCurve[i];
			float x = bounds.left + 60 + chartWidth * ((double)(point.timestamp - startTime) / timeRange);
			float y = bounds.top + 30 + chartHeight * (1.0 - (point.equity - minEquity) / equityRange);
			points[pointCount++] = BPoint(x, y);
		}

		// Bottom right
		float lastX = bounds.left + 60 + chartWidth * ((double)(equityCurve.back().timestamp - startTime) / timeRange);
		points[pointCount++] = BPoint(lastX, bounds.bottom - 40);

		FillPolygon(points, pointCount);
	}

	// Draw equity line
	SetHighColor(isProfit ? 34 : 220, isProfit ? 139 : 38, isProfit ? 34 : 38);  // Green or Red
	SetPenSize(2.5);

	BPoint prevPoint;
	bool firstPoint = true;

	for (size_t i = 0; i < equityCurve.size(); i++) {
		const auto& point = equityCurve[i];

		// Calculate position
		float x = bounds.left + 60 + chartWidth * ((double)(point.timestamp - startTime) / timeRange);
		float y = bounds.top + 30 + chartHeight * (1.0 - (point.equity - minEquity) / equityRange);

		BPoint currentPoint(x, y);

		if (!firstPoint) {
			StrokeLine(prevPoint, currentPoint);
		}

		prevPoint = currentPoint;
		firstPoint = false;
	}

	SetPenSize(1.0);  // Reset pen size
}

void EquityChartView::DrawTradeMarkers(BRect bounds) {
	if (trades.empty() || equityCurve.empty()) return;

	float chartHeight = bounds.Height() - 70;
	float chartWidth = bounds.Width() - 75;
	double equityRange = maxEquity - minEquity;
	time_t timeRange = endTime - startTime;

	if (equityRange <= 0 || timeRange <= 0) return;

	// Draw trade markers
	for (size_t i = 0; i < trades.size(); i++) {
		const auto& trade = trades[i];

		// Only draw closed trades
		if (trade.status != Backtest::TradeStatus::CLOSED) continue;

		bool isSelected = (selectedTradeIndex >= 0 && i == (size_t)selectedTradeIndex);

		// Find equity point closest to entry time
		time_t entryTime = trade.entryTime;
		double equityAtEntry = initialCapital;
		bool foundEntry = false;

		for (const auto& point : equityCurve) {
			if (point.timestamp >= entryTime) {
				equityAtEntry = point.equity;
				foundEntry = true;
				break;
			}
		}

		// Skip if we couldn't find entry in equity curve
		if (!foundEntry && !equityCurve.empty()) {
			equityAtEntry = equityCurve.front().equity;
		}

		// Calculate entry marker position
		float x = bounds.left + 60 + chartWidth * ((double)(entryTime - startTime) / timeRange);
		float y = bounds.top + 30 + chartHeight * (1.0 - (equityAtEntry - minEquity) / equityRange);

		// Skip if entry point is outside visible chart area
		if (x < bounds.left + 60 || x > bounds.right - 20 ||
		    y < bounds.top + 30 || y > bounds.bottom - 50) {
			continue;
		}

		// Draw entry marker (upward triangle for BUY)
		float size = isSelected ? 10.0f : 8.0f;  // Bigger if selected
		SetHighColor(34, 179, 34);  // Bright green
		BPoint entryTriangle[3];
		entryTriangle[0] = BPoint(x, y - size);           // Top
		entryTriangle[1] = BPoint(x - size*0.6f, y + 2);  // Bottom left
		entryTriangle[2] = BPoint(x + size*0.6f, y + 2);  // Bottom right
		FillPolygon(entryTriangle, 3);

		// Draw border (thicker if selected)
		SetPenSize(isSelected ? 2.5f : 1.0f);
		SetHighColor(isSelected ? 255 : 20, isSelected ? 200 : 120, isSelected ? 0 : 20);
		StrokePolygon(entryTriangle, 3);
		SetPenSize(1.0f);

		// Draw exit marker if trade is closed
		if (trade.exitTime > 0) {
			// Find equity at exit
			time_t exitTime = trade.exitTime;
			double equityAtExit = initialCapital;
			bool foundExit = false;

			for (const auto& point : equityCurve) {
				if (point.timestamp >= exitTime) {
					equityAtExit = point.equity;
					foundExit = true;
					break;
				}
			}

			// Skip if we couldn't find exit in equity curve
			if (!foundExit && !equityCurve.empty()) {
				equityAtExit = equityCurve.back().equity;
			}

			// Calculate exit marker position
			float exitX = bounds.left + 60 + chartWidth * ((double)(exitTime - startTime) / timeRange);
			float exitY = bounds.top + 30 + chartHeight * (1.0 - (equityAtExit - minEquity) / equityRange);

			// Check if exit point is outside visible chart area
			bool exitVisible = (exitX >= bounds.left + 60 && exitX <= bounds.right - 20 &&
			                    exitY >= bounds.top + 30 && exitY <= bounds.bottom - 50);

			// Only draw exit marker if it's visible
			if (exitVisible) {
				// Draw exit marker (downward triangle for SELL) - color based on profit/loss
				bool isWin = trade.pnl > 0;
				if (isWin) {
					SetHighColor(34, 179, 34);  // Green for winning trade
				} else {
					SetHighColor(220, 53, 69);  // Red for losing trade
				}

				BPoint exitTriangle[3];
				exitTriangle[0] = BPoint(exitX, exitY + size);           // Bottom
				exitTriangle[1] = BPoint(exitX - size*0.6f, exitY - 2);  // Top left
				exitTriangle[2] = BPoint(exitX + size*0.6f, exitY - 2);  // Top right
				FillPolygon(exitTriangle, 3);

				// Draw border (thicker if selected)
				SetPenSize(isSelected ? 2.5f : 1.0f);
				if (isSelected) {
					SetHighColor(255, 200, 0);  // Orange/yellow for highlight
				} else {
					SetHighColor(isWin ? 20 : 150, isWin ? 120 : 30, isWin ? 20 : 40);
				}
				StrokePolygon(exitTriangle, 3);
				SetPenSize(1.0f);
			}

			// Draw connecting line and highlight circle if selected (only if both points are visible)
			if (isSelected && exitVisible) {
				// Draw thick connecting line
				SetHighColor(255, 200, 0, 200);  // Bright orange
				SetPenSize(3.0f);
				StrokeLine(BPoint(x, y), BPoint(exitX, exitY));
				SetPenSize(1.0f);

				// Draw highlight circles around markers
				SetHighColor(255, 200, 0, 100);  // Semi-transparent orange
				SetPenSize(2.5f);
				StrokeEllipse(BPoint(x, y), 15, 15);
				StrokeEllipse(BPoint(exitX, exitY), 15, 15);
				SetPenSize(1.0f);

				// Draw info label
				SetHighColor(255, 255, 255, 230);
				BRect labelRect(exitX + 20, exitY - 30, exitX + 150, exitY - 5);
				FillRect(labelRect);
				SetHighColor(255, 200, 0);
				StrokeRect(labelRect);

				// Draw P&L text
				SetHighColor(trade.pnl >= 0 ? 34 : 220, trade.pnl >= 0 ? 139 : 38, trade.pnl >= 0 ? 34 : 38);
				BFont smallFont(*be_plain_font);
				smallFont.SetSize(10.0);
				SetFont(&smallFont);

				std::ostringstream labelText;
				labelText << "P&L: " << (trade.pnl >= 0 ? "+" : "") << "$" << std::fixed << std::setprecision(2) << trade.pnl;
				DrawString(labelText.str().c_str(), BPoint(labelRect.left + 5, labelRect.bottom - 5));
				SetFont(be_plain_font);
			}
		}
	}
}

void EquityChartView::DrawAxes(BRect bounds) {
	SetHighColor(70, 70, 80);  // Dark gray for text
	font_height fh;
	GetFontHeight(&fh);

	// Y-axis (equity values)
	int numLabels = 6;
	for (int i = 0; i <= numLabels; i++) {
		double equity = minEquity + (maxEquity - minEquity) * i / numLabels;
		float y = bounds.bottom - 40 - ((bounds.Height() - 70) * i / numLabels);

		// Format equity value
		std::ostringstream ss;
		if (equity >= 1000) {
			ss << std::fixed << std::setprecision(1) << "$" << (equity / 1000.0) << "k";
		} else {
			ss << std::fixed << std::setprecision(0) << "$" << equity;
		}
		std::string label = ss.str();

		// Draw label
		DrawString(label.c_str(), BPoint(bounds.left + 8, y + fh.ascent / 2));
	}

	// X-axis (time labels)
	SetHighColor(90, 90, 100);
	int numTimeLabels = 6;
	for (int i = 0; i <= numTimeLabels; i++) {
		time_t timestamp = startTime + (endTime - startTime) * i / numTimeLabels;
		float x = bounds.left + 60 + (bounds.Width() - 75) * i / numTimeLabels;

		// Format time
		char timeStr[64];
		struct tm* timeinfo = localtime(&timestamp);
		strftime(timeStr, sizeof(timeStr), "%b %d", timeinfo);

		// Draw label
		float labelWidth = StringWidth(timeStr);
		DrawString(timeStr, BPoint(x - labelWidth / 2, bounds.bottom - 22));
	}

	// Title with modern styling
	SetFont(be_bold_font);
	SetHighColor(50, 50, 60);
	const char* title = "PORTFOLIO EQUITY";
	DrawString(title, BPoint(bounds.left + 15, bounds.top + 18));
	SetFont(be_plain_font);
}

void EquityChartView::DrawLegend(BRect bounds) {
	if (equityCurve.empty()) return;

	// Modern card-style legend
	SetHighColor(255, 255, 255, 230);  // White with slight transparency
	BRect legendRect(bounds.right - 220, bounds.top + 30, bounds.right - 15, bounds.top + 110);
	FillRect(legendRect);

	// Subtle shadow effect
	SetHighColor(200, 200, 205);
	StrokeRect(legendRect);

	font_height fh;
	GetFontHeight(&fh);
	float lineHeight = fh.ascent + fh.descent + fh.leading + 4;

	double finalEquity = equityCurve.back().equity;
	double returnPct = ((finalEquity - initialCapital) / initialCapital) * 100.0;
	bool isProfit = returnPct >= 0;

	// Initial capital label
	SetHighColor(100, 100, 110);
	BFont smallFont(*be_plain_font);
	smallFont.SetSize(9.0);
	SetFont(&smallFont);
	DrawString("INITIAL CAPITAL", BPoint(legendRect.left + 10, legendRect.top + 14));

	// Initial capital value
	SetFont(be_plain_font);
	SetHighColor(50, 50, 60);
	std::ostringstream ss1;
	ss1 << "$" << std::fixed << std::setprecision(2) << initialCapital;
	DrawString(ss1.str().c_str(), BPoint(legendRect.left + 10, legendRect.top + 14 + lineHeight));

	// Final equity label
	SetFont(&smallFont);
	SetHighColor(100, 100, 110);
	DrawString("FINAL EQUITY", BPoint(legendRect.left + 10, legendRect.top + 14 + lineHeight * 2 + 3));

	// Final equity value
	SetFont(be_plain_font);
	SetHighColor(50, 50, 60);
	std::ostringstream ss2;
	ss2 << "$" << std::fixed << std::setprecision(2) << finalEquity;
	DrawString(ss2.str().c_str(), BPoint(legendRect.left + 10, legendRect.top + 14 + lineHeight * 3 + 3));

	// Return percentage - highlighted
	BFont boldFont(*be_bold_font);
	boldFont.SetSize(11.0);
	SetFont(&boldFont);

	std::ostringstream ss3;
	ss3 << (isProfit ? "+" : "") << std::fixed << std::setprecision(2) << returnPct << "%";

	SetHighColor(isProfit ? 34 : 220, isProfit ? 139 : 38, isProfit ? 34 : 38);
	DrawString(ss3.str().c_str(), BPoint(legendRect.right - 75, legendRect.top + 14 + lineHeight * 3 + 3));

	SetFont(be_plain_font);
}

void EquityChartView::MouseDown(BPoint where) {
	isDragging = true;
	lastMousePos = where;
}

void EquityChartView::MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage) {
	currentMousePos = where;

	if (isDragging) {
		// Future: implement panning
		lastMousePos = where;
	}
}

void EquityChartView::MouseUp(BPoint where) {
	isDragging = false;
}

void EquityChartView::FrameResized(float newWidth, float newHeight) {
	BView::FrameResized(newWidth, newHeight);
	Invalidate();
}

} // namespace UI
} // namespace Emiglio
