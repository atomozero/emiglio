#include "ChartsView.h"
#include "../strategy/Indicators.h"
#include "../utils/Logger.h"
#include "../utils/Config.h"
#include "../exchange/BinanceAPI.h"

#include <LayoutBuilder.h>
#include <GroupView.h>
#include <Button.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <algorithm>
#include <cmath>
#include <set>
#include <OS.h>

namespace Emiglio {
namespace UI {

// ============================================================================
// CandlestickChartView Implementation
// ============================================================================

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
	// float chartWidth = bounds.Width() - 80;  // Unused
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

		// float labelWidth = font.StringWidth(label);  // Unused
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
	// float volumeTop = volumeBottom - volumeHeight;  // Unused

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

// ============================================================================
// ChartsView Implementation
// ============================================================================

ChartsView::ChartsView()
	: BView("Charts", B_WILL_DRAW)
	, chartView(nullptr)
	, currentSymbol("BTC" + Config::getInstance().getPreferredQuote())
	, currentTimeframe("1h")
	, currentExchange("binance")
	, downloadThread(-1)
	, isDownloading(false)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	BuildLayout();
}

ChartsView::~ChartsView() {
}

void ChartsView::AttachedToWindow() {
	BView::AttachedToWindow();

	if (loadButton) loadButton->SetTarget(this);
	if (zoomInButton) zoomInButton->SetTarget(this);
	if (zoomOutButton) zoomOutButton->SetTarget(this);
	if (resetButton) resetButton->SetTarget(this);
	if (emaButton) emaButton->SetTarget(this);
	if (bollingerButton) bollingerButton->SetTarget(this);
	if (rsiButton) rsiButton->SetTarget(this);

	// Set target for base currency menu items
	if (baseCurrencyMenu && baseCurrencyMenu->Menu()) {
		for (int32 i = 0; i < baseCurrencyMenu->Menu()->CountItems(); i++) {
			BMenuItem* item = baseCurrencyMenu->Menu()->ItemAt(i);
			if (item) item->SetTarget(this);
		}
	}

	// Set target for quote currency menu items
	if (quoteCurrencyMenu && quoteCurrencyMenu->Menu()) {
		for (int32 i = 0; i < quoteCurrencyMenu->Menu()->CountItems(); i++) {
			BMenuItem* item = quoteCurrencyMenu->Menu()->ItemAt(i);
			if (item) item->SetTarget(this);
		}
	}

	// Set target for timeframe menu items
	if (timeframeMenu && timeframeMenu->Menu()) {
		for (int32 i = 0; i < timeframeMenu->Menu()->CountItems(); i++) {
			BMenuItem* item = timeframeMenu->Menu()->ItemAt(i);
			if (item) item->SetTarget(this);
		}
	}

	// Load initial data
	LoadData();
}

void ChartsView::BuildLayout() {
	// Create chart view
	chartView = new CandlestickChartView();
	chartView->SetExplicitMinSize(BSize(600, 400));

	// PERFORMANCE FIX: Use static list of currencies instead of downloading 16MB from Binance
	// This reduces startup time from 13 seconds to instant!
	// Previous code called api.getAllSymbols() which downloads exchangeInfo (16MB, 1592 symbols)

	// Popular base currencies (user can still type any symbol manually)
	std::set<std::string> baseCurrencies = {
		"BTC", "ETH", "BNB", "SOL", "XRP", "ADA", "DOGE", "MATIC", "DOT", "AVAX",
		"LINK", "UNI", "LTC", "BCH", "ATOM", "ETC", "XLM", "ALGO", "VET", "ICP",
		"FIL", "TRX", "EOS", "AAVE", "GRT", "SAND", "MANA", "AXS", "THETA", "FTM",
		"NEAR", "HBAR", "EGLD", "XTZ", "FLOW", "CHZ", "ENJ", "ZEC", "DASH", "COMP"
	};

	// Common quote currencies
	std::set<std::string> quoteCurrencies = {
		"USDT", "BUSD", "USDC", "EUR", "GBP", "BTC", "ETH", "BNB"
	};

	// Base currency menu - simplified for performance
	BPopUpMenu* basePopup = new BPopUpMenu("Base");

	// Add all base currencies directly (already sorted by popularity in the set)
	for (const auto& base : baseCurrencies) {
		BMessage* msg = new BMessage(MSG_BASE_CURRENCY_CHANGED);
		msg->AddString("base", base.c_str());
		basePopup->AddItem(new BMenuItem(base.c_str(), msg));
	}

	basePopup->ItemAt(0)->SetMarked(true);  // Mark first item (BTC)
	baseCurrencyMenu = new BMenuField("Base:", basePopup);

	// Quote currency menu - simplified for performance
	BPopUpMenu* quotePopup = new BPopUpMenu("Quote");

	// Get user's preferred quote from settings
	Config& config = Config::getInstance();
	std::string preferredQuote = config.getPreferredQuote();

	// Add quote currencies in preferred order: USDT, EUR, BTC, ETH, BNB, others
	const std::vector<std::string> quoteOrder = {"USDT", "EUR", "BTC", "ETH", "BNB", "BUSD", "USDC", "GBP"};

	for (const auto& quote : quoteOrder) {
		BMessage* msg = new BMessage(MSG_QUOTE_CURRENCY_CHANGED);
		msg->AddString("quote", quote.c_str());
		BMenuItem* item = new BMenuItem(quote.c_str(), msg);
		quotePopup->AddItem(item);

		// Mark user's preferred quote
		if (quote == preferredQuote) {
			item->SetMarked(true);
		}
	}

	quoteCurrencyMenu = new BMenuField("Quote:", quotePopup);

	// Timeframe menu
	BPopUpMenu* timeframePopup = new BPopUpMenu("Timeframe");
	BMessage* msg1m = new BMessage(MSG_TIMEFRAME_CHANGED);
	msg1m->AddString("timeframe", "1m");
	timeframePopup->AddItem(new BMenuItem("1m", msg1m));

	BMessage* msg5m = new BMessage(MSG_TIMEFRAME_CHANGED);
	msg5m->AddString("timeframe", "5m");
	timeframePopup->AddItem(new BMenuItem("5m", msg5m));

	BMessage* msg15m = new BMessage(MSG_TIMEFRAME_CHANGED);
	msg15m->AddString("timeframe", "15m");
	timeframePopup->AddItem(new BMenuItem("15m", msg15m));

	BMessage* msg1h = new BMessage(MSG_TIMEFRAME_CHANGED);
	msg1h->AddString("timeframe", "1h");
	timeframePopup->AddItem(new BMenuItem("1h", msg1h));

	BMessage* msg4h = new BMessage(MSG_TIMEFRAME_CHANGED);
	msg4h->AddString("timeframe", "4h");
	timeframePopup->AddItem(new BMenuItem("4h", msg4h));

	BMessage* msg1d = new BMessage(MSG_TIMEFRAME_CHANGED);
	msg1d->AddString("timeframe", "1d");
	timeframePopup->AddItem(new BMenuItem("1d", msg1d));

	timeframePopup->ItemAt(3)->SetMarked(true);
	timeframeMenu = new BMenuField("Timeframe:", timeframePopup);

	loadButton = new BButton("Load", new BMessage(MSG_LOAD_DATA));
	zoomInButton = new BButton("Zoom In", new BMessage(MSG_ZOOM_IN));
	zoomOutButton = new BButton("Zoom Out", new BMessage(MSG_ZOOM_OUT));
	resetButton = new BButton("Reset", new BMessage(MSG_RESET_VIEW));

	// Indicator toggles
	emaButton = new BButton("Show EMA(20)", new BMessage(MSG_TOGGLE_EMA));
	bollingerButton = new BButton("Show Bollinger", new BMessage(MSG_TOGGLE_BOLLINGER));
	rsiButton = new BButton("RSI (TODO)", new BMessage(MSG_TOGGLE_RSI));
	rsiButton->SetEnabled(false);

	statusLabel = new BStringView("", "Ready");
	statsLabel = new BStringView("", "");

	// Progress bar for downloads (initially hidden)
	downloadProgress = new BStatusBar("download_progress", nullptr);  // No label on top
	downloadProgress->SetMaxValue(100.0);
	downloadProgress->SetBarHeight(16);
	downloadProgress->SetExplicitMinSize(BSize(200, 20));
	downloadProgress->SetExplicitMaxSize(BSize(300, 20));
	downloadProgress->SetText("Downloading data from Binance");

	// Layout
	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_WINDOW_SPACING)
		.AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING)
			.Add(baseCurrencyMenu)
			.Add(quoteCurrencyMenu)
			.Add(timeframeMenu)
			.Add(loadButton)
			.AddGlue()
			.Add(emaButton)
			.Add(bollingerButton)
			.Add(rsiButton)
		.End()
		.Add(chartView, 10)
		.AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING)
			.Add(zoomInButton)
			.Add(zoomOutButton)
			.Add(resetButton)
			.Add(downloadProgress)
			.AddGlue()
			.Add(statsLabel)
			.Add(statusLabel)
		.End()
		.End();

	// Hide progress bar initially
	downloadProgress->Hide();
}

