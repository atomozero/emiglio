#ifndef ADDINDICATORDIALOG_H
#define ADDINDICATORDIALOG_H

#include <Window.h>
#include <String.h>

class BTextControl;
class BMenuField;
class BButton;

namespace Emiglio {
namespace UI {

class AddIndicatorDialog : public BWindow {
public:
	AddIndicatorDialog(BMessenger target);
	virtual ~AddIndicatorDialog();

	virtual void MessageReceived(BMessage* message) override;

private:
	void BuildLayout();

	BMenuField* indicatorMenu;
	BTextControl* periodControl;
	BTextControl* param1Control;
	BTextControl* param2Control;
	BButton* addButton;
	BButton* cancelButton;

	BMessenger fTarget;

	enum {
		MSG_ADD = 'iadd',
		MSG_CANCEL = 'ican',
		MSG_INDICATOR_SELECTED = 'isel'
	};
};

} // namespace UI
} // namespace Emiglio

#endif // ADDINDICATORDIALOG_H
