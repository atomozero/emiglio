#include "WebSocketClient.h"
#include "../utils/Logger.h"

#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <sstream>
#include <random>
#include <thread>
#include <mutex>
#include <atomic>

namespace Emiglio {

// Base64 encode function
static std::string base64_encode(const unsigned char* input, size_t length) {
    BIO *bio, *b64;
    BUF_MEM *bufferPtr;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, input, length);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bufferPtr);

    std::string result(bufferPtr->data, bufferPtr->length);
    BIO_free_all(bio);

    return result;
}

// Generate random WebSocket key
static std::string generate_websocket_key() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    unsigned char key[16];
    for (int i = 0; i < 16; i++) {
        key[i] = static_cast<unsigned char>(dis(gen));
    }

    return base64_encode(key, 16);
}

// Parse URL into components
struct UrlComponents {
    std::string host;
    std::string port;
    std::string path;
    bool secure;
};

static bool parse_url(const std::string& url, UrlComponents& components) {
    // Expected format: wss://host:port/path or ws://host:port/path
    size_t pos = 0;

    // Check scheme
    if (url.substr(0, 6) == "wss://") {
        components.secure = true;
        pos = 6;
    } else if (url.substr(0, 5) == "ws://") {
        components.secure = false;
        pos = 5;
    } else {
        return false;
    }

    // Find path separator
    size_t path_pos = url.find('/', pos);
    std::string host_port;

    if (path_pos != std::string::npos) {
        host_port = url.substr(pos, path_pos - pos);
        components.path = url.substr(path_pos);
    } else {
        host_port = url.substr(pos);
        components.path = "/";
    }

    // Split host and port
    size_t colon_pos = host_port.find(':');
    if (colon_pos != std::string::npos) {
        components.host = host_port.substr(0, colon_pos);
        components.port = host_port.substr(colon_pos + 1);
    } else {
        components.host = host_port;
        components.port = components.secure ? "443" : "80";
    }

    return true;
}

// WebSocket frame opcodes
enum class Opcode : uint8_t {
    CONTINUATION = 0x0,
    TEXT = 0x1,
    BINARY = 0x2,
    CLOSE = 0x8,
    PING = 0x9,
    PONG = 0xA
};

struct WebSocketClient::Impl {
    int sockfd;
    SSL_CTX* ssl_ctx;
    SSL* ssl;
    bool connected;
    std::atomic<bool> shouldStop;
    std::thread readerThread;

    MessageCallback messageCallback;
    ErrorCallback errorCallback;
    ConnectCallback connectCallback;

    std::mutex writeMutex;
    std::vector<uint8_t> frameBuffer; // Buffer for partial frames

    Impl() : sockfd(-1), ssl_ctx(nullptr), ssl(nullptr), connected(false), shouldStop(false) {
        // Initialize OpenSSL
        SSL_load_error_strings();
        SSL_library_init();
        OpenSSL_add_all_algorithms();
    }

    ~Impl() {
        disconnect();
        if (ssl_ctx) {
            SSL_CTX_free(ssl_ctx);
        }
    }

    bool connect_socket(const std::string& url) {
        UrlComponents components;
        if (!parse_url(url, components)) {
            if (errorCallback) errorCallback("Invalid WebSocket URL");
            return false;
        }

        LOG_INFO("Connecting to " + components.host + ":" + components.port + components.path);

        // Resolve hostname
        struct addrinfo hints, *result;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        int ret = getaddrinfo(components.host.c_str(), components.port.c_str(), &hints, &result);
        if (ret != 0) {
            if (errorCallback) errorCallback("Failed to resolve host: " + std::string(gai_strerror(ret)));
            return false;
        }

        // Create socket
        sockfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (sockfd < 0) {
            if (errorCallback) errorCallback("Failed to create socket");
            freeaddrinfo(result);
            return false;
        }

        // Connect
        if (::connect(sockfd, result->ai_addr, result->ai_addrlen) < 0) {
            if (errorCallback) errorCallback("Failed to connect: " + std::string(strerror(errno)));
            close(sockfd);
            sockfd = -1;
            freeaddrinfo(result);
            return false;
        }

        freeaddrinfo(result);

        // Setup SSL if needed
        if (components.secure) {
            ssl_ctx = SSL_CTX_new(TLS_client_method());
            if (!ssl_ctx) {
                if (errorCallback) errorCallback("Failed to create SSL context");
                close(sockfd);
                sockfd = -1;
                return false;
            }

            ssl = SSL_new(ssl_ctx);
            SSL_set_fd(ssl, sockfd);

            if (SSL_connect(ssl) <= 0) {
                if (errorCallback) errorCallback("SSL handshake failed");
                SSL_free(ssl);
                SSL_CTX_free(ssl_ctx);
                ssl = nullptr;
                ssl_ctx = nullptr;
                close(sockfd);
                sockfd = -1;
                return false;
            }
        }

        // Perform WebSocket handshake
        if (!perform_handshake(components)) {
            disconnect();
            return false;
        }

        connected = true;
        shouldStop = false;

        // Start reader thread
        readerThread = std::thread(&Impl::reader_loop, this);

        if (connectCallback) connectCallback();

        return true;
    }

