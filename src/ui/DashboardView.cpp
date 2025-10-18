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
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	// Initialize credential manager
	if (!credentialManager->init("/boot/home/Emiglio/data/emilio.db")) {
		LOG_ERROR("Failed to initialize CredentialManager in Dashboard");
	}

	BuildLayout();
	RefreshData();
}

DashboardView::~DashboardView() {
	delete autoRefreshRunner;
}

void DashboardView::AttachedToWindow() {
	BView::AttachedToWindow();

	if (runBacktestButton) runBacktestButton->SetTarget(this);
	if (refreshBinanceButton) refreshBinanceButton->SetTarget(this);

	// Start auto-refresh timer (every 5 seconds)
	BMessage refreshMsg(MSG_AUTO_REFRESH);
	autoRefreshRunner = new BMessageRunner(this, &refreshMsg, 5000000); // 5 seconds in microseconds
}

void DashboardView::DetachedFromWindow() {
	// Stop auto-refresh timer
	delete autoRefreshRunner;
	autoRefreshRunner = nullptr;

	BView::DetachedFromWindow();
}

void DashboardView::BuildLayout() {
	// Title
	BStringView* titleView = new BStringView("", "Emiglio Dashboard");
	BFont titleFont(be_bold_font);
	titleFont.SetSize(20);
	titleView->SetFont(&titleFont);

	// Subtitle
	BStringView* subtitleView = new BStringView("", "Educational Trading System - Portfolio Overview");
	BFont subtitleFont(be_plain_font);
	subtitleFont.SetSize(11);
	subtitleView->SetFont(&subtitleFont);
	subtitleView->SetHighColor(100, 100, 100);

	// Portfolio section - SIMULATED DATA (Paper Trading)
	BStringView* simulatedWarning = new BStringView("", "[SIMULATED - NOT REAL MONEY]");
	BFont warningFont(be_bold_font);
	warningFont.SetSize(11);
	simulatedWarning->SetFont(&warningFont);
	simulatedWarning->SetHighColor(150, 100, 0); // Orange warning color

	// Get user's preferred currency symbol for initial display
	Config& config = Config::getInstance();
	std::string currencySymbol = config.getCurrencySymbol();

	totalCapitalLabel = new BStringView("", ("Total Capital: " + currencySymbol + "0.00").c_str());
	availableCashLabel = new BStringView("", ("Available Cash: " + currencySymbol + "0.00").c_str());
	investedLabel = new BStringView("", ("Invested: " + currencySymbol + "0.00").c_str());
	totalPnLLabel = new BStringView("", ("Total P&L: " + currencySymbol + "0.00").c_str());
	totalPnLPercentLabel = new BStringView("", "Total P&L %: 0.00%");

	BFont labelFont(be_plain_font);
	labelFont.SetSize(13);
	totalCapitalLabel->SetFont(&labelFont);
	availableCashLabel->SetFont(&labelFont);
	investedLabel->SetFont(&labelFont);

	// Style simulated data with gray color
	totalCapitalLabel->SetHighColor(80, 80, 80);
	availableCashLabel->SetHighColor(80, 80, 80);
	investedLabel->SetHighColor(80, 80, 80);

	BFont pnlFont(be_bold_font);
	pnlFont.SetSize(15);
	totalPnLLabel->SetFont(&pnlFont);
	totalPnLPercentLabel->SetFont(&pnlFont);

	BBox* portfolioBox = new BBox("portfolio_box");
	portfolioBox->SetLabel("Paper Trading Portfolio (SIMULATED)");

	// Set a lighter background to indicate simulation
	BView* portfolioContent = new BView("portfolio_content", B_WILL_DRAW);
	portfolioContent->SetViewColor(245, 245, 240); // Light beige for simulation

	BLayoutBuilder::Group<>(portfolioContent, B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_SMALL_SPACING)
		.Add(simulatedWarning)
		.AddStrut(B_USE_SMALL_SPACING)
		.Add(totalCapitalLabel)
		.Add(availableCashLabel)
		.Add(investedLabel)
		.AddStrut(B_USE_SMALL_SPACING * 2)
		.Add(totalPnLLabel)
		.Add(totalPnLPercentLabel)
		.End();

	BLayoutBuilder::Group<>(portfolioBox, B_VERTICAL, 0)
		.Add(portfolioContent)
		.End();

	// System stats section - simpler layout
	recipesCountLabel = new BStringView("", "Recipes: 0");
	backtestsCountLabel = new BStringView("", "Backtest Results: 0");
	candlesCountLabel = new BStringView("", "Candles in Database: 0");

	BFont statsFont(be_plain_font);
	statsFont.SetSize(13);
	recipesCountLabel->SetFont(&statsFont);
	backtestsCountLabel->SetFont(&statsFont);
	candlesCountLabel->SetFont(&statsFont);

	BBox* statsBox = new BBox("stats_box");
	statsBox->SetLabel("System Statistics");

	BLayoutBuilder::Group<>(statsBox, B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(recipesCountLabel)
		.Add(backtestsCountLabel)
		.Add(candlesCountLabel)
		.End();

	// Real Portfolio Summary - AGGREGATED across all exchanges
	BStringView* realSummaryLabel = new BStringView("", "[LIVE - REAL MONEY SUMMARY]");
	BFont realSummaryFont(be_bold_font);
	realSummaryFont.SetSize(11);
	realSummaryLabel->SetFont(&realSummaryFont);
	realSummaryLabel->SetHighColor(0, 120, 0); // Dark green

	realTotalValueLabel = new BStringView("", ("Total Value: " + currencySymbol + "0.00").c_str());
	realExchangeCountLabel = new BStringView("", "Active Exchanges: 0");
	realLastUpdateLabel = new BStringView("", "Last Update: Never");

	BFont realFont(be_plain_font);
	realFont.SetSize(14);
	realTotalValueLabel->SetFont(&realFont);
	realTotalValueLabel->SetHighColor(0, 100, 0); // Green

	BFont realSmallFont(be_plain_font);
	realSmallFont.SetSize(12);
	realExchangeCountLabel->SetFont(&realSmallFont);
	realLastUpdateLabel->SetFont(&realSmallFont);
	realExchangeCountLabel->SetHighColor(60, 60, 60);
	realLastUpdateLabel->SetHighColor(60, 60, 60);

	BBox* realSummaryBox = new BBox("real_summary_box");
	realSummaryBox->SetLabel("Real Portfolio Summary (ALL EXCHANGES)");

	// Set light green background
	BView* realSummaryContent = new BView("real_summary_content", B_WILL_DRAW);
	realSummaryContent->SetViewColor(240, 250, 240); // Very light green

	BLayoutBuilder::Group<>(realSummaryContent, B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_SMALL_SPACING)
		.Add(realSummaryLabel)
		.AddStrut(B_USE_SMALL_SPACING)
		.Add(realTotalValueLabel)
		.AddStrut(B_USE_SMALL_SPACING)
		.Add(realExchangeCountLabel)
		.Add(realLastUpdateLabel)
		.End();

	BLayoutBuilder::Group<>(realSummaryBox, B_VERTICAL, 0)
		.Add(realSummaryContent)
		.End();

	// Binance Portfolio section - REAL DATA from live exchange
	BStringView* realDataLabel = new BStringView("", "[LIVE - REAL MONEY]");
	BFont binanceFont(be_bold_font);
	binanceFont.SetSize(11);
	realDataLabel->SetFont(&binanceFont);
	realDataLabel->SetHighColor(0, 120, 0); // Dark green for real data

	binanceStatusLabel = new BStringView("", "Status: Not connected");
	BFont statusFont(be_bold_font);
	statusFont.SetSize(13);
	binanceStatusLabel->SetFont(&statusFont);

	binanceTotalValueLabel = new BStringView("", "");
	BFont valueFont(be_plain_font);
	valueFont.SetSize(12);
	binanceTotalValueLabel->SetFont(&valueFont);
	binanceTotalValueLabel->SetHighColor(0, 100, 0); // Green for real values

	// Create column list view for balances
	binanceBalancesView = new BColumnListView("binance_balances", B_WILL_DRAW, B_FANCY_BORDER, true);

	// Add columns
	binanceBalancesView->AddColumn(new BStringColumn("Asset", 80, 50, 150, B_TRUNCATE_END), 0);
	binanceBalancesView->AddColumn(new BStringColumn("Total", 140, 100, 200, B_TRUNCATE_END), 1);
	binanceBalancesView->AddColumn(new BStringColumn("Free", 140, 100, 200, B_TRUNCATE_END), 2);
	binanceBalancesView->AddColumn(new BStringColumn("Locked", 140, 100, 200, B_TRUNCATE_END), 3);

	binanceBalancesScroll = new BScrollView("binance_scroll", binanceBalancesView,
	                                        0, false, true);

	refreshBinanceButton = new BButton("Refresh Binance Portfolio", new BMessage(MSG_REFRESH_BINANCE));

	BBox* binanceBox = new BBox("binance_box");
	binanceBox->SetLabel("Binance Live Portfolio (REAL)");

	// Set a light green background to indicate real data
	BView* binanceContent = new BView("binance_content", B_WILL_DRAW);
	binanceContent->SetViewColor(240, 250, 240); // Very light green for real data

	BLayoutBuilder::Group<>(binanceContent, B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_SMALL_SPACING)
		.Add(realDataLabel)
		.AddStrut(B_USE_SMALL_SPACING)
		.AddGroup(B_HORIZONTAL)
			.Add(binanceStatusLabel)
			.AddGlue()
			.Add(binanceTotalValueLabel)
		.End()
		.Add(binanceBalancesScroll)
		.Add(refreshBinanceButton)
		.End();

	BLayoutBuilder::Group<>(binanceBox, B_VERTICAL, 0)
		.Add(binanceContent)
		.End();

	// Recent backtests section
	recentBacktestsView = new BListView("recent_backtests");
	recentBacktestsScroll = new BScrollView("recent_scroll", recentBacktestsView,
	                                        0, false, true);

	BBox* recentBox = new BBox("recent_box");
	recentBox->SetLabel("Recent Backtest Results");

	BLayoutBuilder::Group<>(recentBox, B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(recentBacktestsScroll)
		.End();

	// Action buttons
	runBacktestButton = new BButton("Run New Backtest", new BMessage(MSG_RUN_BACKTEST));

	// Main layout - vertical stacking for clarity
	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_WINDOW_SPACING)
		.Add(titleView)
		.Add(subtitleView)
		.AddStrut(B_USE_DEFAULT_SPACING)
		// Top row: Simulated Portfolio and Stats side by side
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.Add(portfolioBox, 1.5)
			.Add(statsBox, 1)
		.End()
		// Second row: Real Portfolio Summary (aggregated)
		.Add(realSummaryBox, 1)
		// Third row: Binance detailed portfolio (full width)
		.Add(binanceBox, 2)
		// Bottom: Recent backtests
		.Add(recentBox, 1.5)
		// Action buttons
		.AddGroup(B_HORIZONTAL)
			.Add(runBacktestButton)
			.AddGlue()
		.End()
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
			LoadBinancePortfolio();
			break;

		default:
			BView::MessageReceived(message);
			break;
	}
}

