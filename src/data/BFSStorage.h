#ifndef BFSSTORAGE_H
#define BFSSTORAGE_H

#include <string>
#include <vector>
#include "DataStorage.h"

namespace Emiglio {

// BFS-native storage using extended attributes and queries
// Haiku's BFS supports:
// - Extended attributes (metadata on files)
// - Live queries (filesystem-level indexing)
// - Fast attribute-based searches
class BFSStorage {
public:
	BFSStorage();
	~BFSStorage();

	// Initialize storage directory
	bool init(const std::string& storagePath);

	// Close and cleanup
	void close();

	// Candle operations using BFS attributes
	bool insertCandle(const Candle& candle);
	bool insertCandles(const std::vector<Candle>& candles);

	// Query using BFS live queries
	std::vector<Candle> getCandles(const std::string& exchange,
	                                const std::string& symbol,
	                                const std::string& timeframe,
	                                time_t startTime,
	                                time_t endTime);

	int getCandleCount(const std::string& exchange,
	                   const std::string& symbol,
	                   const std::string& timeframe);

	// Clear candles
	bool clearCandles(const std::string& exchange,
	                  const std::string& symbol,
	                  const std::string& timeframe);

private:
	class Impl;
	std::unique_ptr<Impl> pImpl;
};

} // namespace Emiglio

#endif // BFSSTORAGE_H