void ChartsView::MessageReceived(BMessage* message) {
	switch (message->what) {
		case MSG_LOAD_DATA:
			// Build symbol from base + quote
			{
				BMenuItem* baseItem = baseCurrencyMenu->Menu()->FindMarked();
				BMenuItem* quoteItem = quoteCurrencyMenu->Menu()->FindMarked();
				if (baseItem && quoteItem) {
					currentSymbol = std::string(baseItem->Label()) + std::string(quoteItem->Label());
				}
			}
			// Get selected timeframe
			{
				BMenuItem* item = timeframeMenu->Menu()->FindMarked();
				if (item) {
					currentTimeframe = item->Label();
				}
			}
			LoadData();
			break;

		case MSG_BASE_CURRENCY_CHANGED:
		case MSG_QUOTE_CURRENCY_CHANGED:
		{
			// Rebuild symbol from current selection
			BMenuItem* baseItem = baseCurrencyMenu->Menu()->FindMarked();
			BMenuItem* quoteItem = quoteCurrencyMenu->Menu()->FindMarked();
			if (baseItem && quoteItem) {
				currentSymbol = std::string(baseItem->Label()) + std::string(quoteItem->Label());
				LoadData();
				// Recalculate indicators if they were active
				CalculateIndicators();
			}
			break;
		}

		case MSG_ZOOM_IN:
			if (chartView) chartView->ZoomIn();
			break;

		case MSG_ZOOM_OUT:
			if (chartView) chartView->ZoomOut();
			break;

		case MSG_RESET_VIEW:
			if (chartView) chartView->ResetView();
			break;

		case MSG_TOGGLE_EMA:
			// Toggle button label
			if (std::string(emaButton->Label()).find("Show") != std::string::npos) {
				emaButton->SetLabel("Hide EMA(20)");
			} else {
				emaButton->SetLabel("Show EMA(20)");
			}
			CalculateIndicators();
			break;

		case MSG_TOGGLE_BOLLINGER:
			// Toggle button label
			if (std::string(bollingerButton->Label()).find("Show") != std::string::npos) {
				bollingerButton->SetLabel("Hide Bollinger");
			} else {
				bollingerButton->SetLabel("Show Bollinger");
			}
			CalculateIndicators();
			break;

		case MSG_TIMEFRAME_CHANGED:
		{
			const char* timeframe;
			if (message->FindString("timeframe", &timeframe) == B_OK) {
				currentTimeframe = timeframe;
				LoadData();
				// Recalculate indicators if they were active
				CalculateIndicators();
			}
			break;
		}

		case MSG_DOWNLOAD_PROGRESS:
		{
			int32 current, total;
			if (message->FindInt32("current", &current) == B_OK &&
			    message->FindInt32("total", &total) == B_OK) {
				if (downloadProgress && total > 0) {
					float progress = (static_cast<float>(current) / total) * 100.0f;
					downloadProgress->Update(progress - downloadProgress->CurrentValue());
				}
				char status[128];
				snprintf(status, sizeof(status), "Downloaded %d candles", current);
				statusLabel->SetText(status);
			}
			break;
		}

		case MSG_DOWNLOAD_COMPLETE:
		{
			isDownloading = false;
			if (downloadProgress) {
				downloadProgress->Hide();
			}
			// Reload data now that download is complete
			LoadData();
			break;
		}

		case MSG_DOWNLOAD_FAILED:
		{
			isDownloading = false;
			if (downloadProgress) {
				downloadProgress->Hide();
			}
			statusLabel->SetText("Download failed");
			break;
		}

		default:
			BView::MessageReceived(message);
			break;
	}
}

