#include "TradesView.h"
#include "ChartsView.h"
#include "../utils/Logger.h"

#include <LayoutBuilder.h>
#include <StringView.h>
#include <Button.h>
#include <ListView.h>
#include <ScrollView.h>
#include <StringItem.h>
#include <Box.h>
#include <Alert.h>

namespace Emiglio {
namespace UI {

TradesView::TradesView()
	: BView("Trades", B_WILL_DRAW)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	BuildLayout();
	LoadTradeHistory();
}

TradesView::~TradesView() {
}

void TradesView::AttachedToWindow() {
	BView::AttachedToWindow();

	if (exportButton) exportButton->SetTarget(this);
}

void TradesView::BuildLayout() {
	// Title
	BStringView* titleView = new BStringView("", "Trade History & Charts");
	BFont titleFont(be_bold_font);
	titleFont.SetSize(16);
	titleView->SetFont(&titleFont);

	// Trade list
	tradesListView = new BListView("trades_list");
	tradesScroll = new BScrollView("trades_scroll", tradesListView,
	                                0, false, true);

	// Stats label
	statsLabel = new BStringView("", "No trades to display");

	// Export button
	exportButton = new BButton("Export CSV", new BMessage(MSG_EXPORT));

	// Create ChartsView
	chartsView = new ChartsView();

	// Main layout - Chart on top, trades list below
	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_WINDOW_SPACING)
		.Add(titleView)
		.Add(statsLabel)
		.Add(chartsView, 3)
		.AddGroup(B_VERTICAL, B_USE_SMALL_SPACING, 1)
			.AddGroup(B_HORIZONTAL)
				.Add(new BStringView("", "Recent Trades"))
				.AddGlue()
				.Add(exportButton)
			.End()
			.Add(tradesScroll)
		.End()
		.End();
}

void TradesView::MessageReceived(BMessage* message) {
	switch (message->what) {
		case MSG_EXPORT:
		{
			BAlert* alert = new BAlert("Export",
			                           "CSV export not yet implemented.\n"
			                           "Would export trade history to CSV file.",
			                           "OK", nullptr, nullptr,
			                           B_WIDTH_AS_USUAL, B_INFO_ALERT);
			alert->Go();
			break;
		}

		case MSG_SHOW_CHART:
			ShowChart();
			break;

		default:
			BView::MessageReceived(message);
			break;
	}
}

void TradesView::LoadTradeHistory() {
	tradesListView->MakeEmpty();

	// For now, show helpful message about where trades come from
	// Full implementation would query DataStorage trades table
	// Show most recent items FIRST (inverted order)
	tradesListView->AddItem(new BStringItem("Trade format example:"));
	tradesListView->AddItem(new BStringItem("  [2024-01-16 14:20] SELL BTCUSDT @ $43,200 | P&L: +$70 (+1.65%)"));
	tradesListView->AddItem(new BStringItem("  [2024-01-15 10:30] BUY BTCUSDT @ $42,500"));
	tradesListView->AddItem(new BStringItem(""));
	tradesListView->AddItem(new BStringItem("Future enhancement: Trades will be stored in database"));
	tradesListView->AddItem(new BStringItem("and displayed here with full history."));
	tradesListView->AddItem(new BStringItem(""));
	tradesListView->AddItem(new BStringItem("To see trades:"));
	tradesListView->AddItem(new BStringItem("1. Go to Backtest tab"));
	tradesListView->AddItem(new BStringItem("2. Run a backtest"));
	tradesListView->AddItem(new BStringItem("3. Check 'Trades' section in results"));
	tradesListView->AddItem(new BStringItem(""));
	tradesListView->AddItem(new BStringItem("Trade History"));

	statsLabel->SetText("Run a backtest to see trade statistics");

	LOG_INFO("Trade history view ready");
}

void TradesView::ShowChart() {
	BAlert* alert = new BAlert("Chart",
	                           "Chart visualization not yet implemented.\n"
	                           "Would show interactive candlestick chart.",
	                           "OK", nullptr, nullptr,
	                           B_WIDTH_AS_USUAL, B_INFO_ALERT);
	alert->Go();
}

} // namespace UI
} // namespace Emiglio
