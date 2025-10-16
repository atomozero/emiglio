#ifndef EMIGLIO_MAINWINDOW_H
#define EMIGLIO_MAINWINDOW_H

#include <Window.h>
#include <MenuBar.h>
#include <Menu.h>
#include <MenuItem.h>
#include <TabView.h>
#include <View.h>

namespace Emiglio {
namespace UI {

// Forward declarations
class BacktestView;
class DashboardView;
class TradesView;
class LiveTradingView;
class RecipeEditorView;
class SettingsView;

// Message constants
enum {
	MSG_FILE_NEW_RECIPE = 'fnew',
	MSG_FILE_OPEN_RECIPE = 'fopn',
	MSG_FILE_SAVE_RECIPE = 'fsav',
	MSG_FILE_QUIT = 'fqit',

	MSG_VIEW_DASHBOARD = 'vdsh',
	MSG_VIEW_BACKTEST = 'vbkt',
	MSG_VIEW_TRADES = 'vtrd',
	MSG_VIEW_RECIPES = 'vrcp',
	MSG_VIEW_SETTINGS = 'vset',

	MSG_HELP_ABOUT = 'habt',
	MSG_HELP_DOCS = 'hdoc',

	MSG_TAB_CHANGED = 'tchg',
	MSG_BACKTEST_COMPLETED = 'bkcp'
};

// Main application window
class MainWindow : public BWindow {
public:
	MainWindow();
	virtual ~MainWindow();

	// BWindow overrides
	virtual void MessageReceived(BMessage* message);
	virtual bool QuitRequested();

	// Tab management
	void SwitchToTab(int32 index);
	void SwitchToBacktestView();
	void SwitchToDashboardView();
	void SwitchToTradesView();
	void SwitchToRecipeEditorView();
	void SwitchToSettingsView();

private:
	// UI setup
	void SetupMenuBar();
	void SetupViews();

	// Menu handlers
	void HandleFileNew();
	void HandleFileOpen();
	void HandleFileSave();
	void HandleHelpAbout();
	void HandleHelpDocs();

	// UI Components
	BMenuBar* menuBar;
	BTabView* tabView;

	// View tabs
	BacktestView* backtestView;
	DashboardView* dashboardView;
	TradesView* tradesView;
	LiveTradingView* liveTradingView;
	RecipeEditorView* recipeEditorView;
	SettingsView* settingsView;

	// State
	int32 currentTab;
};

} // namespace UI
} // namespace Emiglio

#endif // EMIGLIO_MAINWINDOW_H