bool ChartsView::DownloadMissingData(DataStorage& storage) {
	LOG_INFO("No data found, attempting to download from Binance...");
	statusLabel->SetText("Downloading data...");

	// Show and reset progress bar
	if (downloadProgress) {
		downloadProgress->Reset();
		downloadProgress->Show();
		downloadProgress->Window()->UpdateIfNeeded();  // Force UI update
	}

	// Initialize BinanceAPI
	BinanceAPI api;
	if (!api.init("", "")) {
		LOG_ERROR("Failed to initialize Binance API");
		statusLabel->SetText("Failed to connect to Binance");
		if (downloadProgress) downloadProgress->Hide();
		return false;
	}

	// Test connection
	if (!api.ping()) {
		LOG_ERROR("Failed to connect to Binance API");
		statusLabel->SetText("Cannot reach Binance API");
		if (downloadProgress) downloadProgress->Hide();
		return false;
	}

	// Calculate time range (last 30 days by default)
	time_t endTime = std::time(nullptr);
	time_t startTime = endTime - (30 * 24 * 3600);

	// Calculate interval duration in seconds
	int intervalSeconds = 3600; // Default 1h
	if (currentTimeframe == "1m") intervalSeconds = 60;
	else if (currentTimeframe == "5m") intervalSeconds = 300;
	else if (currentTimeframe == "15m") intervalSeconds = 900;
	else if (currentTimeframe == "1h") intervalSeconds = 3600;
	else if (currentTimeframe == "4h") intervalSeconds = 14400;
	else if (currentTimeframe == "1d") intervalSeconds = 86400;

	// Calculate estimated number of candles for progress tracking
	time_t timeRange = endTime - startTime;
	int estimatedCandles = timeRange / intervalSeconds;

	// Download in batches
	const int LIMIT = 1000;
	time_t currentStart = startTime;
	int totalCandles = 0;

	while (currentStart < endTime) {
		// Calculate batch end time
		time_t batchEnd = currentStart + (LIMIT * intervalSeconds);
		if (batchEnd > endTime) batchEnd = endTime;

		// Fetch data
		std::vector<Candle> candles = api.getCandles(currentSymbol, currentTimeframe,
		                                               currentStart, batchEnd, LIMIT);

		if (candles.empty()) {
			LOG_WARNING("No more data available from Binance");
			break;
		}

		// Set exchange and timeframe
		for (auto& candle : candles) {
			candle.exchange = currentExchange;
			candle.timeframe = currentTimeframe;
		}

		// Insert into database
		if (!storage.insertCandles(candles)) {
			LOG_ERROR("Failed to insert candles into database");
			statusLabel->SetText("Failed to save data");
			if (downloadProgress) downloadProgress->Hide();
			return false;
		}

		totalCandles += candles.size();

		// Update progress bar
		if (downloadProgress && estimatedCandles > 0) {
			float progress = (static_cast<float>(totalCandles) / estimatedCandles) * 100.0f;
			if (progress > 100.0f) progress = 100.0f;
			downloadProgress->Update(progress - downloadProgress->CurrentValue());
			downloadProgress->Window()->UpdateIfNeeded();  // Force UI update
		}

		// Update status
		char status[128];
		snprintf(status, sizeof(status), "Downloaded %d candles", totalCandles);
		statusLabel->SetText(status);
		statusLabel->Window()->UpdateIfNeeded();  // Force UI update

		// Update start time for next batch
		currentStart = candles.back().timestamp + intervalSeconds;

		// Check if we got less than requested (end of available data)
		if (candles.size() < static_cast<size_t>(LIMIT)) {
			LOG_INFO("Reached end of available data");
			break;
		}

		// Rate limiting - sleep 100ms between requests
		snooze(100000);
	}

	// Hide progress bar when done
	if (downloadProgress) {
		downloadProgress->Hide();
	}

	LOG_INFO("Downloaded total of " + std::to_string(totalCandles) + " candles");
	return totalCandles > 0;
}

