#ifndef EMIGLIO_SETTINGSVIEW_H
#define EMIGLIO_SETTINGSVIEW_H

#include <View.h>
#include <TextControl.h>
#include <Button.h>
#include <StringView.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include "../utils/CredentialManager.h"
#include <memory>

namespace Emiglio {
namespace UI {

// Message constants
enum {
	MSG_SAVE_CREDENTIALS = 'svcr',
	MSG_DELETE_CREDENTIALS = 'dlcr',
	MSG_TEST_CONNECTION = 'tscn',
	MSG_CURRENCY_CHANGED = 'crch',
	MSG_SAVE_PREFERENCES = 'svpr'
};

class SettingsView : public BView {
public:
	SettingsView();
	virtual ~SettingsView();

	void AttachedToWindow() override;
	void MessageReceived(BMessage* message) override;

private:
	// UI components - API Credentials
	BTextControl* fApiKeyInput;
	BTextControl* fApiSecretInput;
	BButton* fSaveButton;
	BButton* fDeleteButton;
	BButton* fTestButton;
	BStringView* fStatusLabel;

	// UI components - General Preferences
	BMenuField* fCurrencyMenu;
	BButton* fSavePreferencesButton;
	BStringView* fPreferencesStatusLabel;

	// Credential manager
	std::unique_ptr<CredentialManager> fCredentialManager;

	// Helper methods - API Credentials
	void LoadCredentials();
	void SaveCredentials();
	void DeleteCredentials();
	void TestConnection();
	void UpdateStatus(const std::string& message, bool isError = false);

	// Helper methods - General Preferences
	void LoadPreferences();
	void SavePreferences();
	void UpdatePreferencesStatus(const std::string& message, bool isError = false);
	std::string GetSystemCurrency();
};

} // namespace UI
} // namespace Emiglio

#endif
