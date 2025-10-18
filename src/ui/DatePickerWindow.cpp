#include "DatePickerWindow.h"
#include <LayoutBuilder.h>
#include <GroupView.h>
#include <stdio.h>
#include <string.h>

namespace Emiglio {
namespace UI {

// CalendarView implementation
CalendarView::CalendarView()
	: BView("calendar", B_WILL_DRAW | B_FRAME_EVENTS),
	  cellWidth(30.0f),
	  cellHeight(25.0f),
	  headerHeight(25.0f)
{
	// Set to current date
	time_t now = time(nullptr);
	struct tm* tm = localtime(&now);
	currentYear = tm->tm_year + 1900;
	currentMonth = tm->tm_mon + 1;  // 1-12
	selectedDay = tm->tm_mday;

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetExplicitMinSize(BSize(7 * cellWidth, headerHeight + 6 * cellHeight));
	SetExplicitMaxSize(BSize(7 * cellWidth, headerHeight + 6 * cellHeight));
}

CalendarView::~CalendarView() {
}

void CalendarView::AttachedToWindow() {
	BView::AttachedToWindow();
	CalculateLayout();
}

void CalendarView::CalculateLayout() {
	BRect bounds = Bounds();
	cellWidth = bounds.Width() / 7.0f;

	// Calculate day rectangles
	for (int week = 0; week < 6; week++) {
		for (int day = 0; day < 7; day++) {
			int index = week * 7 + day;
			dayRects[index] = BRect(
				day * cellWidth,
				headerHeight + week * cellHeight,
				(day + 1) * cellWidth - 1,
				headerHeight + (week + 1) * cellHeight - 1
			);
		}
	}
}

int CalendarView::GetDaysInMonth(int year, int month) {
	static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))) {
		return 29;  // Leap year
	}
	return days[month - 1];
}

int CalendarView::GetFirstDayOfWeek(int year, int month) {
	struct tm time = {0};
	time.tm_year = year - 1900;
	time.tm_mon = month - 1;
	time.tm_mday = 1;
	mktime(&time);
	return time.tm_wday;  // 0=Sunday, 6=Saturday
}

