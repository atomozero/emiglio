#include "DataStorage.h"
#include "../utils/Logger.h"
#include <sqlite3.h>
#include <sstream>

namespace Emiglio {

// Fixed: RAII wrapper for sqlite3_stmt* to ensure finalize is called on all paths
class StmtHandle {
	sqlite3_stmt* stmt;
public:
	explicit StmtHandle() : stmt(nullptr) {}
	~StmtHandle() { if (stmt) sqlite3_finalize(stmt); }

	sqlite3_stmt* get() { return stmt; }
	sqlite3_stmt** ptr() { return &stmt; }
	operator sqlite3_stmt*() { return stmt; }

	// Delete copy/move to prevent double-finalize
	StmtHandle(const StmtHandle&) = delete;
	StmtHandle& operator=(const StmtHandle&) = delete;
};

// Private implementation (PIMPL pattern)
class DataStorage::Impl {
public:
	sqlite3* db;
	bool initialized;

	Impl() : db(nullptr), initialized(false) {}

	~Impl() {
		if (db) {
			sqlite3_close(db);
		}
	}

	bool executeSQL(const std::string& sql) {
		char* errMsg = nullptr;
		int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);

		if (rc != SQLITE_OK) {
			LOG_ERROR("SQL error: " + std::string(errMsg));
			sqlite3_free(errMsg);
			return false;
		}

		return true;
	}

	// Helper function to safely extract text from SQLite column (NULL check)
	std::string safeColumnText(sqlite3_stmt* stmt, int col) {
		const unsigned char* text = sqlite3_column_text(stmt, col);
		return text ? reinterpret_cast<const char*>(text) : "";
	}

	bool createTables() {
		std::string sql = R"(
			CREATE TABLE IF NOT EXISTS candles (
				id INTEGER PRIMARY KEY AUTOINCREMENT,
				exchange TEXT NOT NULL,
				symbol TEXT NOT NULL,
				timeframe TEXT NOT NULL,
				timestamp INTEGER NOT NULL,
				open REAL NOT NULL,
				high REAL NOT NULL,
				low REAL NOT NULL,
				close REAL NOT NULL,
				volume REAL NOT NULL,
				UNIQUE(exchange, symbol, timeframe, timestamp)
			);

			CREATE INDEX IF NOT EXISTS idx_candles_lookup
			ON candles(exchange, symbol, timeframe, timestamp);

			CREATE TABLE IF NOT EXISTS trades (
				id INTEGER PRIMARY KEY AUTOINCREMENT,
				strategy_name TEXT NOT NULL,
				backtest_id TEXT,
				timestamp INTEGER NOT NULL,
				symbol TEXT NOT NULL,
				side TEXT NOT NULL,
				price REAL NOT NULL,
				quantity REAL NOT NULL,
				commission REAL NOT NULL,
				pnl REAL,
				portfolio_value REAL
			);

			CREATE INDEX IF NOT EXISTS idx_trades_strategy
			ON trades(strategy_name, timestamp);

			CREATE INDEX IF NOT EXISTS idx_trades_backtest
			ON trades(backtest_id);

			CREATE TABLE IF NOT EXISTS backtest_results (
				id TEXT PRIMARY KEY,
				recipe_name TEXT NOT NULL,
				start_date INTEGER NOT NULL,
				end_date INTEGER NOT NULL,
				initial_capital REAL NOT NULL,
				final_capital REAL NOT NULL,
				total_return REAL NOT NULL,
				sharpe_ratio REAL,
				max_drawdown REAL,
				win_rate REAL,
				total_trades INTEGER,
				created_at INTEGER NOT NULL,
				config TEXT
			);

			CREATE INDEX IF NOT EXISTS idx_backtest_recipe
			ON backtest_results(recipe_name, created_at);
		)";

		return executeSQL(sql);
	}
};

DataStorage::DataStorage()
	: pImpl(std::make_unique<Impl>()) {
}

DataStorage::~DataStorage() {
	close();
}

bool DataStorage::init(const std::string& dbPath) {
	if (pImpl->initialized) {
		LOG_WARNING("DataStorage already initialized");
		return true;
	}

	LOG_INFO("Attempting to open database: " + dbPath);

	int rc = sqlite3_open(dbPath.c_str(), &pImpl->db);
	if (rc != SQLITE_OK) {
		std::string errMsg = pImpl->db ? sqlite3_errmsg(pImpl->db) : "Unknown error";
		LOG_ERROR("Cannot open database: " + errMsg + " (code: " + std::to_string(rc) + ")");
		LOG_ERROR("Database path: " + dbPath);
		return false;
	}

	// Enable foreign keys
	pImpl->executeSQL("PRAGMA foreign_keys = ON;");

	// Create tables
	if (!pImpl->createTables()) {
		LOG_ERROR("Failed to create database tables");
		return false;
	}

	pImpl->initialized = true;
	LOG_INFO("DataStorage initialized: " + dbPath);
	return true;
}

