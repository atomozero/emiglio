#include "DashboardView.h"
#include "../data/DataStorage.h"
#include "../utils/Logger.h"
#include "../utils/Config.h"
#include "../utils/CredentialManager.h"
#include "../exchange/BinanceAPI.h"

#include <LayoutBuilder.h>
#include <GroupView.h>
#include <StringView.h>
#include <Button.h>
#include <ScrollView.h>
#include <Box.h>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>
#include <private/interface/ColumnListView.h>
#include <private/interface/ColumnTypes.h>
#include <private/shared/AutoDeleter.h>

#include <sstream>
#include <iomanip>
#include <algorithm>

namespace Emiglio {
namespace UI {

DashboardView::DashboardView()
	: BView("Dashboard", B_WILL_DRAW)
	, autoRefreshRunner(nullptr)
	, credentialManager(std::make_unique<CredentialManager>())
	, binanceAPI(std::make_unique<BinanceAPI>())
	, dataStorage(nullptr)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	// Initialize credential manager
	if (!credentialManager->init("/boot/home/Emiglio/data/emilio.db")) {
		LOG_ERROR("Failed to initialize CredentialManager in Dashboard");
	}

	// Initialize shared database storage
	dataStorage = new DataStorage();
	if (!dataStorage->init("/boot/home/Emiglio/data/emilio.db")) {
		LOG_ERROR("Failed to initialize DataStorage in Dashboard");
		delete dataStorage;
		dataStorage = nullptr;
	}

