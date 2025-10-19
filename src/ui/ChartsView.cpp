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
	rsiButton = new BButton("Show RSI", new BMessage(MSG_TOGGLE_RSI));
	rsiButton->SetEnabled(false); // Not yet implemented

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
