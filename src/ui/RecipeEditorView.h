#ifndef EMIGLIO_RECIPEEDITORVIEW_H
#define EMIGLIO_RECIPEEDITORVIEW_H

#include <View.h>
#include <String.h>
#include <string>
#include <vector>

class BButton;
class BTextControl;
class BMenuField;
class BListView;
class BScrollView;
class BStringView;

namespace Emiglio {

struct Recipe; // Forward declaration

namespace UI {

class RecipeEditorView : public BView {
public:
	RecipeEditorView();
	virtual ~RecipeEditorView();

	virtual void AttachedToWindow() override;
	virtual void MessageReceived(BMessage* message) override;

private:
	void BuildLayout();
	void LoadRecipeList();
	void LoadRecipe(const std::string& path);
	void SaveRecipe();
	void CreateNewRecipe();
	void DeleteRecipe();
	void AddIndicator();
	void RemoveIndicator();
	void AddEntryCondition();
	void RemoveEntryCondition();
	void AddExitCondition();
	void RemoveExitCondition();
	void ValidateAndShowErrors();
	void ClearForm();

	// UI Components - Left panel (recipe list)
	BListView* recipeListView;
	BScrollView* recipeListScroll;

	// UI Components - Right panel (editor)
	// Recipe metadata
	BTextControl* nameControl;
	BTextControl* descriptionControl;
	BMenuField* exchangeMenu;
	BTextControl* symbolControl;
	BMenuField* timeframeMenu;

	// Risk parameters
	BTextControl* stopLossControl;
	BTextControl* takeProfitControl;
	BTextControl* positionSizeControl;

	// Indicators section
	BListView* indicatorsListView;
	BScrollView* indicatorsScroll;
	BButton* addIndicatorButton;
	BButton* removeIndicatorButton;

	// Entry conditions section
	BListView* entryConditionsListView;
	BScrollView* entryConditionsScroll;
	BButton* addEntryConditionButton;
	BButton* removeEntryConditionButton;

	// Exit conditions section
	BListView* exitConditionsListView;
	BScrollView* exitConditionsScroll;
	BButton* addExitConditionButton;
	BButton* removeExitConditionButton;

	// Action buttons
	BButton* newButton;
	BButton* saveButton;
	BButton* deleteButton;
	BButton* validateButton;

	// Status
	BStringView* statusLabel;

	// State
	std::string currentRecipePath;
	std::vector<std::string> availableRecipes;

	// Message constants
	enum {
		MSG_RECIPE_SELECTED = 'rsel',
		MSG_NEW_RECIPE = 'rnew',
		MSG_SAVE_RECIPE = 'rsav',
		MSG_DELETE_RECIPE = 'rdel',
		MSG_VALIDATE = 'rval',
		MSG_ADD_INDICATOR = 'indi',
		MSG_REMOVE_INDICATOR = 'rmin',
		MSG_ADD_ENTRY_CONDITION = 'aent',
		MSG_REMOVE_ENTRY_CONDITION = 'rent',
		MSG_ADD_EXIT_CONDITION = 'aexi',
		MSG_REMOVE_EXIT_CONDITION = 'rexi'
	};
};

} // namespace UI
} // namespace Emiglio

#endif // EMIGLIO_RECIPEEDITORVIEW_H
