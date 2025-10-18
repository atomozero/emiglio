#ifndef EMIGLIO_DASHBOARDVIEW_H
#define EMIGLIO_DASHBOARDVIEW_H

#include <View.h>
#include <StringView.h>
#include <MessageRunner.h>
#include <string>
#include <memory>

class BListView;
class BScrollView;
class BButton;

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

private:
	void BuildLayout();
	void LoadRecentBacktests();
	void LoadPortfolioStats();
	void LoadBinancePortfolio();

	// Portfolio stats (paper trading/backtest)
	BStringView* totalCapitalLabel;
	BStringView* availableCashLabel;
	BStringView* investedLabel;
	BStringView* totalPnLLabel;
	BStringView* totalPnLPercentLabel;

	// Binance portfolio
	BStringView* binanceStatusLabel;
	BListView* binanceBalancesView;
	BScrollView* binanceBalancesScroll;
	BButton* refreshBinanceButton;

	// System stats
	BStringView* recipesCountLabel;
	BStringView* backtestsCountLabel;
	BStringView* candlesCountLabel;

	// Recent backtests list
	BListView* recentBacktestsView;
	BScrollView* recentBacktestsScroll;

	// Buttons
	BButton* runBacktestButton;

	// Auto-refresh
	BMessageRunner* autoRefreshRunner;

	// Binance API
	std::unique_ptr<CredentialManager> credentialManager;
	std::unique_ptr<BinanceAPI> binanceAPI;

	enum {
		MSG_AUTO_REFRESH = 'arfr',
		MSG_RUN_BACKTEST = 'rbkt',
		MSG_REFRESH_BINANCE = 'rfbn'
	};
};

} // namespace UI
} // namespace Emiglio

#endif // EMIGLIO_DASHBOARDVIEW_H
