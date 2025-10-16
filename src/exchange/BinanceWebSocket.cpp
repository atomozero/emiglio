#include "BinanceWebSocket.h"
#include "../utils/Logger.h"
#include "../utils/JsonParser.h"

#include <thread>
#include <mutex>
#include <queue>
#include <map>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <ctime>

#ifdef __HAIKU__
#include <OS.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#else
#include <chrono>
#endif

namespace Emiglio {

// Private implementation using PIMPL pattern
class BinanceWebSocket::Impl {
public:
	// WebSocket connection state
	bool connected;
	std::thread workerThread;
	bool shouldStop;

	// Callbacks
	std::map<std::string, TickerCallback> tickerCallbacks;
	std::map<std::string, TradeCallback> tradeCallbacks;
	std::map<std::string, KlineCallback> klineCallbacks;
	ErrorCallback errorCallback;

	// Message queue for thread-safe communication
	std::queue<std::string> messageQueue;
	std::mutex queueMutex;

	// Subscribed streams
	std::vector<std::string> subscribedStreams;

	Impl() : connected(false), shouldStop(false) {}

	~Impl() {
		disconnect();
	}

	bool connect() {
		if (connected) {
			LOG_WARNING("Already connected to WebSocket");
			return true;
		}

		LOG_INFO("Connecting to Binance WebSocket...");

		// Start worker thread
		shouldStop = false;
		workerThread = std::thread(&Impl::workerLoop, this);

		connected = true;
		LOG_INFO("WebSocket connection established");
		return true;
	}

	void disconnect() {
		if (!connected) return;

		LOG_INFO("Disconnecting from WebSocket...");
		shouldStop = true;

		if (workerThread.joinable()) {
			workerThread.join();
		}

		connected = false;
		subscribedStreams.clear();
		LOG_INFO("WebSocket disconnected");
	}

	// Worker thread loop - connects to real Binance WebSocket
	void workerLoop() {
		LOG_INFO("WebSocket worker thread started");

		while (!shouldStop) {
			// Build stream URLs from subscribed streams
			if (subscribedStreams.empty()) {
#ifdef __HAIKU__
				snooze(100000); // 100ms - wait for subscriptions
#else
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
#endif
				continue;
			}

			// Binance allows multiple streams in one connection: /stream?streams=btcusdt@trade/btcusdt@ticker
			std::string streams;
			for (size_t i = 0; i < subscribedStreams.size(); i++) {
				if (i > 0) streams += "/";
				streams += subscribedStreams[i];
			}

			std::string wsUrl = "wss://stream.binance.com:9443/stream?streams=" + streams;
			LOG_INFO("Connecting to: " + wsUrl);

			// Use curl to connect to WebSocket
			std::string curlCmd = "curl -s -N \"" + wsUrl + "\" 2>&1";
			FILE* pipe = popen(curlCmd.c_str(), "r");

			if (!pipe) {
				LOG_ERROR("Failed to open WebSocket connection with curl");
#ifdef __HAIKU__
				snooze(5000000); // 5 seconds retry
#else
				std::this_thread::sleep_for(std::chrono::seconds(5));
#endif
				continue;
			}

			// Read messages from WebSocket stream
			char buffer[8192];
			std::string partial;

			while (!shouldStop && fgets(buffer, sizeof(buffer), pipe) != nullptr) {
				partial += buffer;

				// Check if we have a complete JSON message
				// Binance sends messages in format: {"stream":"btcusdt@trade","data":{...}}
				size_t pos = 0;
				while ((pos = partial.find("}{", pos)) != std::string::npos) {
					// Found boundary between two JSON objects
					std::string message = partial.substr(0, pos + 1);
					partial = partial.substr(pos + 1);
					pos = 0;

					// Parse and dispatch the message (data is nested)
					JsonParser wrapper;
					if (wrapper.parse(message)) {
						// Access nested data fields using key paths
						std::string eventType = wrapper.getString("data.e", "");
						if (!eventType.empty()) {
							parseAndDispatchNested(wrapper);
						}
					}
				}

				// Check if partial contains a complete message at the end
				if (!partial.empty() && partial.back() == '}') {
					// Count braces to see if complete
					int openCount = 0, closeCount = 0;
					for (char c : partial) {
						if (c == '{') openCount++;
						if (c == '}') closeCount++;
					}

					if (openCount == closeCount && openCount > 0) {
						// Complete message
						JsonParser wrapper;
						if (wrapper.parse(partial)) {
							std::string eventType = wrapper.getString("data.e", "");
							if (!eventType.empty()) {
								parseAndDispatchNested(wrapper);
							}
						}
						partial.clear();
					}
				}
			}

			pclose(pipe);

			if (!shouldStop) {
				LOG_WARNING("WebSocket connection lost, reconnecting in 2 seconds...");
#ifdef __HAIKU__
				snooze(2000000); // 2 seconds
#else
				std::this_thread::sleep_for(std::chrono::seconds(2));
#endif
			}
		}

		LOG_INFO("WebSocket worker thread stopped");
	}

