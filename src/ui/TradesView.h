#ifndef EMIGLIO_TRADESVIEW_H
#define EMIGLIO_TRADESVIEW_H

#include <View.h>
#include <StringView.h>

class BListView;
class BScrollView;
class BButton;

namespace Emiglio {
namespace UI {

class ChartsView; // Forward declaration

// TradesView - Shows trade history and chart visualization
class TradesView : public BView {
public:
	TradesView();
	virtual ~TradesView();

	virtual void AttachedToWindow() override;
	virtual void MessageReceived(BMessage* message) override;

private:
	void BuildLayout();
	void LoadTradeHistory();
	void ShowChart();

	// UI Components
	BListView* tradesListView;
	BScrollView* tradesScroll;
	BButton* exportButton;
	BStringView* statsLabel;
	ChartsView* chartsView;

	enum {
		MSG_EXPORT = 'texp',
		MSG_SHOW_CHART = 'shch'
	};
};

} // namespace UI
} // namespace Emiglio

#endif // EMIGLIO_TRADESVIEW_H
