#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <map>
#include <vector>
#include <memory>

namespace Emiglio {

// Forward declaration for JSON types
class JsonValue;

class Config {
public:
	static Config& getInstance();

	// Load configuration from file
	bool load(const std::string& configPath);

	// Save configuration to file
	bool save(const std::string& configPath);

	// Get string value
	std::string getString(const std::string& key, const std::string& defaultValue = "") const;

	// Get integer value
	int getInt(const std::string& key, int defaultValue = 0) const;

	// Get double value
	double getDouble(const std::string& key, double defaultValue = 0.0) const;

	// Get boolean value
	bool getBool(const std::string& key, bool defaultValue = false) const;

	// Get array of strings
	std::vector<std::string> getStringArray(const std::string& key) const;

	// Set string value
	void setString(const std::string& key, const std::string& value);

	// Set integer value
	void setInt(const std::string& key, int value);

	// Set double value
	void setDouble(const std::string& key, double value);

	// Set boolean value
	void setBool(const std::string& key, bool value);

	// Check if key exists
	bool has(const std::string& key) const;

	// Get config file paths
	std::string getConfigDir() const;
	std::string getDataDir() const;
	std::string getRecipesDir() const;
	std::string getLogFile() const;

	// Helper methods for common settings
	std::string getCurrency() const { return getString("display.currency", "USD"); }
	bool setCurrency(const std::string& currency) {
		setString("display.currency", currency);
		return true;
	}

	// Get preferred quote asset for trading pairs (e.g., "USDT" for USD, "EUR" for EUR)
	std::string getPreferredQuote() const {
		std::string currency = getCurrency();
		// Map fiat currencies to their crypto equivalent
		if (currency == "USD") return "USDT";  // Tether USD
		if (currency == "EUR") return "EUR";   // Euro stablecoin
		if (currency == "GBP") return "GBP";   // British Pound (if available)
		if (currency == "JPY") return "JPY";   // Japanese Yen (if available)
		// For other currencies, default to USDT
		return "USDT";
	}

	// Save to default location
	bool save() { return save(getConfigDir() + "/config.json"); }

	// Delete copy constructor and assignment operator
	Config(const Config&) = delete;
	Config& operator=(const Config&) = delete;

private:
	Config();
	~Config();

	void initDefaultPaths();
	std::vector<std::string> splitKey(const std::string& key) const;

	std::map<std::string, std::string> configMap;
	std::string configDir;
	std::string dataDir;
	std::string recipesDir;
	std::string logFile;
	bool loaded;
};

} // namespace Emiglio

#endif // CONFIG_H