	bool subscribeTicker(const std::string& symbol, TickerCallback callback) {
		std::string lowerSymbol = symbol;
		std::transform(lowerSymbol.begin(), lowerSymbol.end(), lowerSymbol.begin(), ::tolower);

		tickerCallbacks[lowerSymbol] = callback;

		// Build stream name: btcusdt@ticker
		std::string streamName = lowerSymbol + "@ticker";
		subscribedStreams.push_back(streamName);

		LOG_INFO("Subscribed to ticker stream: " + streamName);
		return true;
	}

	bool subscribeTrades(const std::string& symbol, TradeCallback callback) {
		std::string lowerSymbol = symbol;
		std::transform(lowerSymbol.begin(), lowerSymbol.end(), lowerSymbol.begin(), ::tolower);

		tradeCallbacks[lowerSymbol] = callback;

		// Build stream name: btcusdt@trade
		std::string streamName = lowerSymbol + "@trade";
		subscribedStreams.push_back(streamName);

		LOG_INFO("Subscribed to trade stream: " + streamName);
		return true;
	}

	bool subscribeKlines(const std::string& symbol, const std::string& interval, KlineCallback callback) {
		std::string lowerSymbol = symbol;
		std::transform(lowerSymbol.begin(), lowerSymbol.end(), lowerSymbol.begin(), ::tolower);

		std::string key = lowerSymbol + "_" + interval;
		klineCallbacks[key] = callback;

		// Build stream name: btcusdt@kline_1m
		std::string streamName = lowerSymbol + "@kline_" + interval;
		subscribedStreams.push_back(streamName);

		LOG_INFO("Subscribed to kline stream: " + streamName);
		return true;
	}

	void processMessages() {
		std::lock_guard<std::mutex> lock(queueMutex);

		while (!messageQueue.empty()) {
			std::string message = messageQueue.front();
			messageQueue.pop();

			// Parse and dispatch message
			parseAndDispatch(message);
		}
	}

	void parseAndDispatch(const std::string& json) {
		JsonParser parser;
		if (!parser.parse(json)) {
			if (errorCallback) {
				errorCallback("Failed to parse WebSocket message");
			}
			return;
		}

		// Determine message type and dispatch to appropriate callback
		std::string eventType = parser.getString("e", "");

		if (eventType == "24hrTicker") {
			dispatchTicker(parser);
		} else if (eventType == "trade") {
			dispatchTrade(parser);
		} else if (eventType == "kline") {
			dispatchKline(parser);
		} else {
			LOG_WARNING("Unknown WebSocket event type: " + eventType);
		}
	}

	// Parse nested data (for stream format: {"stream":"...","data":{...}})
	void parseAndDispatchNested(JsonParser& wrapper) {
		// Determine message type from nested data.e field
		std::string eventType = wrapper.getString("data.e", "");

		if (eventType == "24hrTicker") {
			dispatchTickerNested(wrapper);
		} else if (eventType == "trade") {
			dispatchTradeNested(wrapper);
		} else if (eventType == "kline") {
			dispatchKlineNested(wrapper);
		} else {
			LOG_WARNING("Unknown WebSocket event type: " + eventType);
		}
	}

