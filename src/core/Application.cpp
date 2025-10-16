#include <Application.h>
#include "../ui/MainWindow.h"
#include "../utils/Logger.h"

namespace Emiglio {

class EmiglioApplication : public BApplication {
public:
	EmiglioApplication()
		: BApplication("application/x-vnd.Emiglio")
		, mainWindow(nullptr)
	{
		LOG_INFO("Emiglio Application starting...");
	}

	virtual ~EmiglioApplication() {
		LOG_INFO("Emiglio Application shutting down");
	}

	virtual void ReadyToRun() {
		// Create and show main window
		mainWindow = new UI::MainWindow();
		mainWindow->Show();
	}

	virtual void AboutRequested() {
		mainWindow->PostMessage(UI::MSG_HELP_ABOUT);
	}

private:
	UI::MainWindow* mainWindow;
};

} // namespace Emiglio

int main() {
	// Initialize logger
	Emiglio::Logger::getInstance().setLogLevel(Emiglio::LogLevel::INFO);

	// Create and run application
	Emiglio::EmiglioApplication app;
	app.Run();

	return 0;
}
