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
	void _BuildLayout();
	void _LoadRecentBacktests();
	void _LoadPortfolioStats();
	void _LoadRealPortfolio();
	void _LoadRealPortfolioSummary();
	void _LoadBinancePortfolio();

	// Portfolio stats (paper trading/backtest) - SIMULATED
	BStringView* fTotalCapitalLabel;
	BStringView* fAvailableCashLabel;
	BStringView* fInvestedLabel;
	BStringView* fTotalPnLLabel;
	BStringView* fTotalPnLPercentLabel;
	BStringView* fWinRateLabel;
	BStringView* fMaxDrawdownLabel;
	BStringView* fOpenPositionsLabel;

	// Real Portfolio stats (from actual exchange accounts)
	BStringView* fRealCapitalLabel;
	BStringView* fRealCashLabel;
	BStringView* fRealInvestedLabel;
	BStringView* fRealPnLLabel;
	BStringView* fRealPnLPercentLabel;

	// Real Portfolio Summary (aggregated across all exchanges)
	BStringView* fRealTotalValueLabel;
	BStringView* fRealExchangeCountLabel;
	BStringView* fRealLastUpdateLabel;

	// Binance portfolio (detailed)
	BStringView* fBinanceStatusLabel;
	BStringView* fBinanceTotalValueLabel;
	BColumnListView* fBinanceBalancesView;
	BScrollView* fBinanceBalancesScroll;
	BButton* fRefreshBinanceButton;

	// System stats
	BStringView* fRecipesCountLabel;
	BStringView* fBacktestsCountLabel;
	BStringView* fCandlesCountLabel;
	BStringView* fAppVersionLabel;

	// Recent backtests tables (split by type)
	BColumnListView* fSimulatedBacktestsView;
	BScrollView* fSimulatedBacktestsScroll;
	BColumnListView* fRealBacktestsView;
	BScrollView* fRealBacktestsScroll;

	// Buttons
	BButton* fRunBacktestButton;

	// Auto-refresh
	BMessageRunner* fAutoRefreshRunner;

	// Binance API
	std::unique_ptr<CredentialManager> fCredentialManager;
	std::unique_ptr<BinanceAPI> fBinanceAPI;

	// Database storage (shared instance)
	std::unique_ptr<DataStorage> fDataStorage;

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
