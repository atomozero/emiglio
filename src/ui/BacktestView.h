#ifndef EMIGLIO_BACKTESTVIEW_H
#define EMIGLIO_BACKTESTVIEW_H

#include <View.h>
#include <Button.h>
#include <TextControl.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <StringView.h>
#include <ListView.h>
#include <ScrollView.h>
#include <StatusBar.h>
#include <GroupView.h>
#include <interface/ColumnListView.h>
#include <interface/ColumnTypes.h>

#include "../backtest/BacktestSimulator.h"
#include "../backtest/PerformanceAnalyzer.h"
#include "../strategy/RecipeLoader.h"
#include "../data/DataStorage.h"
#include "EquityChartView.h"

#include <string>
#include <vector>

namespace Emiglio {
namespace UI {

// Message constants for BacktestView
enum {
	MSG_BACKTEST_RUN = 'btru',
	MSG_BACKTEST_EXPORT = 'btex',
	MSG_RECIPE_SELECTED = 'rcps',
	MSG_BACKTEST_COMPLETE = 'btcp',
	MSG_TRADE_SELECTED = 'trds',
	MSG_BASE_SELECTED = 'bass',
	MSG_QUOTE_SELECTED = 'qots'
};

// Custom column list view that sends selection message on single click
class TradesColumnListView : public BColumnListView {
public:
	TradesColumnListView(const char* name, uint32 flags = B_WILL_DRAW,
	                     border_style border = B_FANCY_BORDER,
	                     bool showHorizontalScrollbar = true);
	virtual ~TradesColumnListView();

	virtual void SelectionChanged() override;
};

// Backtest view - run backtests and view results
class BacktestView : public BView {
public:
	BacktestView();
	virtual ~BacktestView();

	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage* message);

private:
	// UI setup
	void SetupUI();
	void SetupConfigPanel();
	void SetupResultsPanel();

	// Actions
	void RunBacktest();
	void ExportResults();
	void UpdateRecipeList();
	void DisplayResults(const Emiglio::Backtest::BacktestResult& result);
	void ClearResults();

	// Helper
	std::vector<std::string> FindRecipeFiles();
	void SaveResultsToDatabase(const Emiglio::Backtest::BacktestResult& result,
	                            const Recipe& recipe,
	                            const std::vector<Candle>& candles);

	// Config controls
	BMenuField* recipeField;
	BPopUpMenu* recipeMenu;
	BMenuField* baseAssetField;
	BPopUpMenu* baseAssetMenu;
	BMenuField* quoteAssetField;
	BPopUpMenu* quoteAssetMenu;
	BTextControl* initialCapitalControl;
	BTextControl* commissionControl;
	BTextControl* slippageControl;
	BButton* runButton;
	BButton* exportButton;

	// Results controls
	BGroupView* resultsPanel;
	BStringView* statusLabel;
	BStringView* tradesLabel;
	BStringView* winRateLabel;
	BStringView* returnLabel;
	BStringView* sharpeLabel;
	BStringView* drawdownLabel;
	TradesColumnListView* tradesList;
	BStatusBar* progressBar;
	EquityChartView* equityChartView;

	// State
	std::string selectedRecipePath;
	Emiglio::Backtest::BacktestResult lastResult;
	bool backtestRunning;
	int32 selectedTradeIndex;
};

} // namespace UI
} // namespace Emiglio

#endif // EMIGLIO_BACKTESTVIEW_H
