#ifndef CREDENTIALMANAGER_H
#define CREDENTIALMANAGER_H

#include <string>
#include <memory>

namespace Emiglio {

/**
 * Manages encrypted storage of API credentials in SQLite database
 * Uses AES-256-CBC encryption with OpenSSL
 */
class CredentialManager {
public:
	CredentialManager();
	~CredentialManager();

	// Initialize with database path
	bool init(const std::string& dbPath);

	// Store encrypted credentials
	bool saveCredentials(const std::string& exchange,
	                    const std::string& apiKey,
	                    const std::string& apiSecret);

	// Retrieve and decrypt credentials
	bool loadCredentials(const std::string& exchange,
	                    std::string& apiKey,
	                    std::string& apiSecret);

	// Check if credentials exist for exchange
	bool hasCredentials(const std::string& exchange);

	// Delete credentials
	bool deleteCredentials(const std::string& exchange);

	// Get last error message
	std::string getLastError() const;

private:
	class Impl;
	std::unique_ptr<Impl> pImpl;
};

} // namespace Emiglio

#endif // CREDENTIALMANAGER_H
