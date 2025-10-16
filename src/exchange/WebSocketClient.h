#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>

namespace Emiglio {

// Simple WebSocket client using BSD sockets + OpenSSL
class WebSocketClient {
public:
    using MessageCallback = std::function<void(const std::string&)>;
    using ErrorCallback = std::function<void(const std::string&)>;
    using ConnectCallback = std::function<void()>;

    WebSocketClient();
    ~WebSocketClient();

    // Connect to WebSocket server
    // url format: wss://host:port/path
    bool connect(const std::string& url);

    // Send text message
    bool send(const std::string& message);

    // Disconnect
    void disconnect();

    // Check if connected
    bool isConnected() const;

    // Set callbacks
    void onMessage(MessageCallback callback);
    void onError(ErrorCallback callback);
    void onConnect(ConnectCallback callback);

private:
    struct Impl;
    Impl* pImpl;

    // Disable copy
    WebSocketClient(const WebSocketClient&) = delete;
    WebSocketClient& operator=(const WebSocketClient&) = delete;
};

} // namespace Emiglio

#endif // WEBSOCKET_CLIENT_H
