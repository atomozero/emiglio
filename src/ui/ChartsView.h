#ifndef EMIGLIO_CHARTSVIEW_H
#define EMIGLIO_CHARTSVIEW_H

#include <View.h>
#include <StringView.h>
#include <Button.h>
#include <MenuField.h>
#include <TextControl.h>
#include <StatusBar.h>
#include <Messenger.h>
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

// Main charts view with controls
class ChartsView : public BView {
public:
	ChartsView();
	virtual ~ChartsView();

	virtual void AttachedToWindow() override;
	virtual void MessageReceived(BMessage* message) override;

	// Public method to load data
	void LoadChartData(const std::string& exchange, const std::string& symbol,
	                   const std::string& timeframe);

private:
	void BuildLayout();
	void LoadData();
	void UpdateChart();
	void CalculateIndicators();
	bool DownloadMissingData(DataStorage& storage);

	// UI Components
	CandlestickChartView* chartView;
	BMenuField* baseCurrencyMenu;   // First part of pair (BTC, ETH, SOL, etc.)
	BMenuField* quoteCurrencyMenu;  // Second part of pair (USDT, EUR, BTC, etc.)
	BMenuField* timeframeMenu;
	BButton* loadButton;
	BButton* zoomInButton;
	BButton* zoomOutButton;
	BButton* resetButton;
	BStringView* statusLabel;
	BStringView* statsLabel;
	BStatusBar* downloadProgress;

	// Indicator toggles
	BButton* emaButton;
	BButton* bollingerButton;
	BButton* rsiButton;

	// Current data
	std::string currentSymbol;
	std::string currentTimeframe;
	std::string currentExchange;

	// Download state
	thread_id downloadThread;
	bool isDownloading;

	// Thread data structure
	struct DownloadThreadData {
		ChartsView* view;
		std::string symbol;
		std::string timeframe;
		std::string exchange;
		BMessenger* messenger;
	};

	// Thread functions
	static int32 DownloadThreadFunc(void* data);
	void StartAsyncDownload();

	enum {
		MSG_LOAD_DATA = 'load',
		MSG_ZOOM_IN = 'zmin',
		MSG_ZOOM_OUT = 'zmot',
		MSG_RESET_VIEW = 'rset',
		MSG_TOGGLE_EMA = 'tema',
		MSG_TOGGLE_BOLLINGER = 'tbol',
		MSG_TOGGLE_RSI = 'trsi',
		MSG_BASE_CURRENCY_CHANGED = 'bsch',
		MSG_QUOTE_CURRENCY_CHANGED = 'qtch',
		MSG_TIMEFRAME_CHANGED = 'tfch',
		MSG_DOWNLOAD_PROGRESS = 'dpro',
		MSG_DOWNLOAD_COMPLETE = 'dcmp',
		MSG_DOWNLOAD_FAILED = 'dfai'
	};
};

} // namespace UI
} // namespace Emiglio

#endif // EMIGLIO_CHARTSVIEW_H