void DashboardView::RefreshData() {
	LoadPortfolioStats();
	LoadRealPortfolioSummary();
	LoadBinancePortfolio();
	LoadRecentBacktests();
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
	oss << "Total Capital: " << currencySymbol << std::fixed << std::setprecision(2) << currentCapital;
	totalCapitalLabel->SetText(oss.str().c_str());

	oss.str("");
	oss << "Available Cash: " << currencySymbol << std::fixed << std::setprecision(2) << availableCash;
	availableCashLabel->SetText(oss.str().c_str());

	oss.str("");
	oss << "Invested: " << currencySymbol << std::fixed << std::setprecision(2) << invested;
	investedLabel->SetText(oss.str().c_str());

	oss.str("");
	oss << "Total P&L: " << currencySymbol << std::fixed << std::setprecision(2) << pnl;
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
	oss << "Recipes: " << recipeCount << " strategies";
	recipesCountLabel->SetText(oss.str().c_str());

	// Count candles in database
	DataStorage storage;
	int candleCount = 0;
	if (storage.init("/boot/home/Emiglio/data/emilio.db")) {
		candleCount = storage.getCandleCount("binance", "", "");
	}

	oss.str("");
	oss << "Candles: " << candleCount << " data points";
	candlesCountLabel->SetText(oss.str().c_str());

	oss.str("");
	oss << "Backtests: N/A";
	backtestsCountLabel->SetText(oss.str().c_str());

	LOG_INFO("Dashboard stats refreshed");
}

