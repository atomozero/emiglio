#ifndef EMIGLIO_PAPERPORTFOLIO_H
#define EMIGLIO_PAPERPORTFOLIO_H

#include <string>
#include <map>
#include <vector>
#include <ctime>

namespace Emiglio {
namespace Paper {

// Position in the paper portfolio
struct PaperPosition {
	std::string symbol;
	std::string side;           // "LONG" or "SHORT"
	double entryPrice;
	double currentPrice;
	double quantity;
	time_t openTime;
	double unrealizedPnL;
	double unrealizedPnLPercent;
};

// Trade execution record
struct PaperTrade {
	std::string symbol;
	std::string side;           // "BUY" or "SELL"
	double price;
	double quantity;
	double fee;
	time_t timestamp;
	std::string orderId;
};

// Paper Portfolio for simulated trading
class PaperPortfolio {
public:
	PaperPortfolio(double initialBalance = 10000.0);
	~PaperPortfolio();

	// Portfolio operations
	bool buy(const std::string& symbol, double quantity, double price,
	         double slippage = 0.0005);
	bool sell(const std::string& symbol, double quantity, double price,
	          double slippage = 0.0005);

	// Position management
	PaperPosition* getPosition(const std::string& symbol);
	std::vector<PaperPosition> getAllPositions() const;
	void updatePrice(const std::string& symbol, double newPrice);
	void closePosition(const std::string& symbol, double price);

	// Portfolio stats
	double getBalance() const { return balance; }
	double getEquity() const;
	double getTotalPnL() const;
	double getTotalPnLPercent() const;
	double getUsedMargin() const;
	double getAvailableMargin() const;

	// Trade history
	std::vector<PaperTrade> getTradeHistory() const { return tradeHistory; }

	// Reset portfolio
	void reset(double newBalance = 10000.0);

private:
	double initialBalance;
	double balance;              // Available cash
	std::map<std::string, PaperPosition> positions;
	std::vector<PaperTrade> tradeHistory;

	// Trading settings
	double feeRate;              // 0.1% = 0.001
	double defaultSlippage;      // 0.05% = 0.0005

	// Helper functions
	double calculateFee(double quantity, double price) const;
	std::string generateOrderId();
};

} // namespace Paper
} // namespace Emiglio

#endif // EMIGLIO_PAPERPORTFOLIO_H
