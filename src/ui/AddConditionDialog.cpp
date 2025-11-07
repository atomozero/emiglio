#include "AddConditionDialog.h"
#include <LayoutBuilder.h>
#include <GroupView.h>
#include <TextControl.h>
#include <Button.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Messenger.h>
#include <StringView.h>

namespace Emiglio {
namespace UI {

AddConditionDialog::AddConditionDialog(BMessenger target, const char* title)
	: BWindow(BRect(100, 100, 450, 280),
	          title,
	          B_TITLED_WINDOW,
	          B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
	  fTarget(target),
	  fReplyWhat('cadd')
{
	BuildLayout();
	CenterOnScreen();
}

AddConditionDialog::~AddConditionDialog() {
}

void AddConditionDialog::BuildLayout() {
	// Indicator menu (what indicator to check)
	BPopUpMenu* indicatorPopup = new BPopUpMenu("Select Indicator");
	indicatorPopup->AddItem(new BMenuItem("rsi", nullptr));
	indicatorPopup->AddItem(new BMenuItem("sma", nullptr));
	indicatorPopup->AddItem(new BMenuItem("ema", nullptr));
	indicatorPopup->AddItem(new BMenuItem("macd", nullptr));
	indicatorPopup->AddItem(new BMenuItem("macd_signal", nullptr));
	indicatorPopup->AddItem(new BMenuItem("macd_histogram", nullptr));
	indicatorPopup->AddItem(new BMenuItem("bollinger_upper", nullptr));
	indicatorPopup->AddItem(new BMenuItem("bollinger_middle", nullptr));
	indicatorPopup->AddItem(new BMenuItem("bollinger_lower", nullptr));
	indicatorPopup->AddItem(new BMenuItem("atr", nullptr));
	indicatorPopup->AddItem(new BMenuItem("stochastic_k", nullptr));
	indicatorPopup->AddItem(new BMenuItem("stochastic_d", nullptr));
	indicatorPopup->AddItem(new BMenuItem("obv", nullptr));
	indicatorPopup->AddItem(new BMenuItem("adx", nullptr));
	indicatorPopup->AddItem(new BMenuItem("cci", nullptr));
	indicatorPopup->AddItem(new BMenuItem("price", nullptr));
	indicatorPopup->AddItem(new BMenuItem("volume", nullptr));
	indicatorPopup->ItemAt(0)->SetMarked(true);
	indicatorMenu = new BMenuField("Indicator:", indicatorPopup);

	// Operator menu
	BPopUpMenu* operatorPopup = new BPopUpMenu("Select Operator");
	operatorPopup->AddItem(new BMenuItem("<", nullptr));
	operatorPopup->AddItem(new BMenuItem("<=", nullptr));
	operatorPopup->AddItem(new BMenuItem(">", nullptr));
	operatorPopup->AddItem(new BMenuItem(">=", nullptr));
	operatorPopup->AddItem(new BMenuItem("==", nullptr));
	operatorPopup->AddItem(new BMenuItem("crosses_above", nullptr));
	operatorPopup->AddItem(new BMenuItem("crosses_below", nullptr));
	operatorPopup->ItemAt(0)->SetMarked(true);
	operatorMenu = new BMenuField("Operator:", operatorPopup);

	// Value input
	valueControl = new BTextControl("Value:", "0", nullptr);

	// Buttons
	addButton = new BButton("Add", new BMessage(MSG_ADD));
	addButton->MakeDefault(true);
	cancelButton = new BButton("Cancel", new BMessage(MSG_CANCEL));

	// Layout
	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_WINDOW_SPACING)
		.Add(new BStringView("", "Configure trading condition"))
		.AddGrid(B_USE_SMALL_SPACING, B_USE_SMALL_SPACING)
			.Add(indicatorMenu->CreateLabelLayoutItem(), 0, 0)
			.Add(indicatorMenu->CreateMenuBarLayoutItem(), 1, 0)
			.Add(operatorMenu->CreateLabelLayoutItem(), 0, 1)
			.Add(operatorMenu->CreateMenuBarLayoutItem(), 1, 1)
			.Add(valueControl->CreateLabelLayoutItem(), 0, 2)
			.Add(valueControl->CreateTextViewLayoutItem(), 1, 2)
		.End()
		.AddGlue()
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(cancelButton)
			.Add(addButton)
		.End();

	// Set targets
	addButton->SetTarget(this);
	cancelButton->SetTarget(this);
}

void AddConditionDialog::MessageReceived(BMessage* message) {
	switch (message->what) {
		case MSG_ADD: {
			// Build condition string
			BMenuItem* indicatorItem = indicatorMenu->Menu()->FindMarked();
			BMenuItem* operatorItem = operatorMenu->Menu()->FindMarked();

			if (!indicatorItem || !operatorItem) break;

			// Format: "indicator operator value"
			BString conditionText;
			conditionText << indicatorItem->Label() << " "
			             << operatorItem->Label() << " "
			             << valueControl->Text();

			// Send message back to parent with correct 'what'
			BMessage reply(fReplyWhat);
			reply.AddString("condition", conditionText);
			fTarget.SendMessage(&reply);

			PostMessage(B_QUIT_REQUESTED);
			break;
		}

		case MSG_CANCEL:
			PostMessage(B_QUIT_REQUESTED);
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}

} // namespace UI
} // namespace Emiglio
