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
	, minPrice(0.0)
	, maxPrice(100.0)
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

void EquityChartView::SetPriceData(const std::vector<Backtest::EquityPoint>& pricePoints) {
	priceData = pricePoints;
	CalculateScale();
	Invalidate();
}

void EquityChartView::Clear() {
	equityCurve.clear();
	priceData.clear();
	trades.clear();
	initialCapital = 0.0;
	minEquity = 0.0;
	maxEquity = 100.0;
	minPrice = 0.0;
	maxPrice = 100.0;
	startTime = 0;
	endTime = 0;
	selectedTradeIndex = -1;
	Invalidate();
}

void EquityChartView::CalculateScale() {
	if (equityCurve.empty()) {
		minEquity = 0;
		maxEquity = 100;
		minPrice = 0;
		maxPrice = 100;
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

	// Calculate price scale
	if (!priceData.empty()) {
		minPrice = priceData[0].equity;  // Using equity field to store price
		maxPrice = priceData[0].equity;

		for (const auto& point : priceData) {
			if (point.equity < minPrice) minPrice = point.equity;
			if (point.equity > maxPrice) maxPrice = point.equity;
		}

		// Add 5% padding to price
		double pricePadding = (maxPrice - minPrice) * 0.05;
		if (pricePadding < 0.01) pricePadding = maxPrice * 0.05;
		minPrice -= pricePadding;
		maxPrice += pricePadding;
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

	// Draw area fill FIRST (background layer)
	if (equityCurve.size() > 1) {
		float chartHeight = bounds.Height() - 70;
		float chartWidth = bounds.Width() - 75;
		double equityRange = maxEquity - minEquity;
		time_t timeRange = endTime - startTime;

		if (equityRange > 0 && timeRange > 0) {
			SetHighColor(200, 220, 240, 80);  // Light blue semi-transparent

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
	}

	DrawPriceLine(bounds);      // Draw price line on top of area fill
	DrawEquityLine(bounds);     // Draw equity line on top of price
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

	// Draw equity line in blue (neutral color to avoid confusion with markers)
	SetHighColor(65, 105, 225);  // Royal Blue
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

void EquityChartView::DrawPriceLine(BRect bounds) {
	if (priceData.size() < 2) return;

	float chartHeight = bounds.Height() - 70;  // Leave space for axes
	float chartWidth = bounds.Width() - 75;    // Leave space for price axis
	double priceRange = maxPrice - minPrice;
	time_t timeRange = endTime - startTime;

	if (priceRange <= 0 || timeRange <= 0) return;

	// Draw price line in orange/amber (distinct from blue equity line)
	SetHighColor(255, 140, 0);  // Dark orange
	SetPenSize(1.5);

	BPoint prevPoint;
	bool firstPoint = true;

	for (size_t i = 0; i < priceData.size(); i++) {
		const auto& point = priceData[i];

		// Calculate position (using price scale instead of equity scale)
		float x = bounds.left + 60 + chartWidth * ((double)(point.timestamp - startTime) / timeRange);
		float y = bounds.top + 30 + chartHeight * (1.0 - (point.equity - minPrice) / priceRange);

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

		// Determine if trade is winning or losing
		bool isWin = trade.pnl > 0;

		// Draw entry marker - different shapes for wins/losses
		float size = isSelected ? 10.0f : 8.0f;  // Bigger if selected
		BPoint entryTriangle[3];

		if (isWin) {
			// Winning trade: Green upward triangle ▲
			SetHighColor(34, 179, 34);  // Bright green
			entryTriangle[0] = BPoint(x, y - size);           // Top
			entryTriangle[1] = BPoint(x - size*0.6f, y + 2);  // Bottom left
			entryTriangle[2] = BPoint(x + size*0.6f, y + 2);  // Bottom right
		} else {
			// Losing trade: Red downward triangle ▼
			SetHighColor(220, 53, 69);  // Red
			entryTriangle[0] = BPoint(x, y + size);           // Bottom
			entryTriangle[1] = BPoint(x - size*0.6f, y - 2);  // Top left
			entryTriangle[2] = BPoint(x + size*0.6f, y - 2);  // Top right
		}
		FillPolygon(entryTriangle, 3);

		// Draw border (thicker if selected)
		SetPenSize(isSelected ? 2.5f : 1.0f);
		if (isSelected) {
			SetHighColor(255, 200, 0);  // Orange for selected
		} else {
			SetHighColor(isWin ? 20 : 150, isWin ? 120 : 30, isWin ? 20 : 40);
		}
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

			// Draw connecting line and highlight if selected (only if exit is visible)
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
			}

			// Draw detailed tooltip for selected trade (always, even if exit is not visible)
			if (isSelected) {
				BFont smallFont(*be_plain_font);
				smallFont.SetSize(9.5);
				SetFont(&smallFont);

				font_height fh;
				GetFontHeight(&fh);
				float lineHeight = fh.ascent + fh.descent + fh.leading + 2;

				// Prepare tooltip text lines
				std::vector<std::string> tooltipLines;
				std::ostringstream oss;

				// Trade ID
				oss << "Trade #" << (selectedTradeIndex + 1);
				tooltipLines.push_back(oss.str());
				oss.str("");

				// Entry price and time
				oss << "Entry: $" << std::fixed << std::setprecision(2) << trade.entryPrice;
				tooltipLines.push_back(oss.str());
				oss.str("");

				// Exit price and time
				oss << "Exit: $" << std::fixed << std::setprecision(2) << trade.exitPrice;
				tooltipLines.push_back(oss.str());
				oss.str("");

				// Quantity
				oss << "Qty: " << std::fixed << std::setprecision(6) << trade.quantity;
				tooltipLines.push_back(oss.str());
				oss.str("");

				// P&L in dollars
				oss << "P&L: " << (trade.pnl >= 0 ? "+" : "") << "$" << std::fixed << std::setprecision(2) << trade.pnl;
				tooltipLines.push_back(oss.str());
				oss.str("");

				// P&L percentage
				double pnlPercent = (trade.pnl / (trade.entryPrice * trade.quantity)) * 100.0;
				oss << "Return: " << (pnlPercent >= 0 ? "+" : "") << std::fixed << std::setprecision(2) << pnlPercent << "%";
				tooltipLines.push_back(oss.str());
				oss.str("");

				// Exit reason
				oss << "Reason: " << trade.exitReason;
				tooltipLines.push_back(oss.str());

				// Calculate tooltip size
				float maxWidth = 0;
				for (const auto& line : tooltipLines) {
					float width = smallFont.StringWidth(line.c_str());
					if (width > maxWidth) maxWidth = width;
				}

				float tooltipWidth = maxWidth + 16;
				float tooltipHeight = tooltipLines.size() * lineHeight + 10;

				// Position tooltip - prefer near exit if visible, otherwise near entry
				float tooltipX, tooltipY;
				if (exitVisible) {
					tooltipX = exitX + 20;
					tooltipY = exitY - tooltipHeight - 10;
				} else {
					// If exit not visible, place tooltip near entry point
					tooltipX = x + 20;
					tooltipY = y - tooltipHeight - 10;
				}

				// Adjust position if tooltip goes outside bounds
				if (tooltipX + tooltipWidth > bounds.right - 10) {
					tooltipX = (exitVisible ? exitX : x) - tooltipWidth - 20;
				}
				if (tooltipY < bounds.top + 40) {
					tooltipY = (exitVisible ? exitY : y) + 20;
				}
				if (tooltipY + tooltipHeight > bounds.bottom - 50) {
					tooltipY = bounds.bottom - 50 - tooltipHeight;
				}

				// Final safety check - keep tooltip completely in bounds
				if (tooltipX < bounds.left + 10) tooltipX = bounds.left + 10;
				if (tooltipY < bounds.top + 40) tooltipY = bounds.top + 40;

				BRect labelRect(tooltipX, tooltipY, tooltipX + tooltipWidth, tooltipY + tooltipHeight);

				// Draw tooltip background with shadow
				SetHighColor(40, 40, 45, 180);
				FillRect(labelRect.OffsetByCopy(2, 2));

				// Draw tooltip background
				SetHighColor(255, 255, 255, 245);
				FillRect(labelRect);

				// Draw border
				SetHighColor(255, 200, 0);
				SetPenSize(2.0f);
				StrokeRect(labelRect);
				SetPenSize(1.0f);

				// Draw text lines
				float textY = labelRect.top + lineHeight - 2;
				for (size_t i = 0; i < tooltipLines.size(); i++) {
					const auto& line = tooltipLines[i];

					// Use different colors for different types of info
					if (i == 0) {
						// Trade ID - orange
						SetHighColor(255, 150, 0);
					} else if (i == 4 || i == 5) {
						// P&L and Return - green/red based on value
						SetHighColor(trade.pnl >= 0 ? 34 : 220, trade.pnl >= 0 ? 139 : 38, trade.pnl >= 0 ? 34 : 38);
					} else if (i == 6) {
						// Exit reason - gray
						SetHighColor(90, 90, 100);
					} else {
						// Other info - dark gray
						SetHighColor(50, 50, 60);
					}

					DrawString(line.c_str(), BPoint(labelRect.left + 8, textY));
					textY += lineHeight;
				}

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

	// Right Y-axis (price values) - only if price data is available
	if (!priceData.empty()) {
		SetHighColor(255, 140, 0);  // Orange to match price line
		for (int i = 0; i <= numLabels; i++) {
			double price = minPrice + (maxPrice - minPrice) * i / numLabels;
			float y = bounds.bottom - 40 - ((bounds.Height() - 70) * i / numLabels);

			// Format price value
			std::ostringstream ss;
			if (price >= 1000) {
				ss << std::fixed << std::setprecision(1) << "$" << (price / 1000.0) << "k";
			} else {
				ss << std::fixed << std::setprecision(2) << "$" << price;
			}
			std::string label = ss.str();

			// Draw label on the right side
			float labelWidth = StringWidth(label.c_str());
			DrawString(label.c_str(), BPoint(bounds.right - labelWidth - 5, y + fh.ascent / 2));
		}
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

	// Modern card-style legend (taller if price data is available)
	SetHighColor(255, 255, 255, 230);  // White with slight transparency
	float legendHeight = priceData.empty() ? 110 : 140;
	BRect legendRect(bounds.right - 220, bounds.top + 30, bounds.right - 15, bounds.top + 30 + legendHeight);
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

	// Price info if available
	if (!priceData.empty()) {
		double currentPrice = priceData.back().equity;

		// Price label
		SetFont(&smallFont);
		SetHighColor(100, 100, 110);
		DrawString("CURRENT PRICE", BPoint(legendRect.left + 10, legendRect.top + 14 + lineHeight * 4 + 6));

		// Price value in orange
		SetFont(be_plain_font);
		SetHighColor(255, 140, 0);
		std::ostringstream ss4;
		ss4 << "$" << std::fixed << std::setprecision(2) << currentPrice;
		DrawString(ss4.str().c_str(), BPoint(legendRect.left + 10, legendRect.top + 14 + lineHeight * 5 + 6));
	}

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
