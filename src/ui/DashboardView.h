#ifndef EMIGLIO_DASHBOARDVIEW_H
#define EMIGLIO_DASHBOARDVIEW_H

#include <View.h>
#include <StringView.h>
#include <MessageRunner.h>
#include <string>

class BListView;
class BScrollView;
class BButton;

namespace Emiglio {
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

	// Portfolio stats
	BStringView* totalCapitalLabel;
	BStringView* availableCashLabel;
	BStringView* investedLabel;
	BStringView* totalPnLLabel;
	BStringView* totalPnLPercentLabel;

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

	enum {
		MSG_AUTO_REFRESH = 'arfr',
		MSG_RUN_BACKTEST = 'rbkt'
	};
};

} // namespace UI
} // namespace Emiglio

#endif // EMIGLIO_DASHBOARDVIEW_H