void DataStorage::close() {
	if (pImpl->initialized && pImpl->db) {
		sqlite3_close(pImpl->db);
		pImpl->db = nullptr;
		pImpl->initialized = false;
		LOG_INFO("DataStorage closed");
	}
}

bool DataStorage::insertCandle(const Candle& candle) {
	if (!pImpl->initialized) {
		LOG_ERROR("DataStorage not initialized");
		return false;
	}

	const char* sql = R"(
		INSERT OR REPLACE INTO candles
		(exchange, symbol, timeframe, timestamp, open, high, low, close, volume)
		VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
	)";

	// Fixed: Use RAII wrapper to ensure finalize on all paths
	StmtHandle stmt;
	int rc = sqlite3_prepare_v2(pImpl->db, sql, -1, stmt.ptr(), nullptr);
	if (rc != SQLITE_OK) {
		LOG_ERROR("Failed to prepare statement: " + std::string(sqlite3_errmsg(pImpl->db)));
		return false;  // ✅ Automatic cleanup
	}

	sqlite3_bind_text(stmt, 1, candle.exchange.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, candle.symbol.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 3, candle.timeframe.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int64(stmt, 4, candle.timestamp);
	sqlite3_bind_double(stmt, 5, candle.open);
	sqlite3_bind_double(stmt, 6, candle.high);
	sqlite3_bind_double(stmt, 7, candle.low);
	sqlite3_bind_double(stmt, 8, candle.close);
	sqlite3_bind_double(stmt, 9, candle.volume);

	rc = sqlite3_step(stmt);
	// ✅ Automatic cleanup via destructor

	if (rc != SQLITE_DONE) {
		LOG_ERROR("Failed to insert candle: " + std::string(sqlite3_errmsg(pImpl->db)));
		return false;  // ✅ Automatic cleanup
	}

	return true;  // ✅ Automatic cleanup
}

bool DataStorage::insertCandles(const std::vector<Candle>& candles) {
	if (!pImpl->initialized) {
		LOG_ERROR("DataStorage not initialized");
		return false;
	}

	// Use transaction for bulk insert
	pImpl->executeSQL("BEGIN TRANSACTION;");

	for (const auto& candle : candles) {
		if (!insertCandle(candle)) {
			pImpl->executeSQL("ROLLBACK;");
			return false;
		}
	}

	pImpl->executeSQL("COMMIT;");
	LOG_INFO("Inserted " + std::to_string(candles.size()) + " candles");
	return true;
}

std::vector<Candle> DataStorage::getCandles(const std::string& exchange,
                                             const std::string& symbol,
                                             const std::string& timeframe,
                                             time_t startTime,
                                             time_t endTime) {
	std::vector<Candle> result;

	if (!pImpl->initialized) {
		LOG_ERROR("DataStorage not initialized");
		return result;
	}

	const char* sql = R"(
		SELECT exchange, symbol, timeframe, timestamp, open, high, low, close, volume
		FROM candles
		WHERE exchange = ? AND symbol = ? AND timeframe = ?
		AND timestamp >= ? AND timestamp <= ?
		ORDER BY timestamp ASC
	)";

	// Fixed: Use RAII wrapper to ensure finalize on all paths
	StmtHandle stmt;
	int rc = sqlite3_prepare_v2(pImpl->db, sql, -1, stmt.ptr(), nullptr);
	if (rc != SQLITE_OK) {
		LOG_ERROR("Failed to prepare statement: " + std::string(sqlite3_errmsg(pImpl->db)));
		return result;  // ✅ Automatic cleanup
	}

	sqlite3_bind_text(stmt, 1, exchange.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, symbol.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 3, timeframe.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int64(stmt, 4, startTime);
	sqlite3_bind_int64(stmt, 5, endTime);

	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
		Candle candle;
		// Fixed: NULL check for sqlite3_column_text() results
		candle.exchange = pImpl->safeColumnText(stmt, 0);
		candle.symbol = pImpl->safeColumnText(stmt, 1);
		candle.timeframe = pImpl->safeColumnText(stmt, 2);
		candle.timestamp = sqlite3_column_int64(stmt, 3);
		candle.open = sqlite3_column_double(stmt, 4);
		candle.high = sqlite3_column_double(stmt, 5);
		candle.low = sqlite3_column_double(stmt, 6);
		candle.close = sqlite3_column_double(stmt, 7);
		candle.volume = sqlite3_column_double(stmt, 8);
		result.push_back(candle);
	}

	// ✅ Automatic cleanup via destructor
	return result;
}

