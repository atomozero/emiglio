#include "BinanceAPI.h"
#include "../utils/Logger.h"
#include "../utils/JsonParser.h"


#include <sstream>
#include <iomanip>
#include <chrono>
#include <deque>
#include <mutex>   // For thread safety
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <cstdio>  // For popen/pclose
#include <array>   // For std::array
#ifdef __HAIKU__
#include <OS.h>  // For snooze()
#endif

namespace Emiglio {


// Private implementation using PIMPL pattern
class BinanceAPI::Impl {
public:
	std::string apiKey;
	std::string apiSecret;
	std::string baseUrl;
	bool initialized;

	// Cache for ticker data
	struct CachedTicker {
		Ticker ticker;
		time_t timestamp;
	};
	std::map<std::string, CachedTicker> tickerCache;
	int cacheDurationSeconds;

	// Rate limiting
	// Fixed: Added mutex for thread-safe access to shared data
	struct RateLimiter {
		std::deque<time_t> requestTimes;
		int maxRequests;      // 1200 per minute
		int windowSeconds;    // 60 seconds
		mutable std::mutex mtx;  // Thread safety for shared data access

		RateLimiter() : maxRequests(1200), windowSeconds(60) {}

		bool canMakeRequest() {
			std::lock_guard<std::mutex> lock(mtx);  // Thread-safe access
			time_t now = std::time(nullptr);

			// Remove requests older than window
			while (!requestTimes.empty() &&
			       (now - requestTimes.front()) >= windowSeconds) {
				requestTimes.pop_front();
			}

			return requestTimes.size() < static_cast<size_t>(maxRequests);
		}

		void recordRequest() {
			std::lock_guard<std::mutex> lock(mtx);  // Thread-safe access
			requestTimes.push_back(std::time(nullptr));
		}

		int remainingRequests() const {
			std::lock_guard<std::mutex> lock(mtx);  // Thread-safe access
			return maxRequests - static_cast<int>(requestTimes.size());
		}
	};
	RateLimiter rateLimiter;

	Impl() : baseUrl("https://api.binance.com"),
	         initialized(false),
	         cacheDurationSeconds(1) {  // Cache for 1 second by default
	}

	// HTTP request helper (public endpoints) using curl
	std::string httpGet(const std::string& endpoint, const std::map<std::string, std::string>& params = {}) {
		// Rate limiting check
		if (!rateLimiter.canMakeRequest()) {
			LOG_WARNING("Rate limit reached! Waiting 1 second...");
#ifdef __HAIKU__
			snooze(1000000);  // 1 second in microseconds
#else
			std::this_thread::sleep_for(std::chrono::seconds(1));
#endif
		}

		std::string url = baseUrl + endpoint;

		// Add query parameters
		if (!params.empty()) {
			url += "?";
			bool first = true;
			for (const auto& [key, value] : params) {
				if (!first) url += "&";
				url += key + "=" + value;
				first = false;
			}
		}

		LOG_INFO("HTTP GET: " + url);

		// Record request for rate limiting
		rateLimiter.recordRequest();

		// Use curl via popen
		std::string curlCmd = "curl -s \"" + url + "\"";
		FILE* pipe = popen(curlCmd.c_str(), "r");
		if (!pipe) {
			LOG_ERROR("Failed to execute curl");
			return "";
		}

		// Read response
		std::string response;
		std::array<char, 4096> buffer;
		while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
			response += buffer.data();
		}

		int status = pclose(pipe);
		if (status != 0) {
			LOG_WARNING("curl returned non-zero status: " + std::to_string(status));
		}

		LOG_INFO("Response received: " + std::to_string(response.length()) + " bytes");
		return response;
	}

	// HTTP request helper (signed endpoints - requires HMAC) using curl
	std::string httpGetSigned(const std::string& endpoint, std::map<std::string, std::string> params = {}) {
		// Rate limiting check
		if (!rateLimiter.canMakeRequest()) {
			LOG_WARNING("Rate limit reached! Waiting 1 second...");
#ifdef __HAIKU__
			snooze(1000000);  // 1 second in microseconds
#else
			std::this_thread::sleep_for(std::chrono::seconds(1));
#endif
		}

		// Add timestamp
		auto now = std::chrono::system_clock::now();
		auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
		params["timestamp"] = std::to_string(timestamp);

		// Build query string
		std::string queryString;
		bool first = true;
		for (const auto& [key, value] : params) {
			if (!first) queryString += "&";
			queryString += key + "=" + value;
			first = false;
		}

		// Generate HMAC SHA256 signature
		std::string signature = generateSignature(queryString);
		params["signature"] = signature;

		// Make request with API key header
		std::string url = baseUrl + endpoint + "?" + queryString + "&signature=" + signature;

		LOG_INFO("HTTP GET (signed): " + url);

		// Record request for rate limiting
		rateLimiter.recordRequest();

		// Use curl via popen with API key header
		std::string curlCmd = "curl -s -H \"X-MBX-APIKEY: " + apiKey + "\" \"" + url + "\"";
		FILE* pipe = popen(curlCmd.c_str(), "r");
		if (!pipe) {
			LOG_ERROR("Failed to execute curl for signed request");
			return "";
		}

		// Read response
		std::string response;
		std::array<char, 4096> buffer;
		while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
			response += buffer.data();
		}

