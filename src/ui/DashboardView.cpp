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
		.AddGlue()
		.End();

	// ========== SYSTEM STATUS BOX ==========
	BBox* systemBox = new BBox("system_box");
	systemBox->SetLabel("System Status");

	recipesCountLabel = new BStringView("", "Strategies: Loading...");
	candlesCountLabel = new BStringView("", "Data Points: Loading...");
	backtestsCountLabel = new BStringView("", "Backtest Results: Loading...");

	recipesCountLabel->SetFont(&valueFont);
	candlesCountLabel->SetFont(&valueFont);
	backtestsCountLabel->SetFont(&valueFont);

	BLayoutBuilder::Group<>(systemBox, B_VERTICAL, 4)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(recipesCountLabel)
		.Add(candlesCountLabel)
		.Add(backtestsCountLabel)
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

	// ========== RECENT BACKTESTS BOX ==========
	BBox* backtestsBox = new BBox("backtests_box");
	backtestsBox->SetLabel("Recent Backtest Results");

	recentBacktestsView = new BListView("recent_backtests");
	recentBacktestsView->SetExplicitMinSize(BSize(B_SIZE_UNSET, 100));
	recentBacktestsView->SetExplicitMaxSize(BSize(B_SIZE_UNSET, 140));
	recentBacktestsView->SetExplicitPreferredSize(BSize(B_SIZE_UNSET, 120));

	recentBacktestsScroll = new BScrollView("recent_scroll", recentBacktestsView,
	                                        0, false, true);

	runBacktestButton = new BButton("New Backtest", new BMessage(MSG_RUN_BACKTEST));

	BLayoutBuilder::Group<>(backtestsBox, B_VERTICAL, 3)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(recentBacktestsScroll)
		.AddStrut(2)
		.AddGroup(B_HORIZONTAL)
			.Add(runBacktestButton)
			.AddGlue()
		.End()
		.End();

	// ========== MAIN LAYOUT (CLEAN GRID) ==========
	BLayoutBuilder::Group<>(this, B_VERTICAL, 6)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(titleView)
		.Add(subtitleView)
		.AddStrut(4)
		// Row 1: Simulated + System Status
		.AddGroup(B_HORIZONTAL, 6)
			.Add(simulatedBox, 2.0f)
			.Add(systemBox, 1.0f)
		.End()
		// Row 2: Live Trading (full width)
		.Add(liveBox, 1.0f)
		// Row 3: Recent Backtests
		.Add(backtestsBox, 1.0f)
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
	// For now, show placeholder data
	// In a real system, this would query actual portfolio state

	double initialCapital = 10000.0;
	double currentCapital = 10000.0; // Would be calculated from actual trades
	double availableCash = 10000.0;
	double invested = 0.0;
	double pnl = currentCapital - initialCapital;
	double pnlPercent = (pnl / initialCapital) * 100.0;

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

	// Count backtest results (using shared instance)
	int backtestCount = 0;
	if (dataStorage) {
		std::vector<BacktestResult> backtestResults = dataStorage->getAllBacktestResults();
		backtestCount = backtestResults.size();
	}

	oss.str("");
	oss << "Backtest Results: " << backtestCount;
	backtestsCountLabel->SetText(oss.str().c_str());

	LOG_INFO("Dashboard stats refreshed");
}

void DashboardView::LoadRecentBacktests() {
	recentBacktestsView->MakeEmpty();

	// Query database for recent backtest results (using shared instance)
	if (!dataStorage) {
		recentBacktestsView->AddItem(new BStringItem("Failed to load backtest results"));
		LOG_WARNING("DataStorage not initialized");
		return;
	}

	std::vector<BacktestResult> results = dataStorage->getAllBacktestResults();

	if (results.empty()) {
		BStringItem* header = new BStringItem("No backtest results available");
		BFont headerFont(be_bold_font);
		headerFont.SetSize(11);
		recentBacktestsView->AddItem(header);
		recentBacktestsView->AddItem(new BStringItem(""));
		recentBacktestsView->AddItem(new BStringItem("→ Click 'New Backtest' below to run your first backtest"));
		recentBacktestsView->AddItem(new BStringItem("→ Or go to the 'Backtest' tab to configure parameters"));
		recentBacktestsView->AddItem(new BStringItem(""));
		recentBacktestsView->AddItem(new BStringItem("Results will appear here once you run backtests."));
		LOG_INFO("No backtest results in database");
		return;
	}

	// Sort by created_at (newest first) and limit to last 10
	std::sort(results.begin(), results.end(),
	          [](const BacktestResult& a, const BacktestResult& b) {
		          return a.createdAt > b.createdAt;
	          });

	int maxDisplay = std::min(10, static_cast<int>(results.size()));

	// Add header
	recentBacktestsView->AddItem(new BStringItem("Recent Backtest Results:"));
	recentBacktestsView->AddItem(new BStringItem(""));

	// Display results
	for (int i = 0; i < maxDisplay; i++) {
		const BacktestResult& result = results[i];

		// Format: [Recipe] Symbol | Return: +X% | Sharpe: Y.Y | Trades: N
		std::ostringstream item;
		item << std::fixed << std::setprecision(1);

		item << "[" << result.recipeName << "] ";

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
		item << symbol << " | ";

		// Return (color code it)
		item << "Return: ";
		if (result.totalReturn > 0) {
			item << "+";
		}
		item << (result.totalReturn * 100.0) << "% | ";

		// Sharpe ratio
		item << std::setprecision(2);
		item << "Sharpe: " << result.sharpeRatio << " | ";

		// Trades
		item << "Trades: " << result.totalTrades;

		BStringItem* stringItem = new BStringItem(item.str().c_str());

		// Color code based on return
		if (result.totalReturn > 0) {
			// TODO: Can't set item color directly, would need custom list item
			// For now, just add it normally
		}

		recentBacktestsView->AddItem(stringItem);
	}

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
