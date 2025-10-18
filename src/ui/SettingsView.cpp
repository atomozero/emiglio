#include "SettingsView.h"
#include "../utils/Logger.h"
#include "../exchange/BinanceAPI.h"
#include <LayoutBuilder.h>
#include <GroupLayout.h>
#include <GroupView.h>
#include <StringView.h>
#include <Box.h>
#include <Alert.h>

namespace Emiglio {
namespace UI {

SettingsView::SettingsView()
	: BView("Settings", B_WILL_DRAW),
	  fApiKeyInput(nullptr),
	  fApiSecretInput(nullptr),
	  fSaveButton(nullptr),
	  fDeleteButton(nullptr),
	  fTestButton(nullptr),
	  fStatusLabel(nullptr),
	  fCredentialManager(std::make_unique<CredentialManager>())
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	// Initialize credential manager
	if (!fCredentialManager->init("/boot/home/Emiglio/data/emilio.db")) {
		LOG_ERROR("Failed to initialize CredentialManager: " + fCredentialManager->getLastError());
	}

	// Title
	BStringView* title = new BStringView("title", "Binance API Configuration");
	title->SetFont(be_bold_font);

	// Info text
	BStringView* info = new BStringView("info",
		"Enter your Binance API credentials to access your portfolio.");

	// API Key input
	fApiKeyInput = new BTextControl("API Key:", "", nullptr);
	fApiKeyInput->SetModificationMessage(new BMessage('apky'));

	// API Secret input (hidden)
	fApiSecretInput = new BTextControl("API Secret:", "", nullptr);
	fApiSecretInput->TextView()->HideTyping(true);  // Hide password
	fApiSecretInput->SetModificationMessage(new BMessage('apsc'));

	// Buttons
	fSaveButton = new BButton("Save", new BMessage(MSG_SAVE_CREDENTIALS));
	fDeleteButton = new BButton("Delete", new BMessage(MSG_DELETE_CREDENTIALS));
	fTestButton = new BButton("Test Connection", new BMessage(MSG_TEST_CONNECTION));

	// Status label
	fStatusLabel = new BStringView("status", "");

	// Security notice
	BStringView* securityNotice = new BStringView("security",
		"⚠️ Credentials are encrypted using AES-256 and stored locally.");
	securityNotice->SetFont(be_plain_font);

	// Instructions box
	BBox* instructionsBox = new BBox("instructions");
	instructionsBox->SetLabel("How to get Binance API Keys");

	BStringView* step1 = new BStringView("step1", "1. Log in to Binance.com");
	BStringView* step2 = new BStringView("step2", "2. Go to Profile → API Management");
	BStringView* step3 = new BStringView("step3", "3. Create a new API key (Read-only is sufficient)");
	BStringView* step4 = new BStringView("step4", "4. Copy the API Key and Secret here");
	BStringView* step5 = new BStringView("step5", "5. Enable 'Read' permissions only (no trading needed)");

	BLayoutBuilder::Group<>(instructionsBox, B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_SMALL_INSETS)
		.Add(step1)
		.Add(step2)
		.Add(step3)
		.Add(step4)
		.Add(step5)
		.End();

	// Main layout
	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_WINDOW_SPACING)
		.Add(title)
		.Add(info)
		.AddStrut(B_USE_SMALL_SPACING)
		.Add(fApiKeyInput)
		.Add(fApiSecretInput)
		.AddStrut(B_USE_SMALL_SPACING)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.Add(fSaveButton)
			.Add(fTestButton)
			.Add(fDeleteButton)
			.AddGlue()
		.End()
		.Add(fStatusLabel)
		.AddStrut(B_USE_DEFAULT_SPACING)
		.Add(securityNotice)
		.AddStrut(B_USE_DEFAULT_SPACING)
		.Add(instructionsBox)
		.AddGlue()
		.End();
}

SettingsView::~SettingsView() {
}

void SettingsView::AttachedToWindow() {
	BView::AttachedToWindow();

	// Set message targets
	fSaveButton->SetTarget(this);
	fDeleteButton->SetTarget(this);
	fTestButton->SetTarget(this);

	// Load existing credentials if any
	LoadCredentials();
}

void SettingsView::MessageReceived(BMessage* message) {
	switch (message->what) {
		case MSG_SAVE_CREDENTIALS:
			SaveCredentials();
			break;

		case MSG_DELETE_CREDENTIALS:
			DeleteCredentials();
			break;

		case MSG_TEST_CONNECTION:
			TestConnection();
			break;

		default:
			BView::MessageReceived(message);
			break;
	}
}