		int status = pclose(pipe);
		if (status != 0) {
			LOG_WARNING("curl returned non-zero status: " + std::to_string(status));
		}

		LOG_INFO("Signed response received: " + std::to_string(response.length()) + " bytes");
		return response;
	}

	// Generate HMAC SHA256 signature
	std::string generateSignature(const std::string& data) {
		unsigned char hash[SHA256_DIGEST_LENGTH];

		HMAC(EVP_sha256(),
		     apiSecret.c_str(), apiSecret.length(),
		     (unsigned char*)data.c_str(), data.length(),
		     hash, nullptr);

		// Convert to hex string
		std::stringstream ss;
		for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
			ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
		}

		return ss.str();
	}

	// Convert Binance timeframe to milliseconds
	// Fixed: Added LL suffix to all literals to prevent integer overflow on 32-bit systems
	int64_t timeframeToMs(const std::string& timeframe) {
		if (timeframe == "1m") return 60LL * 1000LL;
		if (timeframe == "3m") return 3LL * 60LL * 1000LL;
		if (timeframe == "5m") return 5LL * 60LL * 1000LL;
		if (timeframe == "15m") return 15LL * 60LL * 1000LL;
		if (timeframe == "30m") return 30LL * 60LL * 1000LL;
		if (timeframe == "1h") return 60LL * 60LL * 1000LL;
		if (timeframe == "2h") return 2LL * 60LL * 60LL * 1000LL;
		if (timeframe == "4h") return 4LL * 60LL * 60LL * 1000LL;
		if (timeframe == "6h") return 6LL * 60LL * 60LL * 1000LL;
		if (timeframe == "8h") return 8LL * 60LL * 60LL * 1000LL;
		if (timeframe == "12h") return 12LL * 60LL * 60LL * 1000LL;
		if (timeframe == "1d") return 24LL * 60LL * 60LL * 1000LL;
		if (timeframe == "3d") return 3LL * 24LL * 60LL * 60LL * 1000LL;
		if (timeframe == "1w") return 7LL * 24LL * 60LL * 60LL * 1000LL;
		if (timeframe == "1M") return 30LL * 24LL * 60LL * 60LL * 1000LL;
		return 60LL * 1000LL; // Default 1m
	}
};

BinanceAPI::BinanceAPI()
	: pImpl(std::make_unique<Impl>()) {
}

BinanceAPI::~BinanceAPI() {
	LOG_INFO("BinanceAPI destructor called");
}

bool BinanceAPI::init(const std::string& apiKey, const std::string& apiSecret) {
	pImpl->apiKey = apiKey;
	pImpl->apiSecret = apiSecret;
	pImpl->initialized = true;

	LOG_INFO("BinanceAPI initialized");
	return true;
}

std::string BinanceAPI::getName() const {
	return "Binance";
}

bool BinanceAPI::testConnection() {
	std::string response = pImpl->httpGet("/api/v3/ping");
	return !response.empty();
}

bool BinanceAPI::ping() {
	return testConnection();
}

time_t BinanceAPI::getServerTime() {
	std::string response = pImpl->httpGet("/api/v3/time");

	// Parse JSON response: {"serverTime": 1234567890123}
	JsonParser parser;
	if (parser.parse(response)) {
		int64_t serverTime = parser.getInt64("serverTime", 0);
		return serverTime / 1000; // Convert ms to seconds
	}

	return 0;
}

