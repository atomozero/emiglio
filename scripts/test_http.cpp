#include <Application.h>
#include <UrlProtocolRoster.h>
#include <HttpRequest.h>
#include <HttpResult.h>
#include <Url.h>
#include <DataIO.h>
#include <OS.h>
#include <iostream>

using namespace BPrivate::Network;

class HttpTestApp : public BApplication {
public:
	HttpTestApp() : BApplication("application/x-vnd.test-http") {}

	virtual void ReadyToRun() override {
		std::cout << "Testing HTTP request..." << std::endl;

		// Create URL
		BUrl url("https://api.binance.com/api/v3/ping", true);
		BMallocIO output;

		// Create request
		BUrlRequest* request = BUrlProtocolRoster::MakeRequest(url, &output);

		if (!request) {
			std::cout << "Failed to create request" << std::endl;
			Quit();
			return;
		}

		std::cout << "Request created, starting..." << std::endl;

		// Run request
		thread_id thread = request->Run();
		if (thread < 0) {
			std::cout << "Failed to run request" << std::endl;
			delete request;
			Quit();
			return;
		}

		std::cout << "Request running, waiting..." << std::endl;

		// Wait for completion
		status_t status;
		wait_for_thread(thread, &status);

		std::cout << "Request completed!" << std::endl;

		// Get result
		const BHttpResult& result = dynamic_cast<const BHttpResult&>(request->Result());
		std::cout << "Status code: " << result.StatusCode() << std::endl;

		// Get response
		const void* buffer = output.Buffer();
		size_t length = output.BufferLength();
		if (buffer && length > 0) {
			std::string response(static_cast<const char*>(buffer), length);
			std::cout << "Response: " << response << std::endl;
		}

		delete request;
		Quit();
	}
};

int main() {
	HttpTestApp app;
	app.Run();
	return 0;
}