int DataStorage::getCandleCount(const std::string& exchange,
                                 const std::string& symbol,
                                 const std::string& timeframe) {
	if (!pImpl->initialized) {
		return 0;
	}

	const char* sql = R"(
		SELECT COUNT(*) FROM candles
		WHERE exchange = ? AND symbol = ? AND timeframe = ?
	)";

	// Fixed: Use RAII wrapper to ensure finalize on all paths
	StmtHandle stmt;
	int rc = sqlite3_prepare_v2(pImpl->db, sql, -1, stmt.ptr(), nullptr);
	if (rc != SQLITE_OK) {
		return 0;  // ✅ Automatic cleanup
	}

	sqlite3_bind_text(stmt, 1, exchange.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, symbol.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 3, timeframe.c_str(), -1, SQLITE_TRANSIENT);

	int count = 0;
	if (sqlite3_step(stmt) == SQLITE_ROW) {
		count = sqlite3_column_int(stmt, 0);
	}

	// ✅ Automatic cleanup via destructor
	return count;
}

bool DataStorage::insertTrade(const Trade& trade) {
	if (!pImpl->initialized) {
		LOG_ERROR("DataStorage not initialized");
		return false;
	}

	const char* sql = R"(
		INSERT INTO trades
		(strategy_name, backtest_id, timestamp, symbol, side, price, quantity, commission, pnl, portfolio_value)
		VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
	)";

	// Fixed: Use RAII wrapper to ensure finalize on all paths
	StmtHandle stmt;
	int rc = sqlite3_prepare_v2(pImpl->db, sql, -1, stmt.ptr(), nullptr);
	if (rc != SQLITE_OK) {
		LOG_ERROR("Failed to prepare statement");
		return false;  // ✅ Automatic cleanup
	}

	sqlite3_bind_text(stmt, 1, trade.strategyName.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, trade.backtestId.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int64(stmt, 3, trade.timestamp);
	sqlite3_bind_text(stmt, 4, trade.symbol.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 5, trade.side.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_double(stmt, 6, trade.price);
	sqlite3_bind_double(stmt, 7, trade.quantity);
	sqlite3_bind_double(stmt, 8, trade.commission);
	sqlite3_bind_double(stmt, 9, trade.pnl);
	sqlite3_bind_double(stmt, 10, trade.portfolioValue);

	rc = sqlite3_step(stmt);
	// ✅ Automatic cleanup via destructor

	return (rc == SQLITE_DONE);
}

std::vector<Trade> DataStorage::getTrades(const std::string& strategyName,
                                           time_t startTime,
                                           time_t endTime) {
	std::vector<Trade> result;
	// Implementation similar to getCandles
	return result;
}

std::vector<Trade> DataStorage::getTradesByBacktest(const std::string& backtestId) {
	std::vector<Trade> result;
	// Implementation similar to getCandles
	return result;
}

bool DataStorage::insertBacktestResult(const BacktestResult& result) {
	if (!pImpl->initialized) {
		LOG_ERROR("DataStorage not initialized");
		return false;
	}

	const char* sql = R"(
		INSERT OR REPLACE INTO backtest_results
		(id, recipe_name, start_date, end_date, initial_capital, final_capital,
		 total_return, sharpe_ratio, max_drawdown, win_rate, total_trades, created_at, config)
		VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
	)";

	// Fixed: Use RAII wrapper to ensure finalize on all paths
	StmtHandle stmt;
	int rc = sqlite3_prepare_v2(pImpl->db, sql, -1, stmt.ptr(), nullptr);
	if (rc != SQLITE_OK) {
		LOG_ERROR("Failed to prepare statement");
		return false;  // ✅ Automatic cleanup
	}

	sqlite3_bind_text(stmt, 1, result.id.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, result.recipeName.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int64(stmt, 3, result.startDate);
	sqlite3_bind_int64(stmt, 4, result.endDate);
	sqlite3_bind_double(stmt, 5, result.initialCapital);
	sqlite3_bind_double(stmt, 6, result.finalCapital);
	sqlite3_bind_double(stmt, 7, result.totalReturn);
	sqlite3_bind_double(stmt, 8, result.sharpeRatio);
	sqlite3_bind_double(stmt, 9, result.maxDrawdown);
	sqlite3_bind_double(stmt, 10, result.winRate);
	sqlite3_bind_int(stmt, 11, result.totalTrades);
	sqlite3_bind_int64(stmt, 12, result.createdAt);
	sqlite3_bind_text(stmt, 13, result.config.c_str(), -1, SQLITE_TRANSIENT);

	rc = sqlite3_step(stmt);
	// ✅ Automatic cleanup via destructor

	return (rc == SQLITE_DONE);
}

BacktestResult DataStorage::getBacktestResult(const std::string& id) {
	BacktestResult result;
	// Implementation similar to getCandles
	return result;
}

std::vector<BacktestResult> DataStorage::getAllBacktestResults() {
	std::vector<BacktestResult> results;

	if (!pImpl->initialized) {
		LOG_ERROR("DataStorage not initialized");
		return results;
	}

	const char* sql = R"(
		SELECT id, recipe_name, start_date, end_date, initial_capital, final_capital,
		       total_return, sharpe_ratio, max_drawdown, win_rate, total_trades, created_at, config
		FROM backtest_results
		ORDER BY created_at DESC
	)";

	// Fixed: Use RAII wrapper to ensure finalize on all paths
	StmtHandle stmt;
	int rc = sqlite3_prepare_v2(pImpl->db, sql, -1, stmt.ptr(), nullptr);
	if (rc != SQLITE_OK) {
		LOG_ERROR("Failed to prepare statement: " + std::string(sqlite3_errmsg(pImpl->db)));
		return results;  // ✅ Automatic cleanup
	}

	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
		BacktestResult result;
		// Fixed: NULL check for sqlite3_column_text() results
		result.id = pImpl->safeColumnText(stmt, 0);
		result.recipeName = pImpl->safeColumnText(stmt, 1);
		result.startDate = sqlite3_column_int64(stmt, 2);
		result.endDate = sqlite3_column_int64(stmt, 3);
		result.initialCapital = sqlite3_column_double(stmt, 4);
		result.finalCapital = sqlite3_column_double(stmt, 5);
		result.totalReturn = sqlite3_column_double(stmt, 6);
		result.sharpeRatio = sqlite3_column_double(stmt, 7);
		result.maxDrawdown = sqlite3_column_double(stmt, 8);
		result.winRate = sqlite3_column_double(stmt, 9);
		result.totalTrades = sqlite3_column_int(stmt, 10);
		result.createdAt = sqlite3_column_int64(stmt, 11);

		const unsigned char* configText = sqlite3_column_text(stmt, 12);
		if (configText) {
			result.config = reinterpret_cast<const char*>(configText);
		}

		results.push_back(result);
	}

	// ✅ Automatic cleanup via destructor

	LOG_INFO("Retrieved " + std::to_string(results.size()) + " backtest results");
	return results;
}

