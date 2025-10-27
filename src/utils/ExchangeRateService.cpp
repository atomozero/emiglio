#include "ExchangeRateService.h"
#include "Logger.h"
#include "JsonParser.h"
#include <sstream>
#include <fstream>
#include <iomanip>
#include <cstring>
#include <cstdlib>
#include <curl/curl.h>

namespace Emiglio {

// Callback for curl to write data
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

ExchangeRateService::ExchangeRateService()
	: fLastUpdateTime(0) {
	_LoadDefaultRates();

	// Try to load cached rates from disk
	if (_LoadCacheFromDisk()) {
		LOG_INFO("ExchangeRateService: Loaded rates from disk cache");
	} else {
		LOG_INFO("ExchangeRateService: No disk cache found, will fetch from API");
	}
}

ExchangeRateService::~ExchangeRateService() {
}

ExchangeRateService& ExchangeRateService::getInstance() {
	static ExchangeRateService instance;
	return instance;
}

void ExchangeRateService::_LoadDefaultRates() {
	std::lock_guard<std::mutex> lock(fMutex);

	// Default fallback rates (approximate, updated Oct 2024)
	fRates["USD"] = 1.0;       // Base currency
	fRates["EUR"] = 0.856;     // Euro
	fRates["GBP"] = 0.744;     // British Pound
	fRates["JPY"] = 150.42;    // Japanese Yen
	fRates["CHF"] = 0.792;     // Swiss Franc
	fRates["AUD"] = 1.541;     // Australian Dollar
	fRates["CAD"] = 1.403;     // Canadian Dollar
	fRates["CNY"] = 7.122;     // Chinese Yuan
	fRates["INR"] = 88.0;      // Indian Rupee
	fRates["BRL"] = 5.433;     // Brazilian Real

	LOG_INFO("ExchangeRateService: Loaded default rates");
}

bool ExchangeRateService::_FetchRatesFromAPI() {
	CURL* curl;
	CURLcode res;
	std::string readBuffer;

	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();

	if (!curl) {
		LOG_ERROR("ExchangeRateService: Failed to initialize CURL");
		return false;
	}

	// Frankfurter API endpoint (free, no API key needed)
	const char* url = "https://api.frankfurter.app/latest?from=USD";

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L); // 10 second timeout
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // Follow redirects

	// Enable SSL certificate verification for security
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

	// NOTE: If Haiku OS has certificate bundle issues, install ca-certificates package:
	// pkgman install ca_root_certificates

	res = curl_easy_perform(curl);

	if (res != CURLE_OK) {
		LOG_ERROR("ExchangeRateService: CURL failed: " + std::string(curl_easy_strerror(res)));
		curl_easy_cleanup(curl);
		curl_global_cleanup();
		return false;
	}

	long http_code = 0;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

	curl_easy_cleanup(curl);
	curl_global_cleanup();

	if (http_code != 200) {
		LOG_ERROR("ExchangeRateService: HTTP error " + std::to_string(http_code));
		return false;
	}

	// Parse JSON response
	if (!_ParseRatesJSON(readBuffer)) {
		LOG_ERROR("ExchangeRateService: Failed to parse JSON response");
		return false;
	}

	LOG_INFO("ExchangeRateService: Successfully fetched rates from API");
	return true;
}

bool ExchangeRateService::_ParseRatesJSON(const std::string& json) {
	JsonParser parser;
	if (!parser.parse(json)) {
		LOG_ERROR("ExchangeRateService: JSON parse error: " + parser.getError());
		return false;
	}

	// Check if "rates" object exists
	if (!parser.has("rates")) {
		LOG_ERROR("ExchangeRateService: No 'rates' object in JSON");
		return false;
	}

	std::lock_guard<std::mutex> lock(fMutex);

	// USD is always 1.0 (base currency)
	fRates["USD"] = 1.0;

	// Extract rates for currencies we support
	const char* currencies[] = {
		"EUR", "GBP", "JPY", "CHF", "AUD",
		"CAD", "CNY", "INR", "BRL"
	};

	for (const char* currency : currencies) {
		std::string key = std::string("rates.") + currency;
		if (parser.has(key)) {
			double rate = parser.getDouble(key, 0.0);
			if (rate > 0.0) {
				fRates[currency] = rate;
				LOG_DEBUG("ExchangeRateService: " + std::string(currency) + " = " + std::to_string(rate));
			}
		}
	}

	// Update timestamp
	fLastUpdateTime = std::time(nullptr);

	// Save to disk cache
	_SaveCacheToDisk();

	return true;
}

