#include <HttpRequest.h>
#include <Url.h>
#include <UrlProtocolListener.h>
#include <Application.h>
#include <iostream>

using namespace BPrivate::Network;

class ResponseListener : public BUrlProtocolListener {
public:
    std::string response;
    void DataReceived(BUrlRequest* caller, const char* data,
                      off_t position, ssize_t size) {
        (void)caller; (void)position;
        response.append(data, size);
    }
};

int main() {
    BApplication app("application/x-vnd.test");

    BUrl burl("https://api.binance.com/api/v3/ping");
    BHttpRequest request(burl);  // Try calling constructor directly

    ResponseListener listener;
    request.SetListener(&listener);

    thread_id thread = request.Run();
    status_t status;
    wait_for_thread(thread, &status);

    std::cout << "Response: " << listener.response << std::endl;
    return 0;
}