	void dispatchTicker(JsonParser& parser) {
		TickerUpdate update;
		update.symbol = parser.getString("s", "");
		update.lastPrice = parser.getDouble("c", 0.0);
		update.priceChange = parser.getDouble("p", 0.0);
		update.priceChangePercent = parser.getDouble("P", 0.0);
		update.highPrice = parser.getDouble("h", 0.0);
		update.lowPrice = parser.getDouble("l", 0.0);
		update.volume = parser.getDouble("v", 0.0);
		update.quoteVolume = parser.getDouble("q", 0.0);
		update.timestamp = parser.getInt64("E", 0);

		std::string lowerSymbol = update.symbol;
		std::transform(lowerSymbol.begin(), lowerSymbol.end(), lowerSymbol.begin(), ::tolower);

		auto it = tickerCallbacks.find(lowerSymbol);
		if (it != tickerCallbacks.end()) {
			it->second(update);
		}
	}

	void dispatchTrade(JsonParser& parser) {
		TradeUpdate update;
		update.symbol = parser.getString("s", "");
		update.tradeId = parser.getInt64("t", 0);
		update.price = parser.getDouble("p", 0.0);
		update.quantity = parser.getDouble("q", 0.0);
		update.timestamp = parser.getInt64("T", 0);
		update.isBuyerMaker = parser.getBool("m", false);

		std::string lowerSymbol = update.symbol;
		std::transform(lowerSymbol.begin(), lowerSymbol.end(), lowerSymbol.begin(), ::tolower);

		auto it = tradeCallbacks.find(lowerSymbol);
		if (it != tradeCallbacks.end()) {
			it->second(update);
		}
	}

	void dispatchTickerNested(JsonParser& wrapper) {
		TickerUpdate update;
		update.symbol = wrapper.getString("data.s", "");
		update.lastPrice = wrapper.getDouble("data.c", 0.0);
		update.priceChange = wrapper.getDouble("data.p", 0.0);
		update.priceChangePercent = wrapper.getDouble("data.P", 0.0);
		update.highPrice = wrapper.getDouble("data.h", 0.0);
		update.lowPrice = wrapper.getDouble("data.l", 0.0);
		update.volume = wrapper.getDouble("data.v", 0.0);
		update.quoteVolume = wrapper.getDouble("data.q", 0.0);
		update.timestamp = wrapper.getInt64("data.E", 0);

		std::string lowerSymbol = update.symbol;
		std::transform(lowerSymbol.begin(), lowerSymbol.end(), lowerSymbol.begin(), ::tolower);

		auto it = tickerCallbacks.find(lowerSymbol);
		if (it != tickerCallbacks.end()) {
			it->second(update);
		}
	}

	void dispatchTradeNested(JsonParser& wrapper) {
		TradeUpdate update;
		update.symbol = wrapper.getString("data.s", "");
		update.tradeId = wrapper.getInt64("data.t", 0);
		update.price = wrapper.getDouble("data.p", 0.0);
		update.quantity = wrapper.getDouble("data.q", 0.0);
		update.timestamp = wrapper.getInt64("data.T", 0);
		update.isBuyerMaker = wrapper.getBool("data.m", false);

		std::string lowerSymbol = update.symbol;
		std::transform(lowerSymbol.begin(), lowerSymbol.end(), lowerSymbol.begin(), ::tolower);

		auto it = tradeCallbacks.find(lowerSymbol);
		if (it != tradeCallbacks.end()) {
			it->second(update);
		}
	}

	void dispatchKlineNested(JsonParser& wrapper) {
		// Similar to dispatchKline but with "data." prefix
		KlineUpdate update;
		update.symbol = wrapper.getString("data.s", "");
		update.interval = wrapper.getString("data.i", "");
		update.openTime = wrapper.getInt64("data.t", 0);
		update.closeTime = wrapper.getInt64("data.T", 0);
		update.open = wrapper.getDouble("data.o", 0.0);
		update.high = wrapper.getDouble("data.h", 0.0);
		update.low = wrapper.getDouble("data.l", 0.0);
		update.close = wrapper.getDouble("data.c", 0.0);
		update.volume = wrapper.getDouble("data.v", 0.0);
		update.isClosed = wrapper.getBool("data.x", false);

		std::string lowerSymbol = update.symbol;
		std::transform(lowerSymbol.begin(), lowerSymbol.end(), lowerSymbol.begin(), ::tolower);

		std::string key = lowerSymbol + "_" + update.interval;

		auto it = klineCallbacks.find(key);
		if (it != klineCallbacks.end()) {
			it->second(update);
		}
	}