void ChartsView::LoadData() {
	statusLabel->SetText("Loading data...");

	try {
		DataStorage storage;
		if (!storage.init("/boot/home/Emiglio/data/emilio.db")) {
			statusLabel->SetText("Failed to open database");
			return;
		}

		// Load candles
		time_t startTime = 0;
		time_t endTime = std::time(nullptr);

		std::vector<Candle> candles = storage.getCandles(
			currentExchange,
			currentSymbol,
			currentTimeframe,
			startTime,
			endTime
		);

		// If no data found, start async download
		if (candles.empty()) {
			if (!isDownloading) {
				StartAsyncDownload();
			}
			statusLabel->SetText("Downloading...");
			statsLabel->SetText("");
			return;
		}

		// Set data to chart
		chartView->SetCandles(candles);

		// Update stats
		char stats[128];
		snprintf(stats, sizeof(stats), "%zu candles loaded", candles.size());
		statsLabel->SetText(stats);
		statusLabel->SetText("Ready");

		LOG_INFO("Loaded " + std::to_string(candles.size()) + " candles for chart");

	} catch (const std::exception& e) {
		statusLabel->SetText("Error loading data");
		LOG_ERROR("Failed to load chart data: " + std::string(e.what()));
	}
}

void ChartsView::LoadChartData(const std::string& exchange, const std::string& symbol,
                                const std::string& timeframe) {
	currentExchange = exchange;
	currentSymbol = symbol;
	currentTimeframe = timeframe;

	// Parse symbol into base and quote (e.g., "BTCUSDT" -> "BTC" + "USDT")
	const std::vector<std::string> quotes = {"USDT", "BUSD", "EUR", "BTC", "ETH", "BNB"};
	for (const auto& quote : quotes) {
		if (symbol.length() > quote.length() &&
		    symbol.substr(symbol.length() - quote.length()) == quote) {
			std::string base = symbol.substr(0, symbol.length() - quote.length());

			// Set base currency menu
			BMenuItem* baseItem = baseCurrencyMenu->Menu()->FindItem(base.c_str());
			if (baseItem) {
				baseItem->SetMarked(true);
			}

			// Set quote currency menu
			BMenuItem* quoteItem = quoteCurrencyMenu->Menu()->FindItem(quote.c_str());
			if (quoteItem) {
				quoteItem->SetMarked(true);
			}
			break;
		}
	}

	// Set timeframe menu
	BMenuItem* timeframeItem = timeframeMenu->Menu()->FindItem(timeframe.c_str());
	if (timeframeItem) {
		timeframeItem->SetMarked(true);
	}

	LoadData();
}