Ticker BinanceAPI::getTicker(const std::string& symbol) {
	// Check cache first
	time_t now = std::time(nullptr);
	auto it = pImpl->tickerCache.find(symbol);
	if (it != pImpl->tickerCache.end()) {
		time_t age = now - it->second.timestamp;
		if (age < pImpl->cacheDurationSeconds) {
			LOG_DEBUG("Cache HIT for " + symbol + " (age: " + std::to_string(age) + "s)");
			return it->second.ticker;
		} else {
			LOG_DEBUG("Cache EXPIRED for " + symbol + " (age: " + std::to_string(age) + "s)");
		}
	}

	// Cache miss or expired - fetch from API
	Ticker ticker;
	ticker.symbol = symbol;
	ticker.lastPrice = 0.0;
	ticker.priceChange = 0.0;
	ticker.priceChangePercent = 0.0;
	ticker.highPrice = 0.0;
	ticker.lowPrice = 0.0;
	ticker.volume = 0.0;
	ticker.quoteVolume = 0.0;
	ticker.timestamp = 0;

	std::map<std::string, std::string> params;
	params["symbol"] = symbol;

	std::string response = pImpl->httpGet("/api/v3/ticker/24hr", params);

	// Parse JSON response
	JsonParser parser;
	if (parser.parse(response)) {
		ticker.lastPrice = parser.getDouble("lastPrice", 0.0);
		ticker.priceChange = parser.getDouble("priceChange", 0.0);
		ticker.priceChangePercent = parser.getDouble("priceChangePercent", 0.0);
		ticker.highPrice = parser.getDouble("highPrice", 0.0);
		ticker.lowPrice = parser.getDouble("lowPrice", 0.0);
		ticker.volume = parser.getDouble("volume", 0.0);
		ticker.quoteVolume = parser.getDouble("quoteVolume", 0.0);
		ticker.timestamp = now;

		// Store in cache
		pImpl->tickerCache[symbol] = {ticker, now};
		LOG_DEBUG("Cached ticker for " + symbol);
	}

	return ticker;
}

std::vector<Ticker> BinanceAPI::getAllTickers() {
	std::vector<Ticker> tickers;

	// Use ticker/24hr without symbol parameter to get ALL symbols
	std::string response = pImpl->httpGet("/api/v3/ticker/24hr");

	JsonParser parser;
	if (parser.parse(response)) {
		size_t count = parser.getArraySize("");

		LOG_INFO("Fetched " + std::to_string(count) + " tickers in 1 request (vs " +
		         std::to_string(count) + " individual requests)");

		time_t now = std::time(nullptr);

		for (size_t i = 0; i < count; i++) {
			Ticker ticker;
			ticker.symbol = parser.getArrayObjectString("", i, "symbol", "");
			ticker.lastPrice = parser.getArrayObjectDouble("", i, "lastPrice", 0.0);
			ticker.priceChange = parser.getArrayObjectDouble("", i, "priceChange", 0.0);
			ticker.priceChangePercent = parser.getArrayObjectDouble("", i, "priceChangePercent", 0.0);
			ticker.highPrice = parser.getArrayObjectDouble("", i, "highPrice", 0.0);
			ticker.lowPrice = parser.getArrayObjectDouble("", i, "lowPrice", 0.0);
			ticker.volume = parser.getArrayObjectDouble("", i, "volume", 0.0);
			ticker.quoteVolume = parser.getArrayObjectDouble("", i, "quoteVolume", 0.0);
			ticker.timestamp = now;

			if (!ticker.symbol.empty()) {
				tickers.push_back(ticker);

				// Also populate cache
				pImpl->tickerCache[ticker.symbol] = {ticker, now};
			}
		}

		LOG_INFO("Cached " + std::to_string(tickers.size()) + " tickers");
	}

	return tickers;
}

std::vector<Candle> BinanceAPI::getCandles(const std::string& symbol,
                                             const std::string& timeframe,
                                             time_t startTime,
                                             time_t endTime,
                                             int limit) {
	std::vector<Candle> candles;

	std::map<std::string, std::string> params;
	params["symbol"] = symbol;
	params["interval"] = timeframe;
	params["startTime"] = std::to_string(startTime * 1000); // Convert to ms
	params["endTime"] = std::to_string(endTime * 1000);
	params["limit"] = std::to_string(limit);

	std::string response = pImpl->httpGet("/api/v3/klines", params);

	// Parse JSON response: array of arrays
	// Each candle: [timestamp, open, high, low, close, volume, close_time, quote_volume, trades, taker_buy_base, taker_buy_quote, ignore]
	JsonParser parser;
	if (parser.parse(response)) {
		// Response is array at root level, so keyPath is empty
		size_t count = parser.getArraySize("");

		for (size_t i = 0; i < count; i++) {
			Candle candle;
			candle.symbol = symbol;
			candle.timestamp = parser.getNestedArrayInt64("", i, 0, 0) / 1000; // Convert ms to seconds
			candle.open = parser.getNestedArrayDouble("", i, 1, 0.0);
			candle.high = parser.getNestedArrayDouble("", i, 2, 0.0);
			candle.low = parser.getNestedArrayDouble("", i, 3, 0.0);
			candle.close = parser.getNestedArrayDouble("", i, 4, 0.0);
			candle.volume = parser.getNestedArrayDouble("", i, 5, 0.0);

			candles.push_back(candle);
		}

		LOG_INFO("Parsed " + std::to_string(candles.size()) + " candles");
	} else {
		LOG_ERROR("Failed to parse candles response");
	}

	return candles;
}

