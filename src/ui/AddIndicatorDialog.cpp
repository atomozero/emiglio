#include "AddIndicatorDialog.h"
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

AddIndicatorDialog::AddIndicatorDialog(BMessenger target)
	: BWindow(BRect(100, 100, 500, 350),
	          "Add Indicator",
	          B_TITLED_WINDOW,
	          B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
	  fTarget(target)
{
	BuildLayout();
	CenterOnScreen();
}

AddIndicatorDialog::~AddIndicatorDialog() {
}

void AddIndicatorDialog::BuildLayout() {
	// Indicator type menu
	BPopUpMenu* popup = new BPopUpMenu("Select Indicator");
	popup->AddItem(new BMenuItem("rsi", new BMessage(MSG_INDICATOR_SELECTED)));
	popup->AddItem(new BMenuItem("sma", new BMessage(MSG_INDICATOR_SELECTED)));
	popup->AddItem(new BMenuItem("ema", new BMessage(MSG_INDICATOR_SELECTED)));
	popup->AddItem(new BMenuItem("macd", new BMessage(MSG_INDICATOR_SELECTED)));
	popup->AddItem(new BMenuItem("bollinger", new BMessage(MSG_INDICATOR_SELECTED)));
	popup->AddItem(new BMenuItem("atr", new BMessage(MSG_INDICATOR_SELECTED)));
	popup->AddItem(new BMenuItem("stochastic", new BMessage(MSG_INDICATOR_SELECTED)));
	popup->AddItem(new BMenuItem("obv", new BMessage(MSG_INDICATOR_SELECTED)));
	popup->AddItem(new BMenuItem("adx", new BMessage(MSG_INDICATOR_SELECTED)));
	popup->AddItem(new BMenuItem("cci", new BMessage(MSG_INDICATOR_SELECTED)));
	popup->SetTargetForItems(this);
	popup->ItemAt(0)->SetMarked(true);

	indicatorMenu = new BMenuField("Indicator:", popup);

	// Period input
	periodControl = new BTextControl("Period:", "14", nullptr);

	// Optional parameters (will show/hide based on indicator type)
	param1Control = new BTextControl("Param 1:", "", nullptr);
	param2Control = new BTextControl("Param 2:", "", nullptr);

	// Buttons
	addButton = new BButton("Add", new BMessage(MSG_ADD));
	addButton->MakeDefault(true);
	cancelButton = new BButton("Cancel", new BMessage(MSG_CANCEL));

	// Layout
	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_WINDOW_SPACING)
		.Add(new BStringView("", "Configure indicator parameters"))
		.AddGrid(B_USE_SMALL_SPACING, B_USE_SMALL_SPACING)
			.Add(indicatorMenu->CreateLabelLayoutItem(), 0, 0)
			.Add(indicatorMenu->CreateMenuBarLayoutItem(), 1, 0)
			.Add(periodControl->CreateLabelLayoutItem(), 0, 1)
			.Add(periodControl->CreateTextViewLayoutItem(), 1, 1)
			.Add(param1Control->CreateLabelLayoutItem(), 0, 2)
			.Add(param1Control->CreateTextViewLayoutItem(), 1, 2)
			.Add(param2Control->CreateLabelLayoutItem(), 0, 3)
			.Add(param2Control->CreateTextViewLayoutItem(), 1, 3)
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

void AddIndicatorDialog::MessageReceived(BMessage* message) {
	switch (message->what) {
		case MSG_INDICATOR_SELECTED: {
			// Update param labels based on indicator type
			BMenuItem* item = indicatorMenu->Menu()->FindMarked();
			if (item) {
				BString label = item->Label();
				if (label == "rsi") {
					param1Control->SetLabel("Oversold:");
					param1Control->SetText("30");
					param2Control->SetLabel("Overbought:");
					param2Control->SetText("70");
				} else if (label == "bollinger") {
					param1Control->SetLabel("Std Dev:");
					param1Control->SetText("2.0");
					param2Control->SetLabel("");
					param2Control->SetText("");
				} else if (label == "macd") {
					param1Control->SetLabel("Fast Period:");
					param1Control->SetText("12");
					param2Control->SetLabel("Slow Period:");
					param2Control->SetText("26");
				} else {
					param1Control->SetLabel("Param 1:");
					param1Control->SetText("");
					param2Control->SetLabel("Param 2:");
					param2Control->SetText("");
				}
			}
			break;
		}

		case MSG_ADD: {
			// Build indicator string
			BMenuItem* item = indicatorMenu->Menu()->FindMarked();
			if (!item) break;

			BString indicatorText;
			indicatorText << item->Label() << "(period=" << periodControl->Text();

			// Add optional parameters if not empty
			if (strlen(param1Control->Text()) > 0) {
				// Get param name from label (remove ':')
				BString param1Label = param1Control->Label();
				param1Label.Remove(param1Label.FindLast(":"), 1);
				param1Label.ToLower();
				param1Label.ReplaceAll(" ", "_");

				indicatorText << ", " << param1Label << "=" << param1Control->Text();
			}

			if (strlen(param2Control->Text()) > 0) {
				BString param2Label = param2Control->Label();
				param2Label.Remove(param2Label.FindLast(":"), 1);
				param2Label.ToLower();
				param2Label.ReplaceAll(" ", "_");

				indicatorText << ", " << param2Label << "=" << param2Control->Text();
			}

			indicatorText << ")";

			// Send message back to parent
			BMessage reply(MSG_ADD);
			reply.AddString("indicator", indicatorText);
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
