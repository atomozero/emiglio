#ifndef EMIGLIO_LIVETRADINGVIEW_H
#define EMIGLIO_LIVETRADINGVIEW_H

#include <View.h>
#include <StringView.h>
#include <Button.h>
#include <TextControl.h>
#include <MenuField.h>
#include <MessageRunner.h>
#include "../exchange/BinanceWebSocket.h"
#include "../paper/PaperPortfolio.h"
#include <memory>

class BColumnListView;
class BScrollView;
class BBox;

namespace Emiglio {
namespace UI {

// Live Trading View - Real-time trading interface
class LiveTradingView : public BView {
public:
	LiveTradingView();
	virtual ~LiveTradingView();

	virtual void AttachedToWindow() override;
	virtual void DetachedFromWindow() override;
	virtual void MessageReceived(BMessage* message) override;

private:
	void BuildLayout();
	void ConnectWebSocket();
	void DisconnectWebSocket();
	void UpdateTicker(const TickerUpdate& update);
	void UpdateTrade(const TradeUpdate& update);
	void PlaceOrder();
	void UpdatePositionsTable();
	void UpdateBalanceLabels();
	void UpdateTradesBoxLabel();

	// WebSocket
	std::unique_ptr<BinanceWebSocket> webSocket;

	// WebSocket message processor (timer)
	BMessageRunner* wsMessageProcessor;

	// Paper Trading Portfolio
	std::unique_ptr<Paper::PaperPortfolio> paperPortfolio;

	// UI Components - Market data
	BStringView* priceLabel;
	BStringView* changeLabel;
	BStringView* volumeLabel;
	BStringView* highLowLabel;

	// UI Components - Trading
	BMenuField* baseCurrencyMenu;
	BMenuField* quoteCurrencyMenu;
	BMenuField* orderTypeMenu;
	BMenuField* sideMenu;
	BTextControl* quantityControl;
	BTextControl* priceControl;
	BButton* placeOrderButton;
	BButton* connectButton;

	// UI Components - Recent trades list
	BColumnListView* recentTradesView;
	BScrollView* recentTradesScroll;
	BBox* tradesBox;

	// UI Components - Positions list
	BColumnListView* positionsView;
	BScrollView* positionsScroll;

	// Status
	BStringView* connectionStatusLabel;
	BStringView* balanceLabel;
	BStringView* equityLabel;
	BStringView* pnlLabel;

	// Current state
	std::string currentSymbol;
	bool isConnected;
	double lastTradePrice;
	time_t lastTradeTime;

	enum {
		MSG_CONNECT = 'cnct',
		MSG_DISCONNECT = 'disc',
		MSG_PLACE_ORDER = 'plco',
		MSG_UPDATE_TICKER = 'utck',
		MSG_UPDATE_TRADE = 'utrd',
		MSG_SYMBOL_CHANGED = 'smch',
		MSG_PROCESS_WS_MESSAGES = 'pwsm'
	};

	// Helper to safely update UI from worker thread
	void UpdateTickerSafe(const TickerUpdate& update);
	void UpdateTradeSafe(const TradeUpdate& update);
};

} // namespace UI
} // namespace Emiglio

#endif // EMIGLIO_LIVETRADINGVIEW_H
