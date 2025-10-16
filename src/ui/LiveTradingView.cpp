#include "LiveTradingView.h"
#include "../utils/Logger.h"

#include <LayoutBuilder.h>
#include <GroupView.h>
#include <Box.h>
#include <private/interface/ColumnListView.h>
#include <private/interface/ColumnTypes.h>
#include <ScrollView.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <Alert.h>
#include <Messenger.h>
#include <Looper.h>

#include <sstream>
#include <iomanip>
#include <ctime>

namespace Emiglio {
namespace UI {

// Helper function to split symbol into base and quote
static void SplitSymbol(const std::string& symbol, std::string& base, std::string& quote) {
	// Common quote currencies in order of priority
	const char* quotes[] = {"USDT", "BUSD", "USDC", "EUR", "GBP", "BTC", "ETH", "BNB"};

	for (const char* q : quotes) {
		size_t pos = symbol.find(q);
		if (pos != std::string::npos && pos > 0) {
			base = symbol.substr(0, pos);
			quote = q;
			return;
		}
	}

	// Fallback: assume last 3-4 chars are quote
	if (symbol.length() > 6) {
		base = symbol.substr(0, symbol.length() - 4);
		quote = symbol.substr(symbol.length() - 4);
	} else {
		base = symbol;
		quote = "";
	}
}

// Helper function to get currency symbol
static const char* GetCurrencySymbol(const std::string& currency) {
	if (currency == "EUR") return "€";
	if (currency == "GBP") return "£";
	if (currency == "BTC") return "₿";
	if (currency == "ETH") return "Ξ";
	if (currency == "BNB") return "BNB ";
	if (currency == "USDT" || currency == "BUSD" || currency == "USDC") return "$";
	return "$"; // Default
}

// Custom field that stores color information
class ColoredStringField : public BStringField {
public:
	ColoredStringField(const char* string, rgb_color color)
		: BStringField(string)
		, fBackgroundColor(color)
	{}

	rgb_color GetBackgroundColor() const {
		return fBackgroundColor;
	}

private:
	rgb_color fBackgroundColor;
};

// Custom column that renders with colored fields
class ColoredStringColumn : public BStringColumn {
public:
	ColoredStringColumn(const char* title, float width, float minWidth,
	                    float maxWidth, uint32 truncate)
		: BStringColumn(title, width, minWidth, maxWidth, truncate)
	{}

