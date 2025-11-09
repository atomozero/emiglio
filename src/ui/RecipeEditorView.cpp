#include "RecipeEditorView.h"
#include "AddIndicatorDialog.h"
#include "AddConditionDialog.h"
#include "../strategy/RecipeLoader.h"
#include "../utils/Logger.h"

#include <LayoutBuilder.h>
#include <GroupView.h>
#include <StringView.h>
#include <Button.h>
#include <TextControl.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <ListView.h>
#include <ScrollView.h>
#include <StringItem.h>
#include <Alert.h>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>
#include <File.h>
#include <Messenger.h>

#include <iostream>
#include <fstream>

namespace Emiglio {
namespace UI {

RecipeEditorView::RecipeEditorView()
	: BView("Recipe Editor", B_WILL_DRAW),
	  recipeListView(nullptr),
	  currentRecipePath("")
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	BuildLayout();
	LoadRecipeList();
}

RecipeEditorView::~RecipeEditorView() {
}

void RecipeEditorView::AttachedToWindow() {
	BView::AttachedToWindow();

	// Set targets for buttons
	if (newButton) newButton->SetTarget(this);
	if (saveButton) saveButton->SetTarget(this);
	if (deleteButton) deleteButton->SetTarget(this);
	if (validateButton) validateButton->SetTarget(this);
	if (addIndicatorButton) addIndicatorButton->SetTarget(this);
	if (removeIndicatorButton) removeIndicatorButton->SetTarget(this);
	if (addEntryConditionButton) addEntryConditionButton->SetTarget(this);
	if (removeEntryConditionButton) removeEntryConditionButton->SetTarget(this);
	if (addExitConditionButton) addExitConditionButton->SetTarget(this);
	if (removeExitConditionButton) removeExitConditionButton->SetTarget(this);
	if (recipeListView) recipeListView->SetTarget(this);
}

void RecipeEditorView::BuildLayout() {
	// Left panel - Recipe list
	recipeListView = new BListView("Recipe List");
	recipeListView->SetSelectionMessage(new BMessage(MSG_RECIPE_SELECTED));
	recipeListScroll = new BScrollView("recipe_scroll", recipeListView,
	                                   0, false, true);

	newButton = new BButton("New", new BMessage(MSG_NEW_RECIPE));
	deleteButton = new BButton("Delete", new BMessage(MSG_DELETE_RECIPE));

	auto leftPanel = BLayoutBuilder::Group<>(B_VERTICAL, B_USE_SMALL_SPACING)
		.Add(new BStringView("", "Recipes"))
		.Add(recipeListScroll, 10)
		.AddGroup(B_HORIZONTAL)
			.Add(newButton)
			.Add(deleteButton)
		.End()
		.View();

	// Right panel - Editor
	// Metadata section
	nameControl = new BTextControl("Name:", "", nullptr);
	descriptionControl = new BTextControl("Description:", "", nullptr);

	// Exchange menu
	BPopUpMenu* exchangePopup = new BPopUpMenu("Exchange");
	exchangePopup->AddItem(new BMenuItem("binance", nullptr));
	exchangePopup->AddItem(new BMenuItem("coinbase", nullptr));
	exchangePopup->AddItem(new BMenuItem("kraken", nullptr));
	exchangePopup->ItemAt(0)->SetMarked(true);
	exchangeMenu = new BMenuField("Exchange:", exchangePopup);

	symbolControl = new BTextControl("Symbol:", "BTCUSDT", nullptr);

	// Timeframe menu
	BPopUpMenu* timeframePopup = new BPopUpMenu("Timeframe");
	timeframePopup->AddItem(new BMenuItem("1m", nullptr));
	timeframePopup->AddItem(new BMenuItem("5m", nullptr));
	timeframePopup->AddItem(new BMenuItem("15m", nullptr));
	timeframePopup->AddItem(new BMenuItem("1h", nullptr));
	timeframePopup->AddItem(new BMenuItem("4h", nullptr));
	timeframePopup->AddItem(new BMenuItem("1d", nullptr));
	timeframePopup->ItemAt(3)->SetMarked(true); // Default 1h
	timeframeMenu = new BMenuField("Timeframe:", timeframePopup);

	// Risk parameters
	stopLossControl = new BTextControl("Stop Loss %:", "2.0", nullptr);
	takeProfitControl = new BTextControl("Take Profit %:", "5.0", nullptr);
	positionSizeControl = new BTextControl("Position Size %:", "95.0", nullptr);

	// Indicators section
	indicatorsListView = new BListView("Indicators List");
	indicatorsScroll = new BScrollView("indicators_scroll", indicatorsListView,
	                                   0, false, true);
	addIndicatorButton = new BButton("Add Indicator", new BMessage(MSG_ADD_INDICATOR));
	removeIndicatorButton = new BButton("Remove", new BMessage(MSG_REMOVE_INDICATOR));

	// Entry conditions section
	entryConditionsListView = new BListView("Entry Conditions List");
	entryConditionsScroll = new BScrollView("entry_scroll", entryConditionsListView,
	                                        0, false, true);
	addEntryConditionButton = new BButton("Add Condition", new BMessage(MSG_ADD_ENTRY_CONDITION));
	removeEntryConditionButton = new BButton("Remove", new BMessage(MSG_REMOVE_ENTRY_CONDITION));

	// Exit conditions section
	exitConditionsListView = new BListView("Exit Conditions List");
	exitConditionsScroll = new BScrollView("exit_scroll", exitConditionsListView,
	                                       0, false, true);
	addExitConditionButton = new BButton("Add Condition", new BMessage(MSG_ADD_EXIT_CONDITION));
	removeExitConditionButton = new BButton("Remove", new BMessage(MSG_REMOVE_EXIT_CONDITION));

	// Action buttons
	saveButton = new BButton("Save Recipe", new BMessage(MSG_SAVE_RECIPE));
	validateButton = new BButton("Validate", new BMessage(MSG_VALIDATE));

	statusLabel = new BStringView("", "Ready");

	auto rightPanel = BLayoutBuilder::Group<>(B_VERTICAL, B_USE_SMALL_SPACING)
		.AddGroup(B_HORIZONTAL)
			.AddGrid(B_USE_SMALL_SPACING, B_USE_SMALL_SPACING)
				.Add(nameControl->CreateLabelLayoutItem(), 0, 0)
				.Add(nameControl->CreateTextViewLayoutItem(), 1, 0)
				.Add(descriptionControl->CreateLabelLayoutItem(), 0, 1)
				.Add(descriptionControl->CreateTextViewLayoutItem(), 1, 1)
				.Add(exchangeMenu->CreateLabelLayoutItem(), 0, 2)
				.Add(exchangeMenu->CreateMenuBarLayoutItem(), 1, 2)
				.Add(symbolControl->CreateLabelLayoutItem(), 0, 3)
				.Add(symbolControl->CreateTextViewLayoutItem(), 1, 3)
				.Add(timeframeMenu->CreateLabelLayoutItem(), 0, 4)
				.Add(timeframeMenu->CreateMenuBarLayoutItem(), 1, 4)
				.Add(stopLossControl->CreateLabelLayoutItem(), 0, 5)
				.Add(stopLossControl->CreateTextViewLayoutItem(), 1, 5)
				.Add(takeProfitControl->CreateLabelLayoutItem(), 0, 6)
				.Add(takeProfitControl->CreateTextViewLayoutItem(), 1, 6)
				.Add(positionSizeControl->CreateLabelLayoutItem(), 0, 7)
				.Add(positionSizeControl->CreateTextViewLayoutItem(), 1, 7)
			.End()
		.End()
		.AddGroup(B_VERTICAL, B_USE_SMALL_SPACING)
			.Add(new BStringView("", "Indicators"))
			.Add(indicatorsScroll, 2)
			.AddGroup(B_HORIZONTAL)
				.Add(addIndicatorButton)
				.Add(removeIndicatorButton)
			.End()
		.End()
		.AddGroup(B_VERTICAL, B_USE_SMALL_SPACING)
			.Add(new BStringView("", "Entry Conditions"))
			.Add(entryConditionsScroll, 2)
			.AddGroup(B_HORIZONTAL)
				.Add(addEntryConditionButton)
				.Add(removeEntryConditionButton)
			.End()
		.End()
		.AddGroup(B_VERTICAL, B_USE_SMALL_SPACING)
			.Add(new BStringView("", "Exit Conditions"))
			.Add(exitConditionsScroll, 2)
			.AddGroup(B_HORIZONTAL)
				.Add(addExitConditionButton)
				.Add(removeExitConditionButton)
			.End()
		.End()
		.AddGroup(B_HORIZONTAL)
			.Add(saveButton)
			.Add(validateButton)
			.AddGlue()
			.Add(statusLabel)
		.End()
		.View();

	// Main layout
	BLayoutBuilder::Group<>(this, B_HORIZONTAL)
		.SetInsets(B_USE_WINDOW_SPACING)
		.Add(leftPanel, 1)
		.Add(rightPanel, 3)
		.End();
}

void RecipeEditorView::LoadRecipeList() {
	ClearListView(recipeListView);
	availableRecipes.clear();

	BDirectory dir("/boot/home/Emiglio/recipes");
	if (dir.InitCheck() != B_OK) {
		LOG_ERROR("Failed to open recipes directory");
		return;
	}

	BEntry entry;
	while (dir.GetNextEntry(&entry) == B_OK) {
		char name[B_FILE_NAME_LENGTH];
		if (entry.GetName(name) == B_OK) {
			BString fileName(name);
			if (fileName.EndsWith(".json")) {
				// Remove .json extension for display
				fileName.Remove(fileName.Length() - 5, 5);
				recipeListView->AddItem(new BStringItem(fileName));

				// Store full path
				BPath path;
				entry.GetPath(&path);
				availableRecipes.push_back(path.Path());
			}
		}
	}

	LOG_INFO("Loaded " + std::to_string(availableRecipes.size()) + " recipes");
}

void RecipeEditorView::MessageReceived(BMessage* message) {
	switch (message->what) {
		case MSG_RECIPE_SELECTED: {
			int32 index = recipeListView->CurrentSelection();
			if (index >= 0 && index < static_cast<int32>(availableRecipes.size())) {
				LoadRecipe(availableRecipes[index]);
			}
			break;
		}

		case MSG_NEW_RECIPE:
			CreateNewRecipe();
			break;

		case MSG_SAVE_RECIPE:
			SaveRecipe();
			break;

		case MSG_DELETE_RECIPE:
			DeleteRecipe();
			break;

		case MSG_VALIDATE:
			ValidateAndShowErrors();
			break;

		case MSG_ADD_INDICATOR:
			AddIndicator();
			break;

		case MSG_REMOVE_INDICATOR:
			RemoveIndicator();
			break;

		case MSG_ADD_ENTRY_CONDITION:
			AddEntryCondition();
			break;

		case MSG_REMOVE_ENTRY_CONDITION:
			RemoveEntryCondition();
			break;

		case MSG_ADD_EXIT_CONDITION:
			AddExitCondition();
			break;

		case MSG_REMOVE_EXIT_CONDITION:
			RemoveExitCondition();
			break;

		case MSG_INDICATOR_ADDED: {
			const char* indicator;
			if (message->FindString("indicator", &indicator) == B_OK) {
				// Parse and store structured data
				IndicatorConfig config = ParseIndicatorString(indicator);
				currentIndicators.push_back(config);

				indicatorsListView->AddItem(new BStringItem(indicator));
				statusLabel->SetText("Indicator added");
			}
			break;
		}

		case MSG_ENTRY_CONDITION_ADDED: {
			const char* condition;
			if (message->FindString("condition", &condition) == B_OK) {
				// Parse and store structured data
				TradingRule rule = ParseRuleString(condition);
				currentEntryRules.push_back(rule);

				entryConditionsListView->AddItem(new BStringItem(condition));
				statusLabel->SetText("Entry condition added");
			}
			break;
		}

		case MSG_EXIT_CONDITION_ADDED: {
			const char* condition;
			if (message->FindString("condition", &condition) == B_OK) {
				// Parse and store structured data
				TradingRule rule = ParseRuleString(condition);
				currentExitRules.push_back(rule);

				exitConditionsListView->AddItem(new BStringItem(condition));
				statusLabel->SetText("Exit condition added");
			}
			break;
		}

		default:
			BView::MessageReceived(message);
			break;
	}
}

void RecipeEditorView::LoadRecipe(const std::string& path) {
	RecipeLoader loader;
	Recipe recipe;

	if (!loader.loadFromFile(path, recipe)) {
		ShowError("Failed to load recipe");
		return;
	}

	currentRecipePath = path;

	// Load metadata
	nameControl->SetText(recipe.name.c_str());
	descriptionControl->SetText(recipe.description.c_str());
	symbolControl->SetText(recipe.market.symbol.c_str());

	// Set exchange
	BMenuItem* item = exchangeMenu->Menu()->FindItem(recipe.market.exchange.c_str());
	if (item) item->SetMarked(true);

	// Set timeframe
	item = timeframeMenu->Menu()->FindItem(recipe.market.timeframe.c_str());
	if (item) item->SetMarked(true);

	// Load risk parameters
	char buffer[32];
	snprintf(buffer, sizeof(buffer), "%.2f", recipe.risk.stopLossPercent);
	stopLossControl->SetText(buffer);
	snprintf(buffer, sizeof(buffer), "%.2f", recipe.risk.takeProfitPercent);
	takeProfitControl->SetText(buffer);
	snprintf(buffer, sizeof(buffer), "%.2f", recipe.capital.positionSizePercent);
	positionSizeControl->SetText(buffer);

	// Load indicators
	ClearListView(indicatorsListView);
	currentIndicators.clear();
	for (size_t i = 0; i < recipe.indicators.size(); i++) {
		const auto& indicator = recipe.indicators[i];
		currentIndicators.push_back(indicator);
		std::string text = FormatIndicator(indicator);
		indicatorsListView->AddItem(new BStringItem(text.c_str()));
	}

	// Load entry conditions
	ClearListView(entryConditionsListView);
	currentEntryRules.clear();
	for (size_t i = 0; i < recipe.entryConditions.rules.size(); i++) {
		const auto& rule = recipe.entryConditions.rules[i];
		currentEntryRules.push_back(rule);
		std::string text = FormatRule(rule);
		entryConditionsListView->AddItem(new BStringItem(text.c_str()));
	}

	// Load exit conditions
	ClearListView(exitConditionsListView);
	currentExitRules.clear();
	for (size_t i = 0; i < recipe.exitConditions.rules.size(); i++) {
		const auto& rule = recipe.exitConditions.rules[i];
		currentExitRules.push_back(rule);
		std::string text = FormatRule(rule);
		exitConditionsListView->AddItem(new BStringItem(text.c_str()));
	}

	statusLabel->SetText("Recipe loaded");
	LOG_INFO("Loaded recipe: " + recipe.name);
}

void RecipeEditorView::SaveRecipe() {
	// Build Recipe object from form data
	Recipe recipe;

	// 1. Basic metadata
	recipe.name = nameControl->Text();
	recipe.description = descriptionControl->Text();

	if (recipe.name.empty()) {
		ShowError("Recipe name is required");
		return;
	}

	// 2. Market configuration
	BMenuItem* item = exchangeMenu->Menu()->FindMarked();
	recipe.market.exchange = item ? item->Label() : "binance";

	recipe.market.symbol = symbolControl->Text();
	if (recipe.market.symbol.empty()) {
		ShowError("Symbol is required");
		return;
	}

	item = timeframeMenu->Menu()->FindMarked();
	recipe.market.timeframe = item ? item->Label() : "1h";

	// 3. Capital configuration (use defaults for now)
	recipe.capital.initial = 10000.0;  // Default $10k
	recipe.capital.positionSizePercent = atof(positionSizeControl->Text());
	if (recipe.capital.positionSizePercent <= 0 || recipe.capital.positionSizePercent > 100) {
		recipe.capital.positionSizePercent = 95.0;
	}

	// 4. Risk management
	recipe.risk.stopLossPercent = atof(stopLossControl->Text());
	recipe.risk.takeProfitPercent = atof(takeProfitControl->Text());
	recipe.risk.maxDailyLossPercent = 5.0;  // Default 5%
	recipe.risk.maxOpenPositions = 1;       // Default 1

	// 5. Indicators - Use structured data
	recipe.indicators = currentIndicators;

	// 6. Entry conditions - Use structured data
	recipe.entryConditions.logic = "AND";  // Default AND logic
	recipe.entryConditions.rules = currentEntryRules;

	// 7. Exit conditions - Use structured data
	recipe.exitConditions.logic = "OR";  // Default OR logic
	recipe.exitConditions.rules = currentExitRules;

	// 8. Validate
	std::vector<std::string> validationErrors = ValidateRecipe(recipe);
	if (!validationErrors.empty()) {
		ShowErrorList("Validation Errors", validationErrors);
		return;
	}

	// 9. Determine file path
	std::string savePath;
	if (currentRecipePath.empty()) {
		// New recipe - create new file
		savePath = "/boot/home/Emiglio/recipes/" + recipe.name + ".json";

		// Check if file exists
		BEntry entry(savePath.c_str());
		if (entry.Exists()) {
			if (ShowConfirm("Recipe with this name already exists. Overwrite?",
			               "Cancel", "Overwrite") == 0) {
				return;  // Cancel
			}
		}
	} else {
		// Editing existing recipe
		savePath = currentRecipePath;
	}

	// 10. Save using RecipeLoader
	RecipeLoader loader;
	if (loader.saveToFile(savePath, recipe)) {
		statusLabel->SetText("Recipe saved successfully");
		currentRecipePath = savePath;

		// Refresh recipe list
		LoadRecipeList();

		ShowInfo("Recipe saved successfully!");
	} else {
		std::string error = "Failed to save recipe: " + loader.getLastError();
		ShowError(error.c_str());
		statusLabel->SetText("Save failed");
	}
}

void RecipeEditorView::CreateNewRecipe() {
	currentRecipePath = "";
	ClearForm();
	statusLabel->SetText("New recipe");
}

void RecipeEditorView::DeleteRecipe() {
	if (currentRecipePath.empty()) {
		ShowError("No recipe selected");
		return;
	}

	if (ShowConfirm("Are you sure you want to delete this recipe?", "Cancel", "Delete") == 1) { // Delete button
		if (remove(currentRecipePath.c_str()) == 0) {
			statusLabel->SetText("Recipe deleted");
			LoadRecipeList();
			ClearForm();
		} else {
			ShowError("Failed to delete recipe");
		}
	}
}

void RecipeEditorView::AddIndicator() {
	// Open dialog for adding indicator
	BMessenger messenger(this);
	AddIndicatorDialog* dialog = new AddIndicatorDialog(messenger);
	dialog->Show();
}

void RecipeEditorView::RemoveIndicator() {
	int32 index = indicatorsListView->CurrentSelection();
	if (index >= 0) {
		delete indicatorsListView->RemoveItem(index);
		// Also remove from structured data
		if (index < static_cast<int32>(currentIndicators.size())) {
			currentIndicators.erase(currentIndicators.begin() + index);
		}
		statusLabel->SetText("Indicator removed");
	}
}

void RecipeEditorView::AddEntryCondition() {
	// Open dialog for adding entry condition
	BMessage msg(MSG_ENTRY_CONDITION_ADDED);
	BMessenger messenger(this);
	AddConditionDialog* dialog = new AddConditionDialog(messenger, "Add Entry Condition");
	dialog->SetMessageWhat(MSG_ENTRY_CONDITION_ADDED);
	dialog->Show();
}

void RecipeEditorView::RemoveEntryCondition() {
	int32 index = entryConditionsListView->CurrentSelection();
	if (index >= 0) {
		delete entryConditionsListView->RemoveItem(index);
		// Also remove from structured data
		if (index < static_cast<int32>(currentEntryRules.size())) {
			currentEntryRules.erase(currentEntryRules.begin() + index);
		}
		statusLabel->SetText("Entry condition removed");
	}
}

void RecipeEditorView::AddExitCondition() {
	// Open dialog for adding exit condition
	BMessenger messenger(this);
	AddConditionDialog* dialog = new AddConditionDialog(messenger, "Add Exit Condition");
	dialog->SetMessageWhat(MSG_EXIT_CONDITION_ADDED);
	dialog->Show();
}

void RecipeEditorView::RemoveExitCondition() {
	int32 index = exitConditionsListView->CurrentSelection();
	if (index >= 0) {
		delete exitConditionsListView->RemoveItem(index);
		// Also remove from structured data
		if (index < static_cast<int32>(currentExitRules.size())) {
			currentExitRules.erase(currentExitRules.begin() + index);
		}
		statusLabel->SetText("Exit condition removed");
	}
}

void RecipeEditorView::ValidateAndShowErrors() {
	// Simple validation
	std::vector<std::string> errors;

	if (strlen(nameControl->Text()) == 0) {
		errors.push_back("- Name is required");
	}

	if (strlen(symbolControl->Text()) == 0) {
		errors.push_back("- Symbol is required");
	}

	if (indicatorsListView->CountItems() == 0) {
		errors.push_back("- At least one indicator is required");
	}

	if (entryConditionsListView->CountItems() == 0) {
		errors.push_back("- At least one entry condition is required");
	}

	if (exitConditionsListView->CountItems() == 0) {
		errors.push_back("- At least one exit condition is required");
	}

	if (errors.empty()) {
		ShowInfo("Recipe is valid!");
		statusLabel->SetText("Valid");
	} else {
		ShowErrorList("Validation Errors", errors);
		statusLabel->SetText("Invalid");
	}
}

void RecipeEditorView::ClearForm() {
	nameControl->SetText("");
	descriptionControl->SetText("");
	symbolControl->SetText("BTCUSDT");
	stopLossControl->SetText("2.0");
	takeProfitControl->SetText("5.0");
	positionSizeControl->SetText("95.0");

	ClearListView(indicatorsListView);
	ClearListView(entryConditionsListView);
	ClearListView(exitConditionsListView);

	// Clear structured data
	currentIndicators.clear();
	currentEntryRules.clear();
	currentExitRules.clear();

	currentRecipePath = "";
	statusLabel->SetText("Ready");
}

// Helper functions for UI dialogs
void RecipeEditorView::ShowError(const char* message) {
	BAlert* alert = new BAlert("Error", message, "OK", nullptr, nullptr,
	                           B_WIDTH_AS_USUAL, B_STOP_ALERT);
	alert->Go();
	delete alert;
}

void RecipeEditorView::ShowInfo(const char* message) {
	BAlert* alert = new BAlert("Information", message, "OK", nullptr, nullptr,
	                           B_WIDTH_AS_USUAL, B_INFO_ALERT);
	alert->Go();
	delete alert;
}

int32 RecipeEditorView::ShowConfirm(const char* message,
                                     const char* button0,
                                     const char* button1) {
	BAlert* alert = new BAlert("Confirm", message, button0, button1, nullptr,
	                           B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	int32 result = alert->Go();
	delete alert;
	return result;
}

void RecipeEditorView::ShowErrorList(const char* title,
                                      const std::vector<std::string>& errors) {
	std::string message;
	for (const auto& error : errors) {
		message += error + "\n";
	}
	ShowError(message.c_str());
}

// Helper functions for list management
void RecipeEditorView::ClearListView(BListView* listView) {
	// Delete all items before clearing
	for (int32 i = 0; i < listView->CountItems(); i++) {
		delete listView->ItemAt(i);
	}
	listView->MakeEmpty();
}

void RecipeEditorView::RemoveFromListView(BListView* listView, const char* itemType) {
	int32 index = listView->CurrentSelection();
	if (index >= 0) {
		delete listView->RemoveItem(index);
		BString status;
		status << itemType << " removed";
		statusLabel->SetText(status);
	} else {
		ShowError("No item selected");
	}
}

std::string RecipeEditorView::FormatIndicator(const IndicatorConfig& indicator) {
	std::string result = indicator.name + "(period=" + std::to_string(indicator.period);
	for (const auto& param : indicator.params) {
		result += ", " + param.first + "=" + std::to_string(param.second);
	}
	result += ")";
	return result;
}

std::string RecipeEditorView::FormatRule(const TradingRule& rule) {
	std::string result = rule.indicator + " " + rule.operatorStr + " ";
	if (!rule.compareWith.empty()) {
		result += rule.compareWith;
	} else {
		result += std::to_string(rule.value);
	}
	return result;
}

IndicatorConfig RecipeEditorView::ParseIndicatorString(const std::string& text) {
	IndicatorConfig indicator;
	indicator.period = 14; // Default

	// Parse format: "rsi(period=14, oversold=30)"
	size_t openParen = text.find("(");
	if (openParen == std::string::npos) {
		indicator.name = text;
		return indicator;
	}

	indicator.name = text.substr(0, openParen);

	// Extract parameters
	size_t closeParen = text.find(")", openParen);
	if (closeParen == std::string::npos) return indicator;

	std::string params = text.substr(openParen + 1, closeParen - openParen - 1);
	std::istringstream iss(params);
	std::string token;

	while (std::getline(iss, token, ',')) {
		// Trim whitespace
		size_t start = token.find_first_not_of(" \t");
		size_t end = token.find_last_not_of(" \t");
		if (start == std::string::npos) continue;
		token = token.substr(start, end - start + 1);

		// Parse key=value
		size_t eq = token.find("=");
		if (eq == std::string::npos) continue;

		std::string key = token.substr(0, eq);
		std::string value = token.substr(eq + 1);

		if (key == "period") {
			indicator.period = std::atoi(value.c_str());
		} else {
			try {
				indicator.params[key] = std::stod(value);
			} catch (const std::exception& e) {
				LOG_WARNING("Invalid parameter value: " + key + "=" + value);
			}
		}
	}

	return indicator;
}

TradingRule RecipeEditorView::ParseRuleString(const std::string& text) {
	TradingRule rule;
	rule.value = 0.0;

	// Parse format: "rsi < 30" or "sma crosses_above ema"
	std::istringstream iss(text);
	std::string ind, op, val;

	if (iss >> ind >> op >> val) {
		rule.indicator = ind;
		rule.operatorStr = op;

		// Check if value is numeric or another indicator
		try {
			rule.value = std::stod(val);
			rule.compareWith = "";
		} catch (const std::exception&) {
			// Not numeric, must be comparing with another indicator
			rule.compareWith = val;
			rule.value = 0.0;
		}
	}

	return rule;
}

std::vector<std::string> RecipeEditorView::ValidateRecipe(const Recipe& recipe) {
	std::vector<std::string> errors;

	if (recipe.name.empty()) {
		errors.push_back("Recipe name is required");
	}

	if (recipe.market.symbol.empty()) {
		errors.push_back("Symbol is required");
	}

	if (recipe.indicators.empty()) {
		errors.push_back("At least one indicator is required");
	}

	if (recipe.entryConditions.rules.empty()) {
		errors.push_back("At least one entry condition is required");
	}

	if (recipe.exitConditions.rules.empty()) {
		errors.push_back("At least one exit condition is required");
	}

	return errors;
}

} // namespace UI
} // namespace Emiglio