    bool perform_handshake(const UrlComponents& components) {
        std::string key = generate_websocket_key();

        std::ostringstream request;
        request << "GET " << components.path << " HTTP/1.1\r\n";
        request << "Host: " << components.host << "\r\n";
        request << "Upgrade: websocket\r\n";
        request << "Connection: Upgrade\r\n";
        request << "Sec-WebSocket-Key: " << key << "\r\n";
        request << "Sec-WebSocket-Version: 13\r\n";
        request << "\r\n";

        std::string req_str = request.str();
        if (!write_data(req_str.c_str(), req_str.length())) {
            if (errorCallback) errorCallback("Failed to send handshake");
            return false;
        }

        // Read response
        char buffer[4096];
        int received = read_data(buffer, sizeof(buffer) - 1);
        if (received <= 0) {
            if (errorCallback) errorCallback("Failed to receive handshake response");
            return false;
        }

        buffer[received] = '\0';
        std::string response(buffer);

        // Check for 101 Switching Protocols
        if (response.find("101") == std::string::npos) {
            if (errorCallback) errorCallback("WebSocket handshake failed: " + response.substr(0, 100));
            return false;
        }

        LOG_INFO("WebSocket handshake successful");
        return true;
    }

    void disconnect() {
        if (!connected) return;

        shouldStop = true;
        connected = false;

        if (readerThread.joinable()) {
            readerThread.join();
        }

        if (ssl) {
            SSL_shutdown(ssl);
            SSL_free(ssl);
            ssl = nullptr;
        }

        if (sockfd >= 0) {
            close(sockfd);
            sockfd = -1;
        }

        LOG_INFO("WebSocket disconnected");
    }

    bool write_data(const char* data, size_t length) {
        if (ssl) {
            return SSL_write(ssl, data, length) > 0;
        } else {
            return ::send(sockfd, data, length, 0) > 0;
        }
    }

    int read_data(char* buffer, size_t length) {
        if (ssl) {
            return SSL_read(ssl, buffer, length);
        } else {
            return ::recv(sockfd, buffer, length, 0);
        }
    }

    bool send_frame(Opcode opcode, const std::string& payload) {
        std::lock_guard<std::mutex> lock(writeMutex);

        std::vector<uint8_t> frame;

        // First byte: FIN + opcode
        frame.push_back(0x80 | static_cast<uint8_t>(opcode));

        // Second byte: MASK + payload length
        size_t payload_len = payload.length();
        if (payload_len < 126) {
            frame.push_back(0x80 | static_cast<uint8_t>(payload_len));
        } else if (payload_len < 65536) {
            frame.push_back(0x80 | 126);
            frame.push_back((payload_len >> 8) & 0xFF);
            frame.push_back(payload_len & 0xFF);
        } else {
            frame.push_back(0x80 | 127);
            for (int i = 7; i >= 0; i--) {
                frame.push_back((payload_len >> (i * 8)) & 0xFF);
            }
        }

        // Masking key (client must mask)
        uint8_t mask[4];
        for (int i = 0; i < 4; i++) {
            mask[i] = rand() % 256;
            frame.push_back(mask[i]);
        }

        // Masked payload
        for (size_t i = 0; i < payload_len; i++) {
            frame.push_back(payload[i] ^ mask[i % 4]);
        }

        return write_data(reinterpret_cast<const char*>(frame.data()), frame.size());
    }

