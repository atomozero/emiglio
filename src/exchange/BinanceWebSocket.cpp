#include "BinanceWebSocket.h"
#include "WebSocketClient.h"
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
#else
#include <chrono>
#endif

namespace Emiglio {

// Private implementation using PIMPL pattern
class BinanceWebSocket::Impl {
public:
	// WebSocket client
	WebSocketClient wsClient;
	bool connected;

	// Callbacks
	std::map<std::string, TickerCallback> tickerCallbacks;
	std::map<std::string, TradeCallback> tradeCallbacks;
	std::map<std::string, KlineCallback> klineCallbacks;
	ErrorCallback errorCallback;

	// Subscribed streams
	std::vector<std::string> subscribedStreams;

	// CRITICAL FIX: Message queue for thread-safe callback handling
	std::mutex messageMutex;
	std::queue<std::string> messageQueue;

	Impl() : connected(false) {
		// Setup WebSocket callbacks
		wsClient.onConnect([this]() {
			LOG_INFO("WebSocket connected successfully");
			connected = true;
		});

		wsClient.onMessage([this](const std::string& message) {
			// CRITICAL FIX: Queue messages instead of processing them directly
			// This avoids calling UI callbacks from background thread
			std::lock_guard<std::mutex> lock(messageMutex);
			messageQueue.push(message);
		});

		wsClient.onError([this](const std::string& error) {
			LOG_ERROR("WebSocket error: " + error);
			if (errorCallback) {
				errorCallback(error);
			}
		});
	}

	~Impl() {
		disconnect();
	}

	bool connect() {
		if (connected) {
			LOG_WARNING("Already connected to WebSocket");
			return true;
		}

		if (subscribedStreams.empty()) {
			LOG_WARNING("No streams subscribed, cannot connect");
			return false;
		}

		LOG_INFO("Connecting to Binance WebSocket...");

		// Build URL with streams
		std::string streams;
		for (size_t i = 0; i < subscribedStreams.size(); i++) {
			if (i > 0) streams += "/";
			streams += subscribedStreams[i];
		}

		std::string url = "wss://stream.binance.com:9443/stream?streams=" + streams;
		LOG_INFO("WebSocket URL: " + url);

		return wsClient.connect(url);
	}

	void disconnect() {
		if (!connected) return;

		LOG_INFO("Disconnecting from WebSocket...");
		wsClient.disconnect();
		connected = false;
		subscribedStreams.clear();
		LOG_INFO("WebSocket disconnected");
	}

	// Handle incoming WebSocket messages
	void handleMessage(const std::string& message) {
		// Parse JSON message
		JsonParser wrapper;
		if (!wrapper.parse(message)) {
			LOG_WARNING("Failed to parse WebSocket message");
			return;
		}

		// Binance sends messages in format: {"stream":"btcusdt@trade","data":{...}}
		std::string eventType = wrapper.getString("data.e", "");
		if (!eventType.empty()) {
			parseAndDispatchNested(wrapper);
		}
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
	// CRITICAL FIX: Process queued messages in main thread
	// This function MUST be called periodically from the main thread (e.g., via BMessageRunner)
	std::lock_guard<std::mutex> lock(pImpl->messageMutex);

	// Process all queued messages
	while (!pImpl->messageQueue.empty()) {
		std::string message = pImpl->messageQueue.front();
		pImpl->messageQueue.pop();

		// Now it's safe to call handleMessage (we're in the main thread)
		pImpl->handleMessage(message);
	}
}

} // namespace Emiglio