	void dispatchKline(JsonParser& parser) {
		// Kline data is nested in "k" object
		JsonParser klineParser;
		// TODO: Extract nested "k" object
		// For now, parse top-level fields

		KlineUpdate update;
		update.symbol = parser.getString("s", "");
		update.interval = parser.getString("i", "");
		update.openTime = parser.getInt64("t", 0);
		update.closeTime = parser.getInt64("T", 0);
		update.open = parser.getDouble("o", 0.0);
		update.high = parser.getDouble("h", 0.0);
		update.low = parser.getDouble("l", 0.0);
		update.close = parser.getDouble("c", 0.0);
		update.volume = parser.getDouble("v", 0.0);
		update.isClosed = parser.getBool("x", false);

		std::string lowerSymbol = update.symbol;
		std::transform(lowerSymbol.begin(), lowerSymbol.end(), lowerSymbol.begin(), ::tolower);

		std::string key = lowerSymbol + "_" + update.interval;

		auto it = klineCallbacks.find(key);
		if (it != klineCallbacks.end()) {
			it->second(update);
		}
	}
};

// ============================================================================
// Public API Implementation
// ============================================================================

BinanceWebSocket::BinanceWebSocket()
	: pImpl(new Impl())
{
}

BinanceWebSocket::~BinanceWebSocket() {
}

bool BinanceWebSocket::connect() {
	return pImpl->connect();
}

void BinanceWebSocket::disconnect() {
	pImpl->disconnect();
}

bool BinanceWebSocket::isConnected() const {
	return pImpl->connected;
}

bool BinanceWebSocket::subscribeTicker(const std::string& symbol, TickerCallback callback) {
	return pImpl->subscribeTicker(symbol, callback);
}

bool BinanceWebSocket::subscribeTrades(const std::string& symbol, TradeCallback callback) {
	return pImpl->subscribeTrades(symbol, callback);
}

bool BinanceWebSocket::subscribeKlines(const std::string& symbol, const std::string& interval, KlineCallback callback) {
	return pImpl->subscribeKlines(symbol, interval, callback);
}

void BinanceWebSocket::unsubscribeTicker(const std::string& symbol) {
	std::string lowerSymbol = symbol;
	std::transform(lowerSymbol.begin(), lowerSymbol.end(), lowerSymbol.begin(), ::tolower);
	pImpl->tickerCallbacks.erase(lowerSymbol);
	LOG_INFO("Unsubscribed from ticker: " + symbol);
}

void BinanceWebSocket::unsubscribeTrades(const std::string& symbol) {
	std::string lowerSymbol = symbol;
	std::transform(lowerSymbol.begin(), lowerSymbol.end(), lowerSymbol.begin(), ::tolower);
	pImpl->tradeCallbacks.erase(lowerSymbol);
	LOG_INFO("Unsubscribed from trades: " + symbol);
}

void BinanceWebSocket::unsubscribeKlines(const std::string& symbol, const std::string& interval) {
	std::string lowerSymbol = symbol;
	std::transform(lowerSymbol.begin(), lowerSymbol.end(), lowerSymbol.begin(), ::tolower);
	std::string key = lowerSymbol + "_" + interval;
	pImpl->klineCallbacks.erase(key);
	LOG_INFO("Unsubscribed from klines: " + symbol + " " + interval);
}

void BinanceWebSocket::setErrorCallback(ErrorCallback callback) {
	pImpl->errorCallback = callback;
}

void BinanceWebSocket::processMessages() {
	pImpl->processMessages();
}

} // namespace Emiglio
