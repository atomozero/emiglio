#include "SettingsView.h"
#include <LayoutBuilder.h>
#include <StringView.h>

namespace Emiglio {
namespace UI {

SettingsView::SettingsView()
	: BView("Settings", B_WILL_DRAW)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	BStringView* label = new BStringView("label", "Settings - Coming Soon");
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_WINDOW_SPACING)
		.AddGlue()
		.Add(label)
		.AddGlue()
		.End();
}

SettingsView::~SettingsView() {
}

} // namespace UI
} // namespace Emiglio
