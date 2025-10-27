#include "Config.h"
#include "Logger.h"
#include "JsonParser.h"
#include "ExchangeRateService.h"
#include <fstream>
#include <sstream>
#include <filesystem>

namespace Emiglio {

Config::Config()
	: fLoaded(false) {
	_InitDefaultPaths();
}

Config::~Config() {
}

Config& Config::getInstance() {
	static Config instance;
	return instance;
}

void Config::_InitDefaultPaths() {
	// Get home directory
	const char* home = std::getenv("HOME");
	if (!home) {
		home = "/boot/home";
	}

	fConfigDir = std::string(home) + "/config/settings/Emiglio";
	fDataDir = std::string(home) + "/config/settings/Emiglio/data";
	fRecipesDir = std::string(home) + "/config/settings/Emiglio/recipes";
	fLogFile = fConfigDir + "/emilio.log";

	// Set default values
	fConfigMap["app.name"] = "Emiglio";
	fConfigMap["app.version"] = "1.0.0";
	fConfigMap["log.level"] = "INFO";
	fConfigMap["log.file"] = fLogFile;
	fConfigMap["data.dir"] = fDataDir;
	fConfigMap["recipes.dir"] = fRecipesDir;
}

bool Config::load(const std::string& configPath) {
	std::ifstream file(configPath);
	if (!file.is_open()) {
		LOG_WARNING("Config file not found: " + configPath + ", using defaults");
		return false;
	}

	try {
		JsonParser parser;
		if (parser.parseFile(configPath)) {
			// Read known configuration keys from JSON
			// Load app settings
			if (parser.has("app.name")) {
				fConfigMap["app.name"] = parser.getString("app.name");
			}
			if (parser.has("app.version")) {
				fConfigMap["app.version"] = parser.getString("app.version");
			}

			// Load log settings
			if (parser.has("log.level")) {
				fConfigMap["log.level"] = parser.getString("log.level");
			}
			if (parser.has("log.file")) {
				fConfigMap["log.file"] = parser.getString("log.file");
			}

			// Load directory paths
			if (parser.has("data.dir")) {
				fConfigMap["data.dir"] = parser.getString("data.dir");
			}
			if (parser.has("recipes.dir")) {
				fConfigMap["recipes.dir"] = parser.getString("recipes.dir");
			}

			// Load display settings (IMPORTANT: currency preference)
			if (parser.has("display.currency")) {
				fConfigMap["display.currency"] = parser.getString("display.currency");
				LOG_INFO("Config::load() - Loaded currency: " + fConfigMap["display.currency"]);
			} else {
				LOG_WARNING("Config::load() - No display.currency found in config file!");
			}

			fLoaded = true;
			LOG_INFO("Configuration loaded from: " + configPath);
			return true;
		}
	} catch (const std::exception& e) {
		LOG_ERROR("Failed to parse config file: " + std::string(e.what()));
		return false;
	}

	return false;
}

bool Config::save(const std::string& configPath) {
	// Ensure config directory exists
	size_t lastSlash = configPath.find_last_of('/');
	if (lastSlash != std::string::npos) {
		std::string dir = configPath.substr(0, lastSlash);
		// Create directory recursively using C++17 filesystem
		std::error_code ec;
		if (!std::filesystem::create_directories(dir, ec) && ec) {
			LOG_ERROR("Failed to create config directory: " + dir + " - " + ec.message());
			return false;
		}
	}

	std::ofstream file(configPath);
	if (!file.is_open()) {
		LOG_ERROR("Failed to open config file for writing: " + configPath);
		return false;
	}

	// Write JSON header
	file << "{\n";

	// Write all config values
	bool first = true;
	for (const auto& pair : fConfigMap) {
		if (!first) {
			file << ",\n";
		}
		file << "  \"" << pair.first << "\": ";

		// Try to determine type and format accordingly
		if (pair.second == "true" || pair.second == "false") {
			file << pair.second;
		} else if (pair.second.find_first_not_of("0123456789.-") == std::string::npos) {
			file << pair.second;
		} else {
			file << "\"" << pair.second << "\"";
		}

		first = false;
	}

	file << "\n}\n";
	file.close();

	LOG_INFO("Configuration saved to: " + configPath);
	return true;
}

std::string Config::getString(const std::string& key, const std::string& defaultValue) const {
	auto it = fConfigMap.find(key);
	if (it != fConfigMap.end()) {
		return it->second;
	}
	return defaultValue;
}

int Config::getInt(const std::string& key, int defaultValue) const {
	auto it = fConfigMap.find(key);
	if (it != fConfigMap.end()) {
		try {
			return std::stoi(it->second);
		} catch (...) {
			return defaultValue;
		}
	}
	return defaultValue;
}

double Config::getDouble(const std::string& key, double defaultValue) const {
	auto it = fConfigMap.find(key);
	if (it != fConfigMap.end()) {
		try {
			return std::stod(it->second);
		} catch (...) {
			return defaultValue;
		}
	}
	return defaultValue;
}

bool Config::getBool(const std::string& key, bool defaultValue) const {
	auto it = fConfigMap.find(key);
	if (it != fConfigMap.end()) {
		return (it->second == "true" || it->second == "1");
	}
	return defaultValue;
}

std::vector<std::string> Config::getStringArray(const std::string& key) const {
	std::vector<std::string> result;
	// Simplified implementation - look for keys with array indices
	// e.g., "exchanges.0", "exchanges.1", etc.
	int index = 0;
	while (true) {
		std::string arrayKey = key + "." + std::to_string(index);
		if (!has(arrayKey)) {
			break;
		}
		result.push_back(getString(arrayKey));
		index++;
	}
	return result;
}

void Config::setString(const std::string& key, const std::string& value) {
	fConfigMap[key] = value;
}

void Config::setInt(const std::string& key, int value) {
	fConfigMap[key] = std::to_string(value);
}

void Config::setDouble(const std::string& key, double value) {
	fConfigMap[key] = std::to_string(value);
}

void Config::setBool(const std::string& key, bool value) {
	fConfigMap[key] = value ? "true" : "false";
}

bool Config::has(const std::string& key) const {
	return fConfigMap.find(key) != fConfigMap.end();
}

std::string Config::getConfigDir() const {
	return fConfigDir;
}

std::string Config::getDataDir() const {
	return fDataDir;
}

std::string Config::getRecipesDir() const {
	return fRecipesDir;
}

std::string Config::getLogFile() const {
	return fLogFile;
}

std::vector<std::string> Config::_SplitKey(const std::string& key) const {
	std::vector<std::string> parts;
	std::stringstream ss(key);
	std::string part;
	while (std::getline(ss, part, '.')) {
		parts.push_back(part);
	}
	return parts;
}

double Config::getExchangeRate() const {
	std::string currency = getCurrency();
	ExchangeRateService& rateService = ExchangeRateService::getInstance();
	return rateService.getRate(currency);
}

bool Config::refreshExchangeRates() {
	ExchangeRateService& rateService = ExchangeRateService::getInstance();
	return rateService.refreshRates();
}

} // namespace Emiglio
