#ifndef ADDCONDITIONDIALOG_H
#define ADDCONDITIONDIALOG_H

#include <Window.h>
#include <String.h>

class BTextControl;
class BMenuField;
class BButton;

namespace Emiglio {
namespace UI {

class AddConditionDialog : public BWindow {
public:
	AddConditionDialog(BMessenger target, const char* title = "Add Condition");
	virtual ~AddConditionDialog();

	void SetMessageWhat(uint32 what) { fReplyWhat = what; }
	virtual void MessageReceived(BMessage* message) override;

private:
	void BuildLayout();

	BMenuField* indicatorMenu;
	BMenuField* operatorMenu;
	BTextControl* valueControl;
	BButton* addButton;
	BButton* cancelButton;

	BMessenger fTarget;
	uint32 fReplyWhat;

	enum {
		MSG_ADD = 'cadd',
		MSG_CANCEL = 'ccan'
	};
};

} // namespace UI
} // namespace Emiglio

#endif // ADDCONDITIONDIALOG_H
