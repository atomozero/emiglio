#include "RecipeEditorView.h"
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
	recipeListView->MakeEmpty();
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

		default:
			BView::MessageReceived(message);
			break;
	}
}

void RecipeEditorView::LoadRecipe(const std::string& path) {
	RecipeLoader loader;
	Recipe recipe;

	if (!loader.loadFromFile(path, recipe)) {
		BAlert* alert = new BAlert("Error", "Failed to load recipe",
		                           "OK", nullptr, nullptr,
		                           B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->Go();
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
	indicatorsListView->MakeEmpty();
	for (size_t i = 0; i < recipe.indicators.size(); i++) {
		const auto& indicator = recipe.indicators[i];
		BString text;
		text << indicator.name.c_str() << "(period=" << indicator.period;
		for (auto pit = indicator.params.begin(); pit != indicator.params.end(); ++pit) {
			text << ", " << pit->first.c_str() << "=" << pit->second;
		}
		text << ")";
		indicatorsListView->AddItem(new BStringItem(text));
	}

	// Load entry conditions
	entryConditionsListView->MakeEmpty();
	for (size_t i = 0; i < recipe.entryConditions.rules.size(); i++) {
		const auto& rule = recipe.entryConditions.rules[i];
		BString text;
		text << rule.indicator.c_str() << " " << rule.operatorStr.c_str() << " ";
		if (!rule.compareWith.empty()) {
			text << rule.compareWith.c_str();
		} else {
			text << rule.value;
		}
		entryConditionsListView->AddItem(new BStringItem(text));
	}

	// Load exit conditions
	exitConditionsListView->MakeEmpty();
	for (size_t i = 0; i < recipe.exitConditions.rules.size(); i++) {
		const auto& rule = recipe.exitConditions.rules[i];
		BString text;
		text << rule.indicator.c_str() << " " << rule.operatorStr.c_str() << " ";
		if (!rule.compareWith.empty()) {
			text << rule.compareWith.c_str();
		} else {
			text << rule.value;
		}
		exitConditionsListView->AddItem(new BStringItem(text));
	}

	statusLabel->SetText("Recipe loaded");
	LOG_INFO("Loaded recipe: " + recipe.name);
}

void RecipeEditorView::SaveRecipe() {
	// For now, show a message that saving is not yet implemented
	// Full implementation would require building a Recipe object from form data
	// and writing it as JSON

	BAlert* alert = new BAlert("Not Implemented",
	                           "Recipe saving is not yet fully implemented.\n"
	                           "This would serialize the form data to JSON format.",
	                           "OK", nullptr, nullptr,
	                           B_WIDTH_AS_USUAL, B_INFO_ALERT);
	alert->Go();

	statusLabel->SetText("Save not yet implemented");
}

void RecipeEditorView::CreateNewRecipe() {
	currentRecipePath = "";
	ClearForm();
	statusLabel->SetText("New recipe");
}

void RecipeEditorView::DeleteRecipe() {
	if (currentRecipePath.empty()) {
		BAlert* alert = new BAlert("Error", "No recipe selected",
		                           "OK", nullptr, nullptr,
		                           B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->Go();
		return;
	}

	BAlert* alert = new BAlert("Confirm",
	                           "Are you sure you want to delete this recipe?",
	                           "Cancel", "Delete", nullptr,
	                           B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	if (alert->Go() == 1) { // Delete button
		if (remove(currentRecipePath.c_str()) == 0) {
			statusLabel->SetText("Recipe deleted");
			LoadRecipeList();
			ClearForm();
		} else {
			BAlert* errorAlert = new BAlert("Error", "Failed to delete recipe",
			                                 "OK", nullptr, nullptr,
			                                 B_WIDTH_AS_USUAL, B_STOP_ALERT);
			errorAlert->Go();
		}
	}
}

void RecipeEditorView::AddIndicator() {
	// Simple dialog for adding indicator
	// Full implementation would use a custom dialog
	BAlert* alert = new BAlert("Add Indicator",
	                           "Adding indicators via dialog not yet implemented.\n"
	                           "Edit the JSON file directly for now.",
	                           "OK", nullptr, nullptr,
	                           B_WIDTH_AS_USUAL, B_INFO_ALERT);
	alert->Go();
}

void RecipeEditorView::RemoveIndicator() {
	int32 index = indicatorsListView->CurrentSelection();
	if (index >= 0) {
		delete indicatorsListView->RemoveItem(index);
		statusLabel->SetText("Indicator removed");
	}
}

void RecipeEditorView::AddEntryCondition() {
	BAlert* alert = new BAlert("Add Condition",
	                           "Adding conditions via dialog not yet implemented.\n"
	                           "Edit the JSON file directly for now.",
	                           "OK", nullptr, nullptr,
	                           B_WIDTH_AS_USUAL, B_INFO_ALERT);
	alert->Go();
}

void RecipeEditorView::RemoveEntryCondition() {
	int32 index = entryConditionsListView->CurrentSelection();
	if (index >= 0) {
		delete entryConditionsListView->RemoveItem(index);
		statusLabel->SetText("Entry condition removed");
	}
}

void RecipeEditorView::AddExitCondition() {
	BAlert* alert = new BAlert("Add Condition",
	                           "Adding conditions via dialog not yet implemented.\n"
	                           "Edit the JSON file directly for now.",
	                           "OK", nullptr, nullptr,
	                           B_WIDTH_AS_USUAL, B_INFO_ALERT);
	alert->Go();
}

void RecipeEditorView::RemoveExitCondition() {
	int32 index = exitConditionsListView->CurrentSelection();
	if (index >= 0) {
		delete exitConditionsListView->RemoveItem(index);
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
		BAlert* alert = new BAlert("Validation",
		                           "Recipe is valid!",
		                           "OK", nullptr, nullptr,
		                           B_WIDTH_AS_USUAL, B_INFO_ALERT);
		alert->Go();
		statusLabel->SetText("Valid");
	} else {
		std::string message = "Validation errors:\n\n";
		for (const auto& error : errors) {
			message += error + "\n";
		}

		BAlert* alert = new BAlert("Validation Errors",
		                           message.c_str(),
		                           "OK", nullptr, nullptr,
		                           B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->Go();
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

	indicatorsListView->MakeEmpty();
	entryConditionsListView->MakeEmpty();
	exitConditionsListView->MakeEmpty();

	currentRecipePath = "";
	statusLabel->SetText("Ready");
}

} // namespace UI
} // namespace Emiglio
