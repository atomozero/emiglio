#include "DatePickerWindow.h"
#include "BacktestView.h"
#include <LayoutBuilder.h>
#include <GroupView.h>
#include <DateFormat.h>
#include <private/shared/CalendarView.h>
#include <stdio.h>
#include <string.h>

namespace Emiglio {
namespace UI {

// CalendarView implementation
CalendarView::CalendarView()
	: BView("calendar", B_WILL_DRAW | B_FRAME_EVENTS)
{
	// Create native BCalendarView
	calendarView = new BPrivate::BCalendarView(Bounds(), "calendarView", B_FOLLOW_ALL);

	// Create navigation buttons with Unicode characters for modern look
	prevMonthButton = new BButton("－", new BMessage(MSG_PREV_MONTH));
	prevMonthButton->SetFlat(true);
	prevMonthButton->SetFont(be_bold_font);
	prevMonthButton->SetExplicitMinSize(BSize(prevMonthButton->StringWidth("－") * 2, B_SIZE_UNSET));

	nextMonthButton = new BButton("＋", new BMessage(MSG_NEXT_MONTH));
	nextMonthButton->SetFlat(true);
	nextMonthButton->SetFont(be_bold_font);
	nextMonthButton->SetExplicitMinSize(BSize(nextMonthButton->StringWidth("＋") * 2, B_SIZE_UNSET));

	prevYearButton = new BButton("－", new BMessage(MSG_PREV_YEAR));
	prevYearButton->SetFlat(true);
	prevYearButton->SetFont(be_bold_font);
	prevYearButton->SetExplicitMinSize(BSize(prevYearButton->StringWidth("－") * 2, B_SIZE_UNSET));

	nextYearButton = new BButton("＋", new BMessage(MSG_NEXT_YEAR));
	nextYearButton->SetFlat(true);
	nextYearButton->SetFont(be_bold_font);
	nextYearButton->SetExplicitMinSize(BSize(nextYearButton->StringWidth("＋") * 2, B_SIZE_UNSET));

	monthLabel = new BStringView("monthLabel", "");
	yearLabel = new BStringView("yearLabel", "");

	// Build layout
	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_HALF_ITEM_SPACING)
		.SetInsets(0.0)
		.AddGroup(B_HORIZONTAL, B_USE_HALF_ITEM_SPACING)
			.AddGlue()
			.Add(prevMonthButton)
			.Add(monthLabel)
			.Add(nextMonthButton)
			.AddGlue(3.0)
			.Add(prevYearButton)
			.Add(yearLabel)
			.Add(nextYearButton)
			.AddGlue()
		.End()
		.Add(calendarView);

	UpdateLabels();
}

CalendarView::~CalendarView() {
}

void CalendarView::AttachedToWindow() {
	BView::AttachedToWindow();

	// Set button targets
	prevMonthButton->SetTarget(this);
	nextMonthButton->SetTarget(this);
	prevYearButton->SetTarget(this);
	nextYearButton->SetTarget(this);
}

void CalendarView::MessageReceived(BMessage* message) {
	switch (message->what) {
		case MSG_PREV_YEAR:
			calendarView->SetYear(calendarView->Year() - 1);
			UpdateLabels();
			break;

		case MSG_NEXT_YEAR:
			calendarView->SetYear(calendarView->Year() + 1);
			UpdateLabels();
			break;

		case MSG_PREV_MONTH:
			if (calendarView->Month() == 1) {
				calendarView->SetMonth(12);
				calendarView->SetYear(calendarView->Year() - 1);
			} else {
				calendarView->SetMonth(calendarView->Month() - 1);
			}
			UpdateLabels();
			break;

		case MSG_NEXT_MONTH:
			if (calendarView->Month() == 12) {
				calendarView->SetMonth(1);
				calendarView->SetYear(calendarView->Year() + 1);
			} else {
				calendarView->SetMonth(calendarView->Month() + 1);
			}
			UpdateLabels();
			break;

		default:
			BView::MessageReceived(message);
			break;
	}
}

void CalendarView::SetDate(int32 year, int32 month, int32 day) {
	if (year > 0)
		calendarView->SetYear(year);
	if (month > 0)
		calendarView->SetMonth(month);
	if (day > 0)
		calendarView->SetDay(day);

	UpdateLabels();
}

void CalendarView::GetSelectedDate(int32& year, int32& month, int32& day) {
	year = calendarView->Year();
	month = calendarView->Month();
	day = calendarView->Day();
}

void CalendarView::UpdateLabels() {
	// Update year label
	BString yearString;
	yearString << calendarView->Year();
	yearLabel->SetText(yearString);

	// Update month label using localized month name
	BString monthString;
	BDateFormat().GetMonthName(calendarView->Month(), monthString);
	monthLabel->SetText(monthString);
}

// DatePickerWindow implementation
DatePickerWindow::DatePickerWindow(BPoint position, DateButton* targetButton)
	: BWindow(BRect(position.x, position.y, position.x + 260, position.y + 260),
	          "Select Date",
	          B_FLOATING_WINDOW_LOOK,
	          B_FLOATING_ALL_WINDOW_FEEL,
	          B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS),
	  target(targetButton),
	  targetMessenger(targetButton)
{
	// Parse current date from target button
	const char* dateStr = targetButton->GetDate();
	int32 year = 0, month = 0, day = 0;

	if (sscanf(dateStr, "%d-%d-%d", &year, &month, &day) == 3) {
		// Successfully parsed date
	} else {
		// Use current date if parsing fails
		time_t now = time(nullptr);
		struct tm* nowTm = localtime(&now);
		year = nowTm->tm_year + 1900;
		month = nowTm->tm_mon + 1;
		day = nowTm->tm_mday;
	}

	// Create calendar view
	calendarView = new CalendarView();
	calendarView->SetDate(year, month, day);

	// Build layout
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(calendarView)
	.End();
}

DatePickerWindow::~DatePickerWindow() {
}

void DatePickerWindow::MessageReceived(BMessage* message) {
	switch (message->what) {
		case MSG_DATE_SELECTED:
			SendDateToTarget();
			PostMessage(B_QUIT_REQUESTED);
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}

void DatePickerWindow::SendDateToTarget() {
	int32 year, month, day;
	calendarView->GetSelectedDate(year, month, day);

	char dateStr[32];
	snprintf(dateStr, sizeof(dateStr), "%04d-%02d-%02d", year, month, day);

	if (target && target->LockLooper()) {
		target->SetDate(dateStr);
		target->UnlockLooper();
	}
}

bool DatePickerWindow::QuitRequested() {
	return true;
}

} // namespace UI
} // namespace Emiglio
