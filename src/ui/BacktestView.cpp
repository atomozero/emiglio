#include "BacktestView.h"
#include "MainWindow.h"
#include "../utils/Logger.h"
#include "../exchange/BinanceAPI.h"

#include <LayoutBuilder.h>
#include <Box.h>
#include <Alert.h>
#include <StringItem.h>
#include <Path.h>
#include <Directory.h>
#include <Entry.h>
#include <Messenger.h>

#include <sstream>
#include <iomanip>
#include <ctime>

namespace Emiglio {
namespace UI {

// TradesColumnListView implementation
TradesColumnListView::TradesColumnListView(const char* name, uint32 flags,
                                           border_style border,
                                           bool showHorizontalScrollbar)
	: BColumnListView(name, flags, border, showHorizontalScrollbar)
{
}

TradesColumnListView::~TradesColumnListView() {
}

void TradesColumnListView::SelectionChanged() {
	BColumnListView::SelectionChanged();

	// Send selection message whenever selection changes
	BMessage msg(MSG_TRADE_SELECTED);
	BMessenger messenger(Target());
	messenger.SendMessage(&msg);
}

// DateButton implementation
DateButton::DateButton(const char* name, const char* date)
	: BButton(name, date, new BMessage(name[0] == 's' ? MSG_START_DATE_CLICKED : MSG_END_DATE_CLICKED))
	, dateString(date)
{
}

DateButton::~DateButton() {
}

void DateButton::OpenDatePicker() {
	// Calculate position for popup (below the button)
	BPoint screenPos = ConvertToScreen(BPoint(0, Bounds().Height()));

	// Create and show date picker window
	DatePickerWindow* picker = new DatePickerWindow(screenPos, this);
	picker->Show();
}

void DateButton::SetDate(const char* date) {
	dateString = date;
	SetLabel(date);
}

BacktestView::BacktestView()
	: BView("Backtest", B_WILL_DRAW)
	, recipeField(nullptr)
	, recipeMenu(nullptr)
	, baseAssetField(nullptr)
	, baseAssetMenu(nullptr)
	, quoteAssetField(nullptr)
	, quoteAssetMenu(nullptr)
	, initialCapitalControl(nullptr)
	, commissionControl(nullptr)
	, slippageControl(nullptr)
	, runButton(nullptr)
	, exportButton(nullptr)
	, resultsPanel(nullptr)
	, statusLabel(nullptr)
	, tradesLabel(nullptr)
	, winRateLabel(nullptr)
	, returnLabel(nullptr)
	, sharpeLabel(nullptr)
	, drawdownLabel(nullptr)
	, tradesList(nullptr)
	, progressBar(nullptr)
	, equityChartView(nullptr)
	, selectedRecipePath("")
	, backtestRunning(false)
	, selectedTradeIndex(-1)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetupUI();
}

BacktestView::~BacktestView() {
}

void BacktestView::AttachedToWindow() {
	BView::AttachedToWindow();
	UpdateRecipeList();

	// Set message targets
	runButton->SetTarget(this);
	exportButton->SetTarget(this);
	startDateButton->SetTarget(this);
	endDateButton->SetTarget(this);

	// Debug: verify tradesList target
	if (tradesList) {
		tradesList->SetTarget(this);
		LOG_INFO("TradesList target set to BacktestView");
	}
}

void BacktestView::SetupUI() {
	// Modern title with better styling
	BStringView* titleView = new BStringView("title", "Strategy Backtest");
	BFont titleFont(*be_bold_font);
	titleFont.SetSize(15.0);
	titleView->SetFont(&titleFont);
	titleView->SetHighColor(40, 40, 50);

	// Setup panels
	SetupConfigPanel();
	SetupResultsPanel();

	// Create equity chart view
	equityChartView = new EquityChartView();
	equityChartView->SetExplicitMinSize(BSize(B_SIZE_UNSET, 350));

	// Layout - Redesign with trades in sidebar
	BLayoutBuilder::Group<>(this, B_VERTICAL, 10)
		.SetInsets(B_USE_WINDOW_SPACING)
		.Add(titleView)
		.AddGroup(B_HORIZONTAL, 12)
			// Left: Configuration panel + Trades list (22%)
			.AddGroup(B_VERTICAL, 8, 0.22f)
				// Configuration section
				.AddGroup(B_VERTICAL, 5)
					.Add(recipeField)
					.AddGroup(B_HORIZONTAL, 5)
						.Add(baseAssetField)
						.Add(quoteAssetField)
						.End()
					.Add(periodField)
					.AddGroup(B_HORIZONTAL, 5)
						.AddGroup(B_VERTICAL, 2)
							.Add(new BStringView("startLabel", "Start:"))
							.Add(startDateButton)
							.End()
						.AddGroup(B_VERTICAL, 2)
							.Add(new BStringView("endLabel", "End:"))
							.Add(endDateButton)
							.End()
						.End()
					.Add(initialCapitalControl)
					.Add(commissionControl)
					.Add(slippageControl)
					.End()
				.AddStrut(10)
				.Add(runButton)
				.Add(progressBar)
				.AddStrut(15)
				// Trades list section
				.Add(new BStringView("tradesTitle", "Recent Trades"))
				.Add(tradesList, 2.0f)
				.End()
			// Right: Main content area (78%)
			.AddGroup(B_VERTICAL, 8, 0.78f)
				// Equity chart (dominant element)
				.Add(equityChartView, 5.0f)
				// Status bar
				.Add(statusLabel)
				// Metrics panel (compact, bottom)
				.Add(resultsPanel)
				.End()
			.End()
		.End();
}

void BacktestView::SetupConfigPanel() {
	// Recipe selector
	recipeMenu = new BPopUpMenu("Select Recipe");
	recipeField = new BMenuField("recipe", "Recipe:", recipeMenu);

	// Base asset selector (most popular cryptocurrencies)
	baseAssetMenu = new BPopUpMenu("Select Base");
	const char* baseAssets[] = {
		"BTC", "ETH", "BNB", "ADA", "XRP", "SOL", "DOT", "DOGE",
		"AVAX", "MATIC", "LINK", "UNI", "ATOM", "LTC", "ETC", "XLM",
		"ALGO", "VET", "FIL", "TRX", "AAVE", "SAND", "MANA", "AXS"
	};
	for (const char* asset : baseAssets) {
		BMessage* msg = new BMessage(MSG_BASE_SELECTED);
		msg->AddString("asset", asset);
		BMenuItem* item = new BMenuItem(asset, msg);
		item->SetTarget(this);
		baseAssetMenu->AddItem(item);
	}
	baseAssetMenu->ItemAt(0)->SetMarked(true); // BTC default
	baseAssetField = new BMenuField("base", "Base:", baseAssetMenu);

	// Quote asset selector (stablecoins and major currencies)
	quoteAssetMenu = new BPopUpMenu("Select Quote");
	const char* quoteAssets[] = {
		"USDT", "USDC", "BUSD", "USD", "EUR", "BTC", "ETH", "BNB"
	};
	for (const char* asset : quoteAssets) {
		BMessage* msg = new BMessage(MSG_QUOTE_SELECTED);
		msg->AddString("asset", asset);
		BMenuItem* item = new BMenuItem(asset, msg);
		item->SetTarget(this);
		quoteAssetMenu->AddItem(item);
	}
	quoteAssetMenu->ItemAt(0)->SetMarked(true); // USDT default
	quoteAssetField = new BMenuField("quote", "Quote:", quoteAssetMenu);

	// Period selector (preset date ranges)
	periodMenu = new BPopUpMenu("Select Period");
	const char* periods[] = {"30 days", "90 days", "180 days", "365 days", "Custom"};
	for (int i = 0; i < 5; i++) {
		BMessage* msg = new BMessage(MSG_PERIOD_SELECTED);
		msg->AddInt32("days", i == 0 ? 30 : i == 1 ? 90 : i == 2 ? 180 : i == 3 ? 365 : 0);
		BMenuItem* item = new BMenuItem(periods[i], msg);
		item->SetTarget(this);
		periodMenu->AddItem(item);
	}
	periodMenu->ItemAt(1)->SetMarked(true); // 90 days default
	periodField = new BMenuField("period", "Period:", periodMenu);

	// Date range controls
	// Default: last 3 months ending yesterday (to ensure data availability)
	time_t now = std::time(nullptr);
	time_t yesterday = now - (24 * 60 * 60);  // Yesterday
	time_t threeMonthsAgo = yesterday - (90 * 24 * 60 * 60);  // 90 days before yesterday

	char startDateStr[32];
	char endDateStr[32];

	// Format start date
	struct tm tmStart;
	localtime_r(&threeMonthsAgo, &tmStart);
	std::strftime(startDateStr, sizeof(startDateStr), "%Y-%m-%d", &tmStart);

	// Format end date
	struct tm tmEnd;
	localtime_r(&yesterday, &tmEnd);
	std::strftime(endDateStr, sizeof(endDateStr), "%Y-%m-%d", &tmEnd);

	startDateButton = new DateButton("startdate", startDateStr);
	endDateButton = new DateButton("enddate", endDateStr);

	// Initial capital
	initialCapitalControl = new BTextControl("capital", "Initial Capital:", "10000", nullptr);

	// Commission
	commissionControl = new BTextControl("commission", "Commission (%):", "0.1", nullptr);

	// Slippage
	slippageControl = new BTextControl("slippage", "Slippage (%):", "0.05", nullptr);

	// Run button
	runButton = new BButton("run", "Run Backtest", new BMessage(MSG_BACKTEST_RUN));
	runButton->MakeDefault(true);

	// Export button
	exportButton = new BButton("export", "Export Report", new BMessage(MSG_BACKTEST_EXPORT));
	exportButton->SetEnabled(false);

	// Progress bar
	progressBar = new BStatusBar("progress", "Progress");
	progressBar->SetMaxValue(100.0);
	progressBar->Hide();
}

void BacktestView::SetupResultsPanel() {
	// Results group - just metrics now (trades moved to sidebar)
	resultsPanel = new BGroupView(B_HORIZONTAL, 12);
	resultsPanel->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	// Status
	statusLabel = new BStringView("status", "No backtest run yet");

	// Metrics - we'll create these as cards
	tradesLabel = new BStringView("trades", "Trades: -");
	winRateLabel = new BStringView("winrate", "Win Rate: -");
	returnLabel = new BStringView("return", "Return: -");
	sharpeLabel = new BStringView("sharpe", "Sharpe: -");
	drawdownLabel = new BStringView("drawdown", "Max DD: -");

	// Trades list - using custom TradesColumnListView for better display
	tradesList = new TradesColumnListView("tradeslist", 0, B_FANCY_BORDER, true);
	tradesList->SetSelectionMode(B_SINGLE_SELECTION_LIST);

	// Set target for selection messages
	tradesList->SetTarget(this);

	// Add columns (only P&L and Reason - details in tooltip)
	tradesList->AddColumn(new ColoredColumn("P&L", 90, 70, 120, 0), 0);
	tradesList->AddColumn(new ColoredColumn("Reason", 150, 100, 250, 0), 1);

	// Build results panel - horizontal metrics layout
	BLayoutBuilder::Group<>(resultsPanel, B_HORIZONTAL, 12)
		.AddGroup(B_VERTICAL, 2)
			.Add(tradesLabel)
			.Add(winRateLabel)
			.End()
		.AddGroup(B_VERTICAL, 2)
			.Add(returnLabel)
			.Add(sharpeLabel)
			.End()
		.AddGroup(B_VERTICAL, 2)
			.Add(drawdownLabel)
			.End()
		.AddGlue()
		.Add(exportButton)
		.End();
}

void BacktestView::MessageReceived(BMessage* message) {
	// Debug: log all messages
	char msgCode[5];
	msgCode[0] = (message->what >> 24) & 0xFF;
	msgCode[1] = (message->what >> 16) & 0xFF;
	msgCode[2] = (message->what >> 8) & 0xFF;
	msgCode[3] = message->what & 0xFF;
	msgCode[4] = '\0';
	LOG_INFO(std::string("BacktestView received message: ") + msgCode);

	switch (message->what) {
		case MSG_BACKTEST_RUN:
			RunBacktest();
			break;

		case MSG_BACKTEST_EXPORT:
			ExportResults();
			break;

		case MSG_RECIPE_SELECTED: {
			const char* path;
			if (message->FindString("path", &path) == B_OK) {
				selectedRecipePath = path;
				LOG_INFO("Recipe selected: " + selectedRecipePath);
			}
			break;
		}

		case MSG_TRADE_SELECTED: {
			BRow* row = tradesList->CurrentSelection();
			if (row) {
				selectedTradeIndex = tradesList->IndexOf(row);
				LOG_INFO("Trade selected: " + std::to_string(selectedTradeIndex) + " of " + std::to_string(lastResult.trades.size()));

				// Update equity chart to highlight selected trade
				equityChartView->SetSelectedTradeIndex(selectedTradeIndex);

				// Update status to show which trade is selected
				if (selectedTradeIndex < (int32)lastResult.trades.size()) {
					const auto& trade = lastResult.trades[selectedTradeIndex];
					std::ostringstream status;
					status << "Selected trade #" << (selectedTradeIndex + 1)
					       << " - P&L: $" << std::fixed << std::setprecision(2) << trade.pnl
					       << " (" << trade.exitReason << ")";
					statusLabel->SetText(status.str().c_str());
				}
			}
			break;
		}

		case MSG_PERIOD_SELECTED: {
			int32 days = 0;
			if (message->FindInt32("days", &days) == B_OK && days > 0) {
				// Calculate new date range
				time_t now = std::time(nullptr);
				time_t endDate = now - (24 * 60 * 60);  // Yesterday
				// For N days of data, we need to go back N-1 days from endDate
				// because endDate itself counts as day 1
				time_t startDate = endDate - ((days - 1) * 24 * 60 * 60);

				char startDateStr[32];
				char endDateStr[32];

				// Format start date
				struct tm tmStart;
				localtime_r(&startDate, &tmStart);
				std::strftime(startDateStr, sizeof(startDateStr), "%Y-%m-%d", &tmStart);

				// Format end date
				struct tm tmEnd;
				localtime_r(&endDate, &tmEnd);
				std::strftime(endDateStr, sizeof(endDateStr), "%Y-%m-%d", &tmEnd);

				// Update date buttons
				startDateButton->SetDate(startDateStr);
				endDateButton->SetDate(endDateStr);
			}
			// If days == 0, it means "Custom" was selected, do nothing
			break;
		}

		case MSG_START_DATE_CLICKED:
			if (startDateButton) {
				startDateButton->OpenDatePicker();
			}
			break;

		case MSG_END_DATE_CLICKED:
			if (endDateButton) {
				endDateButton->OpenDatePicker();
			}
			break;

		default:
			BView::MessageReceived(message);
			break;
	}
}

void BacktestView::UpdateRecipeList() {
	// Clear existing menu
	while (recipeMenu->CountItems() > 0) {
		delete recipeMenu->RemoveItem((int32)0);
	}

	// Find recipe files
	std::vector<std::string> recipes = FindRecipeFiles();

	if (recipes.empty()) {
		BMenuItem* item = new BMenuItem("No recipes found", nullptr);
		item->SetEnabled(false);
		recipeMenu->AddItem(item);
		return;
	}

	// Add recipes to menu
	for (const auto& recipePath : recipes) {
		// Extract filename from path
		BPath path(recipePath.c_str());
		BString label(path.Leaf());

		BMessage* msg = new BMessage(MSG_RECIPE_SELECTED);
		msg->AddString("path", recipePath.c_str());

		BMenuItem* item = new BMenuItem(label.String(), msg);
		item->SetTarget(this);
		recipeMenu->AddItem(item);
	}

	// Select first recipe
	if (recipes.size() > 0) {
		BMenuItem* first = recipeMenu->ItemAt(0);
		if (first) {
			first->SetMarked(true);
			const char* path;
			if (first->Message()->FindString("path", &path) == B_OK) {
				selectedRecipePath = path;
			}
		}
	}
}

std::vector<std::string> BacktestView::FindRecipeFiles() {
	std::vector<std::string> recipes;

	// Search in recipes directory
	BDirectory dir("/boot/home/Emiglio/recipes");
	if (dir.InitCheck() != B_OK) {
		LOG_WARNING("Recipes directory not found: /boot/home/Emiglio/recipes");
		return recipes;
	}

	BEntry entry;
	while (dir.GetNextEntry(&entry) == B_OK) {
		BPath path;
		if (entry.GetPath(&path) == B_OK) {
			BString filename(path.Leaf());
			if (filename.EndsWith(".json")) {
				recipes.push_back(path.Path());
			}
		}
	}

	LOG_INFO("Found " + std::to_string(recipes.size()) + " recipe files");
	return recipes;
}

void BacktestView::RunBacktest() {
	if (backtestRunning) {
		BAlert* alert = new BAlert("Running", "Backtest is already running!",
		                            "OK", nullptr, nullptr,
		                            B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->Go();
		return;
	}

	if (selectedRecipePath.empty()) {
		BAlert* alert = new BAlert("No Recipe", "Please select a recipe first!",
		                            "OK", nullptr, nullptr,
		                            B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->Go();
		return;
	}

	LOG_INFO("Starting backtest with recipe: " + selectedRecipePath);

	// Show progress
	progressBar->Reset();
	progressBar->Show();
	runButton->SetEnabled(false);
	backtestRunning = true;
	statusLabel->SetText("Running backtest...");

	try {
		// Load recipe
		Recipe recipe;
		RecipeLoader loader;
		if (!loader.loadFromFile(selectedRecipePath, recipe)) {
			throw std::runtime_error("Failed to load recipe: " + loader.getLastError());
		}

		LOG_INFO("Recipe loaded: " + recipe.name);

		// Get parameters from UI
		double initialCapital = std::stod(initialCapitalControl->Text());
		double commissionPercent = std::stod(commissionControl->Text()) / 100.0;
		double slippagePercent = std::stod(slippageControl->Text()) / 100.0;

		// Get base and quote from menus
		BMenuItem* baseItem = baseAssetMenu->FindMarked();
		BMenuItem* quoteItem = quoteAssetMenu->FindMarked();

		if (!baseItem || !quoteItem) {
			throw std::runtime_error("Please select both base and quote assets");
		}

		std::string base = baseItem->Label();
		std::string quote = quoteItem->Label();
		std::string symbol = base + quote;

		// Configure backtest
		Backtest::BacktestConfig config;
		config.initialCapital = initialCapital;
		config.commissionPercent = commissionPercent;
		config.slippagePercent = slippagePercent;
		config.useStopLoss = true;
		config.useTakeProfit = true;
		config.maxOpenPositions = recipe.risk.maxOpenPositions;

		// Parse date range from buttons
		const char* startDateStr = startDateButton->GetDate();
		const char* endDateStr = endDateButton->GetDate();

		struct tm tmStart = {}, tmEnd = {};
		if (sscanf(startDateStr, "%d-%d-%d", &tmStart.tm_year, &tmStart.tm_mon, &tmStart.tm_mday) != 3) {
			throw std::runtime_error("Invalid start date format. Use YYYY-MM-DD");
		}
		if (sscanf(endDateStr, "%d-%d-%d", &tmEnd.tm_year, &tmEnd.tm_mon, &tmEnd.tm_mday) != 3) {
			throw std::runtime_error("Invalid end date format. Use YYYY-MM-DD");
		}

		// Adjust tm struct (year is years since 1900, month is 0-11)
		tmStart.tm_year -= 1900;
		tmStart.tm_mon -= 1;
		tmStart.tm_hour = 0;
		tmStart.tm_min = 0;
		tmStart.tm_sec = 0;
		tmEnd.tm_year -= 1900;
		tmEnd.tm_mon -= 1;
		tmEnd.tm_hour = 23;
		tmEnd.tm_min = 59;
		tmEnd.tm_sec = 59;

		time_t startTime = std::mktime(&tmStart);
		time_t endTime = std::mktime(&tmEnd);

		if (startTime >= endTime) {
			throw std::runtime_error("Start date must be before end date");
		}

		// Load or download historical data
		DataStorage storage;
		if (!storage.init("/boot/home/Emiglio/data/emilio.db")) {
			throw std::runtime_error("Failed to initialize database");
		}

		LOG_INFO("Loading candles for " + symbol + " from " + startDateStr + " to " + endDateStr);

		// Try to get candles from database first
		std::vector<Candle> candles = storage.getCandles(
			recipe.market.exchange,
			symbol,
			recipe.market.timeframe,
			startTime,
			endTime
		);

		// If not enough data in database, download from Binance
		if (candles.empty()) {
			LOG_INFO("No data in database, downloading from Binance...");
			progressBar->Update(15.0, "Downloading historical data from Binance...");

			// Download from Binance
			BinanceAPI api;
			if (!api.init("", "")) {  // Public API, no keys needed
				throw std::runtime_error("Failed to initialize Binance API");
			}

			// Download in chunks (Binance limit is 1000 per request)
			// Calculate timeframe duration in seconds
			int timeframeSec = 3600; // Default 1h
			if (recipe.market.timeframe == "1m") timeframeSec = 60;
			else if (recipe.market.timeframe == "5m") timeframeSec = 300;
			else if (recipe.market.timeframe == "15m") timeframeSec = 900;
			else if (recipe.market.timeframe == "30m") timeframeSec = 1800;
			else if (recipe.market.timeframe == "1h") timeframeSec = 3600;
			else if (recipe.market.timeframe == "4h") timeframeSec = 14400;
			else if (recipe.market.timeframe == "1d") timeframeSec = 86400;

			time_t currentStart = startTime;
			int totalChunks = 0;
			int consecutiveEmptyChunks = 0;

			// Download all data in the requested range, no limits on empty chunks
			while (currentStart < endTime) {
				std::vector<Candle> chunk = api.getCandles(
					symbol,
					recipe.market.timeframe,
					currentStart,
					endTime,
					1000  // Max limit
				);

				totalChunks++;

				if (chunk.empty()) {
					consecutiveEmptyChunks++;
					// Move forward by 1000 candles worth to skip gaps
					currentStart += (1000 * timeframeSec);
					LOG_INFO("Empty chunk #" + std::to_string(consecutiveEmptyChunks) +
					         " received, continuing to search...");

					// Update progress even for empty chunks
					double progress = 15.0 + (30.0 * (double)(currentStart - startTime) / (endTime - startTime));
					std::string statusMsg = "Searching... (" + std::to_string(candles.size()) + " candles found)";
					progressBar->Update(progress, statusMsg.c_str());
					continue;
				}

				consecutiveEmptyChunks = 0; // Reset empty counter

				// Save to database
				storage.insertCandles(chunk);

				// Merge with main candles vector
				candles.insert(candles.end(), chunk.begin(), chunk.end());

				// Move to next chunk: start from the last candle + 1 timeframe
				currentStart = chunk.back().timestamp + timeframeSec;

				// Update progress
				double progress = 15.0 + (30.0 * (double)(currentStart - startTime) / (endTime - startTime));
				std::string statusMsg = "Downloaded " + std::to_string(candles.size()) + " candles...";
				progressBar->Update(progress, statusMsg.c_str());

				LOG_INFO("Downloaded chunk " + std::to_string(totalChunks) +
				         ": " + std::to_string(chunk.size()) +
				         " candles (total: " + std::to_string(candles.size()) + ")");
			}

			LOG_INFO("Downloaded and saved " + std::to_string(candles.size()) + " candles");
		}

		if (candles.empty()) {
			throw std::runtime_error("No candles available for " + symbol +
			                          " in the specified date range");
		}

		LOG_INFO("Loaded " + std::to_string(candles.size()) + " candles");

		progressBar->Update(30.0, "Calculating indicators...");

		// Run backtest
		Backtest::BacktestSimulator simulator(recipe, config);
		Backtest::BacktestResult result = simulator.run(candles);

		progressBar->Update(70.0, "Analyzing performance...");

		// Analyze performance
		Backtest::PerformanceAnalyzer analyzer;
		analyzer.analyze(result);

		progressBar->Update(100.0, "Complete!");

		// Display results
		lastResult = result;
		lastCandles = candles;  // Save candles for chart
		DisplayResults(result);

		// Save results to database
		SaveResultsToDatabase(result, recipe, candles);

		exportButton->SetEnabled(true);

		// Notify MainWindow that backtest is complete (to refresh Dashboard)
		BMessage notification(MSG_BACKTEST_COMPLETED);
		Window()->PostMessage(&notification);

		LOG_INFO("Backtest completed successfully");

	} catch (const std::exception& e) {
		LOG_ERROR("Backtest failed: " + std::string(e.what()));

		BAlert* alert = new BAlert("Backtest Failed",
		                            (std::string("Backtest failed:\n") + e.what()).c_str(),
		                            "OK", nullptr, nullptr,
		                            B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->Go();

		statusLabel->SetText("Backtest failed");
	}

	// Reset UI
	progressBar->Hide();
	runButton->SetEnabled(true);
	backtestRunning = false;
}

void BacktestView::DisplayResults(const Backtest::BacktestResult& result) {
	// Update equity chart
	equityChartView->SetEquityCurve(result.equityCurve);
	equityChartView->SetTrades(result.trades);
	equityChartView->SetInitialCapital(result.initialCapital);

	// Convert candles to price data for chart
	if (!lastCandles.empty()) {
		std::vector<Backtest::EquityPoint> pricePoints;
		for (const auto& candle : lastCandles) {
			Backtest::EquityPoint point;
			point.timestamp = candle.timestamp;
			point.equity = candle.close;  // Using equity field to store price
			point.cash = 0.0;
			point.positionValue = 0.0;
			pricePoints.push_back(point);
		}
		equityChartView->SetPriceData(pricePoints);
	}

	// Status
	std::ostringstream status;
	status << "Backtest completed: " << result.recipeName
	       << " on " << result.symbol
	       << " (" << result.totalCandles << " candles)";
	statusLabel->SetText(status.str().c_str());

	// Metrics - formatted with better styling
	std::ostringstream trades;
	trades << "Trades: " << result.totalTrades
	       << " (" << result.winningTrades << "W / "
	       << result.losingTrades << "L)";
	tradesLabel->SetText(trades.str().c_str());

	std::ostringstream winrate;
	winrate << std::fixed << std::setprecision(1);
	winrate << "Win Rate: " << result.winRate << "%";
	winRateLabel->SetText(winrate.str().c_str());

	std::ostringstream ret;
	ret << std::fixed << std::setprecision(2);
	ret << "Return: " << (result.totalReturnPercent >= 0 ? "+" : "")
	    << result.totalReturnPercent << "%";
	returnLabel->SetText(ret.str().c_str());

	// Color code the return
	if (result.totalReturnPercent >= 0) {
		returnLabel->SetHighColor(34, 139, 34);  // Green
	} else {
		returnLabel->SetHighColor(220, 38, 38);  // Red
	}

	std::ostringstream sharpe;
	sharpe << std::fixed << std::setprecision(2);
	sharpe << "Sharpe: " << result.sharpeRatio;
	sharpeLabel->SetText(sharpe.str().c_str());

	std::ostringstream dd;
	dd << std::fixed << std::setprecision(2);
	dd << "Max DD: -" << result.maxDrawdownPercent << "%";
	drawdownLabel->SetText(dd.str().c_str());
	drawdownLabel->SetHighColor(220, 38, 38);  // Red for drawdown

	// Trades list - populate with column data
	while (tradesList->CountRows() > 0) {
		BRow* row = tradesList->RowAt(0);
		tradesList->RemoveRow(row);
		delete row;
	}

	for (const auto& trade : result.trades) {
		BRow* row = new BRow();

		// Determine row color based on profit/loss
		rgb_color rowColor;
		if (trade.pnl > 0) {
			rowColor = {220, 255, 220, 255};  // Light green for winning trades
		} else {
			rowColor = {255, 220, 220, 255};  // Light red/pink for losing trades
		}

		// P&L column (with colored background)
		std::ostringstream pnlStr;
		pnlStr << std::fixed << std::setprecision(2);
		pnlStr << (trade.pnl >= 0 ? "+" : "") << "$" << trade.pnl;
		row->SetField(new ColoredStringField(pnlStr.str().c_str(), rowColor), 0);

		// Exit reason (with same colored background)
		row->SetField(new ColoredStringField(trade.exitReason.c_str(), rowColor), 1);

		tradesList->AddRow(row);
	}
}

void BacktestView::ClearResults() {
	equityChartView->Clear();
	statusLabel->SetText("No backtest run yet");
	tradesLabel->SetText("Trades: -");
	winRateLabel->SetText("Win Rate: -");
	returnLabel->SetText("Return: -");
	sharpeLabel->SetText("Sharpe: -");
	drawdownLabel->SetText("Max DD: -");

	while (tradesList->CountRows() > 0) {
		BRow* row = tradesList->RowAt(0);
		tradesList->RemoveRow(row);
		delete row;
	}

	exportButton->SetEnabled(false);
	selectedTradeIndex = -1;
}

void BacktestView::ExportResults() {
	if (lastResult.totalTrades == 0) {
		BAlert* alert = new BAlert("No Results", "No results to export!",
		                            "OK", nullptr, nullptr,
		                            B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->Go();
		return;
	}

	try {
		Backtest::PerformanceAnalyzer analyzer;

		// Generate reports
		std::string textReport = analyzer.generateTextReport(lastResult);
		std::string jsonReport = analyzer.generateJSONReport(lastResult);

		// Save to files
		std::string baseFilename = "/boot/home/Desktop/backtest_" +
		                            lastResult.recipeName + "_" +
		                            std::to_string(std::time(nullptr));

		std::ofstream textFile(baseFilename + ".txt");
		textFile << textReport;
		textFile.close();

		std::ofstream jsonFile(baseFilename + ".json");
		jsonFile << jsonReport;
		jsonFile.close();

		LOG_INFO("Reports exported: " + baseFilename);

		BAlert* alert = new BAlert("Export Success",
		                            ("Reports exported to Desktop:\n" +
		                             baseFilename + ".txt\n" +
		                             baseFilename + ".json").c_str(),
		                            "OK", nullptr, nullptr,
		                            B_WIDTH_AS_USUAL, B_INFO_ALERT);
		alert->Go();

	} catch (const std::exception& e) {
		LOG_ERROR("Export failed: " + std::string(e.what()));

		BAlert* alert = new BAlert("Export Failed",
		                            (std::string("Export failed:\n") + e.what()).c_str(),
		                            "OK", nullptr, nullptr,
		                            B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->Go();
	}
}

void BacktestView::SaveResultsToDatabase(const Backtest::BacktestResult& result,
                                          const Recipe& recipe,
                                          const std::vector<Candle>& candles) {
	try {
		// Open database
		DataStorage storage;
		if (!storage.init("/boot/home/Emiglio/data/emilio.db")) {
			LOG_WARNING("Failed to initialize database for saving results");
			return;
		}

		// Create BacktestResult structure for database
		BacktestResult dbResult;

		// Generate unique ID
		std::ostringstream idStream;
		idStream << recipe.name << "_" << result.symbol << "_" << std::time(nullptr);
		dbResult.id = idStream.str();

		dbResult.recipeName = recipe.name;
		dbResult.startDate = candles.empty() ? 0 : candles.front().timestamp;
		dbResult.endDate = candles.empty() ? 0 : candles.back().timestamp;
		dbResult.initialCapital = result.initialCapital;
		dbResult.finalCapital = result.finalEquity;
		dbResult.totalReturn = result.totalReturnPercent / 100.0; // Convert to decimal
		dbResult.sharpeRatio = result.sharpeRatio;
		dbResult.maxDrawdown = result.maxDrawdownPercent / 100.0; // Convert to decimal
		dbResult.winRate = result.winRate / 100.0; // Convert to decimal
		dbResult.totalTrades = result.totalTrades;
		dbResult.createdAt = std::time(nullptr);

		// Create config JSON
		std::ostringstream configStream;
		configStream << "{";
		configStream << "\"symbol\":\"" << result.symbol << "\",";
		configStream << "\"timeframe\":\"" << recipe.market.timeframe << "\",";
		configStream << "\"exchange\":\"" << recipe.market.exchange << "\",";
		configStream << "\"commission\":" << result.totalCommission << ",";
		configStream << "\"candles\":" << result.totalCandles;
		configStream << "}";
		dbResult.config = configStream.str();

		// Insert into database
		if (storage.insertBacktestResult(dbResult)) {
			LOG_INFO("Backtest results saved to database: " + dbResult.id);
		} else {
			LOG_WARNING("Failed to save backtest results to database");
		}

	} catch (const std::exception& e) {
		LOG_ERROR("Error saving backtest results: " + std::string(e.what()));
	}
}

} // namespace UI
} // namespace Emiglio
