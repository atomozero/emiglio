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

#include "../backtest/BacktestSimulator.h"
#include "../backtest/PerformanceAnalyzer.h"
#include "../strategy/RecipeLoader.h"
#include "../data/DataStorage.h"

#include <string>
#include <vector>

namespace Emiglio {
namespace UI {

// Message constants for BacktestView
enum {
	MSG_BACKTEST_RUN = 'btru',
	MSG_BACKTEST_EXPORT = 'btex',
	MSG_RECIPE_SELECTED = 'rcps',
	MSG_BACKTEST_COMPLETE = 'btcp'
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
	BTextControl* symbolControl;
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
	BListView* tradesList;
	BScrollView* tradesScrollView;
	BStatusBar* progressBar;

	// State
	std::string selectedRecipePath;
	Emiglio::Backtest::BacktestResult lastResult;
	bool backtestRunning;
};

} // namespace UI
} // namespace Emiglio

#endif // EMIGLIO_BACKTESTVIEW_H
