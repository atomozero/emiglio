#include "DashboardView.h"
#include "../data/DataStorage.h"
#include "../utils/Logger.h"
#include "../utils/CredentialManager.h"
#include "../exchange/BinanceAPI.h"

#include <LayoutBuilder.h>
#include <GroupView.h>
#include <StringView.h>
#include <Button.h>
#include <ListView.h>
#include <ScrollView.h>
#include <StringItem.h>
#include <Box.h>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>

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
	BStringView* titleView = new BStringView("", "Trading System Dashboard");
	BFont titleFont(be_bold_font);
	titleFont.SetSize(18);
	titleView->SetFont(&titleFont);

	// Portfolio section
	totalCapitalLabel = new BStringView("", "Total Capital: $0.00");
	availableCashLabel = new BStringView("", "Available Cash: $0.00");
	investedLabel = new BStringView("", "Invested: $0.00");
	totalPnLLabel = new BStringView("", "Total P&L: $0.00");
	totalPnLPercentLabel = new BStringView("", "Total P&L %: 0.00%");

	BFont boldFont(be_plain_font);
	boldFont.SetFace(B_BOLD_FACE);
	totalPnLLabel->SetFont(&boldFont);
	totalPnLPercentLabel->SetFont(&boldFont);

	BBox* portfolioBox = new BBox("portfolio_box");
	portfolioBox->SetLabel("Portfolio Overview");

	BLayoutBuilder::Group<>(portfolioBox, B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(totalCapitalLabel)
		.Add(availableCashLabel)
		.Add(investedLabel)
		.AddStrut(B_USE_SMALL_SPACING)
		.Add(totalPnLLabel)
		.Add(totalPnLPercentLabel)
		.End();

	// System stats section
	recipesCountLabel = new BStringView("", "Recipes: 0");
	backtestsCountLabel = new BStringView("", "Backtest Results: 0");
	candlesCountLabel = new BStringView("", "Candles in Database: 0");

	BBox* statsBox = new BBox("stats_box");
	statsBox->SetLabel("System Statistics");

	BLayoutBuilder::Group<>(statsBox, B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(recipesCountLabel)
		.Add(backtestsCountLabel)
		.Add(candlesCountLabel)
		.End();

	// Binance Portfolio section
	binanceStatusLabel = new BStringView("", "Not connected");
	binanceBalancesView = new BListView("binance_balances");
	binanceBalancesScroll = new BScrollView("binance_scroll", binanceBalancesView,
	                                        0, false, true);
	refreshBinanceButton = new BButton("Refresh", new BMessage(MSG_REFRESH_BINANCE));

	BBox* binanceBox = new BBox("binance_box");
	binanceBox->SetLabel("Binance Portfolio");

	BLayoutBuilder::Group<>(binanceBox, B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(binanceStatusLabel)
		.Add(binanceBalancesScroll)
		.Add(refreshBinanceButton)
		.End();

	// Recent backtests section
	recentBacktestsView = new BListView("recent_backtests");
	recentBacktestsScroll = new BScrollView("recent_scroll", recentBacktestsView,
	                                        0, false, true);

	BBox* recentBox = new BBox("recent_box");
	recentBox->SetLabel("Recent Activity");

	BLayoutBuilder::Group<>(recentBox, B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(recentBacktestsScroll)
		.End();

	// Action buttons
	runBacktestButton = new BButton("Run Backtest", new BMessage(MSG_RUN_BACKTEST));

	// Main layout
	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_WINDOW_SPACING)
		.Add(titleView)
		.AddStrut(B_USE_DEFAULT_SPACING)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.Add(portfolioBox, 1)
			.Add(statsBox, 1)
		.End()
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.Add(binanceBox, 1)
			.Add(recentBox, 1)
		.End()
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

	// Format values
	std::ostringstream oss;

	oss.str("");
	oss << "Total Capital: $" << std::fixed << std::setprecision(2) << currentCapital;
	totalCapitalLabel->SetText(oss.str().c_str());

	oss.str("");
	oss << "Available Cash: $" << std::fixed << std::setprecision(2) << availableCash;
	availableCashLabel->SetText(oss.str().c_str());

	oss.str("");
	oss << "Invested: $" << std::fixed << std::setprecision(2) << invested;
	investedLabel->SetText(oss.str().c_str());

	oss.str("");
	oss << "Total P&L: $" << std::fixed << std::setprecision(2) << pnl;
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
	oss << "Recipes: " << recipeCount;
	recipesCountLabel->SetText(oss.str().c_str());

	// Count candles in database
	DataStorage storage;
	int candleCount = 0;
	if (storage.init("/boot/home/Emiglio/data/emilio.db")) {
		candleCount = storage.getCandleCount("binance", "", "");
	}

	oss.str("");
	oss << "Candles in Database: " << candleCount;
	candlesCountLabel->SetText(oss.str().c_str());

	oss.str("");
	oss << "Backtest Results: N/A (not stored yet)";
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
	binanceBalancesView->MakeEmpty();

	// Check if credentials exist
	if (!credentialManager->hasCredentials("binance")) {
		binanceStatusLabel->SetText("⚠️ Not configured");
		binanceStatusLabel->SetHighColor(200, 100, 0); // Orange
		binanceBalancesView->AddItem(new BStringItem("No Binance API credentials configured."));
		binanceBalancesView->AddItem(new BStringItem(""));
		binanceBalancesView->AddItem(new BStringItem("Go to Settings tab to configure your API keys."));
		return;
	}

	// Load credentials
	std::string apiKey, apiSecret;
	if (!credentialManager->loadCredentials("binance", apiKey, apiSecret)) {
		binanceStatusLabel->SetText("⚠️ Failed to load credentials");
		binanceStatusLabel->SetHighColor(200, 0, 0); // Red
		binanceBalancesView->AddItem(new BStringItem("Failed to decrypt credentials."));
		LOG_ERROR("Failed to load Binance credentials: " + credentialManager->getLastError());
		return;
	}

	// Initialize Binance API
	if (!binanceAPI->init(apiKey, apiSecret)) {
		binanceStatusLabel->SetText("⚠️ API initialization failed");
		binanceStatusLabel->SetHighColor(200, 0, 0); // Red
		binanceBalancesView->AddItem(new BStringItem("Failed to initialize Binance API."));
		LOG_ERROR("Failed to initialize BinanceAPI");
		return;
	}

	binanceStatusLabel->SetText("🔄 Loading...");
	binanceStatusLabel->SetHighColor(0, 0, 0); // Black
	binanceStatusLabel->Invalidate();

	// Fetch balances
	std::vector<Balance> balances = binanceAPI->getBalances();

	if (balances.empty()) {
		binanceStatusLabel->SetText("ℹ️ No balances");
		binanceStatusLabel->SetHighColor(0, 100, 200); // Blue
		binanceBalancesView->AddItem(new BStringItem("No cryptocurrency holdings found."));
		binanceBalancesView->AddItem(new BStringItem(""));
		binanceBalancesView->AddItem(new BStringItem("Your Binance account appears to be empty."));
		LOG_INFO("No balances found in Binance account");
		return;
	}

	// Sort balances by total value (descending)
	std::sort(balances.begin(), balances.end(),
	          [](const Balance& a, const Balance& b) {
		          return a.total > b.total;
	          });

	// Display balances
	binanceStatusLabel->SetText("✓ Connected");
	binanceStatusLabel->SetHighColor(0, 150, 0); // Green

	// Add header
	binanceBalancesView->AddItem(new BStringItem("Asset Holdings:"));
	binanceBalancesView->AddItem(new BStringItem(""));

	// Display each balance
	for (const auto& balance : balances) {
		std::ostringstream item;
		item << std::fixed << std::setprecision(8);

		// Format: ASSET: 123.45678900 (Free: 100.00, Locked: 23.45678900)
		item << balance.asset << ": " << balance.total;

		if (balance.locked > 0) {
			item << std::setprecision(8);
			item << " (Free: " << balance.free << ", Locked: " << balance.locked << ")";
		}

		binanceBalancesView->AddItem(new BStringItem(item.str().c_str()));
	}

	// Add summary
	binanceBalancesView->AddItem(new BStringItem(""));
	std::ostringstream summary;
	summary << "Total: " << balances.size() << " different assets";
	binanceBalancesView->AddItem(new BStringItem(summary.str().c_str()));

	LOG_INFO("Loaded " + std::to_string(balances.size()) + " Binance balances");
}

} // namespace UI
} // namespace Emiglio
