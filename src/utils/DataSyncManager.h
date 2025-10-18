#ifndef EMIGLIO_DATASYNCMANAGER_H
#define EMIGLIO_DATASYNCMANAGER_H

#include <string>
#include <vector>
#include <functional>

namespace Emiglio {

class DataSyncManager {
public:
	static DataSyncManager& getInstance();

	// Sync all data from last available date
	// Returns true if sync was successful
	bool syncAllData();

	// Sync specific symbol from last available date
	bool syncSymbol(const std::string& exchange, const std::string& symbol,
	                const std::string& timeframe);

	// Get list of symbols that need syncing
	std::vector<std::string> getSymbolsNeedingSync();

	// Set callback for progress updates
	void setProgressCallback(std::function<void(int, int, const std::string&)> callback);

	// Delete copy constructor and assignment operator
	DataSyncManager(const DataSyncManager&) = delete;
	DataSyncManager& operator=(const DataSyncManager&) = delete;

private:
	DataSyncManager();
	~DataSyncManager();

	std::function<void(int, int, const std::string&)> progressCallback;

	// Helper to get last timestamp for a symbol
	int64_t getLastTimestamp(const std::string& exchange,
	                         const std::string& symbol,
	                         const std::string& timeframe);

	// Helper to download data for a date range
	bool downloadRange(const std::string& exchange,
	                   const std::string& symbol,
	                   const std::string& timeframe,
	                   int64_t startTime,
	                   int64_t endTime);
};

} // namespace Emiglio

#endif // EMIGLIO_DATASYNCMANAGER_H
