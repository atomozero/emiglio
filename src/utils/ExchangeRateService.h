#ifndef EXCHANGERATESERVICE_H
#define EXCHANGERATESERVICE_H

#include <string>
#include <map>
#include <ctime>
#include <mutex>

namespace Emiglio {

/**
 * Service for fetching and caching currency exchange rates
 * Uses Frankfurter API (free, no API key required)
 *
 * Features:
 * - Fetches real-time rates from Frankfurter API
 * - Caches rates for 24 hours to minimize API calls
 * - Thread-safe for concurrent access
 * - Fallback to default rates if API fails
 */
class ExchangeRateService {
public:
	// Get singleton instance
	static ExchangeRateService& getInstance();

	// Get exchange rate from USD to target currency
	// Returns the cached rate if available and fresh (< 24h old)
	// Otherwise fetches from API
	double getRate(const std::string& currency);

	// Force refresh rates from API (ignores cache)
	bool refreshRates();

	// Get last update timestamp
	time_t GetLastUpdate() const { return fLastUpdateTime; }

	// Check if rates are stale (older than 24 hours)
	bool IsStale() const;

	// Get all available rates
	std::map<std::string, double> GetAllRates() const;

	// Delete copy constructor and assignment operator
	ExchangeRateService(const ExchangeRateService&) = delete;
	ExchangeRateService& operator=(const ExchangeRateService&) = delete;

private:
	ExchangeRateService();
	~ExchangeRateService();

	// Fetch rates from Frankfurter API
	bool _FetchRatesFromAPI();

	// Get default fallback rates (static, approximate)
	void _LoadDefaultRates();

	// Parse JSON response from API
	bool _ParseRatesJSON(const std::string& json);

	// Save rates to disk cache
	bool _SaveCacheToDisk();

	// Load rates from disk cache
	bool _LoadCacheFromDisk();

	// Get cache file path
	std::string _GetCacheFilePath() const;

	// Currency rates (from USD to currency)
	std::map<std::string, double> fRates;

	// Last update timestamp
	time_t fLastUpdateTime;

	// Mutex for thread safety
	mutable std::mutex fMutex;

	// Cache duration (24 hours in seconds)
	static constexpr time_t kCacheDuration = 24 * 60 * 60;
};

} // namespace Emiglio

#endif // EXCHANGERATESERVICE_H
