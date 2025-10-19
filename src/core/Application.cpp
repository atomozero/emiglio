#include <Application.h>
#include "../ui/MainWindow.h"
#include "../utils/Logger.h"
#include "../utils/NetworkManager.h"
#include "../utils/DataSyncManager.h"
#include "../utils/Config.h"

#include <thread>

namespace Emiglio {

class EmiglioApplication : public BApplication {
public:
	EmiglioApplication()
		: BApplication("application/x-vnd.Emiglio")
		, mainWindow(nullptr)
		, syncInProgress(false)
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

		// Check internet connection and sync data in background
		CheckConnectionAndSync();
	}

	void CheckConnectionAndSync() {
		// Run in background thread to not block UI
		std::thread([this]() {
			NetworkManager& netMgr = NetworkManager::getInstance();

			LOG_INFO("Checking internet connectivity...");
			bool hasInternet = netMgr.hasInternetConnection();

			if (hasInternet) {
				LOG_INFO("Internet connection available - starting data sync");
				netMgr.showNotification(
					"Emiglio - Online Mode",
					"Internet connection detected. Ready to use!",
					false
				);

				// PERFORMANCE FIX: Disable automatic data sync on startup
				// This was causing the app to be slow during first 30 seconds
				// User can manually sync from Dashboard "Refresh" button if needed

				/* DISABLED - Too slow on startup
				syncInProgress = true;
				DataSyncManager& syncMgr = DataSyncManager::getInstance();

				// Set progress callback
				syncMgr.setProgressCallback(
					[&netMgr](int current, int total, const std::string& status) {
						LOG_INFO("Sync progress: " + status);
					}
				);

				bool syncSuccess = syncMgr.syncAllData();
				*/

				// Just log that we're online, don't sync automatically
				LOG_INFO("Internet connection available - data sync disabled for faster startup");
				bool syncSuccess = true;  // Fake success to avoid notification

				if (syncSuccess) {
					// Skip success notification since we didn't actually sync
					// netMgr.showNotification(...);
					LOG_INFO("Startup complete - sync skipped for performance");
				} else {
					netMgr.showNotification(
						"Emiglio - Network Error",
						"Could not verify internet connection",
						true
					);
					LOG_WARNING("Data sync completed with warnings");
				}

				syncInProgress = false;
			} else {
				LOG_WARNING("No internet connection - running in offline mode");
				netMgr.showNotification(
					"Emiglio - Offline Mode",
					"No internet connection detected. Running with cached data only.",
					true
				);
			}
		}).detach();
	}

	virtual void AboutRequested() {
		mainWindow->PostMessage(UI::MSG_HELP_ABOUT);
	}

private:
	UI::MainWindow* mainWindow;
	bool syncInProgress;
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