void ChartsView::UpdateChart() {
	if (chartView) {
		chartView->Invalidate();
	}
}

void ChartsView::CalculateIndicators() {
	// Get current candles from chart
	// For now, reload from storage
	try {
		DataStorage storage;
		if (!storage.init("/boot/home/Emiglio/data/emilio.db")) {
			return;
		}

		time_t startTime = 0;
		time_t endTime = std::time(nullptr);

		std::vector<Candle> candles = storage.getCandles(
			currentExchange,
			currentSymbol,
			currentTimeframe,
			startTime,
			endTime
		);

		if (candles.empty()) return;

		// Clear existing indicators
		chartView->ClearIndicators();

		// Calculate EMA if button says "Hide"
		if (std::string(emaButton->Label()).find("Hide") != std::string::npos) {
			std::vector<double> closes = Indicators::getClosePrices(candles);
			std::vector<double> ema = Indicators::ema(closes, 20);
			chartView->SetIndicatorData("EMA(20)", ema);
		}

		// Calculate Bollinger Bands if button says "Hide"
		if (std::string(bollingerButton->Label()).find("Hide") != std::string::npos) {
			std::vector<double> closes = Indicators::getClosePrices(candles);
			auto bands = Indicators::bollingerBands(closes, 20, 2.0);
			chartView->SetIndicatorData("Bollinger_Upper", bands.upper);
			chartView->SetIndicatorData("Bollinger_Lower", bands.lower);
		}

		chartView->Invalidate();

	} catch (const std::exception& e) {
		LOG_ERROR("Failed to calculate indicators: " + std::string(e.what()));
	}
}

void ChartsView::StartAsyncDownload() {
	if (isDownloading) return;

	isDownloading = true;

	// Show progress bar
	if (downloadProgress) {
		downloadProgress->Reset();
		downloadProgress->Show();
	}

	// Create thread data
	DownloadThreadData* data = new DownloadThreadData();
	data->view = this;
	data->symbol = currentSymbol;
	data->timeframe = currentTimeframe;
	data->exchange = currentExchange;
	data->messenger = new BMessenger(this);

	// Start download thread
	downloadThread = spawn_thread(DownloadThreadFunc, "download_thread",
	                                B_NORMAL_PRIORITY, data);
	if (downloadThread >= 0) {
		resume_thread(downloadThread);
	} else {
		delete data->messenger;
		delete data;
		isDownloading = false;
		if (downloadProgress) downloadProgress->Hide();
		statusLabel->SetText("Failed to start download");
	}
}

