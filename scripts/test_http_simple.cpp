#include <Application.h>
#include <HttpRequest.h>
#include <HttpResult.h>
#include <HttpSession.h>
#include <Url.h>
#include <iostream>

using namespace BPrivate::Network;

class TestApp : public BApplication {
public:
	TestApp() : BApplication("application/x-vnd.Test-HTTP") {}

	virtual void ReadyToRun() override {
		std::cout << "Testing simple HTTP request..." << std::endl;

		BUrl url("https://api.binance.com/api/v3/ping", true);
		std::cout << "URL created: " << url.UrlString().String() << std::endl;

		BHttpSession session;
		std::cout << "Session created" << std::endl;

		BHttpRequest request(url);
		std::cout << "Request created, executing..." << std::endl;

		// Try with timeout
		BHttpResult result = session.Execute(std::move(request));
		std::cout << "Request executed!" << std::endl;

		int16 status = static_cast<int16>(result.Status().StatusCode());
		std::cout << "Status code: " << status << std::endl;

		if (result.Body().text.has_value()) {
			std::cout << "Body: " << result.Body().text.value().String() << std::endl;
		} else {
			std::cout << "No body" << std::endl;
		}

		Quit();
	}
};

int main() {
	TestApp app;
	app.Run();
	return 0;
}
