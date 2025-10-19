#ifndef EMIGLIO_DASHBOARDVIEW_H
#define EMIGLIO_DASHBOARDVIEW_H

#include <View.h>
#include <StringView.h>
#include <MessageRunner.h>
#include <ListView.h>
#include <string>
#include <memory>
#include "../data/DataStorage.h"

class BScrollView;
class BButton;
class BColumnListView;

namespace Emiglio {

// Forward declarations
class CredentialManager;
class BinanceAPI;

namespace UI {

// Dashboard view - shows overview of system
class DashboardView : public BView {
public:
	DashboardView();
	virtual ~DashboardView();

	virtual void AttachedToWindow() override;
	virtual void DetachedFromWindow() override;
	virtual void MessageReceived(BMessage* message) override;

	// Public method to refresh dashboard data
	void RefreshData();

	// Public message constants (for other views to send messages)
	enum {
		MSG_SETTINGS_CHANGED = 'stch'
	};

private:
	void BuildLayout();
	void LoadRecentBacktests();
	void LoadPortfolioStats();
	void LoadRealPortfolioSummary();
	void LoadBinancePortfolio();

	// Portfolio stats (paper trading/backtest) - SIMULATED
	BStringView* totalCapitalLabel;
	BStringView* availableCashLabel;
	BStringView* investedLabel;
	BStringView* totalPnLLabel;
	BStringView* totalPnLPercentLabel;

	// Real Portfolio Summary (aggregated across all exchanges)
	BStringView* realTotalValueLabel;
	BStringView* realExchangeCountLabel;
	BStringView* realLastUpdateLabel;

	// Binance portfolio (detailed)
	BStringView* binanceStatusLabel;
	BStringView* binanceTotalValueLabel;
	BColumnListView* binanceBalancesView;
	BScrollView* binanceBalancesScroll;
	BButton* refreshBinanceButton;

	// System stats
	BStringView* recipesCountLabel;
	BStringView* backtestsCountLabel;
	BStringView* candlesCountLabel;

	// Recent backtests tables (split by type)
	BColumnListView* simulatedBacktestsView;
	BScrollView* simulatedBacktestsScroll;
	BColumnListView* realBacktestsView;
	BScrollView* realBacktestsScroll;

	// Buttons
	BButton* runBacktestButton;

	// Auto-refresh
	BMessageRunner* autoRefreshRunner;

	// Binance API
	std::unique_ptr<CredentialManager> credentialManager;
	std::unique_ptr<BinanceAPI> binanceAPI;

	// Database storage (shared instance)
	DataStorage* dataStorage;

	// Private message constants (for internal use)
	enum {
		MSG_AUTO_REFRESH = 'arfr',
		MSG_RUN_BACKTEST = 'rbkt',
		MSG_REFRESH_BINANCE = 'rfbn'
	};
};

} // namespace UI
} // namespace Emiglio

#endif // EMIGLIO_DASHBOARDVIEW_H