OrderBook BinanceAPI::getOrderBook(const std::string& symbol, int limit) {
	OrderBook orderBook;
	orderBook.symbol = symbol;
	orderBook.timestamp = std::time(nullptr);

	std::map<std::string, std::string> params;
	params["symbol"] = symbol;
	params["limit"] = std::to_string(limit);

	std::string response = pImpl->httpGet("/api/v3/depth", params);

	// Parse JSON response: {"bids": [[price, quantity], ...], "asks": [[price, quantity], ...]}
	JsonParser parser;
	if (parser.parse(response)) {
		// Parse bids
		size_t bidCount = parser.getArraySize("bids");
		for (size_t i = 0; i < bidCount; i++) {
			OrderBookLevel bid;
			bid.price = parser.getNestedArrayDouble("bids", i, 0, 0.0);
			bid.quantity = parser.getNestedArrayDouble("bids", i, 1, 0.0);
			orderBook.bids.push_back(bid);
		}

		// Parse asks
		size_t askCount = parser.getArraySize("asks");
		for (size_t i = 0; i < askCount; i++) {
			OrderBookLevel ask;
			ask.price = parser.getNestedArrayDouble("asks", i, 0, 0.0);
			ask.quantity = parser.getNestedArrayDouble("asks", i, 1, 0.0);
			orderBook.asks.push_back(ask);
		}

		LOG_INFO("Parsed order book: " + std::to_string(orderBook.bids.size()) +
		         " bids, " + std::to_string(orderBook.asks.size()) + " asks");
	} else {
		LOG_ERROR("Failed to parse order book response");
	}

	return orderBook;
}

std::vector<Trade> BinanceAPI::getRecentTrades(const std::string& symbol, int limit) {
	std::vector<Trade> trades;

	std::map<std::string, std::string> params;
	params["symbol"] = symbol;
	params["limit"] = std::to_string(limit);

	std::string response = pImpl->httpGet("/api/v3/trades", params);

	// Parse JSON response: array of {id, price, qty, time, isBuyerMaker, ...}
	JsonParser parser;
	if (parser.parse(response)) {
		size_t count = parser.getArraySize("");

		for (size_t i = 0; i < count; i++) {
			Trade trade;
			trade.id = parser.getArrayObjectInt64("", i, "id", 0);
			trade.symbol = symbol;
			trade.price = parser.getArrayObjectDouble("", i, "price", 0.0);
			trade.quantity = parser.getArrayObjectDouble("", i, "qty", 0.0);
			trade.timestamp = parser.getArrayObjectInt64("", i, "time", 0) / 1000;  // ms to seconds
			trade.isBuyerMaker = parser.getArrayObjectString("", i, "isBuyerMaker", "false") == "true";

			trades.push_back(trade);
		}

		LOG_INFO("Parsed " + std::to_string(trades.size()) + " trades");
	} else {
		LOG_ERROR("Failed to parse trades response");
	}

	return trades;
}

