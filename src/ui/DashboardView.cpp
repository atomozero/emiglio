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
	, fAutoRefreshRunner(nullptr)
	, fCredentialManager(std::make_unique<CredentialManager>())
	, fBinanceAPI(std::make_unique<BinanceAPI>())
	, fDataStorage(std::make_unique<DataStorage>())
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	// Initialize credential manager
	if (!fCredentialManager->init("/boot/home/Emiglio/data/emilio.db")) {
		LOG_ERROR("Failed to initialize CredentialManager in Dashboard");
	}

	// Initialize shared database storage
	if (!fDataStorage->init("/boot/home/Emiglio/data/emilio.db")) {
		LOG_ERROR("Failed to initialize DataStorage in Dashboard");
		fDataStorage.reset();
	}

	_BuildLayout();
	// Don't auto-refresh on construction - wait for AttachedToWindow
}

DashboardView::~DashboardView() {
	delete fAutoRefreshRunner;
	// fDataStorage is now a unique_ptr and will be automatically cleaned up
}

void DashboardView::AttachedToWindow() {
	BView::AttachedToWindow();

	if (fRunBacktestButton) fRunBacktestButton->SetTarget(this);
	if (fRefreshBinanceButton) fRefreshBinanceButton->SetTarget(this);

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
	delete fAutoRefreshRunner;
	fAutoRefreshRunner = nullptr;

	BView::DetachedFromWindow();
}

