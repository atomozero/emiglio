#include <iostream>
#include <string>
#include <iomanip>
#include "../src/exchange/BinanceAPI.h"
#include "../src/utils/Logger.h"

using namespace Emiglio;

int main(int argc, char* argv[]) {
	std::cout << "=== Binance API Login Test ===" << std::endl;
	std::cout << std::endl;

	// Check arguments
	if (argc != 3) {
		std::cerr << "Usage: " << argv[0] << " <API_KEY> <API_SECRET>" << std::endl;
		std::cerr << std::endl;
		std::cerr << "Example:" << std::endl;
		std::cerr << "  " << argv[0] << " \"your_api_key_here\" \"your_api_secret_here\"" << std::endl;
		std::cerr << std::endl;
		std::cerr << "Note: Get your API keys from Binance.com:" << std::endl;
		std::cerr << "  1. Log in to Binance" << std::endl;
		std::cerr << "  2. Profile → API Management" << std::endl;
		std::cerr << "  3. Create new API key (Read-only permissions)" << std::endl;
		return 1;
	}

	std::string apiKey = argv[1];
	std::string apiSecret = argv[2];

	// Validate inputs
	if (apiKey.empty() || apiSecret.empty()) {
		std::cerr << "Error: API Key and Secret cannot be empty" << std::endl;
		return 1;
	}

	std::cout << "API Key: " << apiKey.substr(0, 8) << "..." << std::endl;
	std::cout << "API Secret: " << std::string(apiSecret.length(), '*') << std::endl;
	std::cout << std::endl;

	// Initialize API
	std::cout << "Step 1: Initializing Binance API..." << std::endl;
	BinanceAPI api;
	if (!api.init(apiKey, apiSecret)) {
		std::cerr << "✗ Failed to initialize Binance API" << std::endl;
		return 1;
	}
	std::cout << "✓ API initialized successfully" << std::endl;
	std::cout << std::endl;

	// Test connection
	std::cout << "Step 2: Testing connection (ping)..." << std::endl;
	if (!api.ping()) {
		std::cerr << "✗ Ping failed - Cannot reach Binance API" << std::endl;
		return 1;
	}
	std::cout << "✓ Ping successful" << std::endl;
	std::cout << std::endl;

	// Get server time
	std::cout << "Step 3: Getting server time..." << std::endl;
	time_t serverTime = api.getServerTime();
	if (serverTime == 0) {
		std::cerr << "✗ Failed to get server time" << std::endl;
		return 1;
	}
	std::cout << "✓ Server time: " << serverTime << std::endl;
	std::cout << std::endl;

	// Test connection with authentication
	std::cout << "Step 4: Testing authenticated connection..." << std::endl;
	if (!api.testConnection()) {
		std::cerr << "✗ Authentication test failed" << std::endl;
		std::cerr << "  Possible reasons:" << std::endl;
		std::cerr << "  - Invalid API Key or Secret" << std::endl;
		std::cerr << "  - API keys don't have required permissions" << std::endl;
		std::cerr << "  - IP restriction on API keys" << std::endl;
		return 1;
	}
	std::cout << "✓ Authentication successful" << std::endl;
	std::cout << std::endl;

	// Get account balances
	std::cout << "Step 5: Fetching account balances..." << std::endl;
	std::vector<Balance> balances = api.getBalances();

	if (balances.empty()) {
		std::cout << "ℹ️  No balances found (account is empty)" << std::endl;
	} else {
		std::cout << "✓ Found " << balances.size() << " non-zero balances:" << std::endl;
		std::cout << std::endl;
		std::cout << "Asset          Total           Free            Locked" << std::endl;
		std::cout << "--------------------------------------------------------------" << std::endl;

		for (const auto& balance : balances) {
			std::cout << std::left << std::setw(15) << balance.asset;
			std::cout << std::right << std::setw(16) << std::fixed << std::setprecision(8) << balance.total;
			std::cout << std::setw(16) << balance.free;
			std::cout << std::setw(16) << balance.locked;
			std::cout << std::endl;
		}
	}
	std::cout << std::endl;

	// Summary
	std::cout << "=== Test Summary ===" << std::endl;
	std::cout << "✓ All tests passed!" << std::endl;
	std::cout << "✓ API credentials are valid" << std::endl;
	std::cout << "✓ Connection to Binance is working" << std::endl;

	if (!balances.empty()) {
		std::cout << "✓ Successfully retrieved " << balances.size() << " asset balances" << std::endl;
	}

	std::cout << std::endl;
	std::cout << "You can now use these credentials in Emiglio Settings tab." << std::endl;

	return 0;
}
