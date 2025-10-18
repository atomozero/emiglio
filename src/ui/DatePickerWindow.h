#ifndef EMIGLIO_DATE_PICKER_WINDOW_H
#define EMIGLIO_DATE_PICKER_WINDOW_H

#include <Window.h>
#include <View.h>
#include <Button.h>
#include <StringView.h>
#include <TextControl.h>
#include <Messenger.h>
#include <ctime>

namespace Emiglio {
namespace UI {

// Message constants
enum {
	MSG_DATE_SELECTED = 'dtsl',
	MSG_PREV_MONTH = 'prvm',
	MSG_NEXT_MONTH = 'nxtm'
};

// Calendar view that shows a month grid
class CalendarView : public BView {
public:
	CalendarView();
	virtual ~CalendarView();

	virtual void AttachedToWindow();
	virtual void Draw(BRect updateRect);
	virtual void MouseDown(BPoint where);

	void SetDate(int year, int month, int day);
	void SetMonth(int year, int month);
	void GetSelectedDate(int& year, int& month, int& day);

private:
	void CalculateLayout();
	int GetDayAt(BPoint point);
	int GetDaysInMonth(int year, int month);
	int GetFirstDayOfWeek(int year, int month);  // 0=Sunday, 6=Saturday

	int currentYear;
	int currentMonth;
	int selectedDay;

	BRect dayRects[42];  // Max 6 weeks * 7 days
	float cellWidth;
	float cellHeight;
	float headerHeight;
};

// Date picker popup window
class DatePickerWindow : public BWindow {
public:
	DatePickerWindow(BPoint position, BTextControl* targetControl);
	virtual ~DatePickerWindow();

	virtual void MessageReceived(BMessage* message);
	virtual bool QuitRequested();

private:
	void UpdateMonthLabel();
	void SendDateToTarget();

	BTextControl* target;
	BMessenger targetMessenger;

	BStringView* monthLabel;
	BButton* prevButton;
	BButton* nextButton;
	CalendarView* calendarView;

	int currentYear;
	int currentMonth;
};

} // namespace UI
} // namespace Emiglio

#endif // EMIGLIO_DATE_PICKER_WINDOW_H