void DashboardView::LoadRecentBacktests() {
	recentBacktestsView->MakeEmpty();

	// Query database for recent backtest results
	DataStorage storage;
	if (!storage.init("/boot/home/Emiglio/data/emilio.db")) {
		recentBacktestsView->AddItem(new BStringItem("Failed to load backtest results"));
		LOG_WARNING("Failed to initialize database");
		return;
	}

	std::vector<BacktestResult> results = storage.getAllBacktestResults();

	if (results.empty()) {
		recentBacktestsView->AddItem(new BStringItem("No backtest results yet"));
		recentBacktestsView->AddItem(new BStringItem(""));
		recentBacktestsView->AddItem(new BStringItem("Run backtests from the Backtest tab"));
		recentBacktestsView->AddItem(new BStringItem("and results will appear here."));
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
	oss << "Total Value: " << currencySymbol << std::fixed << std::setprecision(2) << totalValue;
	if (totalValue == 0.0) {
		oss << " (approx, price conversion needed)";
	}
	realTotalValueLabel->SetText(oss.str().c_str());

	oss.str("");
	oss << "Active Exchanges: " << activeExchanges;
	if (activeExchanges > 0) {
		oss << " (Binance";
		// Add more exchanges as they are implemented
		oss << ")";
	} else {
		oss << " (none configured)";
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