void DashboardView::_BuildLayout() {
	// Get user's preferred currency symbol
	Config& config = Config::getInstance();
	std::string currency = config.getCurrency();
	std::string currencySymbol = config.getCurrencySymbol();
	LOG_INFO("DashboardView::BuildLayout() - Currency: " + currency + ", Symbol: " + currencySymbol);

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
	simulatedBox->SetLabel("Backtest Portfolio");

	BStringView* simModeLabel = new BStringView("", "Source: Historical Simulations");
	simModeLabel->SetFont(&labelFont);
	rgb_color mutedColor = tint_color(ui_color(B_PANEL_TEXT_COLOR), B_LIGHTEN_1_TINT);
	simModeLabel->SetHighColor(mutedColor);

	fTotalCapitalLabel = new BStringView("", "Capital: Loading...");
	fAvailableCashLabel = new BStringView("", "Cash: Loading...");
	fInvestedLabel = new BStringView("", "Invested: Loading...");

	fTotalCapitalLabel->SetFont(&valueFont);
	fAvailableCashLabel->SetFont(&valueFont);
	fInvestedLabel->SetFont(&valueFont);

	fTotalPnLLabel = new BStringView("", "P&L: Loading...");
	fTotalPnLPercentLabel = new BStringView("", "Total P&L %: Loading...");
	fTotalPnLLabel->SetFont(&bigValueFont);
	fTotalPnLPercentLabel->SetFont(&valueFont);

	fWinRateLabel = new BStringView("", "Win Rate: Loading...");
	fMaxDrawdownLabel = new BStringView("", "Max Drawdown: Loading...");
	fOpenPositionsLabel = new BStringView("", "Open Positions: Loading...");
	fWinRateLabel->SetFont(&valueFont);
	fMaxDrawdownLabel->SetFont(&valueFont);
	fOpenPositionsLabel->SetFont(&valueFont);

	BLayoutBuilder::Group<>(simulatedBox, B_VERTICAL, 4)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(simModeLabel)
		.AddStrut(2)
		.Add(fTotalCapitalLabel)
		.Add(fAvailableCashLabel)
		.Add(fInvestedLabel)
		.AddStrut(4)
		.Add(fTotalPnLLabel)
		.Add(fTotalPnLPercentLabel)
		.AddStrut(4)
		.Add(fWinRateLabel)
		.Add(fMaxDrawdownLabel)
		.Add(fOpenPositionsLabel)
		.AddGlue()
		.End();

	// ========== LIVE PORTFOLIO BOX ==========
	BBox* realBox = new BBox("real_box");
	realBox->SetLabel("Live Portfolio");

	BStringView* realModeLabel = new BStringView("", "Source: Exchange Account (Real Money)");
	realModeLabel->SetFont(&labelFont);
	realModeLabel->SetHighColor(mutedColor);

	fRealCapitalLabel = new BStringView("", ("Capital: " + currencySymbol + "0.00").c_str());
	fRealCashLabel = new BStringView("", ("Cash: " + currencySymbol + "0.00").c_str());
	fRealInvestedLabel = new BStringView("", ("Invested: " + currencySymbol + "0.00").c_str());

	fRealCapitalLabel->SetFont(&bigValueFont);
	fRealCashLabel->SetFont(&valueFont);
	fRealInvestedLabel->SetFont(&valueFont);

	fRealPnLLabel = new BStringView("", ("P&L: " + currencySymbol + "0.00").c_str());
	fRealPnLPercentLabel = new BStringView("", "Total P&L %: 0.00%");

	fRealPnLLabel->SetFont(&valueFont);
	fRealPnLPercentLabel->SetFont(&valueFont);

	BLayoutBuilder::Group<>(realBox, B_VERTICAL, 4)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(realModeLabel)
		.AddStrut(2)
		.Add(fRealCapitalLabel)
		.Add(fRealCashLabel)
		.Add(fRealInvestedLabel)
		.AddStrut(4)
		.Add(fRealPnLLabel)
		.Add(fRealPnLPercentLabel)
		.AddGlue()
		.End();

	// ========== SYSTEM STATUS BOX ==========
	BBox* systemBox = new BBox("system_box");
	systemBox->SetLabel("System Status");

	fRecipesCountLabel = new BStringView("", "Strategies: Loading...");
	fCandlesCountLabel = new BStringView("", "Data Points: Loading...");
	fBacktestsCountLabel = new BStringView("", "Backtest Results: Loading...");
	fAppVersionLabel = new BStringView("", "Version: Loading...");

	fRecipesCountLabel->SetFont(&valueFont);
	fCandlesCountLabel->SetFont(&valueFont);
	fBacktestsCountLabel->SetFont(&valueFont);
	fAppVersionLabel->SetFont(&valueFont);

	BLayoutBuilder::Group<>(systemBox, B_VERTICAL, 4)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(fRecipesCountLabel)
		.Add(fCandlesCountLabel)
		.Add(fBacktestsCountLabel)
		.Add(fAppVersionLabel)
		.AddGlue()
		.End();

	// ========== EXCHANGE ACCOUNT BOX ==========
	BBox* liveBox = new BBox("live_box");
	liveBox->SetLabel("Exchange Account Details");

	fBinanceStatusLabel = new BStringView("", "Status: Not connected");
	fBinanceStatusLabel->SetFont(&labelFont);
	fBinanceStatusLabel->SetHighColor(mutedColor);

	fRealTotalValueLabel = new BStringView("", ("Total: " + currencySymbol + "0.00").c_str());
	fRealTotalValueLabel->SetFont(&bigValueFont);

	fRealExchangeCountLabel = new BStringView("", "Exchanges: 0");
	fRealLastUpdateLabel = new BStringView("", "Last Update: Never");
	fRealExchangeCountLabel->SetFont(&valueFont);
	fRealLastUpdateLabel->SetFont(&labelFont);
	fRealLastUpdateLabel->SetHighColor(mutedColor);

	// Balances table (compact) with value column
	fBinanceBalancesView = new BColumnListView("binance_balances", B_WILL_DRAW, B_FANCY_BORDER, true);
	fBinanceBalancesView->AddColumn(new BStringColumn("Asset", 70, 50, 90, B_TRUNCATE_END), 0);
	fBinanceBalancesView->AddColumn(new BStringColumn("Total", 90, 70, 120, B_TRUNCATE_END), 1);
	fBinanceBalancesView->AddColumn(new BStringColumn("Free", 90, 70, 120, B_TRUNCATE_END), 2);
	fBinanceBalancesView->AddColumn(new BStringColumn("Locked", 80, 60, 110, B_TRUNCATE_END), 3);
	fBinanceBalancesView->AddColumn(new BStringColumn(("Value " + currencySymbol).c_str(), 100, 80, 140, B_TRUNCATE_END), 4);
	fBinanceBalancesView->SetExplicitMinSize(BSize(B_SIZE_UNSET, 80));
	fBinanceBalancesView->SetExplicitMaxSize(BSize(B_SIZE_UNSET, 100));
	fBinanceBalancesView->SetExplicitPreferredSize(BSize(B_SIZE_UNSET, 90));

	fBinanceBalancesScroll = new BScrollView("binance_scroll", fBinanceBalancesView,
	                                        0, false, true);

	fRefreshBinanceButton = new BButton("Refresh Binance", new BMessage(MSG_REFRESH_BINANCE));

	fBinanceTotalValueLabel = new BStringView("", "");
	fBinanceTotalValueLabel->SetFont(&valueFont);

	BLayoutBuilder::Group<>(liveBox, B_VERTICAL, 3)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(fBinanceStatusLabel)
		.AddStrut(2)
		.Add(fRealTotalValueLabel)
		.Add(fRealExchangeCountLabel)
		.Add(fRealLastUpdateLabel)
		.AddStrut(4)
		.Add(fBinanceBalancesScroll)
		.AddStrut(2)
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(fRefreshBinanceButton)
		.End()
		.End();

	// ========== RECENT BACKTESTS - SIMULATED ==========
	BBox* simulatedBacktestsBox = new BBox("simulated_backtests_box");
	simulatedBacktestsBox->SetLabel("Recent Backtests - Simulated");

	fSimulatedBacktestsView = new BColumnListView("simulated_backtests", B_WILL_DRAW, B_FANCY_BORDER);
	fSimulatedBacktestsView->AddColumn(new BStringColumn("Strategy", 110, 80, 160, B_TRUNCATE_END), 0);
	fSimulatedBacktestsView->AddColumn(new BStringColumn("Symbol", 70, 50, 100, B_TRUNCATE_END), 1);
	fSimulatedBacktestsView->AddColumn(new BStringColumn("Return %", 70, 60, 90, B_TRUNCATE_END), 2);
	fSimulatedBacktestsView->AddColumn(new BStringColumn("Sharpe", 60, 50, 80, B_TRUNCATE_END), 3);
	fSimulatedBacktestsView->AddColumn(new BStringColumn("Trades", 60, 50, 80, B_TRUNCATE_END), 4);
	fSimulatedBacktestsView->AddColumn(new BStringColumn("Date", 85, 70, 100, B_TRUNCATE_END), 5);
	fSimulatedBacktestsView->SetExplicitMinSize(BSize(B_SIZE_UNSET, 100));
	fSimulatedBacktestsView->SetExplicitMaxSize(BSize(B_SIZE_UNSET, 140));
	fSimulatedBacktestsView->SetExplicitPreferredSize(BSize(B_SIZE_UNSET, 120));

	fSimulatedBacktestsScroll = new BScrollView("simulated_scroll", fSimulatedBacktestsView,
	                                           0, false, true);

	fRunBacktestButton = new BButton("New Backtest", new BMessage(MSG_RUN_BACKTEST));

	BLayoutBuilder::Group<>(simulatedBacktestsBox, B_VERTICAL, 3)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(fSimulatedBacktestsScroll)
		.AddStrut(2)
		.AddGroup(B_HORIZONTAL)
			.Add(fRunBacktestButton)
			.AddGlue()
		.End()
		.End();

	// ========== RECENT BACKTESTS - REAL TRADING ==========
	BBox* realBacktestsBox = new BBox("real_backtests_box");
	realBacktestsBox->SetLabel("Recent Backtests - Real Trading");

	fRealBacktestsView = new BColumnListView("real_backtests", B_WILL_DRAW, B_FANCY_BORDER);
	fRealBacktestsView->AddColumn(new BStringColumn("Strategy", 110, 80, 160, B_TRUNCATE_END), 0);
	fRealBacktestsView->AddColumn(new BStringColumn("Symbol", 70, 50, 100, B_TRUNCATE_END), 1);
	fRealBacktestsView->AddColumn(new BStringColumn("Return %", 70, 60, 90, B_TRUNCATE_END), 2);
	fRealBacktestsView->AddColumn(new BStringColumn("Sharpe", 60, 50, 80, B_TRUNCATE_END), 3);
	fRealBacktestsView->AddColumn(new BStringColumn("Trades", 60, 50, 80, B_TRUNCATE_END), 4);
	fRealBacktestsView->AddColumn(new BStringColumn("Date", 85, 70, 100, B_TRUNCATE_END), 5);
	fRealBacktestsView->SetExplicitMinSize(BSize(B_SIZE_UNSET, 100));
	fRealBacktestsView->SetExplicitMaxSize(BSize(B_SIZE_UNSET, 140));
	fRealBacktestsView->SetExplicitPreferredSize(BSize(B_SIZE_UNSET, 120));

	fRealBacktestsScroll = new BScrollView("real_scroll", fRealBacktestsView,
	                                      0, false, true);

	BLayoutBuilder::Group<>(realBacktestsBox, B_VERTICAL, 3)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(fRealBacktestsScroll)
		.End();

	// ========== MAIN LAYOUT (ORGANIZED BY TYPE) ==========
	BLayoutBuilder::Group<>(this, B_VERTICAL, 6)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(titleView)
		.Add(subtitleView)
		.AddStrut(4)
		// Row 1: Backtest/Live Portfolios (left) + Live Portfolio/Exchange (right)
		.AddGroup(B_HORIZONTAL, 6)
			// Left column: Backtest + Simulated Backtests sections
			.AddGroup(B_VERTICAL, 6)
				.Add(simulatedBox, 1.0f)  // Backtest Portfolio
				.Add(simulatedBacktestsBox, 2.0f)  // Simulated Backtests
			.End()
			// Right column: Live Portfolio + Exchange Account + Real Backtests
			.AddGroup(B_VERTICAL, 6)
				.Add(realBox, 1.0f)        // Live Portfolio
				.Add(liveBox, 1.0f)        // Exchange Account Details
				.Add(realBacktestsBox, 2.0f)  // Real Backtests
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
			// Load all real portfolio data (balances, summary, stats)
			_LoadBinancePortfolio();
			_LoadRealPortfolio();
			_LoadRealPortfolioSummary();
			break;

		case MSG_SETTINGS_CHANGED:
			// Settings changed (e.g., currency preference)
			// Refresh all data with new settings
			LOG_INFO("Settings changed - refreshing dashboard");
			RefreshData();
			// Also refresh real portfolio to update currency symbols
			_LoadRealPortfolio();
			_LoadRealPortfolioSummary();
			break;

		default:
			BView::MessageReceived(message);
			break;
	}
}

