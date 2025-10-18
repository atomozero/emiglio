#ifndef EMIGLIO_DATE_PICKER_WINDOW_H
#define EMIGLIO_DATE_PICKER_WINDOW_H

#include <Window.h>
#include <View.h>
#include <Button.h>
#include <StringView.h>
#include <Messenger.h>
#include <ctime>

namespace BPrivate {
	class BCalendarView;
}

namespace Emiglio {
namespace UI {

// Forward declaration
class DateButton;

// Message constants
enum {
	MSG_DATE_SELECTED = 'dtsl',
	MSG_PREV_MONTH = 'prvm',
	MSG_NEXT_MONTH = 'nxtm',
	MSG_PREV_YEAR = 'prvy',
	MSG_NEXT_YEAR = 'nxty'
};

// Calendar view using native BCalendarView
class CalendarView : public BView {
public:
	CalendarView();
	virtual ~CalendarView();

	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage* message);

	void SetDate(int32 year, int32 month, int32 day);
	void GetSelectedDate(int32& year, int32& month, int32& day);

private:
	void UpdateLabels();

	BPrivate::BCalendarView* calendarView;
	BButton* prevMonthButton;
	BButton* nextMonthButton;
	BButton* prevYearButton;
	BButton* nextYearButton;
	BStringView* monthLabel;
	BStringView* yearLabel;
};

// Date picker popup window
class DatePickerWindow : public BWindow {
public:
	DatePickerWindow(BPoint position, DateButton* targetButton);
	virtual ~DatePickerWindow();

	virtual void MessageReceived(BMessage* message);
	virtual bool QuitRequested();

private:
	void SendDateToTarget();

	DateButton* target;
	BMessenger targetMessenger;
	CalendarView* calendarView;
};

} // namespace UI
} // namespace Emiglio

#endif // EMIGLIO_DATE_PICKER_WINDOW_H
