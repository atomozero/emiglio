#include "MainWindow.h"
#include "BacktestView.h"
#include "DashboardView.h"
#include "TradesView.h"
#include "LiveTradingView.h"
#include "RecipeEditorView.h"
#include "SettingsView.h"
#include "../utils/Logger.h"

#include <Application.h>
#include <Alert.h>
#include <LayoutBuilder.h>
#include <GroupLayout.h>
#include <StringView.h>
#include <ScrollView.h>
#include <Screen.h>

namespace Emiglio {
namespace UI {

MainWindow::MainWindow()
	: BWindow(BRect(50, 50, 1400, 900), "Emiglio - Trading Bot",
	          B_TITLED_WINDOW,
	          B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS)
	, menuBar(nullptr)
	, tabView(nullptr)
	, backtestView(nullptr)
	, dashboardView(nullptr)
	, tradesView(nullptr)
	, liveTradingView(nullptr)
	, recipeEditorView(nullptr)
	, settingsView(nullptr)
	, currentTab(0)
{
	LOG_INFO("Initializing MainWindow");

	// Set size limits - allow resizing
	SetSizeLimits(1000, 3000, 700, 2000);

	// Center window on screen
	BScreen screen;
	BRect screenFrame = screen.Frame();
	BRect windowFrame = Frame();
	MoveTo((screenFrame.Width() - windowFrame.Width()) / 2,
	       (screenFrame.Height() - windowFrame.Height()) / 2);

	// Setup UI
	SetupMenuBar();
	SetupViews();

	LOG_INFO("MainWindow initialized successfully");
}

MainWindow::~MainWindow() {
	LOG_INFO("MainWindow destroyed");
}

void MainWindow::SetupMenuBar() {
	// Create menu bar
	menuBar = new BMenuBar("menubar");

	// File menu
	BMenu* fileMenu = new BMenu("File");
	fileMenu->AddItem(new BMenuItem("New Recipe", new BMessage(MSG_FILE_NEW_RECIPE), 'N'));
	fileMenu->AddItem(new BMenuItem("Open Recipe...", new BMessage(MSG_FILE_OPEN_RECIPE), 'O'));
	fileMenu->AddItem(new BMenuItem("Save Recipe", new BMessage(MSG_FILE_SAVE_RECIPE), 'S'));
	fileMenu->AddSeparatorItem();
	fileMenu->AddItem(new BMenuItem("Quit", new BMessage(MSG_FILE_QUIT), 'Q'));
	menuBar->AddItem(fileMenu);

	// View menu
	BMenu* viewMenu = new BMenu("View");
	viewMenu->AddItem(new BMenuItem("Dashboard", new BMessage(MSG_VIEW_DASHBOARD), '1'));
	viewMenu->AddItem(new BMenuItem("Backtest", new BMessage(MSG_VIEW_BACKTEST), '2'));
	viewMenu->AddItem(new BMenuItem("Trades", new BMessage(MSG_VIEW_TRADES), '3'));
	viewMenu->AddItem(new BMenuItem("Recipe Editor", new BMessage(MSG_VIEW_RECIPES), '4'));
	viewMenu->AddItem(new BMenuItem("Settings", new BMessage(MSG_VIEW_SETTINGS), '5'));
	menuBar->AddItem(viewMenu);

	// Help menu
	BMenu* helpMenu = new BMenu("Help");
	helpMenu->AddItem(new BMenuItem("Documentation", new BMessage(MSG_HELP_DOCS)));
	helpMenu->AddSeparatorItem();
	helpMenu->AddItem(new BMenuItem("About Emiglio", new BMessage(MSG_HELP_ABOUT)));
	menuBar->AddItem(helpMenu);

	AddChild(menuBar);
}

void MainWindow::SetupViews() {
	// Create tab view
	tabView = new BTabView("tabview");
	tabView->SetBorder(B_NO_BORDER);

	// Create views
	dashboardView = new DashboardView();
	backtestView = new BacktestView();
	tradesView = new TradesView();
	liveTradingView = new LiveTradingView();
	recipeEditorView = new RecipeEditorView();
	settingsView = new SettingsView();

	// Add tabs
	BTab* tab1 = new BTab();
	tabView->AddTab(dashboardView, tab1);
	tab1->SetLabel("Dashboard");

	BTab* tab2 = new BTab();
	tabView->AddTab(backtestView, tab2);
	tab2->SetLabel("Backtest");

	BTab* tab3 = new BTab();
	tabView->AddTab(tradesView, tab3);
	tab3->SetLabel("Trades");

	BTab* tab4 = new BTab();
	tabView->AddTab(liveTradingView, tab4);
	tab4->SetLabel("Live Trading");

	BTab* tab5 = new BTab();
	tabView->AddTab(recipeEditorView, tab5);
	tab5->SetLabel("Recipe Editor");

	BTab* tab6 = new BTab();
	tabView->AddTab(settingsView, tab6);
	tab6->SetLabel("Settings");

	// Build layout
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(tabView)
		.End();
}

void MainWindow::MessageReceived(BMessage* message) {
	switch (message->what) {
		case MSG_FILE_NEW_RECIPE:
			HandleFileNew();
			break;

		case MSG_FILE_OPEN_RECIPE:
			HandleFileOpen();
			break;

		case MSG_FILE_SAVE_RECIPE:
			HandleFileSave();
			break;

		case MSG_FILE_QUIT:
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;

		case MSG_VIEW_DASHBOARD:
			SwitchToDashboardView();
			break;

		case MSG_VIEW_BACKTEST:
			SwitchToBacktestView();
			break;

		case MSG_VIEW_TRADES:
			SwitchToTradesView();
			break;

		case MSG_VIEW_RECIPES:
			SwitchToRecipeEditorView();
			break;

		case MSG_VIEW_SETTINGS:
			SwitchToSettingsView();
			break;

		case MSG_HELP_ABOUT:
			HandleHelpAbout();
			break;

		case MSG_HELP_DOCS:
			HandleHelpDocs();
			break;

		case MSG_BACKTEST_COMPLETED:
			// Refresh Dashboard when backtest completes
			if (dashboardView) {
				dashboardView->RefreshData();
				LOG_INFO("Dashboard refreshed after backtest completion");
			}
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}

bool MainWindow::QuitRequested() {
	LOG_INFO("MainWindow quit requested");
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

void MainWindow::SwitchToTab(int32 index) {
	if (index >= 0 && index < tabView->CountTabs()) {
		tabView->Select(index);
		currentTab = index;
		LOG_DEBUG("Switched to tab: " + std::to_string(index));
	}
}

void MainWindow::SwitchToDashboardView() {
	SwitchToTab(0);
}

void MainWindow::SwitchToBacktestView() {
	SwitchToTab(1);
}

void MainWindow::SwitchToTradesView() {
	SwitchToTab(2);
}

void MainWindow::SwitchToRecipeEditorView() {
	SwitchToTab(3);
}

void MainWindow::SwitchToSettingsView() {
	SwitchToTab(4);
}

void MainWindow::HandleFileNew() {
	LOG_INFO("File > New Recipe");
	SwitchToRecipeEditorView();
	// TODO: Clear recipe editor and start new
}

void MainWindow::HandleFileOpen() {
	LOG_INFO("File > Open Recipe");
	// TODO: Show file panel to select recipe
	BAlert* alert = new BAlert("Not Implemented",
	                            "Open Recipe feature coming soon!",
	                            "OK", nullptr, nullptr,
	                            B_WIDTH_AS_USUAL, B_INFO_ALERT);
	alert->Go();
}

void MainWindow::HandleFileSave() {
	LOG_INFO("File > Save Recipe");
	// TODO: Save current recipe from editor
	BAlert* alert = new BAlert("Not Implemented",
	                            "Save Recipe feature coming soon!",
	                            "OK", nullptr, nullptr,
	                            B_WIDTH_AS_USUAL, B_INFO_ALERT);
	alert->Go();
}

void MainWindow::HandleHelpAbout() {
	BAlert* alert = new BAlert("About Emiglio",
	                            "Emiglio Trading Bot v0.1\n\n"
	                            "A cryptocurrency trading bot with backtesting capabilities.\n\n"
	                            "Built with Haiku OS and BeAPI\n"
	                            "© 2025 Emiglio Project",
	                            "OK", nullptr, nullptr,
	                            B_WIDTH_AS_USUAL, B_INFO_ALERT);
	alert->Go();
}

void MainWindow::HandleHelpDocs() {
	LOG_INFO("Help > Documentation");
	BAlert* alert = new BAlert("Documentation",
	                            "Documentation is available at:\n"
	                            "/boot/home/Emiglio/docs/\n\n"
	                            "Key files:\n"
	                            "- PHASE4_PLAN.md\n"
	                            "- PHASE5_PLAN.md\n"
	                            "- OPTIMIZATION_RESULTS.md",
	                            "OK", nullptr, nullptr,
	                            B_WIDTH_AS_USUAL, B_INFO_ALERT);
	alert->Go();
}

} // namespace UI
} // namespace Emiglio