std::vector<Balance> BinanceAPI::getBalances() {
	std::vector<Balance> balances;

	if (!pImpl->initialized) {
		LOG_ERROR("BinanceAPI not initialized with API keys");
		return balances;
	}

	std::string response = pImpl->httpGetSigned("/api/v3/account");

	// Parse JSON response
	JsonParser parser;
	if (parser.parse(response)) {
		// Binance returns: {"balances": [{"asset": "BTC", "free": "0.00000000", "locked": "0.00000000"}, ...]}
		if (parser.has("balances") && parser.isArray("balances")) {
			size_t count = parser.getArraySize("balances");
			LOG_INFO("Parsing " + std::to_string(count) + " balances from account");

			for (size_t i = 0; i < count; i++) {
				Balance balance;
				balance.asset = parser.getArrayObjectString("balances", i, "asset", "");
				balance.free = parser.getArrayObjectDouble("balances", i, "free", 0.0);
				balance.locked = parser.getArrayObjectDouble("balances", i, "locked", 0.0);
				balance.total = balance.free + balance.locked;

				// Only add balances with non-zero amounts to reduce clutter
				if (balance.total > 0.0) {
					balances.push_back(balance);
					LOG_DEBUG("Balance: " + balance.asset + " = " +
					         std::to_string(balance.total) + " (free: " +
					         std::to_string(balance.free) + ", locked: " +
					         std::to_string(balance.locked) + ")");
				}
			}

			LOG_INFO("Loaded " + std::to_string(balances.size()) + " non-zero balances");
		} else {
			LOG_ERROR("Failed to find 'balances' array in response");
		}
	} else {
		LOG_ERROR("Failed to parse account response: " + parser.getError());
	}

	return balances;
}

Balance BinanceAPI::getBalance(const std::string& asset) {
	Balance balance;
	balance.asset = asset;
	balance.free = 0.0;
	balance.locked = 0.0;
	balance.total = 0.0;

	// Get all balances and find the requested asset
	auto balances = getBalances();
	for (const auto& b : balances) {
		if (b.asset == asset) {
			return b;
		}
	}

	return balance;
}

Order BinanceAPI::createOrder(const std::string& symbol,
                               OrderSide side,
                               OrderType type,
                               double quantity,
                               double price) {
	(void)symbol; (void)side; (void)type; (void)quantity; (void)price;  // Unused parameters
	Order order;

	if (!pImpl->initialized) {
		LOG_ERROR("BinanceAPI not initialized with API keys");
		return order;
	}

	// TODO: Implement order creation
	LOG_WARNING("Order creation not yet implemented");

	return order;
}

bool BinanceAPI::cancelOrder(const std::string& symbol, const std::string& orderId) {
	(void)symbol; (void)orderId;  // Unused parameters
	if (!pImpl->initialized) {
		LOG_ERROR("BinanceAPI not initialized with API keys");
		return false;
	}

	// TODO: Implement order cancellation
	LOG_WARNING("Order cancellation not yet implemented");
	return false;
}

Order BinanceAPI::getOrder(const std::string& symbol, const std::string& orderId) {
	(void)symbol; (void)orderId;  // Unused parameters
	Order order;

	if (!pImpl->initialized) {
		LOG_ERROR("BinanceAPI not initialized with API keys");
		return order;
	}

	// TODO: Implement get order
	LOG_WARNING("Get order not yet implemented");
	return order;
}

std::vector<Order> BinanceAPI::getOpenOrders(const std::string& symbol) {
	(void)symbol;  // Unused parameter
	std::vector<Order> orders;

	if (!pImpl->initialized) {
		LOG_ERROR("BinanceAPI not initialized with API keys");
		return orders;
	}

	// TODO: Implement get open orders
	LOG_WARNING("Get open orders not yet implemented");
	return orders;
}

std::vector<Order> BinanceAPI::getAllOrders(const std::string& symbol, int limit) {
	(void)symbol; (void)limit;  // Unused parameters
	std::vector<Order> orders;

	if (!pImpl->initialized) {
		LOG_ERROR("BinanceAPI not initialized with API keys");
		return orders;
	}

	// TODO: Implement get all orders
	LOG_WARNING("Get all orders not yet implemented");
	return orders;
}

std::string BinanceAPI::getExchangeInfo() {
	std::string response = pImpl->httpGet("/api/v3/exchangeInfo");
	return response;
}

std::vector<std::string> BinanceAPI::getAllSymbols() {
	std::vector<std::string> symbols;

	std::string response = pImpl->httpGet("/api/v3/exchangeInfo");

	// Parse JSON response to extract symbols
	JsonParser parser;
	if (parser.parse(response)) {
		size_t count = parser.getArraySize("symbols");

		for (size_t i = 0; i < count; i++) {
			std::string symbol = parser.getArrayObjectString("symbols", i, "symbol", "");
			std::string status = parser.getArrayObjectString("symbols", i, "status", "");

			// Only include TRADING symbols
			if (!symbol.empty() && status == "TRADING") {
				symbols.push_back(symbol);
			}
		}

		LOG_INFO("Fetched " + std::to_string(symbols.size()) + " trading symbols from Binance");
	} else {
		LOG_ERROR("Failed to parse exchangeInfo response");
	}

	return symbols;
}

} // namespace Emiglio