	BuildLayout();
	// Don't auto-refresh on construction - wait for AttachedToWindow
}

DashboardView::~DashboardView() {
	delete autoRefreshRunner;
	if (dataStorage) {
		delete dataStorage;
		dataStorage = nullptr;
	}
}

void DashboardView::AttachedToWindow() {
	BView::AttachedToWindow();

	if (runBacktestButton) runBacktestButton->SetTarget(this);
	if (refreshBinanceButton) refreshBinanceButton->SetTarget(this);

	// DISABLED: Auto-refresh was blocking the UI with synchronous network calls
	// TODO: Move to background thread if needed
	// Start auto-refresh timer (every 5 seconds)
	// BMessage refreshMsg(MSG_AUTO_REFRESH);
	// autoRefreshRunner = new BMessageRunner(this, &refreshMsg, 5000000); // 5 seconds in microseconds

	// Do initial refresh once
	RefreshData();
}

void DashboardView::DetachedFromWindow() {
	// Stop auto-refresh timer
	delete autoRefreshRunner;
	autoRefreshRunner = nullptr;

	BView::DetachedFromWindow();
}

void DashboardView::BuildLayout() {
	// Get user's preferred currency symbol
	Config& config = Config::getInstance();
	std::string currencySymbol = config.getCurrencySymbol();

	// ========== HEADER SECTION ==========
	BStringView* titleView = new BStringView("", "Dashboard");
	BFont titleFont(be_bold_font);
	titleFont.SetSize(18);
	titleView->SetFont(&titleFont);

	BStringView* subtitleView = new BStringView("", "Portfolio Overview & System Status");
	BFont subtitleFont(be_plain_font);
	subtitleFont.SetSize(11);
	subtitleView->SetFont(&subtitleFont);
	rgb_color subtitleColor = tint_color(ui_color(B_PANEL_TEXT_COLOR), B_LIGHTEN_1_TINT);
	subtitleView->SetHighColor(subtitleColor);

	// Common fonts
	BFont labelFont(be_plain_font);
	labelFont.SetSize(11);

	BFont valueFont(be_plain_font);
	valueFont.SetSize(12);

	BFont bigValueFont(be_bold_font);
	bigValueFont.SetSize(15);

	// ========== SIMULATED PORTFOLIO BOX ==========
	BBox* simulatedBox = new BBox("simulated_box");
	simulatedBox->SetLabel("Simulated Portfolio");

	BStringView* simModeLabel = new BStringView("", "Mode: Paper Trading");
	simModeLabel->SetFont(&labelFont);
	rgb_color mutedColor = tint_color(ui_color(B_PANEL_TEXT_COLOR), B_LIGHTEN_1_TINT);
	simModeLabel->SetHighColor(mutedColor);

	totalCapitalLabel = new BStringView("", "Capital: Loading...");
	availableCashLabel = new BStringView("", "Cash: Loading...");
	investedLabel = new BStringView("", "Invested: Loading...");

	totalCapitalLabel->SetFont(&valueFont);
	availableCashLabel->SetFont(&valueFont);
	investedLabel->SetFont(&valueFont);

	totalPnLLabel = new BStringView("", "P&L: Loading...");
	totalPnLPercentLabel = new BStringView("", "Total P&L %: Loading...");
	totalPnLLabel->SetFont(&bigValueFont);
	totalPnLPercentLabel->SetFont(&valueFont);

	winRateLabel = new BStringView("", "Win Rate: Loading...");
	maxDrawdownLabel = new BStringView("", "Max Drawdown: Loading...");
	openPositionsLabel = new BStringView("", "Open Positions: Loading...");
	winRateLabel->SetFont(&valueFont);
	maxDrawdownLabel->SetFont(&valueFont);
	openPositionsLabel->SetFont(&valueFont);

	BLayoutBuilder::Group<>(simulatedBox, B_VERTICAL, 4)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(simModeLabel)
		.AddStrut(2)
		.Add(totalCapitalLabel)
		.Add(availableCashLabel)
		.Add(investedLabel)
		.AddStrut(4)
		.Add(totalPnLLabel)
		.Add(totalPnLPercentLabel)
		.AddStrut(4)
		.Add(winRateLabel)
		.Add(maxDrawdownLabel)
		.Add(openPositionsLabel)
		.AddGlue()
		.End();

	// ========== SYSTEM STATUS BOX ==========
	BBox* systemBox = new BBox("system_box");
	systemBox->SetLabel("System Status");

	recipesCountLabel = new BStringView("", "Strategies: Loading...");
	candlesCountLabel = new BStringView("", "Data Points: Loading...");
	backtestsCountLabel = new BStringView("", "Backtest Results: Loading...");
	appVersionLabel = new BStringView("", "Version: Loading...");

	recipesCountLabel->SetFont(&valueFont);
	candlesCountLabel->SetFont(&valueFont);
	backtestsCountLabel->SetFont(&valueFont);
	appVersionLabel->SetFont(&valueFont);

	BLayoutBuilder::Group<>(systemBox, B_VERTICAL, 4)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(recipesCountLabel)
		.Add(candlesCountLabel)
		.Add(backtestsCountLabel)
		.Add(appVersionLabel)
		.AddGlue()
		.End();

	// ========== LIVE PORTFOLIO BOX ==========
	BBox* liveBox = new BBox("live_box");
	liveBox->SetLabel("Live Trading");

	binanceStatusLabel = new BStringView("", "Status: Not connected");
	binanceStatusLabel->SetFont(&labelFont);
	binanceStatusLabel->SetHighColor(mutedColor);

	realTotalValueLabel = new BStringView("", ("Total: " + currencySymbol + "0.00").c_str());
	realTotalValueLabel->SetFont(&bigValueFont);

	realExchangeCountLabel = new BStringView("", "Exchanges: 0");
	realLastUpdateLabel = new BStringView("", "Last Update: Never");
	realExchangeCountLabel->SetFont(&valueFont);
	realLastUpdateLabel->SetFont(&labelFont);
	realLastUpdateLabel->SetHighColor(mutedColor);

	// Balances table (compact)
	binanceBalancesView = new BColumnListView("binance_balances", B_WILL_DRAW, B_FANCY_BORDER, true);
	binanceBalancesView->AddColumn(new BStringColumn("Asset", 80, 60, 100, B_TRUNCATE_END), 0);
	binanceBalancesView->AddColumn(new BStringColumn("Total", 100, 80, 140, B_TRUNCATE_END), 1);
	binanceBalancesView->AddColumn(new BStringColumn("Free", 100, 80, 140, B_TRUNCATE_END), 2);
	binanceBalancesView->AddColumn(new BStringColumn("Locked", 100, 80, 140, B_TRUNCATE_END), 3);
	binanceBalancesView->SetExplicitMinSize(BSize(B_SIZE_UNSET, 80));
	binanceBalancesView->SetExplicitMaxSize(BSize(B_SIZE_UNSET, 100));
	binanceBalancesView->SetExplicitPreferredSize(BSize(B_SIZE_UNSET, 90));

	binanceBalancesScroll = new BScrollView("binance_scroll", binanceBalancesView,
	                                        0, false, true);

	refreshBinanceButton = new BButton("Refresh Binance", new BMessage(MSG_REFRESH_BINANCE));

	binanceTotalValueLabel = new BStringView("", "");
	binanceTotalValueLabel->SetFont(&valueFont);

	BLayoutBuilder::Group<>(liveBox, B_VERTICAL, 3)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(binanceStatusLabel)
		.AddStrut(2)
		.Add(realTotalValueLabel)
		.Add(realExchangeCountLabel)
		.Add(realLastUpdateLabel)
		.AddStrut(4)
		.Add(binanceBalancesScroll)
		.AddStrut(2)
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(refreshBinanceButton)
		.End()
		.End();

	// ========== RECENT BACKTESTS - SIMULATED ==========
	BBox* simulatedBacktestsBox = new BBox("simulated_backtests_box");
	simulatedBacktestsBox->SetLabel("Recent Backtests - Simulated");

	simulatedBacktestsView = new BColumnListView("simulated_backtests", B_WILL_DRAW, B_FANCY_BORDER);
	simulatedBacktestsView->AddColumn(new BStringColumn("Strategy", 110, 80, 160, B_TRUNCATE_END), 0);
	simulatedBacktestsView->AddColumn(new BStringColumn("Symbol", 70, 50, 100, B_TRUNCATE_END), 1);
	simulatedBacktestsView->AddColumn(new BStringColumn("Return %", 70, 60, 90, B_TRUNCATE_END), 2);
	simulatedBacktestsView->AddColumn(new BStringColumn("Sharpe", 60, 50, 80, B_TRUNCATE_END), 3);
	simulatedBacktestsView->AddColumn(new BStringColumn("Trades", 60, 50, 80, B_TRUNCATE_END), 4);
	simulatedBacktestsView->AddColumn(new BStringColumn("Date", 85, 70, 100, B_TRUNCATE_END), 5);
	simulatedBacktestsView->SetExplicitMinSize(BSize(B_SIZE_UNSET, 100));
	simulatedBacktestsView->SetExplicitMaxSize(BSize(B_SIZE_UNSET, 140));
	simulatedBacktestsView->SetExplicitPreferredSize(BSize(B_SIZE_UNSET, 120));

	simulatedBacktestsScroll = new BScrollView("simulated_scroll", simulatedBacktestsView,
	                                           0, false, true);

	runBacktestButton = new BButton("New Backtest", new BMessage(MSG_RUN_BACKTEST));

	BLayoutBuilder::Group<>(simulatedBacktestsBox, B_VERTICAL, 3)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(simulatedBacktestsScroll)
		.AddStrut(2)
		.AddGroup(B_HORIZONTAL)
			.Add(runBacktestButton)
			.AddGlue()
		.End()
		.End();

	// ========== RECENT BACKTESTS - REAL TRADING ==========
	BBox* realBacktestsBox = new BBox("real_backtests_box");
	realBacktestsBox->SetLabel("Recent Backtests - Real Trading");

	realBacktestsView = new BColumnListView("real_backtests", B_WILL_DRAW, B_FANCY_BORDER);
	realBacktestsView->AddColumn(new BStringColumn("Strategy", 110, 80, 160, B_TRUNCATE_END), 0);
	realBacktestsView->AddColumn(new BStringColumn("Symbol", 70, 50, 100, B_TRUNCATE_END), 1);
	realBacktestsView->AddColumn(new BStringColumn("Return %", 70, 60, 90, B_TRUNCATE_END), 2);
	realBacktestsView->AddColumn(new BStringColumn("Sharpe", 60, 50, 80, B_TRUNCATE_END), 3);
	realBacktestsView->AddColumn(new BStringColumn("Trades", 60, 50, 80, B_TRUNCATE_END), 4);
	realBacktestsView->AddColumn(new BStringColumn("Date", 85, 70, 100, B_TRUNCATE_END), 5);
	realBacktestsView->SetExplicitMinSize(BSize(B_SIZE_UNSET, 100));
	realBacktestsView->SetExplicitMaxSize(BSize(B_SIZE_UNSET, 140));
	realBacktestsView->SetExplicitPreferredSize(BSize(B_SIZE_UNSET, 120));

	realBacktestsScroll = new BScrollView("real_scroll", realBacktestsView,
	                                      0, false, true);

	BLayoutBuilder::Group<>(realBacktestsBox, B_VERTICAL, 3)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(realBacktestsScroll)
		.End();

	// ========== MAIN LAYOUT (ORGANIZED BY TYPE) ==========
	BLayoutBuilder::Group<>(this, B_VERTICAL, 6)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(titleView)
		.Add(subtitleView)
		.AddStrut(4)
		// Row 1: Simulated (left) + Real Trading (right)
		.AddGroup(B_HORIZONTAL, 6)
			// Left column: All Simulated sections
			.AddGroup(B_VERTICAL, 6)
				.Add(simulatedBox, 1.0f)
				.Add(simulatedBacktestsBox, 2.0f)
			.End()
			// Right column: All Real Trading sections
			.AddGroup(B_VERTICAL, 6)
				.Add(liveBox, 1.0f)
				.Add(realBacktestsBox, 2.0f)
			.End()
		.End()
		// Row 2: System Status (full width)
		.Add(systemBox, 0.5f)
		.AddGlue()
		.End();
}

void DashboardView::MessageReceived(BMessage* message) {
	switch (message->what) {
		case MSG_AUTO_REFRESH:
			RefreshData();
			break;

		case MSG_RUN_BACKTEST:
			// Switch to backtest tab
			LOG_INFO("Switching to Backtest tab...");
			break;

		case MSG_REFRESH_BINANCE:
			// Load both Binance portfolio and real portfolio summary
			LoadBinancePortfolio();
			LoadRealPortfolioSummary();
			break;

		case MSG_SETTINGS_CHANGED:
			// Settings changed (e.g., currency preference)
			// Refresh all data with new settings
			LOG_INFO("Settings changed - refreshing dashboard");
			RefreshData();
			break;

		default:
			BView::MessageReceived(message);
			break;
	}
}

void DashboardView::RefreshData() {
	LoadPortfolioStats();
	LoadRecentBacktests();
	// REMOVED: LoadBinancePortfolio() and LoadRealPortfolioSummary()
	// These make blocking network calls and freeze the UI
	// User must click "Refresh" button manually to load Binance data
}

void DashboardView::LoadPortfolioStats() {
	// Calculate real portfolio stats from backtest results
	double initialCapital = 10000.0;
	double currentCapital = initialCapital;
	double availableCash = initialCapital;
	double invested = 0.0;
	double totalWinRate = 0.0;
	double worstMaxDrawdown = 0.0;
	int totalOpenPositions = 0;
	int backtestCount = 0;

	// Get all backtest results to calculate aggregate metrics
	if (dataStorage) {
		std::vector<BacktestResult> results = dataStorage->getAllBacktestResults();
		backtestCount = results.size();

		if (backtestCount > 0) {
			// Calculate average metrics from all backtests
			double sumWinRate = 0.0;
			double sumMaxDrawdown = 0.0;

			for (const auto& result : results) {
				sumWinRate += result.winRate;
				sumMaxDrawdown += result.maxDrawdown;

				// Track worst drawdown
				if (result.maxDrawdown > worstMaxDrawdown) {
					worstMaxDrawdown = result.maxDrawdown;
				}
			}

			// Average win rate across all backtests
			totalWinRate = sumWinRate / backtestCount;

			// Use worst max drawdown (most conservative estimate)
			worstMaxDrawdown = worstMaxDrawdown;

			// For simulated portfolio: assume we're running the best strategy
			// Find the backtest with highest total return
			double bestReturn = -100.0;
			for (const auto& result : results) {
				if (result.totalReturn > bestReturn) {
					bestReturn = result.totalReturn;
					currentCapital = result.finalCapital;
				}
			}
		}
	}

	double pnl = currentCapital - initialCapital;
	double pnlPercent = (pnl / initialCapital) * 100.0;
	availableCash = currentCapital - invested;

	// Format values with user's preferred currency symbol
	Config& config = Config::getInstance();
	std::string currencySymbol = config.getCurrencySymbol();
	std::ostringstream oss;

	oss.str("");
	oss << "Capital: " << currencySymbol << std::fixed << std::setprecision(2) << currentCapital;
	totalCapitalLabel->SetText(oss.str().c_str());

	oss.str("");
	oss << "Cash: " << currencySymbol << std::fixed << std::setprecision(2) << availableCash;
	availableCashLabel->SetText(oss.str().c_str());

	oss.str("");
	oss << "Invested: " << currencySymbol << std::fixed << std::setprecision(2) << invested;
	investedLabel->SetText(oss.str().c_str());

	oss.str("");
	oss << "P&L: " << currencySymbol << std::fixed << std::setprecision(2) << pnl;
	totalPnLLabel->SetText(oss.str().c_str());

	// Color code P&L
	if (pnl > 0) {
		totalPnLLabel->SetHighColor(0, 150, 0); // Green
		totalPnLPercentLabel->SetHighColor(0, 150, 0);
	} else if (pnl < 0) {
		totalPnLLabel->SetHighColor(200, 0, 0); // Red
		totalPnLPercentLabel->SetHighColor(200, 0, 0);
	}

	oss.str("");
	oss << "Total P&L %: " << std::fixed << std::setprecision(2) << pnlPercent << "%";
	totalPnLPercentLabel->SetText(oss.str().c_str());

	// Display real performance metrics
	oss.str("");
	oss << "Win Rate: " << std::fixed << std::setprecision(1) << totalWinRate << "%";
	winRateLabel->SetText(oss.str().c_str());

	oss.str("");
	oss << "Max Drawdown: " << std::fixed << std::setprecision(2) << worstMaxDrawdown << "%";
	maxDrawdownLabel->SetText(oss.str().c_str());

	oss.str("");
	oss << "Open Positions: " << totalOpenPositions;
	openPositionsLabel->SetText(oss.str().c_str());

	// System stats
	// Count recipes
	int recipeCount = 0;
	BDirectory recipeDir("/boot/home/Emiglio/recipes");
	if (recipeDir.InitCheck() == B_OK) {
		BEntry entry;
		while (recipeDir.GetNextEntry(&entry) == B_OK) {
			char name[B_FILE_NAME_LENGTH];
			if (entry.GetName(name) == B_OK) {
				BString fileName(name);
				if (fileName.EndsWith(".json")) {
					recipeCount++;
				}
			}
		}
	}

	oss.str("");
	oss << "Strategies: " << recipeCount;
	recipesCountLabel->SetText(oss.str().c_str());

	// Count candles in database (using shared instance)
	int candleCount = 0;
	if (dataStorage) {
		candleCount = dataStorage->getCandleCount("binance", "", "");
	}

	oss.str("");
	oss << "Data Points: " << candleCount;
	candlesCountLabel->SetText(oss.str().c_str());

	// Use backtestCount already calculated above
	oss.str("");
	oss << "Backtest Results: " << backtestCount;
	backtestsCountLabel->SetText(oss.str().c_str());

	// App version from Config (reuse config from above)
	std::string appVersion = config.getString("app.version", "1.0.0");
	oss.str("");
	oss << "Version: " << appVersion;
	appVersionLabel->SetText(oss.str().c_str());

	LOG_INFO("Dashboard stats refreshed");
}

void DashboardView::LoadRecentBacktests() {
	simulatedBacktestsView->Clear();
	realBacktestsView->Clear();

	// Query database for recent backtest results (using shared instance)
	if (!dataStorage) {
		BRow* simRow = new BRow();
		simRow->SetField(new BStringField("Failed to load results"), 0);
		simRow->SetField(new BStringField(""), 1);
		simRow->SetField(new BStringField(""), 2);
		simRow->SetField(new BStringField(""), 3);
		simRow->SetField(new BStringField(""), 4);
		simRow->SetField(new BStringField(""), 5);
		simulatedBacktestsView->AddRow(simRow);

		BRow* realRow = new BRow();
		realRow->SetField(new BStringField("Failed to load results"), 0);
		realRow->SetField(new BStringField(""), 1);
		realRow->SetField(new BStringField(""), 2);
		realRow->SetField(new BStringField(""), 3);
		realRow->SetField(new BStringField(""), 4);
		realRow->SetField(new BStringField(""), 5);
		realBacktestsView->AddRow(realRow);

		LOG_WARNING("DataStorage not initialized");
		return;
	}

	std::vector<BacktestResult> results = dataStorage->getAllBacktestResults();

	if (results.empty()) {
		// Add placeholder row for simulated backtests
		BRow* simRow = new BRow();
		simRow->SetField(new BStringField("No results yet"), 0);
		simRow->SetField(new BStringField(""), 1);
		simRow->SetField(new BStringField(""), 2);
		simRow->SetField(new BStringField(""), 3);
		simRow->SetField(new BStringField(""), 4);
		simRow->SetField(new BStringField(""), 5);
		simulatedBacktestsView->AddRow(simRow);

		// Add placeholder row for real trading
		BRow* realRow = new BRow();
		realRow->SetField(new BStringField("No real trading results"), 0);
		realRow->SetField(new BStringField(""), 1);
		realRow->SetField(new BStringField(""), 2);
		realRow->SetField(new BStringField(""), 3);
		realRow->SetField(new BStringField(""), 4);
		realRow->SetField(new BStringField(""), 5);
		realBacktestsView->AddRow(realRow);

		LOG_INFO("No backtest results in database");
		return;
	}

	// Sort by created_at (newest first)
	std::sort(results.begin(), results.end(),
	          [](const BacktestResult& a, const BacktestResult& b) {
		          return a.createdAt > b.createdAt;
	          });

	int maxDisplay = std::min(5, static_cast<int>(results.size()));

	// For now, show all results in simulated (since we don't have a type field yet)
	// Real trading results section is empty for now
	for (int i = 0; i < maxDisplay; i++) {
		const BacktestResult& result = results[i];

		// Extract symbol from config JSON if available
		std::string symbol = "???";
		if (!result.config.empty()) {
			// Simple JSON parsing for symbol
			size_t symPos = result.config.find("\"symbol\":\"");
			if (symPos != std::string::npos) {
				symPos += 11; // Length of "symbol":""
				size_t endPos = result.config.find("\"", symPos);
				if (endPos != std::string::npos) {
					symbol = result.config.substr(symPos, endPos - symPos);
				}
			}
		}

		// Format return percentage
		std::ostringstream returnStr;
		returnStr << std::fixed << std::setprecision(1);
		if (result.totalReturn > 0) {
			returnStr << "+";
		}
		returnStr << (result.totalReturn * 100.0) << "%";

		// Format sharpe ratio
		std::ostringstream sharpeStr;
		sharpeStr << std::fixed << std::setprecision(2);
		sharpeStr << result.sharpeRatio;

		// Format trades count
		std::ostringstream tradesStr;
		tradesStr << result.totalTrades;

		// Format date (MM/DD/YY)
		std::string dateStr = "???";
		if (result.createdAt > 0) {
			struct tm* timeinfo = localtime(&result.createdAt);
			char buffer[20];
			strftime(buffer, sizeof(buffer), "%m/%d/%y", timeinfo);
			dateStr = buffer;
		}

		// Create row and add fields
		BRow* row = new BRow();
		row->SetField(new BStringField(result.recipeName.c_str()), 0);
		row->SetField(new BStringField(symbol.c_str()), 1);
		row->SetField(new BStringField(returnStr.str().c_str()), 2);
		row->SetField(new BStringField(sharpeStr.str().c_str()), 3);
		row->SetField(new BStringField(tradesStr.str().c_str()), 4);
		row->SetField(new BStringField(dateStr.c_str()), 5);
		simulatedBacktestsView->AddRow(row);
	}

	// Add placeholder for real trading
	BRow* realRow = new BRow();
	realRow->SetField(new BStringField("No real trading results"), 0);
	realRow->SetField(new BStringField(""), 1);
	realRow->SetField(new BStringField(""), 2);
	realRow->SetField(new BStringField(""), 3);
	realRow->SetField(new BStringField(""), 4);
	realRow->SetField(new BStringField(""), 5);
	realBacktestsView->AddRow(realRow);

	// Update count label
	std::ostringstream countStr;
	countStr << "Backtest Results: " << results.size();
	backtestsCountLabel->SetText(countStr.str().c_str());

	LOG_INFO("Loaded " + std::to_string(results.size()) + " backtest results");
}

void DashboardView::LoadBinancePortfolio() {
	// Clear existing rows
	binanceBalancesView->Clear();

	// Check if credentials exist
	if (!credentialManager->hasCredentials("binance")) {
		binanceStatusLabel->SetText("Status: Not configured");
		binanceStatusLabel->SetHighColor(200, 100, 0); // Orange
		binanceTotalValueLabel->SetText("");

		BRow* row = new BRow();
		row->SetField(new BStringField("N/A"), 0);
		row->SetField(new BStringField("No API credentials configured"), 1);
		row->SetField(new BStringField("Go to Settings tab"), 2);
		row->SetField(new BStringField(""), 3);
		binanceBalancesView->AddRow(row);
		return;
	}

	// Load credentials
	std::string apiKey, apiSecret;
	if (!credentialManager->loadCredentials("binance", apiKey, apiSecret)) {
		binanceStatusLabel->SetText("Status: Failed to load credentials");
		binanceStatusLabel->SetHighColor(200, 0, 0); // Red
		binanceTotalValueLabel->SetText("");

		BRow* row = new BRow();
		row->SetField(new BStringField("Error"), 0);
		row->SetField(new BStringField("Failed to decrypt credentials"), 1);
		row->SetField(new BStringField("Check configuration"), 2);
		row->SetField(new BStringField(""), 3);
		binanceBalancesView->AddRow(row);

		LOG_ERROR("Failed to load Binance credentials: " + credentialManager->getLastError());
		return;
	}

	// Initialize Binance API
	if (!binanceAPI->init(apiKey, apiSecret)) {
		binanceStatusLabel->SetText("Status: API initialization failed");
		binanceStatusLabel->SetHighColor(200, 0, 0); // Red
		binanceTotalValueLabel->SetText("");

		BRow* row = new BRow();
		row->SetField(new BStringField("Error"), 0);
		row->SetField(new BStringField("Failed to initialize Binance API"), 1);
		row->SetField(new BStringField("Check API keys"), 2);
		row->SetField(new BStringField(""), 3);
		binanceBalancesView->AddRow(row);

		LOG_ERROR("Failed to initialize BinanceAPI");
		return;
	}

	binanceStatusLabel->SetText("Status: Loading...");
	binanceStatusLabel->SetHighColor(0, 0, 0); // Black
	binanceTotalValueLabel->SetText("");
	binanceStatusLabel->Invalidate();

	// Fetch balances
	std::vector<Balance> balances = binanceAPI->getBalances();

	if (balances.empty()) {
		binanceStatusLabel->SetText("Status: Connected");
		binanceStatusLabel->SetHighColor(0, 150, 0); // Green
		binanceTotalValueLabel->SetText("Total Assets: 0");

		BRow* row = new BRow();
		row->SetField(new BStringField("N/A"), 0);
		row->SetField(new BStringField("No holdings found"), 1);
		row->SetField(new BStringField("Account is empty"), 2);
		row->SetField(new BStringField(""), 3);
		binanceBalancesView->AddRow(row);

		LOG_INFO("No balances found in Binance account");
		return;
	}

	// Sort balances by total value (descending)
	std::sort(balances.begin(), balances.end(),
	          [](const Balance& a, const Balance& b) {
		          return a.total > b.total;
	          });

	// Display balances
	binanceStatusLabel->SetText("Status: Connected & Loaded");
	binanceStatusLabel->SetHighColor(0, 150, 0); // Green

	// Calculate total count
	std::ostringstream totalText;
	totalText << "Total Assets: " << balances.size() << " different cryptocurrencies";
	binanceTotalValueLabel->SetText(totalText.str().c_str());

	// Display each balance in column list
	for (const auto& balance : balances) {
		BRow* row = new BRow();

		// Asset name
		row->SetField(new BStringField(balance.asset.c_str()), 0);

		// Total balance
		std::ostringstream totalStr;
		totalStr << std::fixed << std::setprecision(8) << balance.total;
		row->SetField(new BStringField(totalStr.str().c_str()), 1);

		// Free balance
		std::ostringstream freeStr;
		freeStr << std::fixed << std::setprecision(8) << balance.free;
		row->SetField(new BStringField(freeStr.str().c_str()), 2);

		// Locked balance
		std::ostringstream lockedStr;
		if (balance.locked > 0.00000001) {  // Only show if significant
			lockedStr << std::fixed << std::setprecision(8) << balance.locked;
		} else {
			lockedStr << "—";
		}
		row->SetField(new BStringField(lockedStr.str().c_str()), 3);

		binanceBalancesView->AddRow(row);
	}

	LOG_INFO("Loaded " + std::to_string(balances.size()) + " Binance balances");
}

void DashboardView::LoadRealPortfolioSummary() {
	// Aggregate real portfolio data across all exchanges
	double totalValue = 0.0;
	int activeExchanges = 0;
	time_t lastUpdate = 0;

	// Check Binance
	if (credentialManager->hasCredentials("binance")) {
		std::string apiKey, apiSecret;
		if (credentialManager->loadCredentials("binance", apiKey, apiSecret)) {
			if (binanceAPI->init(apiKey, apiSecret)) {
				std::vector<Balance> balances = binanceAPI->getBalances();
				if (!balances.empty()) {
					activeExchanges++;
					// For now, just sum up all balances
					// TODO: Convert to USD using price API
					for (const auto& balance : balances) {
						// Approximate value (needs real price conversion)
						if (balance.asset == "USDT" || balance.asset == "USDC" || balance.asset == "BUSD") {
							totalValue += balance.total;
						}
						// For other assets, we'd need to fetch current price
						// This is a simplified version
					}
					lastUpdate = time(nullptr);
				}
			}
		}
	}

	// TODO: Add other exchanges here when implemented
	// if (credentialManager->hasCredentials("coinbase")) { ... }
	// if (credentialManager->hasCredentials("kraken")) { ... }

	// Update labels with user's preferred currency symbol
	Config& config = Config::getInstance();
	std::string currencySymbol = config.getCurrencySymbol();
	std::ostringstream oss;

	oss.str("");
	oss << "Total: " << currencySymbol << std::fixed << std::setprecision(2) << totalValue;
	realTotalValueLabel->SetText(oss.str().c_str());

	oss.str("");
	oss << "Exchanges: " << activeExchanges;
	if (activeExchanges > 0) {
		oss << " (Binance)";
	}
	realExchangeCountLabel->SetText(oss.str().c_str());

	oss.str("");
	if (lastUpdate > 0) {
		char timeStr[64];
		struct tm* timeinfo = localtime(&lastUpdate);
		strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);
		oss << "Last Update: " << timeStr;
	} else {
		oss << "Last Update: Never";
	}
	realLastUpdateLabel->SetText(oss.str().c_str());

	LOG_INFO("Real portfolio summary: " + currencySymbol + std::to_string(totalValue) + " across " + std::to_string(activeExchanges) + " exchanges");
}

} // namespace UI
} // namespace Emiglio