	virtual void DrawField(BField* field, BRect rect, BView* parent) override {
		// Cast to ColoredStringField
		ColoredStringField* coloredField = dynamic_cast<ColoredStringField*>(field);

		if (coloredField) {
			// Fill background with field color
			parent->SetLowColor(coloredField->GetBackgroundColor());
			parent->FillRect(rect, B_SOLID_LOW);
		}

		// Draw the text
		BStringColumn::DrawField(field, rect, parent);
	}
};

LiveTradingView::LiveTradingView()
	: BView("Live Trading", B_WILL_DRAW)
	, webSocket(nullptr)
	, paperPortfolio(std::make_unique<Paper::PaperPortfolio>(10000.0))
	, currentSymbol("BTCUSDT")
	, isConnected(false)
	, lastTradePrice(0.0)
	, lastTradeTime(0)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	BuildLayout();
}

LiveTradingView::~LiveTradingView() {
	DisconnectWebSocket();
}

void LiveTradingView::AttachedToWindow() {
	BView::AttachedToWindow();

	if (connectButton) connectButton->SetTarget(this);
	if (placeOrderButton) placeOrderButton->SetTarget(this);

	// Set targets for base and quote menus
	if (baseCurrencyMenu && baseCurrencyMenu->Menu()) {
		for (int32 i = 0; i < baseCurrencyMenu->Menu()->CountItems(); i++) {
			BMenuItem* item = baseCurrencyMenu->Menu()->ItemAt(i);
			if (item) item->SetTarget(this);
		}
	}
	if (quoteCurrencyMenu && quoteCurrencyMenu->Menu()) {
		for (int32 i = 0; i < quoteCurrencyMenu->Menu()->CountItems(); i++) {
			BMenuItem* item = quoteCurrencyMenu->Menu()->ItemAt(i);
			if (item) item->SetTarget(this);
		}
	}
}

void LiveTradingView::DetachedFromWindow() {
	DisconnectWebSocket();
	BView::DetachedFromWindow();
}

void LiveTradingView::BuildLayout() {
	// Title
	BStringView* titleView = new BStringView("", "Live Trading");
	BFont titleFont(be_bold_font);
	titleFont.SetSize(18);
	titleView->SetFont(&titleFont);

	// Connection status
	connectionStatusLabel = new BStringView("", "Status: Disconnected");
	balanceLabel = new BStringView("", "Balance: $10,000.00");
	equityLabel = new BStringView("", "Equity: $10,000.00");
	pnlLabel = new BStringView("", "P&L: $0.00 (0.00%)");

	// Market data display
	priceLabel = new BStringView("", "Price: --");
	changeLabel = new BStringView("", "24h Change: --");
	volumeLabel = new BStringView("", "24h Volume: --");
	highLowLabel = new BStringView("", "24h High/Low: --");

	BFont boldFont(be_plain_font);
	boldFont.SetFace(B_BOLD_FACE);
	boldFont.SetSize(14);
	priceLabel->SetFont(&boldFont);

	BBox* marketBox = new BBox("market_box");
	marketBox->SetLabel("Market Data");

	BLayoutBuilder::Group<>(marketBox, B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(priceLabel)
		.Add(changeLabel)
		.Add(volumeLabel)
		.Add(highLowLabel)
		.End();

	// Trading panel - Base and Quote currency selection
	BPopUpMenu* basePopup = new BPopUpMenu("Base");
	const char* baseCurrencies[] = {"BTC", "ETH", "BNB", "SOL", "XRP", "ADA"};
	for (const char* base : baseCurrencies) {
		BMessage* msg = new BMessage(MSG_SYMBOL_CHANGED);
		basePopup->AddItem(new BMenuItem(base, msg));
	}
	basePopup->ItemAt(0)->SetMarked(true);
	baseCurrencyMenu = new BMenuField("Base:", basePopup);

	BPopUpMenu* quotePopup = new BPopUpMenu("Quote");
	const char* quoteCurrencies[] = {"USDT", "EUR", "BTC", "ETH", "BNB", "BUSD"};
	for (const char* quote : quoteCurrencies) {
		BMessage* msg = new BMessage(MSG_SYMBOL_CHANGED);
		quotePopup->AddItem(new BMenuItem(quote, msg));
	}
	quotePopup->ItemAt(0)->SetMarked(true);
	quoteCurrencyMenu = new BMenuField("Quote:", quotePopup);

	BPopUpMenu* sidePopup = new BPopUpMenu("Side");
	sidePopup->AddItem(new BMenuItem("BUY", nullptr));
	sidePopup->AddItem(new BMenuItem("SELL", nullptr));
	sidePopup->ItemAt(0)->SetMarked(true);
	sideMenu = new BMenuField("Side:", sidePopup);

	BPopUpMenu* typePopup = new BPopUpMenu("Type");
	typePopup->AddItem(new BMenuItem("MARKET", nullptr));
	typePopup->AddItem(new BMenuItem("LIMIT", nullptr));
	typePopup->ItemAt(0)->SetMarked(true);
	orderTypeMenu = new BMenuField("Type:", typePopup);

	quantityControl = new BTextControl("Quantity:", "0.001", nullptr);
	priceControl = new BTextControl("Price:", "0.00", nullptr);
	priceControl->SetEnabled(false); // Disabled for MARKET orders

	placeOrderButton = new BButton("Place Order", new BMessage(MSG_PLACE_ORDER));
	connectButton = new BButton("Connect", new BMessage(MSG_CONNECT));

	BBox* tradingBox = new BBox("trading_box");
	tradingBox->SetLabel("Place Order");

	BLayoutBuilder::Group<>(tradingBox, B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.AddGroup(B_HORIZONTAL)
			.Add(baseCurrencyMenu)
			.Add(quoteCurrencyMenu)
		.End()
		.AddGroup(B_HORIZONTAL)
			.Add(sideMenu)
			.Add(orderTypeMenu)
		.End()
		.Add(quantityControl)
		.Add(priceControl)
		.Add(placeOrderButton)
		.End();

	// Recent trades list - BColumnListView with multiple columns
	recentTradesView = new BColumnListView("recent_trades", 0, B_FANCY_BORDER, true);

	// Add columns: Time, Side, Price, Quantity, Total, Spread, Delay (using ColoredStringColumn)
	recentTradesView->AddColumn(new ColoredStringColumn("Time", 80, 50, 120, 0), 0);
	recentTradesView->AddColumn(new ColoredStringColumn("Side", 50, 40, 80, 0), 1);
	recentTradesView->AddColumn(new ColoredStringColumn("Price", 100, 80, 150, 0), 2);
	recentTradesView->AddColumn(new ColoredStringColumn("Quantity", 90, 70, 140, 0), 3);
	recentTradesView->AddColumn(new ColoredStringColumn("Total", 100, 80, 150, 0), 4);
	recentTradesView->AddColumn(new ColoredStringColumn("Spread", 70, 50, 100, 0), 5);
	recentTradesView->AddColumn(new ColoredStringColumn("Delay", 60, 40, 100, 0), 6);

	recentTradesView->SetSortingEnabled(false);

	tradesBox = new BBox("trades_box");
	tradesBox->SetLabel("Recent Trades (Live) - BTCUSDT");

	BLayoutBuilder::Group<>(tradesBox, B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(recentTradesView)
		.End();

	// Positions table
	positionsView = new BColumnListView("positions", 0, B_FANCY_BORDER, true);

	// Add columns: Symbol, Side, Entry Price, Current Price, Quantity, P&L, P&L%
	positionsView->AddColumn(new ColoredStringColumn("Symbol", 80, 60, 120, 0), 0);
	positionsView->AddColumn(new ColoredStringColumn("Side", 60, 50, 80, 0), 1);
	positionsView->AddColumn(new ColoredStringColumn("Entry", 90, 70, 130, 0), 2);
	positionsView->AddColumn(new ColoredStringColumn("Current", 90, 70, 130, 0), 3);
	positionsView->AddColumn(new ColoredStringColumn("Qty", 80, 60, 120, 0), 4);
	positionsView->AddColumn(new ColoredStringColumn("P&L", 100, 80, 150, 0), 5);
	positionsView->AddColumn(new ColoredStringColumn("P&L%", 80, 60, 120, 0), 6);

	positionsView->SetSortingEnabled(false);

	BBox* positionsBox = new BBox("positions_box");
	positionsBox->SetLabel("Open Positions");

	BLayoutBuilder::Group<>(positionsBox, B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(positionsView)
		.End();

	// Main layout
	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_WINDOW_SPACING)
		.Add(titleView)
		.AddGroup(B_HORIZONTAL)
			.Add(connectionStatusLabel)
			.AddGlue()
			.Add(balanceLabel)
			.Add(equityLabel)
			.Add(pnlLabel)
			.Add(connectButton)
		.End()
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.Add(marketBox, 1)
			.Add(tradingBox, 1)
		.End()
		.Add(positionsBox, 1)
		.Add(tradesBox, 2)
		.End();
}

void LiveTradingView::MessageReceived(BMessage* message) {
	switch (message->what) {
		case MSG_CONNECT:
			if (isConnected) {
				DisconnectWebSocket();
			} else {
				ConnectWebSocket();
			}
			break;

		case MSG_PLACE_ORDER:
			PlaceOrder();
			break;

		case MSG_SYMBOL_CHANGED:
		{
			// Rebuild symbol from base and quote
			BMenuItem* baseItem = baseCurrencyMenu ? baseCurrencyMenu->Menu()->FindMarked() : nullptr;
			BMenuItem* quoteItem = quoteCurrencyMenu ? quoteCurrencyMenu->Menu()->FindMarked() : nullptr;

			if (baseItem && quoteItem) {
				std::string newSymbol = std::string(baseItem->Label()) + quoteItem->Label();

				if (newSymbol != currentSymbol) {
					// Save old symbol before updating
					std::string oldSymbol = currentSymbol;
					currentSymbol = newSymbol;
					LOG_INFO("Changed symbol from " + oldSymbol + " to: " + currentSymbol);

					// Update trades box label
					UpdateTradesBoxLabel();

					// Clear recent trades view when switching symbols
					while (recentTradesView->CountRows() > 0) {
						BRow* row = recentTradesView->RowAt(0);
						recentTradesView->RemoveRow(row);
						delete row;
					}
					lastTradePrice = 0.0;
					lastTradeTime = 0;

					// Resubscribe if connected
					if (isConnected && webSocket) {
						// Unsubscribe from OLD symbol
						webSocket->unsubscribeTicker(oldSymbol);
						webSocket->unsubscribeTrades(oldSymbol);

						// Subscribe to NEW symbol
						webSocket->subscribeTicker(currentSymbol,
							[this](const TickerUpdate& update) {
								this->UpdateTickerSafe(update);
							});

						webSocket->subscribeTrades(currentSymbol,
							[this](const TradeUpdate& update) {
								this->UpdateTradeSafe(update);
							});

						// Disconnect and reconnect WebSocket to fully clear old streams
						LOG_INFO("Reconnecting WebSocket to apply symbol change...");
						webSocket->disconnect();

						if (!webSocket->connect()) {
							LOG_ERROR("Failed to reconnect WebSocket");
						} else {
							// Resubscribe after reconnecting
							webSocket->subscribeTicker(currentSymbol,
								[this](const TickerUpdate& update) {
									this->UpdateTickerSafe(update);
								});

							webSocket->subscribeTrades(currentSymbol,
								[this](const TradeUpdate& update) {
									this->UpdateTradeSafe(update);
								});
							LOG_INFO("WebSocket reconnected successfully");
						}
					}
				}
			}
			break;
		}

		case MSG_UPDATE_TICKER:
		{
			// Extract ticker data from message
			TickerUpdate update;
			const char* symbol;
			if (message->FindString("symbol", &symbol) == B_OK &&
			    message->FindDouble("lastPrice", &update.lastPrice) == B_OK &&
			    message->FindDouble("priceChange", &update.priceChange) == B_OK &&
			    message->FindDouble("priceChangePercent", &update.priceChangePercent) == B_OK &&
			    message->FindDouble("volume", &update.volume) == B_OK &&
			    message->FindDouble("highPrice", &update.highPrice) == B_OK &&
			    message->FindDouble("lowPrice", &update.lowPrice) == B_OK)
			{
				update.symbol = symbol;
				UpdateTicker(update);
			}
			break;
		}

		case MSG_UPDATE_TRADE:
		{
			// Extract trade data from message
			TradeUpdate update;
			const char* symbol;
			if (message->FindString("symbol", &symbol) == B_OK &&
			    message->FindDouble("price", &update.price) == B_OK &&
			    message->FindDouble("quantity", &update.quantity) == B_OK &&
			    message->FindBool("isBuyerMaker", &update.isBuyerMaker) == B_OK)
			{
				update.symbol = symbol;
				UpdateTrade(update);
			}
			break;
		}

		default:
			BView::MessageReceived(message);
			break;
	}
}

void LiveTradingView::ConnectWebSocket() {
	LOG_INFO("Connecting to Binance WebSocket...");

	webSocket = std::make_unique<BinanceWebSocket>();

	if (!webSocket->connect()) {
		LOG_ERROR("Failed to connect WebSocket");
		BAlert* alert = new BAlert("Error", "Failed to connect to WebSocket",
		                           "OK", nullptr, nullptr, B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->Go();
		webSocket.reset();
		return;
	}

	// Subscribe to ticker and trades
	webSocket->subscribeTicker(currentSymbol,
		[this](const TickerUpdate& update) {
			this->UpdateTickerSafe(update);
		});

	webSocket->subscribeTrades(currentSymbol,
		[this](const TradeUpdate& update) {
			this->UpdateTradeSafe(update);
		});

	isConnected = true;
	connectionStatusLabel->SetText("Status: Connected");
	connectButton->SetLabel("Disconnect");

	LOG_INFO("WebSocket connected successfully");
}

void LiveTradingView::DisconnectWebSocket() {
	if (!webSocket) return;

	LOG_INFO("Disconnecting WebSocket...");

	webSocket->disconnect();
	webSocket.reset();

	isConnected = false;
	connectionStatusLabel->SetText("Status: Disconnected");
	connectButton->SetLabel("Connect");

	LOG_INFO("WebSocket disconnected");
}

void LiveTradingView::UpdateTicker(const TickerUpdate& update) {
	// Get currency symbol
	std::string base, quote;
	SplitSymbol(currentSymbol, base, quote);
	const char* currencySymbol = GetCurrencySymbol(quote);

	// Update market data labels
	std::ostringstream oss;

	oss.str("");
	oss << "Price: " << currencySymbol << std::fixed << std::setprecision(2) << update.lastPrice;
	priceLabel->SetText(oss.str().c_str());

	oss.str("");
	oss << "24h Change: ";
	if (update.priceChange > 0) {
		oss << "+";
		changeLabel->SetHighColor(0, 150, 0); // Green
	} else {
		changeLabel->SetHighColor(200, 0, 0); // Red
	}
	oss << std::fixed << std::setprecision(2) << update.priceChangePercent << "%";
	changeLabel->SetText(oss.str().c_str());

	oss.str("");
	oss << "24h Volume: " << std::fixed << std::setprecision(2) << update.volume;
	volumeLabel->SetText(oss.str().c_str());

	oss.str("");
	oss << "24h High/Low: " << currencySymbol << std::fixed << std::setprecision(2)
	    << update.highPrice << " / " << currencySymbol << update.lowPrice;
	highLowLabel->SetText(oss.str().c_str());

	// Force redraw
	Invalidate();
}

void LiveTradingView::UpdateTrade(const TradeUpdate& update) {
	// Get currency symbol
	std::string base, quote;
	SplitSymbol(currentSymbol, base, quote);
	const char* currencySymbol = GetCurrencySymbol(quote);

	// Determine if this is a buy (green) or sell (red)
	bool isBuy = !update.isBuyerMaker;

	// Set row color
	rgb_color rowColor;
	if (isBuy) {
		rowColor = make_color(220, 255, 220); // Light green for BUY
	} else {
		rowColor = make_color(255, 220, 220); // Light red for SELL
	}

	// Create a normal row
	BRow* row = new BRow();

	// Column 0: Time (HH:MM:SS)
	time_t now = time(nullptr);
	struct tm* timeinfo = localtime(&now);
	char timeStr[16];
	strftime(timeStr, sizeof(timeStr), "%H:%M:%S", timeinfo);
	row->SetField(new ColoredStringField(timeStr, rowColor), 0);

	// Column 1: Side (BUY/SELL)
	const char* side = isBuy ? "BUY" : "SELL";
	row->SetField(new ColoredStringField(side, rowColor), 1);

	// Column 2: Price
	std::ostringstream priceOss;
	priceOss << currencySymbol << std::fixed << std::setprecision(2) << update.price;
	row->SetField(new ColoredStringField(priceOss.str().c_str(), rowColor), 2);

	// Column 3: Quantity
	std::ostringstream qtyOss;
	qtyOss << std::fixed << std::setprecision(4) << update.quantity;
	row->SetField(new ColoredStringField(qtyOss.str().c_str(), rowColor), 3);

	// Column 4: Total (Price × Quantity)
	double total = update.price * update.quantity;
	std::ostringstream totalOss;
	totalOss << currencySymbol << std::fixed << std::setprecision(2) << total;
	row->SetField(new ColoredStringField(totalOss.str().c_str(), rowColor), 4);

	// Column 5: Spread (difference from last trade)
	std::ostringstream spreadOss;
	if (lastTradePrice > 0.0) {
		double spread = update.price - lastTradePrice;
		double spreadPercent = (spread / lastTradePrice) * 100.0;

		if (spread > 0) {
			spreadOss << "+";
		}
		spreadOss << std::fixed << std::setprecision(2) << spreadPercent << "%";
	} else {
		spreadOss << "--";
	}
	row->SetField(new ColoredStringField(spreadOss.str().c_str(), rowColor), 5);

	// Column 6: Delay (time since last trade)
	std::ostringstream delayOss;
	if (lastTradeTime > 0) {
		double delay = difftime(now, lastTradeTime);

		if (delay < 1.0) {
			// Less than 1 second - show milliseconds
			delayOss << std::fixed << std::setprecision(1) << (delay * 1000.0) << "ms";
		} else if (delay < 60.0) {
			// Less than 1 minute - show seconds
			delayOss << std::fixed << std::setprecision(1) << delay << "s";
		} else if (delay < 3600.0) {
			// Less than 1 hour - show minutes
			delayOss << std::fixed << std::setprecision(1) << (delay / 60.0) << "m";
		} else {
			// Show hours
			delayOss << std::fixed << std::setprecision(1) << (delay / 3600.0) << "h";
		}
	} else {
		delayOss << "--";
	}
	row->SetField(new ColoredStringField(delayOss.str().c_str(), rowColor), 6);

	// Update last trade info
	lastTradePrice = update.price;
	lastTradeTime = now;

	// Insert at top (index 0)
	recentTradesView->AddRow(row, 0);

	// Keep only last 50 trades
	while (recentTradesView->CountRows() > 50) {
		BRow* oldRow = recentTradesView->RowAt(50);
		recentTradesView->RemoveRow(oldRow);
		delete oldRow;
	}

	// Update paper portfolio with new price
	if (paperPortfolio) {
		paperPortfolio->updatePrice(update.symbol, update.price);

		// Update positions table and balance labels if we have this position
		Paper::PaperPosition* pos = paperPortfolio->getPosition(update.symbol);
		if (pos) {
			UpdatePositionsTable();
			UpdateBalanceLabels();
		}
	}
}

void LiveTradingView::PlaceOrder() {
	if (!isConnected) {
		BAlert* alert = new BAlert("Error", "Not connected to WebSocket",
		                           "OK", nullptr, nullptr, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->Go();
		return;
	}

	// Get order parameters
	BMenuItem* sideItem = sideMenu->Menu()->FindMarked();
	BMenuItem* typeItem = orderTypeMenu->Menu()->FindMarked();

	std::string side = sideItem ? sideItem->Label() : "BUY";
	std::string type = typeItem ? typeItem->Label() : "MARKET";
	std::string quantityStr = quantityControl->Text();
	std::string priceStr = priceControl->Text();

	// Validate quantity
	double quantity = 0.0;
	try {
		quantity = std::stod(quantityStr);
	} catch (...) {
		BAlert* alert = new BAlert("Error", "Invalid quantity value",
		                           "OK", nullptr, nullptr, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->Go();
		return;
	}

	if (quantity <= 0.0) {
		BAlert* alert = new BAlert("Error", "Quantity must be greater than zero",
		                           "OK", nullptr, nullptr, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->Go();
		return;
	}

	// Use last trade price for market orders
	double executionPrice = lastTradePrice;
	if (executionPrice <= 0.0) {
		BAlert* alert = new BAlert("Error", "No market price available yet",
		                           "OK", nullptr, nullptr, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->Go();
		return;
	}

	// Get currency symbol
	std::string base, quote;
	SplitSymbol(currentSymbol, base, quote);
	const char* currencySymbol = GetCurrencySymbol(quote);

	// Show confirmation
	std::ostringstream oss;
	oss << "Place " << side << " " << type << " order:\n\n"
	    << "Symbol: " << currentSymbol << "\n"
	    << "Quantity: " << quantity << "\n"
	    << "Price: " << currencySymbol << std::fixed << std::setprecision(2) << executionPrice;

	oss << "\n\nThis is PAPER TRADING. No real money will be used.";

	BAlert* alert = new BAlert("Confirm Order", oss.str().c_str(),
	                           "Cancel", "Confirm", nullptr,
	                           B_WIDTH_AS_USUAL, B_INFO_ALERT);

	int32 result = alert->Go();

	if (result == 1) { // Confirmed
		bool success = false;

		if (side == "BUY") {
			success = paperPortfolio->buy(currentSymbol, quantity, executionPrice);
		} else {
			success = paperPortfolio->sell(currentSymbol, quantity, executionPrice);
		}

		if (success) {
			LOG_INFO("Paper order executed: " + side + " " + quantityStr + " " + currentSymbol +
			         " @ $" + std::to_string(executionPrice));

			// Update UI
			UpdatePositionsTable();
			UpdateBalanceLabels();

			BAlert* successAlert = new BAlert("Success",
			                                  "Paper order placed successfully!",
			                                  "OK", nullptr, nullptr,
			                                  B_WIDTH_AS_USUAL, B_INFO_ALERT);
			successAlert->Go();
		} else {
			std::string errorMsg = "Failed to execute paper order.\n";
			if (side == "BUY") {
				errorMsg += "Insufficient balance?";
			} else {
				errorMsg += "No position or insufficient quantity?";
			}

			BAlert* errorAlert = new BAlert("Error", errorMsg.c_str(),
			                                "OK", nullptr, nullptr,
			                                B_WIDTH_AS_USUAL, B_STOP_ALERT);
			errorAlert->Go();
		}
	}
}

// Thread-safe wrapper for UpdateTicker - posts message to main thread
void LiveTradingView::UpdateTickerSafe(const TickerUpdate& update) {
	BLooper* looper = Looper();
	if (!looper) return;

	BMessage msg(MSG_UPDATE_TICKER);
	msg.AddString("symbol", update.symbol.c_str());
	msg.AddDouble("lastPrice", update.lastPrice);
	msg.AddDouble("priceChange", update.priceChange);
	msg.AddDouble("priceChangePercent", update.priceChangePercent);
	msg.AddDouble("volume", update.volume);
	msg.AddDouble("highPrice", update.highPrice);
	msg.AddDouble("lowPrice", update.lowPrice);

	BMessenger messenger(this, looper);
	messenger.SendMessage(&msg);
}

// Thread-safe wrapper for UpdateTrade - posts message to main thread
void LiveTradingView::UpdateTradeSafe(const TradeUpdate& update) {
	BLooper* looper = Looper();
	if (!looper) return;

	BMessage msg(MSG_UPDATE_TRADE);
	msg.AddString("symbol", update.symbol.c_str());
	msg.AddDouble("price", update.price);
	msg.AddDouble("quantity", update.quantity);
	msg.AddBool("isBuyerMaker", update.isBuyerMaker);

	BMessenger messenger(this, looper);
	messenger.SendMessage(&msg);
}

void LiveTradingView::UpdatePositionsTable() {
	// Clear existing rows
	while (positionsView->CountRows() > 0) {
		BRow* row = positionsView->RowAt(0);
		positionsView->RemoveRow(row);
		delete row;
	}

	// Get all positions from portfolio
	std::vector<Paper::PaperPosition> positions = paperPortfolio->getAllPositions();

	for (const auto& pos : positions) {
		// Get currency symbol for this position
		std::string base, quote;
		SplitSymbol(pos.symbol, base, quote);
		const char* currencySymbol = GetCurrencySymbol(quote);

		// Determine row color based on P&L
		rgb_color rowColor;
		if (pos.unrealizedPnL > 0) {
			rowColor = make_color(220, 255, 220); // Light green for profit
		} else if (pos.unrealizedPnL < 0) {
			rowColor = make_color(255, 220, 220); // Light red for loss
		} else {
			rowColor = make_color(245, 245, 245); // Light gray for break-even
		}

		BRow* row = new BRow();

		// Column 0: Symbol
		row->SetField(new ColoredStringField(pos.symbol.c_str(), rowColor), 0);

		// Column 1: Side
		row->SetField(new ColoredStringField(pos.side.c_str(), rowColor), 1);

		// Column 2: Entry Price
		std::ostringstream entryOss;
		entryOss << currencySymbol << std::fixed << std::setprecision(2) << pos.entryPrice;
		row->SetField(new ColoredStringField(entryOss.str().c_str(), rowColor), 2);

		// Column 3: Current Price
		std::ostringstream currentOss;
		currentOss << currencySymbol << std::fixed << std::setprecision(2) << pos.currentPrice;
		row->SetField(new ColoredStringField(currentOss.str().c_str(), rowColor), 3);

		// Column 4: Quantity
		std::ostringstream qtyOss;
		qtyOss << std::fixed << std::setprecision(4) << pos.quantity;
		row->SetField(new ColoredStringField(qtyOss.str().c_str(), rowColor), 4);

		// Column 5: P&L
		std::ostringstream pnlOss;
		if (pos.unrealizedPnL >= 0) {
			pnlOss << "+";
		}
		pnlOss << currencySymbol << std::fixed << std::setprecision(2) << pos.unrealizedPnL;
		row->SetField(new ColoredStringField(pnlOss.str().c_str(), rowColor), 5);

		// Column 6: P&L%
		std::ostringstream pnlPercentOss;
		if (pos.unrealizedPnLPercent >= 0) {
			pnlPercentOss << "+";
		}
		pnlPercentOss << std::fixed << std::setprecision(2) << pos.unrealizedPnLPercent << "%";
		row->SetField(new ColoredStringField(pnlPercentOss.str().c_str(), rowColor), 6);

		positionsView->AddRow(row);
	}
}

void LiveTradingView::UpdateBalanceLabels() {
	if (!paperPortfolio) return;

	// Get portfolio stats
	double balance = paperPortfolio->getBalance();
	double equity = paperPortfolio->getEquity();
	double totalPnL = paperPortfolio->getTotalPnL();
	double totalPnLPercent = paperPortfolio->getTotalPnLPercent();

	// Update balance label
	std::ostringstream balanceOss;
	balanceOss << "Balance: $" << std::fixed << std::setprecision(2) << balance;
	balanceLabel->SetText(balanceOss.str().c_str());

	// Update equity label
	std::ostringstream equityOss;
	equityOss << "Equity: $" << std::fixed << std::setprecision(2) << equity;
	equityLabel->SetText(equityOss.str().c_str());

	// Update P&L label with color
	std::ostringstream pnlOss;
	pnlOss << "P&L: ";
	if (totalPnL >= 0) {
		pnlOss << "+";
		pnlLabel->SetHighColor(0, 150, 0); // Green
	} else {
		pnlLabel->SetHighColor(200, 0, 0); // Red
	}
	pnlOss << "$" << std::fixed << std::setprecision(2) << totalPnL
	       << " (" << std::setprecision(2) << totalPnLPercent << "%)";
	pnlLabel->SetText(pnlOss.str().c_str());

	// Force redraw
	Invalidate();
}

void LiveTradingView::UpdateTradesBoxLabel() {
	if (!tradesBox) return;

	std::string label = "Recent Trades (Live) - " + currentSymbol;
	tradesBox->SetLabel(label.c_str());
}

} // namespace UI
} // namespace Emiglio