void DashboardView::RefreshData() {
	// Reload configuration to get latest settings (including currency)
	Config& config = Config::getInstance();
	std::string configPath = config.getConfigDir() + "/config.json";
	config.load(configPath);

	_LoadPortfolioStats();
	_LoadRecentBacktests();
	// REMOVED: LoadBinancePortfolio() and LoadRealPortfolioSummary()
	// These make blocking network calls and freeze the UI
	// User must click "Refresh" button manually to load Binance data
}

void DashboardView::_LoadPortfolioStats() {
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
	if (fDataStorage) {
		std::vector<BacktestResult> results = fDataStorage->getAllBacktestResults();
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

	// Convert from USD to user's preferred currency (display only)
	double convertedCapital = config.convertFromUSD(currentCapital);
	double convertedCash = config.convertFromUSD(availableCash);
	double convertedInvested = config.convertFromUSD(invested);
	double convertedPnL = config.convertFromUSD(pnl);

	std::ostringstream oss;

	oss.str("");
	oss << "Capital: " << currencySymbol << std::fixed << std::setprecision(2) << convertedCapital;
	fTotalCapitalLabel->SetText(oss.str().c_str());

	oss.str("");
	oss << "Cash: " << currencySymbol << std::fixed << std::setprecision(2) << convertedCash;
	fAvailableCashLabel->SetText(oss.str().c_str());

	oss.str("");
	oss << "Invested: " << currencySymbol << std::fixed << std::setprecision(2) << convertedInvested;
	fInvestedLabel->SetText(oss.str().c_str());

	oss.str("");
	oss << "P&L: " << currencySymbol << std::fixed << std::setprecision(2) << convertedPnL;
	fTotalPnLLabel->SetText(oss.str().c_str());

	// Color code P&L
	if (pnl > 0) {
		fTotalPnLLabel->SetHighColor(0, 150, 0); // Green
		fTotalPnLPercentLabel->SetHighColor(0, 150, 0);
	} else if (pnl < 0) {
		fTotalPnLLabel->SetHighColor(200, 0, 0); // Red
		fTotalPnLPercentLabel->SetHighColor(200, 0, 0);
	}

	oss.str("");
	oss << "Total P&L %: " << std::fixed << std::setprecision(2) << pnlPercent << "%";
	fTotalPnLPercentLabel->SetText(oss.str().c_str());

	// Display real performance metrics
	oss.str("");
	oss << "Win Rate: " << std::fixed << std::setprecision(1) << totalWinRate << "%";
	fWinRateLabel->SetText(oss.str().c_str());

	oss.str("");
	oss << "Max Drawdown: " << std::fixed << std::setprecision(2) << worstMaxDrawdown << "%";
	fMaxDrawdownLabel->SetText(oss.str().c_str());

	oss.str("");
	oss << "Open Positions: " << totalOpenPositions;
	fOpenPositionsLabel->SetText(oss.str().c_str());

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
	fRecipesCountLabel->SetText(oss.str().c_str());

	// Count candles in database (using shared instance)
	int candleCount = 0;
	if (fDataStorage) {
		candleCount = fDataStorage->getCandleCount("binance", "", "");
	}

	oss.str("");
	oss << "Data Points: " << candleCount;
	fCandlesCountLabel->SetText(oss.str().c_str());

	// Use backtestCount already calculated above
	oss.str("");
	oss << "Backtest Results: " << backtestCount;
	fBacktestsCountLabel->SetText(oss.str().c_str());

	// App version from Config (reuse config from above)
	std::string appVersion = config.getString("app.version", "1.0.0");
	oss.str("");
	oss << "Version: " << appVersion;
	fAppVersionLabel->SetText(oss.str().c_str());

	LOG_INFO("Dashboard stats refreshed");
}

void DashboardView::_LoadRealPortfolio() {
	// Load real portfolio data from exchange accounts (Binance, etc.)
	double realCapital = 0.0;
	double realCash = 0.0;
	double realInvested = 0.0;
	double initialCapital = 10000.0; // TODO: Store initial capital from first deposit

	// Get user's preferred currency symbol
	Config& config = Config::getInstance();
	std::string currencySymbol = config.getCurrencySymbol();

	// Try to load Binance balances
	if (fCredentialManager && fCredentialManager->hasCredentials("binance")) {
		std::string apiKey, apiSecret;
		if (fCredentialManager->loadCredentials("binance", apiKey, apiSecret)) {
			if (fBinanceAPI && fBinanceAPI->init(apiKey, apiSecret)) {
				std::vector<Balance> balances = fBinanceAPI->getBalances();

				if (!balances.empty()) {
					// Calculate total capital from all assets
					for (const auto& balance : balances) {
						// For stablecoins, use 1:1 value
						if (balance.asset == "USDT" || balance.asset == "USDC" ||
						    balance.asset == "BUSD" || balance.asset == "USD") {
							realCapital += balance.total;
							realCash += balance.free;
							realInvested += balance.locked;
						}
						// For other assets, we'd need price conversion
						// This is a simplified version - real implementation would
						// query current prices and convert all assets to USD
					}
				}
			}
		}
	}

	// Calculate P&L
	double realPnL = realCapital - initialCapital;
	double realPnLPercent = initialCapital > 0 ? (realPnL / initialCapital) * 100.0 : 0.0;

	// Convert from USD to user's preferred currency (display only)
	double convertedRealCapital = config.convertFromUSD(realCapital);
	double convertedRealCash = config.convertFromUSD(realCash);
	double convertedRealInvested = config.convertFromUSD(realInvested);
	double convertedRealPnL = config.convertFromUSD(realPnL);

	// Format and display values
	std::ostringstream oss;

	oss.str("");
	oss << "Capital: " << currencySymbol << std::fixed << std::setprecision(2) << convertedRealCapital;
	fRealCapitalLabel->SetText(oss.str().c_str());

	oss.str("");
	oss << "Cash: " << currencySymbol << std::fixed << std::setprecision(2) << convertedRealCash;
	fRealCashLabel->SetText(oss.str().c_str());

	oss.str("");
	oss << "Invested: " << currencySymbol << std::fixed << std::setprecision(2) << convertedRealInvested;
	fRealInvestedLabel->SetText(oss.str().c_str());

	oss.str("");
	oss << "P&L: " << currencySymbol << std::fixed << std::setprecision(2) << convertedRealPnL;
	fRealPnLLabel->SetText(oss.str().c_str());

	// Color code P&L
	if (realPnL > 0) {
		fRealPnLLabel->SetHighColor(0, 150, 0); // Green
		fRealPnLPercentLabel->SetHighColor(0, 150, 0);
	} else if (realPnL < 0) {
		fRealPnLLabel->SetHighColor(200, 0, 0); // Red
		fRealPnLPercentLabel->SetHighColor(200, 0, 0);
	}

	oss.str("");
	oss << "Total P&L %: " << std::fixed << std::setprecision(2) << realPnLPercent << "%";
	fRealPnLPercentLabel->SetText(oss.str().c_str());

	LOG_INFO("Real portfolio stats loaded");
}

void DashboardView::_LoadRecentBacktests() {
	fSimulatedBacktestsView->Clear();
	fRealBacktestsView->Clear();

	// Query database for recent backtest results (using shared instance)
	if (!fDataStorage) {
		BRow* simRow = new BRow();
		simRow->SetField(new BStringField("Failed to load results"), 0);
		simRow->SetField(new BStringField(""), 1);
		simRow->SetField(new BStringField(""), 2);
		simRow->SetField(new BStringField(""), 3);
		simRow->SetField(new BStringField(""), 4);
		simRow->SetField(new BStringField(""), 5);
		fSimulatedBacktestsView->AddRow(simRow);

		BRow* realRow = new BRow();
		realRow->SetField(new BStringField("Failed to load results"), 0);
		realRow->SetField(new BStringField(""), 1);
		realRow->SetField(new BStringField(""), 2);
		realRow->SetField(new BStringField(""), 3);
		realRow->SetField(new BStringField(""), 4);
		realRow->SetField(new BStringField(""), 5);
		fRealBacktestsView->AddRow(realRow);

		LOG_WARNING("DataStorage not initialized");
		return;
	}

	std::vector<BacktestResult> results = fDataStorage->getAllBacktestResults();

	if (results.empty()) {
		// Add placeholder row for simulated backtests
		BRow* simRow = new BRow();
		simRow->SetField(new BStringField("No results yet"), 0);
		simRow->SetField(new BStringField(""), 1);
		simRow->SetField(new BStringField(""), 2);
		simRow->SetField(new BStringField(""), 3);
		simRow->SetField(new BStringField(""), 4);
		simRow->SetField(new BStringField(""), 5);
		fSimulatedBacktestsView->AddRow(simRow);

		// Add placeholder row for real trading
		BRow* realRow = new BRow();
		realRow->SetField(new BStringField("No real trading results"), 0);
		realRow->SetField(new BStringField(""), 1);
		realRow->SetField(new BStringField(""), 2);
		realRow->SetField(new BStringField(""), 3);
		realRow->SetField(new BStringField(""), 4);
		realRow->SetField(new BStringField(""), 5);
		fRealBacktestsView->AddRow(realRow);

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
		fSimulatedBacktestsView->AddRow(row);
	}

	// Add placeholder for real trading
	BRow* realRow = new BRow();
	realRow->SetField(new BStringField("No real trading results"), 0);
	realRow->SetField(new BStringField(""), 1);
	realRow->SetField(new BStringField(""), 2);
	realRow->SetField(new BStringField(""), 3);
	realRow->SetField(new BStringField(""), 4);
	realRow->SetField(new BStringField(""), 5);
	fRealBacktestsView->AddRow(realRow);

	// Update count label
	std::ostringstream countStr;
	countStr << "Backtest Results: " << results.size();
	fBacktestsCountLabel->SetText(countStr.str().c_str());

	LOG_INFO("Loaded " + std::to_string(results.size()) + " backtest results");
}

void DashboardView::_LoadBinancePortfolio() {
	// Clear existing rows
	fBinanceBalancesView->Clear();

	// Check if credentials exist
	if (!fCredentialManager->hasCredentials("binance")) {
		fBinanceStatusLabel->SetText("Status: Not configured");
		fBinanceStatusLabel->SetHighColor(200, 100, 0); // Orange
		fBinanceTotalValueLabel->SetText("");

		BRow* row = new BRow();
		row->SetField(new BStringField("N/A"), 0);
		row->SetField(new BStringField("No API credentials configured"), 1);
		row->SetField(new BStringField("Go to Settings tab"), 2);
		row->SetField(new BStringField(""), 3);
		row->SetField(new BStringField(""), 4);
		fBinanceBalancesView->AddRow(row);
		return;
	}

	// Load credentials
	std::string apiKey, apiSecret;
	if (!fCredentialManager->loadCredentials("binance", apiKey, apiSecret)) {
		fBinanceStatusLabel->SetText("Status: Failed to load credentials");
		fBinanceStatusLabel->SetHighColor(200, 0, 0); // Red
		fBinanceTotalValueLabel->SetText("");

		BRow* row = new BRow();
		row->SetField(new BStringField("Error"), 0);
		row->SetField(new BStringField("Failed to decrypt credentials"), 1);
		row->SetField(new BStringField("Check configuration"), 2);
		row->SetField(new BStringField(""), 3);
		row->SetField(new BStringField(""), 4);
		fBinanceBalancesView->AddRow(row);

		LOG_ERROR("Failed to load Binance credentials: " + fCredentialManager->getLastError());
		return;
	}

	// Initialize Binance API
	if (!fBinanceAPI->init(apiKey, apiSecret)) {
		fBinanceStatusLabel->SetText("Status: API initialization failed");
		fBinanceStatusLabel->SetHighColor(200, 0, 0); // Red
		fBinanceTotalValueLabel->SetText("");

		BRow* row = new BRow();
		row->SetField(new BStringField("Error"), 0);
		row->SetField(new BStringField("Failed to initialize Binance API"), 1);
		row->SetField(new BStringField("Check API keys"), 2);
		row->SetField(new BStringField(""), 3);
		row->SetField(new BStringField(""), 4);
		fBinanceBalancesView->AddRow(row);

		LOG_ERROR("Failed to initialize BinanceAPI");
		return;
	}

	fBinanceStatusLabel->SetText("Status: Loading...");
	fBinanceStatusLabel->SetHighColor(0, 0, 0); // Black
	fBinanceTotalValueLabel->SetText("");
	fBinanceStatusLabel->Invalidate();

	// Fetch balances
	std::vector<Balance> balances = fBinanceAPI->getBalances();

	if (balances.empty()) {
		fBinanceStatusLabel->SetText("Status: Connected");
		fBinanceStatusLabel->SetHighColor(0, 150, 0); // Green
		fBinanceTotalValueLabel->SetText("Total Assets: 0");

		BRow* row = new BRow();
		row->SetField(new BStringField("N/A"), 0);
		row->SetField(new BStringField("No holdings found"), 1);
		row->SetField(new BStringField("Account is empty"), 2);
		row->SetField(new BStringField(""), 3);
		row->SetField(new BStringField(""), 4);
		fBinanceBalancesView->AddRow(row);

		LOG_INFO("No balances found in Binance account");
		return;
	}

	// Sort balances by total value (descending)
	std::sort(balances.begin(), balances.end(),
	          [](const Balance& a, const Balance& b) {
		          return a.total > b.total;
	          });

	// Display balances
	fBinanceStatusLabel->SetText("Status: Connected & Loaded");
	fBinanceStatusLabel->SetHighColor(0, 150, 0); // Green

	// Calculate total count
	std::ostringstream totalText;
	totalText << "Total Assets: " << balances.size() << " different cryptocurrencies";
	fBinanceTotalValueLabel->SetText(totalText.str().c_str());

	// Get user's preferred currency for display
	Config& config = Config::getInstance();
	std::string currencySymbol = config.getCurrencySymbol();

	// Display each balance in column list with converted value
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

		// Value in user's preferred currency
		std::ostringstream valueStr;
		if (balance.asset == "USDT" || balance.asset == "USDC" || balance.asset == "BUSD") {
			// Stablecoins are 1:1 with USD
			double convertedValue = config.convertFromUSD(balance.total);
			valueStr << currencySymbol << std::fixed << std::setprecision(2) << convertedValue;
		} else {
			// Try to get price from trading pair
			try {
				Ticker ticker = fBinanceAPI->getTicker(balance.asset + "USDT");
				if (ticker.lastPrice > 0.0) {
					double valueInUSDT = balance.total * ticker.lastPrice;
					double convertedValue = config.convertFromUSD(valueInUSDT);
					valueStr << currencySymbol << std::fixed << std::setprecision(2) << convertedValue;
				} else {
					valueStr << "N/A";
				}
			} catch (...) {
				// Price not available
				valueStr << "N/A";
			}
		}
		row->SetField(new BStringField(valueStr.str().c_str()), 4);

		fBinanceBalancesView->AddRow(row);
	}

	LOG_INFO("Loaded " + std::to_string(balances.size()) + " Binance balances");
}