void SettingsView::LoadCredentials() {
	std::string apiKey, apiSecret;
	if (fCredentialManager->hasCredentials("binance")) {
		if (fCredentialManager->loadCredentials("binance", apiKey, apiSecret)) {
			fApiKeyInput->SetText(apiKey.c_str());
			fApiSecretInput->SetText(apiSecret.c_str());
			UpdateStatus("✓ Credentials loaded from secure storage", false);
			LOG_INFO("Binance credentials loaded successfully");
		} else {
			UpdateStatus("⚠️ Failed to load credentials: " + fCredentialManager->getLastError(), true);
		}
	} else {
		UpdateStatus("ℹ️ No credentials saved. Enter your Binance API keys above.", false);
	}
}

void SettingsView::SaveCredentials() {
	std::string apiKey = fApiKeyInput->Text();
	std::string apiSecret = fApiSecretInput->Text();

	// Validation
	if (apiKey.empty() || apiSecret.empty()) {
		UpdateStatus("⚠️ Please enter both API Key and API Secret", true);

		BAlert* alert = new BAlert("Error",
			"Both API Key and API Secret are required.",
			"OK", nullptr, nullptr,
			B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->Go();
		return;
	}

	// Save encrypted credentials
	if (fCredentialManager->saveCredentials("binance", apiKey, apiSecret)) {
		UpdateStatus("✓ Credentials saved successfully", false);

		BAlert* alert = new BAlert("Success",
			"Your Binance API credentials have been encrypted and saved securely.",
			"OK", nullptr, nullptr,
			B_WIDTH_AS_USUAL, B_INFO_ALERT);
		alert->Go();

		LOG_INFO("Binance credentials saved successfully");
	} else {
		UpdateStatus("⚠️ Failed to save credentials: " + fCredentialManager->getLastError(), true);

		BAlert* alert = new BAlert("Error",
			("Failed to save credentials: " + fCredentialManager->getLastError()).c_str(),
			"OK", nullptr, nullptr,
			B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->Go();
	}
}

void SettingsView::DeleteCredentials() {
	if (!fCredentialManager->hasCredentials("binance")) {
		UpdateStatus("ℹ️ No credentials to delete", false);
		return;
	}

	// Confirm deletion
	BAlert* alert = new BAlert("Confirm Delete",
		"Are you sure you want to delete your Binance API credentials?",
		"Cancel", "Delete", nullptr,
		B_WIDTH_AS_USUAL, B_WARNING_ALERT);

	int32 choice = alert->Go();
	if (choice == 1) {  // Delete button
		if (fCredentialManager->deleteCredentials("binance")) {
			fApiKeyInput->SetText("");
			fApiSecretInput->SetText("");
			UpdateStatus("✓ Credentials deleted successfully", false);
			LOG_INFO("Binance credentials deleted");
		} else {
			UpdateStatus("⚠️ Failed to delete credentials: " + fCredentialManager->getLastError(), true);
		}
	}
}

void SettingsView::TestConnection() {
	std::string apiKey = fApiKeyInput->Text();
	std::string apiSecret = fApiSecretInput->Text();

	if (apiKey.empty() || apiSecret.empty()) {
		UpdateStatus("⚠️ Please enter API credentials first", true);
		return;
	}

	UpdateStatus("Testing connection...", false);

	// Test connection with BinanceAPI
	BinanceAPI api;
	if (api.init(apiKey, apiSecret)) {
		if (api.testConnection()) {
			UpdateStatus("✓ Connection successful! API credentials are valid.", false);

			BAlert* alert = new BAlert("Success",
				"Successfully connected to Binance!\n\n"
				"Your API credentials are valid and working.",
				"OK", nullptr, nullptr,
				B_WIDTH_AS_USUAL, B_INFO_ALERT);
			alert->Go();

			LOG_INFO("Binance API connection test successful");
		} else {
			UpdateStatus("⚠️ Connection failed. Please check your credentials.", true);

			BAlert* alert = new BAlert("Connection Failed",
				"Failed to connect to Binance.\n\n"
				"Please verify:\n"
				"• Your API Key and Secret are correct\n"
				"• Your internet connection is working\n"
				"• Binance API is accessible",
				"OK", nullptr, nullptr,
				B_WIDTH_AS_USUAL, B_WARNING_ALERT);
			alert->Go();

			LOG_ERROR("Binance API connection test failed");
		}
	} else {
		UpdateStatus("⚠️ Failed to initialize Binance API", true);
		LOG_ERROR("Failed to initialize BinanceAPI");
	}
}

void SettingsView::UpdateStatus(const std::string& message, bool isError) {
	fStatusLabel->SetText(message.c_str());

	// Set color based on status
	if (isError) {
		fStatusLabel->SetHighColor(255, 0, 0);  // Red for errors
	} else if (message.find("✓") != std::string::npos) {
		fStatusLabel->SetHighColor(0, 128, 0);  // Green for success
	} else {
		fStatusLabel->SetHighColor(0, 0, 0);  // Black for info
	}

	fStatusLabel->Invalidate();
}

} // namespace UI
} // namespace Emiglio
