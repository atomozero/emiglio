#ifndef EMIGLIO_SETTINGSVIEW_H
#define EMIGLIO_SETTINGSVIEW_H

#include <View.h>
#include <TextControl.h>
#include <Button.h>
#include <StringView.h>
#include "../utils/CredentialManager.h"
#include <memory>

namespace Emiglio {
namespace UI {

// Message constants
enum {
	MSG_SAVE_CREDENTIALS = 'svcr',
	MSG_DELETE_CREDENTIALS = 'dlcr',
	MSG_TEST_CONNECTION = 'tscn'
};

class SettingsView : public BView {
public:
	SettingsView();
	virtual ~SettingsView();

	void AttachedToWindow() override;
	void MessageReceived(BMessage* message) override;

private:
	// UI components
	BTextControl* fApiKeyInput;
	BTextControl* fApiSecretInput;
	BButton* fSaveButton;
	BButton* fDeleteButton;
	BButton* fTestButton;
	BStringView* fStatusLabel;

	// Credential manager
	std::unique_ptr<CredentialManager> fCredentialManager;

	// Helper methods
	void LoadCredentials();
	void SaveCredentials();
	void DeleteCredentials();
	void TestConnection();
	void UpdateStatus(const std::string& message, bool isError = false);
};

} // namespace UI
} // namespace Emiglio

#endif
