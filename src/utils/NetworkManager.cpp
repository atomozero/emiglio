#include "NetworkManager.h"
#include "Logger.h"

#include <Notification.h>
#include <NetworkAddress.h>
#include <NetworkInterface.h>
#include <NetworkRoster.h>

#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

namespace Emiglio {

NetworkManager::NetworkManager()
	: online(false)
	, lastCheckTime(0)
{
}

NetworkManager::~NetworkManager() {
}

NetworkManager& NetworkManager::getInstance() {
	static NetworkManager instance;
	return instance;
}

bool NetworkManager::hasInternetConnection() {
	// Try to connect to multiple reliable hosts
	const char* testHosts[] = {
		"www.google.com",
		"www.cloudflare.com",
		"8.8.8.8"  // Google DNS
	};

	for (const char* host : testHosts) {
		if (canReachHost(host, 80)) {
			online = true;
			lastCheckTime = time(nullptr);
			LOG_INFO("Internet connection available (verified via " + std::string(host) + ")");
			return true;
		}
	}

	online = false;
	lastCheckTime = time(nullptr);
	LOG_WARNING("No internet connection detected");
	return false;
}

bool NetworkManager::canReachHost(const std::string& host, int port) {
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		LOG_ERROR("Failed to create socket: " + std::string(strerror(errno)));
		return false;
	}

	// Set socket timeout to 3 seconds
	struct timeval timeout;
	timeout.tv_sec = 3;
	timeout.tv_usec = 0;

	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
		LOG_WARNING("Failed to set socket timeout");
	}

	if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
		LOG_WARNING("Failed to set socket send timeout");
	}

	struct hostent* server = gethostbyname(host.c_str());
	if (server == nullptr) {
		close(sock);
		return false;
	}

	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
	serv_addr.sin_port = htons(port);

	// Try to connect
	int result = connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
	close(sock);

	if (result == 0) {
		return true;
	}

	return false;
}

void NetworkManager::showNotification(const std::string& title,
                                      const std::string& message,
                                      bool isError) {
	BNotification notification(isError ? B_ERROR_NOTIFICATION : B_INFORMATION_NOTIFICATION);

	notification.SetGroup("Emiglio");
	notification.SetTitle(title.c_str());
	notification.SetContent(message.c_str());
	notification.SetMessageID("emiglio_network_status");

	// Note: SetIcon() requires BBitmap*, not icon name
	// For now we'll rely on the notification type for visual distinction

	notification.Send();

	LOG_INFO("Notification sent: " + title + " - " + message);
}

} // namespace Emiglio