bool DataStorage::clearCandles(const std::string& exchange,
                                const std::string& symbol,
                                const std::string& timeframe) {
	if (!pImpl->initialized) {
		return false;
	}

	// Fixed: SQL injection vulnerability - use prepared statement instead of string concatenation
	const char* sql = "DELETE FROM candles WHERE exchange = ? AND symbol = ? AND timeframe = ?";

	// Fixed: Use RAII wrapper to ensure finalize on all paths
	StmtHandle stmt;
	int rc = sqlite3_prepare_v2(pImpl->db, sql, -1, stmt.ptr(), nullptr);
	if (rc != SQLITE_OK) {
		LOG_ERROR("Failed to prepare delete statement: " + std::string(sqlite3_errmsg(pImpl->db)));
		return false;  // ✅ Automatic cleanup
	}

	sqlite3_bind_text(stmt, 1, exchange.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, symbol.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 3, timeframe.c_str(), -1, SQLITE_TRANSIENT);

	rc = sqlite3_step(stmt);
	// ✅ Automatic cleanup via destructor

	if (rc != SQLITE_DONE) {
		LOG_ERROR("Failed to delete candles: " + std::string(sqlite3_errmsg(pImpl->db)));
		return false;  // ✅ Automatic cleanup
	}

	return true;  // ✅ Automatic cleanup
}

bool DataStorage::vacuum() {
	if (!pImpl->initialized) {
		return false;
	}

	LOG_INFO("Vacuuming database...");
	return pImpl->executeSQL("VACUUM;");
}

} // namespace Emiglio