void DashboardView::_LoadRealPortfolioSummary() {
	// Aggregate real portfolio data across all exchanges
	double totalValue = 0.0;
	int activeExchanges = 0;
	time_t lastUpdate = 0;

	// Check Binance
	if (fCredentialManager->hasCredentials("binance")) {
		std::string apiKey, apiSecret;
		if (fCredentialManager->loadCredentials("binance", apiKey, apiSecret)) {
			if (fBinanceAPI->init(apiKey, apiSecret)) {
				std::vector<Balance> balances = fBinanceAPI->getBalances();
				if (!balances.empty()) {
					activeExchanges++;
					// Convert crypto assets to USD value
					for (const auto& balance : balances) {
						// Stablecoins are 1:1 with USD
						if (balance.asset == "USDT" || balance.asset == "USDC" || balance.asset == "BUSD") {
							totalValue += balance.total;
						} else {
							// For other cryptocurrencies, fetch current price
							try {
								// Try to get price from {ASSET}USDT trading pair
								Ticker ticker = fBinanceAPI->getTicker(balance.asset + "USDT");
								if (ticker.lastPrice > 0.0) {
									double valueInUSDT = balance.total * ticker.lastPrice;
									totalValue += valueInUSDT;
									LOG_DEBUG("DashboardView: " + balance.asset + " value = " + std::to_string(valueInUSDT) + " USDT");
								}
							} catch (...) {
								// Silently skip if ticker not available (e.g., no USDT pair)
								LOG_DEBUG("DashboardView: Could not fetch price for " + balance.asset);
							}
						}
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

	// Convert from USD to user's preferred currency (display only)
	double convertedTotalValue = config.convertFromUSD(totalValue);

	std::ostringstream oss;

	oss.str("");
	oss << "Total: " << currencySymbol << std::fixed << std::setprecision(2) << convertedTotalValue;
	fRealTotalValueLabel->SetText(oss.str().c_str());

	oss.str("");
	oss << "Exchanges: " << activeExchanges;
	if (activeExchanges > 0) {
		oss << " (Binance)";
	}
	fRealExchangeCountLabel->SetText(oss.str().c_str());

	oss.str("");
	if (lastUpdate > 0) {
		char timeStr[64];
		struct tm* timeinfo = localtime(&lastUpdate);
		strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);
		oss << "Last Update: " << timeStr;
	} else {
		oss << "Last Update: Never";
	}
	fRealLastUpdateLabel->SetText(oss.str().c_str());

	LOG_INFO("Real portfolio summary: " + currencySymbol + std::to_string(totalValue) + " across " + std::to_string(activeExchanges) + " exchanges");
}

} // namespace UI
} // namespace Emiglio