    void reader_loop() {
        std::vector<uint8_t> readBuffer;
        readBuffer.resize(8192);

        while (!shouldStop && connected) {
            int received = read_data(reinterpret_cast<char*>(readBuffer.data()), readBuffer.size());

            if (received <= 0) {
                if (!shouldStop) {
                    if (errorCallback) errorCallback("Connection lost");
                    connected = false;
                }
                break;
            }

            // Append new data to frame buffer
            frameBuffer.insert(frameBuffer.end(), readBuffer.begin(), readBuffer.begin() + received);

            // Parse WebSocket frames from buffer
            size_t offset = 0;
            while (offset + 2 <= frameBuffer.size()) {
                uint8_t byte1 = frameBuffer[offset];
                uint8_t byte2 = frameBuffer[offset + 1];

                bool fin = (byte1 & 0x80) != 0;
                Opcode opcode = static_cast<Opcode>(byte1 & 0x0F);
                bool masked = (byte2 & 0x80) != 0;
                uint64_t payload_len = byte2 & 0x7F;

                size_t header_size = 2;

                // Extended payload length
                if (payload_len == 126) {
                    if (offset + 4 > frameBuffer.size()) break; // Need more data
                    payload_len = (frameBuffer[offset + 2] << 8) | frameBuffer[offset + 3];
                    header_size += 2;
                } else if (payload_len == 127) {
                    if (offset + 10 > frameBuffer.size()) break; // Need more data
                    payload_len = 0;
                    for (int i = 0; i < 8; i++) {
                        payload_len = (payload_len << 8) | frameBuffer[offset + 2 + i];
                    }
                    header_size += 8;
                }

                // Skip mask (server should not mask)
                if (masked) {
                    header_size += 4;
                }

                // Check if we have the complete frame
                if (offset + header_size + payload_len > frameBuffer.size()) {
                    break; // Need more data
                }

                // Extract payload
                std::string payload(
                    reinterpret_cast<char*>(&frameBuffer[offset + header_size]),
                    payload_len
                );

                // Move offset past this frame
                offset += header_size + payload_len;

                // Handle frame
                if (opcode == Opcode::TEXT && messageCallback) {
                    messageCallback(payload);
                } else if (opcode == Opcode::CLOSE) {
                    LOG_INFO("WebSocket close frame received");
                    connected = false;
                    // Remove processed data and exit
                    frameBuffer.erase(frameBuffer.begin(), frameBuffer.begin() + offset);
                    return;
                } else if (opcode == Opcode::PING) {
                    // Respond with PONG
                    send_frame(Opcode::PONG, payload);
                }
            }

            // Remove processed data from buffer
            if (offset > 0) {
                frameBuffer.erase(frameBuffer.begin(), frameBuffer.begin() + offset);
            }

            // Prevent buffer from growing indefinitely if we have garbage data
            if (frameBuffer.size() > 1024 * 1024) { // 1MB limit
                LOG_ERROR("Frame buffer exceeded 1MB, clearing (possible corrupt data)");
                frameBuffer.clear();
            }
        }
    }
};

// Public API implementation
WebSocketClient::WebSocketClient() : pImpl(new Impl()) {}

WebSocketClient::~WebSocketClient() {
    delete pImpl;
}

bool WebSocketClient::connect(const std::string& url) {
    return pImpl->connect_socket(url);
}

bool WebSocketClient::send(const std::string& message) {
    if (!pImpl->connected) return false;
    return pImpl->send_frame(Opcode::TEXT, message);
}

void WebSocketClient::disconnect() {
    pImpl->disconnect();
}

bool WebSocketClient::isConnected() const {
    return pImpl->connected;
}

void WebSocketClient::onMessage(MessageCallback callback) {
    pImpl->messageCallback = callback;
}

void WebSocketClient::onError(ErrorCallback callback) {
    pImpl->errorCallback = callback;
}

void WebSocketClient::onConnect(ConnectCallback callback) {
    pImpl->connectCallback = callback;
}

} // namespace Emiglio