void CalendarView::Draw(BRect updateRect) {
	BView::Draw(updateRect);

	// Recalculate layout in case bounds changed
	CalculateLayout();

	// Draw day headers (Sun, Mon, Tue, etc.)
	SetHighColor(0, 0, 0);
	SetFont(be_bold_font);
	const char* dayNames[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	for (int i = 0; i < 7; i++) {
		float x = i * cellWidth + cellWidth / 2 - StringWidth(dayNames[i]) / 2;
		DrawString(dayNames[i], BPoint(x, headerHeight - 8));
	}

	// Draw separator line
	StrokeLine(BPoint(0, headerHeight), BPoint(Bounds().Width(), headerHeight));

	// Calculate days to show
	int daysInMonth = GetDaysInMonth(currentYear, currentMonth);
	int firstDay = GetFirstDayOfWeek(currentYear, currentMonth);

	// Draw day numbers
	SetFont(be_plain_font);
	int dayNum = 1;
	for (int week = 0; week < 6; week++) {
		for (int day = 0; day < 7; day++) {
			int index = week * 7 + day;

			// Only draw days that are in the first week after start day, or subsequent weeks
			if ((week == 0 && day >= firstDay) || (week > 0 && dayNum <= daysInMonth)) {
				BRect rect = dayRects[index];

				// Highlight selected day
				if (dayNum == selectedDay) {
					SetHighColor(100, 150, 255);
					FillRect(rect);
				}

				// Draw day number
				SetHighColor(0, 0, 0);
				char dayStr[3];
				snprintf(dayStr, sizeof(dayStr), "%d", dayNum);
				float x = rect.left + rect.Width() / 2 - StringWidth(dayStr) / 2;
				float y = rect.top + rect.Height() / 2 + 5;
				DrawString(dayStr, BPoint(x, y));

				dayNum++;
				if (dayNum > daysInMonth)
					break;
			}
		}
		if (dayNum > daysInMonth)
			break;
	}
}

void CalendarView::MouseDown(BPoint where) {
	int day = GetDayAt(where);
	if (day > 0) {
		selectedDay = day;
		Invalidate();

		// Send message to parent window
		BMessage msg(MSG_DATE_SELECTED);
		Window()->PostMessage(&msg);
	}
}

int CalendarView::GetDayAt(BPoint point) {
	if (point.y < headerHeight)
		return -1;

	int daysInMonth = GetDaysInMonth(currentYear, currentMonth);
	int firstDay = GetFirstDayOfWeek(currentYear, currentMonth);

	int dayNum = 1;
	for (int week = 0; week < 6; week++) {
		for (int day = 0; day < 7; day++) {
			if ((week == 0 && day >= firstDay) || (week > 0 && dayNum <= daysInMonth)) {
				int index = week * 7 + day;
				if (dayRects[index].Contains(point)) {
					return dayNum;
				}
				dayNum++;
				if (dayNum > daysInMonth)
					return -1;
			}
		}
	}
	return -1;
}

void CalendarView::SetDate(int year, int month, int day) {
	currentYear = year;
	currentMonth = month;
	selectedDay = day;
	Invalidate();
}

void CalendarView::SetMonth(int year, int month) {
	currentYear = year;
	currentMonth = month;
	// Keep selected day if valid, otherwise set to 1
	int daysInMonth = GetDaysInMonth(year, month);
	if (selectedDay > daysInMonth)
		selectedDay = 1;
	Invalidate();
}

void CalendarView::GetSelectedDate(int& year, int& month, int& day) {
	year = currentYear;
	month = currentMonth;
	day = selectedDay;
}

// DatePickerWindow implementation
DatePickerWindow::DatePickerWindow(BPoint position, BTextControl* targetControl)
	: BWindow(BRect(position.x, position.y, position.x + 240, position.y + 240),
	          "Select Date",
	          B_FLOATING_WINDOW_LOOK,
	          B_FLOATING_ALL_WINDOW_FEEL,
	          B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS),
	  target(targetControl),
	  targetMessenger(targetControl)
{
	// Parse current date from target control
	const char* dateStr = targetControl->Text();
	struct tm tm = {0};
	if (sscanf(dateStr, "%d-%d-%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday) == 3) {
		currentYear = tm.tm_year;
		currentMonth = tm.tm_mon;
	} else {
		// Use current date if parsing fails
		time_t now = time(nullptr);
		struct tm* nowTm = localtime(&now);
		currentYear = nowTm->tm_year + 1900;
		currentMonth = nowTm->tm_mon + 1;
	}

	// Create UI
	prevButton = new BButton("<", new BMessage(MSG_PREV_MONTH));
	nextButton = new BButton(">", new BMessage(MSG_NEXT_MONTH));
	monthLabel = new BStringView("month", "");
	calendarView = new CalendarView();
	calendarView->SetMonth(currentYear, currentMonth);

	UpdateMonthLabel();

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.AddGroup(B_HORIZONTAL)
			.Add(prevButton)
			.AddGlue()
			.Add(monthLabel)
			.AddGlue()
			.Add(nextButton)
		.End()
		.Add(calendarView)
	.End();
}

DatePickerWindow::~DatePickerWindow() {
}

void DatePickerWindow::UpdateMonthLabel() {
	const char* monthNames[] = {
		"January", "February", "March", "April", "May", "June",
		"July", "August", "September", "October", "November", "December"
	};

	char label[64];
	snprintf(label, sizeof(label), "%s %d", monthNames[currentMonth - 1], currentYear);
	monthLabel->SetText(label);
}

void DatePickerWindow::MessageReceived(BMessage* message) {
	switch (message->what) {
		case MSG_PREV_MONTH:
			currentMonth--;
			if (currentMonth < 1) {
				currentMonth = 12;
				currentYear--;
			}
			calendarView->SetMonth(currentYear, currentMonth);
			UpdateMonthLabel();
			break;

		case MSG_NEXT_MONTH:
			currentMonth++;
			if (currentMonth > 12) {
				currentMonth = 1;
				currentYear++;
			}
			calendarView->SetMonth(currentYear, currentMonth);
			UpdateMonthLabel();
			break;

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
	int year, month, day;
	calendarView->GetSelectedDate(year, month, day);

	char dateStr[32];
	snprintf(dateStr, sizeof(dateStr), "%04d-%02d-%02d", year, month, day);

	if (target && target->LockLooper()) {
		target->SetText(dateStr);
		target->UnlockLooper();
	}
}

bool DatePickerWindow::QuitRequested() {
	return true;
}

} // namespace UI
} // namespace Emiglio