bool ExchangeRateService::refreshRates() {
	LOG_INFO("ExchangeRateService: Refreshing exchange rates from API...");

	if (_FetchRatesFromAPI()) {
		return true;
	}

	// If API fetch fails, keep using cached/default rates
	LOG_WARNING("ExchangeRateService: Failed to fetch from API, using cached rates");
	return false;
}

double ExchangeRateService::getRate(const std::string& currency) {
	// Auto-refresh if cache is stale (older than 24 hours)
	if (IsStale()) {
		LOG_INFO("ExchangeRateService: Cache is stale, refreshing...");
		refreshRates();
	}

	std::lock_guard<std::mutex> lock(fMutex);

	auto it = fRates.find(currency);
	if (it != fRates.end()) {
		return it->second;
	}

	// Default to 1.0 if currency not found
	LOG_WARNING("ExchangeRateService: Currency not found: " + currency + ", returning 1.0");
	return 1.0;
}

bool ExchangeRateService::IsStale() const {
	if (fLastUpdateTime == 0) {
		return true; // Never updated
	}

	time_t now = std::time(nullptr);
	return (now - fLastUpdateTime) > kCacheDuration;
}

std::map<std::string, double> ExchangeRateService::GetAllRates() const {
	std::lock_guard<std::mutex> lock(fMutex);
	return fRates;
}

std::string ExchangeRateService::_GetCacheFilePath() const {
	const char* home = std::getenv("HOME");
	if (!home) {
		home = "/boot/home";
	}
	return std::string(home) + "/config/settings/Emiglio/exchange_rates_cache.json";
}

bool ExchangeRateService::_SaveCacheToDisk() {
	std::string cacheFile = _GetCacheFilePath();

	// Copy rates to local variable with mutex locked
	std::map<std::string, double> localRates;
	time_t localTimestamp;
	{
		std::lock_guard<std::mutex> lock(fMutex);
		localRates = fRates;
		localTimestamp = fLastUpdateTime;
	}

	std::ofstream file(cacheFile);
	if (!file.is_open()) {
		LOG_ERROR("ExchangeRateService: Failed to open cache file for writing: " + cacheFile);
		return false;
	}

	// Write JSON manually (simple format)
	file << "{\n";
	file << "  \"timestamp\": " << localTimestamp << ",\n";
	file << "  \"rates\": {\n";

	bool first = true;
	for (const auto& pair : localRates) {
		if (!first) {
			file << ",\n";
		}
		file << "    \"" << pair.first << "\": " << std::fixed << std::setprecision(6) << pair.second;
		first = false;
	}

	file << "\n  }\n";
	file << "}\n";

	file.close();

	LOG_INFO("ExchangeRateService: Saved rates to disk cache: " + cacheFile);
	return true;
}

bool ExchangeRateService::_LoadCacheFromDisk() {
	std::string cacheFile = _GetCacheFilePath();

	std::ifstream file(cacheFile);
	if (!file.is_open()) {
		LOG_DEBUG("ExchangeRateService: No disk cache file found: " + cacheFile);
		return false;
	}

	std::stringstream buffer;
	buffer << file.rdbuf();
	file.close();

	// Parse JSON
	JsonParser parser;
	if (!parser.parse(buffer.str())) {
		LOG_ERROR("ExchangeRateService: Failed to parse disk cache: " + parser.getError());
		return false;
	}

	// Load timestamp
	if (parser.has("timestamp")) {
		fLastUpdateTime = parser.getInt64("timestamp", 0);
	} else {
		LOG_WARNING("ExchangeRateService: No timestamp in cache file");
		return false;
	}

	// Load rates
	if (!parser.has("rates")) {
		LOG_ERROR("ExchangeRateService: No rates object in cache file");
		return false;
	}

	std::lock_guard<std::mutex> lock(fMutex);

	const char* currencies[] = {
		"USD", "EUR", "GBP", "JPY", "CHF", "AUD",
		"CAD", "CNY", "INR", "BRL"
	};

	for (const char* currency : currencies) {
		std::string key = std::string("rates.") + currency;
		if (parser.has(key)) {
			double rate = parser.getDouble(key, 0.0);
			if (rate > 0.0) {
				fRates[currency] = rate;
			}
		}
	}

	LOG_INFO("ExchangeRateService: Loaded " + std::to_string(fRates.size()) + " rates from disk cache");
	return true;
}

} // namespace Emiglio