int32 ChartsView::DownloadThreadFunc(void* userData) {
	DownloadThreadData* data = static_cast<DownloadThreadData*>(userData);
	BMessenger* messenger = data->messenger;

	// Initialize DataStorage
	DataStorage storage;
	if (!storage.init("/boot/home/Emiglio/data/emilio.db")) {
		BMessage failMsg(MSG_DOWNLOAD_FAILED);
		messenger->SendMessage(&failMsg);
		delete messenger;
		delete data;
		return 1;
	}

	// Initialize BinanceAPI
	BinanceAPI api;
	if (!api.init("", "")) {
		BMessage failMsg(MSG_DOWNLOAD_FAILED);
		messenger->SendMessage(&failMsg);
		delete messenger;
		delete data;
		return 1;
	}

	// Test connection
	if (!api.ping()) {
		BMessage failMsg(MSG_DOWNLOAD_FAILED);
		messenger->SendMessage(&failMsg);
		delete messenger;
		delete data;
		return 1;
	}

	// Calculate time range
	time_t endTime = std::time(nullptr);
	time_t startTime;

	// Check if we already have data - if so, download from last timestamp
	std::vector<Candle> existingData = storage.getCandles(
		data->exchange,
		data->symbol,
		data->timeframe,
		0,
		endTime
	);

	if (!existingData.empty()) {
		// Calculate gap between last data and now
		time_t lastTimestamp = existingData.back().timestamp;
		time_t gap = endTime - lastTimestamp;
		time_t thirtyDays = 30 * 24 * 3600;

		if (gap > thirtyDays) {
			// Gap too large (> 30 days), download fresh data
			startTime = endTime - thirtyDays;
			LOG_INFO("Data too old (gap > 30 days), downloading last 30 days");
		} else {
			// Normal update - download from last timestamp
			startTime = lastTimestamp + 1;
			LOG_INFO("Found existing data, downloading from last timestamp");
		}
	} else {
		// No data, download last 30 days
		startTime = endTime - (30 * 24 * 3600);
		LOG_INFO("No existing data, downloading last 30 days");
	}

	// Calculate interval duration
	int intervalSeconds = 3600;
	if (data->timeframe == "1m") intervalSeconds = 60;
	else if (data->timeframe == "5m") intervalSeconds = 300;
	else if (data->timeframe == "15m") intervalSeconds = 900;
	else if (data->timeframe == "1h") intervalSeconds = 3600;
	else if (data->timeframe == "4h") intervalSeconds = 14400;
	else if (data->timeframe == "1d") intervalSeconds = 86400;

	// Calculate estimated candles
	time_t timeRange = endTime - startTime;
	int estimatedCandles = timeRange / intervalSeconds;

	// Download in batches
	const int LIMIT = 1000;
	time_t currentStart = startTime;
	int totalCandles = 0;

	while (currentStart < endTime) {
		time_t batchEnd = currentStart + (LIMIT * intervalSeconds);
		if (batchEnd > endTime) batchEnd = endTime;

		// Fetch data
		std::vector<Candle> candles = api.getCandles(data->symbol, data->timeframe,
		                                               currentStart, batchEnd, LIMIT);

		if (candles.empty()) {
			break;
		}

		// Set exchange and timeframe
		for (auto& candle : candles) {
			candle.exchange = data->exchange;
			candle.timeframe = data->timeframe;
		}

		// Insert into database
		if (!storage.insertCandles(candles)) {
			BMessage failMsg(MSG_DOWNLOAD_FAILED);
			messenger->SendMessage(&failMsg);
			delete messenger;
			delete data;
			return 1;
		}

		totalCandles += candles.size();

		// Send progress update
		BMessage progressMsg(MSG_DOWNLOAD_PROGRESS);
		progressMsg.AddInt32("current", totalCandles);
		progressMsg.AddInt32("total", estimatedCandles);
		messenger->SendMessage(&progressMsg);

		// Update start time
		currentStart = candles.back().timestamp + intervalSeconds;

		// Check if we got less than requested
		if (candles.size() < static_cast<size_t>(LIMIT)) {
			break;
		}

		// Rate limiting
		snooze(100000);
	}

	// Send completion message
	BMessage completeMsg(MSG_DOWNLOAD_COMPLETE);
	messenger->SendMessage(&completeMsg);

	delete messenger;
	delete data;
	return 0;
}

} // namespace UI
} // namespace Emiglio
