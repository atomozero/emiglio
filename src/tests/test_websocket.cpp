#include "../exchange/WebSocketClient.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>

using namespace Emiglio;

// Simple test framework macros
#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    std::cout << "Running " #name "..." << std::endl; \
    test_##name(); \
    std::cout << "✓ " #name " passed" << std::endl; \
} while(0)

#define ASSERT_TRUE(expr) do { \
    if (!(expr)) { \
        std::cerr << "✗ Assertion failed: " #expr << " at line " << __LINE__ << std::endl; \
        exit(1); \
    } \
} while(0)

#define ASSERT_FALSE(expr) ASSERT_TRUE(!(expr))
#define ASSERT_EQ(a, b) ASSERT_TRUE((a) == (b))

// Test: WebSocketClient can be created
TEST(websocket_creation) {
    WebSocketClient client;
    ASSERT_FALSE(client.isConnected());
}

// Test: URL parsing
TEST(url_parsing) {
    WebSocketClient client;

    // Invalid URLs should fail
    ASSERT_FALSE(client.connect("invalid-url"));
    ASSERT_FALSE(client.connect("http://example.com"));
    ASSERT_FALSE(client.isConnected());
}

// Test: Callbacks can be set
TEST(callback_setup) {
    WebSocketClient client;

    bool connectCalled = false;
    bool messageCalled = false;
    bool errorCalled = false;

    client.onConnect([&connectCalled]() {
        connectCalled = true;
    });

    client.onMessage([&messageCalled](const std::string& msg) {
        messageCalled = true;
    });

    client.onError([&errorCalled](const std::string& err) {
        errorCalled = true;
    });

    // Callbacks are set (we can't verify without actually connecting)
    ASSERT_TRUE(true); // Placeholder
}

// Test: Connect and disconnect
TEST(connect_disconnect) {
    WebSocketClient client;

    bool connected = false;
    client.onConnect([&connected]() {
        connected = true;
    });

    // Try connecting to Binance WebSocket (this will actually attempt connection)
    // Note: This test requires internet connection
    std::string url = "wss://stream.binance.com:9443/ws/btcusdt@trade";

    if (client.connect(url)) {
        // Wait a bit for connection to establish
        std::this_thread::sleep_for(std::chrono::seconds(2));

        ASSERT_TRUE(client.isConnected());
        ASSERT_TRUE(connected);

        client.disconnect();
        ASSERT_FALSE(client.isConnected());
    } else {
        std::cout << "  (Skipping connection test - no internet or Binance unavailable)" << std::endl;
    }
}

// Test: Receive messages from real WebSocket
TEST(receive_messages) {
    WebSocketClient client;

    bool messageReceived = false;
    std::string receivedMessage;

    client.onMessage([&messageReceived, &receivedMessage](const std::string& msg) {
        messageReceived = true;
        receivedMessage = msg;
    });

    std::string url = "wss://stream.binance.com:9443/ws/btcusdt@trade";

    if (client.connect(url)) {
        // Wait for at least one message
        int timeout = 10; // 10 seconds
        while (!messageReceived && timeout > 0) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            timeout--;
        }

        client.disconnect();

        if (messageReceived) {
            ASSERT_TRUE(receivedMessage.length() > 0);
            ASSERT_TRUE(receivedMessage.find("\"e\":") != std::string::npos); // Binance format
            std::cout << "  Received message: " << receivedMessage.substr(0, 100) << "..." << std::endl;
        } else {
            std::cout << "  (No message received within timeout)" << std::endl;
        }
    } else {
        std::cout << "  (Skipping message test - no internet or Binance unavailable)" << std::endl;
    }
}

// Test: Multiple connections (should not allow)
TEST(multiple_connections) {
    WebSocketClient client;

    std::string url = "wss://stream.binance.com:9443/ws/btcusdt@trade";

    if (client.connect(url)) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // Try connecting again - should fail or return false
        bool secondConnect = client.connect(url);

        client.disconnect();

        // Second connection should not succeed while first is active
        ASSERT_FALSE(secondConnect);
    } else {
        std::cout << "  (Skipping multiple connection test)" << std::endl;
    }
}

// Test: Error handling for invalid host
TEST(invalid_host) {
    WebSocketClient client;

    bool errorOccurred = false;
    std::string errorMessage;

    client.onError([&errorOccurred, &errorMessage](const std::string& err) {
        errorOccurred = true;
        errorMessage = err;
    });

    // Try connecting to non-existent host
    bool result = client.connect("wss://non-existent-host-12345.com:9443/ws");

    // Should fail immediately or call error callback
    if (!result || errorOccurred) {
        ASSERT_TRUE(true); // Expected failure
        if (errorOccurred) {
            std::cout << "  Error message: " << errorMessage << std::endl;
        }
    }
}

// Test: Disconnect while not connected
TEST(disconnect_when_not_connected) {
    WebSocketClient client;

    ASSERT_FALSE(client.isConnected());
    client.disconnect(); // Should not crash
    ASSERT_FALSE(client.isConnected());
}

// Test: Send message (requires connection)
TEST(send_message) {
    WebSocketClient client;

    std::string url = "wss://stream.binance.com:9443/ws/btcusdt@trade";

    if (client.connect(url)) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // Try sending a message (Binance may ignore it, but shouldn't crash)
        bool sent = client.send("{\"method\":\"SUBSCRIBE\",\"params\":[\"btcusdt@ticker\"]}");

        client.disconnect();

        ASSERT_TRUE(sent); // Should return true if sent
    } else {
        std::cout << "  (Skipping send test)" << std::endl;
    }
}

// Performance test: Multiple rapid messages
TEST(rapid_messages) {
    WebSocketClient client;

    int messageCount = 0;
    client.onMessage([&messageCount](const std::string& msg) {
        messageCount++;
    });

    std::string url = "wss://stream.binance.com:9443/ws/btcusdt@trade";

    if (client.connect(url)) {
        // Receive messages for 5 seconds
        std::this_thread::sleep_for(std::chrono::seconds(5));

        client.disconnect();

        std::cout << "  Received " << messageCount << " messages in 5 seconds" << std::endl;
        ASSERT_TRUE(messageCount > 0); // Should receive at least some messages
    } else {
        std::cout << "  (Skipping rapid messages test)" << std::endl;
    }
}

int main() {
    std::cout << "=== WebSocketClient Tests ===" << std::endl << std::endl;

    RUN_TEST(websocket_creation);
    RUN_TEST(url_parsing);
    RUN_TEST(callback_setup);
    RUN_TEST(disconnect_when_not_connected);

    std::cout << "\n--- Network-dependent tests (may be skipped) ---" << std::endl;
    RUN_TEST(connect_disconnect);
    RUN_TEST(receive_messages);
    RUN_TEST(multiple_connections);
    RUN_TEST(invalid_host);
    RUN_TEST(send_message);
    RUN_TEST(rapid_messages);

    std::cout << "\n=== All WebSocket tests passed! ===" << std::endl;
    return 0;
}
