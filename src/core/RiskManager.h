#ifndef EMIGLIO_CORE_RISKMANAGER_H
#define EMIGLIO_CORE_RISKMANAGER_H

#include "RecipeLoader.h"
#include "Trade.h"
#include <string>
#include <vector>
#include <map>
#include <ctime>

namespace Emiglio {
namespace Core {

// Position information for live trading
struct Position {
	std::string id;              // Position ID
	std::string symbol;          // Trading pair
	double entryPrice;           // Entry price
	double quantity;             // Position size
	time_t entryTime;            // Entry timestamp
	double stopLossPrice;        // Stop-loss price (0 = no SL)
	double takeProfitPrice;      // Take-profit price (0 = no TP)
	bool trailingStopEnabled;    // Trailing stop enabled
	double trailingStopPercent;  // Trailing stop percentage
	double highestPrice;         // Highest price since entry (for trailing stop)
	double currentPnL;           // Current unrealized P&L
	double currentPnLPercent;    // Current unrealized P&L %
};

// Daily statistics for loss tracking
struct DailyStats {
	time_t date;                 // Date (midnight timestamp)
	double startingCapital;      // Capital at start of day
	double currentCapital;       // Current capital
	double realizedPnL;          // Realized P&L for the day
	double maxDrawdown;          // Max drawdown during the day
	int tradesExecuted;          // Number of trades
	int winningTrades;           // Winning trades count
	int losingTrades;            // Losing trades count
};

// Risk Manager - Manages risk controls for live trading
class RiskManager {
public:
	RiskManager();
	~RiskManager();

	// Configuration
	void configure(const RiskConfig& config);
	void setCapital(double totalCapital);

	// Position validation BEFORE opening
	bool canOpenPosition(const std::string& symbol, double price, double quantity) const;
	bool isWithinRiskLimits(double positionValue) const;
	bool isMaxPositionsReached() const;
	bool isWithinDailyLossLimit() const;

	// Position sizing
	double calculatePositionSize(double currentPrice, double totalCapital) const;
	double calculateQuantity(double positionValue, double currentPrice) const;

	// Stop-loss and take-profit calculation
	double calculateStopLossPrice(double entryPrice, bool isLong) const;
	double calculateTakeProfitPrice(double entryPrice, bool isLong) const;

	// Position tracking
	void addPosition(const Position& position);
	void removePosition(const std::string& positionId);
	void updatePosition(const std::string& positionId, double currentPrice);
	std::vector<Position> getOpenPositions() const;
	Position* getPosition(const std::string& positionId);
	int getOpenPositionsCount() const;

	// Stop-loss/take-profit monitoring
	struct TriggerResult {
		bool triggered;
		std::string reason;        // "stop-loss", "take-profit", "trailing-stop"
		double exitPrice;
	};
	TriggerResult checkStopLoss(const Position& position, double currentPrice) const;
	TriggerResult checkTakeProfit(const Position& position, double currentPrice) const;
	TriggerResult checkTrailingStop(Position& position, double currentPrice);
	TriggerResult shouldClosePosition(Position& position, double currentPrice);

	// Daily statistics
	void recordTrade(double pnl, bool isWinner);
	void updateDailyStats(double currentCapital);
	void resetDailyStats();  // Called at midnight
	DailyStats getDailyStats() const;

	// Portfolio metrics
	double getTotalExposure() const;         // Sum of all position values
	double getAvailableCapital() const;      // Capital not in positions
	double getUsedCapitalPercent() const;    // Percentage of capital in use
	double getTotalUnrealizedPnL() const;    // Total unrealized P&L
	double getDailyPnL() const;              // Today's realized P&L
	double getDailyPnLPercent() const;       // Today's P&L as %

	// Risk metrics
	double getMaxPositionValue() const;      // Max allowed position value
	double getRemainingDailyLoss() const;    // How much can lose today before limit
	bool isDailyLossLimitHit() const;        // Has daily limit been reached?

	// Emergency controls
	void enableTrading();
	void disableTrading();
	bool isTradingEnabled() const;
	void setEmergencyStopLoss(bool enabled);  // Force close all on limit

	// Reset
	void reset();
	void clearPositions();

	// Getters for configuration
	const RiskConfig& getRiskConfig() const;
	double getTotalCapital() const;

private:
	// Configuration
	RiskConfig config;
	double totalCapital;
	bool tradingEnabled;
	bool emergencyStopLoss;

	// Open positions
	std::vector<Position> openPositions;
	std::map<std::string, int> positionIndex;  // positionId -> index in vector

	// Daily tracking
	DailyStats dailyStats;
	time_t lastResetTime;

	// Helper methods
	bool isNewDay() const;
	void autoResetIfNewDay();
	time_t getTodayMidnight() const;
	double calculatePositionValue(const Position& pos) const;
	void updatePositionPnL(Position& pos, double currentPrice);
};

} // namespace Core
} // namespace Emiglio

#endif // EMIGLIO_CORE_RISKMANAGER_H
