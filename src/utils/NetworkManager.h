#ifndef EMIGLIO_NETWORKMANAGER_H
#define EMIGLIO_NETWORKMANAGER_H

#include <string>
#include <functional>

namespace Emiglio {

class NetworkManager {
public:
	static NetworkManager& getInstance();

	// Check if internet connection is available
	bool hasInternetConnection();

	// Check if we can reach a specific host
	bool canReachHost(const std::string& host, int port = 443);

	// Show system notification
	void showNotification(const std::string& title, const std::string& message,
	                      bool isError = false);

	// Get last check time
	time_t getLastCheckTime() const { return lastCheckTime; }

	// Get connection status
	bool isOnline() const { return online; }

	// Delete copy constructor and assignment operator
	NetworkManager(const NetworkManager&) = delete;
	NetworkManager& operator=(const NetworkManager&) = delete;

private:
	NetworkManager();
	~NetworkManager();

	bool online;
	time_t lastCheckTime;
};

} // namespace Emiglio

#endif // EMIGLIO_NETWORKMANAGER_H
