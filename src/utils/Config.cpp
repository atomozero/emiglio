#include "Config.h"
#include "Logger.h"
#include "JsonParser.h"
#include <fstream>
#include <sstream>
#include <cstdlib>

namespace Emiglio {

Config::Config()
	: loaded(false) {
	initDefaultPaths();
}

Config::~Config() {
}

Config& Config::getInstance() {
	static Config instance;
	return instance;
}

void Config::initDefaultPaths() {
	// Get home directory
	const char* home = std::getenv("HOME");
	if (!home) {
		home = "/boot/home";
	}

	configDir = std::string(home) + "/config/settings/Emiglio";
	dataDir = std::string(home) + "/config/settings/Emiglio/data";
	recipesDir = std::string(home) + "/config/settings/Emiglio/recipes";
	logFile = configDir + "/emilio.log";

	// Set default values
	configMap["app.name"] = "Emiglio";
	configMap["app.version"] = "1.0.0";
	configMap["log.level"] = "INFO";
	configMap["log.file"] = logFile;
	configMap["data.dir"] = dataDir;
	configMap["recipes.dir"] = recipesDir;
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
			// Read all key-value pairs from JSON
			// This is a simplified implementation
			// In a real implementation, we'd traverse the JSON tree
			loaded = true;
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
		// Create directory recursively
		std::string cmd = "mkdir -p \"" + dir + "\"";
		system(cmd.c_str());
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
	for (const auto& pair : configMap) {
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
	auto it = configMap.find(key);
	if (it != configMap.end()) {
		return it->second;
	}
	return defaultValue;
}

int Config::getInt(const std::string& key, int defaultValue) const {
	auto it = configMap.find(key);
	if (it != configMap.end()) {
		try {
			return std::stoi(it->second);
		} catch (...) {
			return defaultValue;
		}
	}
	return defaultValue;
}

double Config::getDouble(const std::string& key, double defaultValue) const {
	auto it = configMap.find(key);
	if (it != configMap.end()) {
		try {
			return std::stod(it->second);
		} catch (...) {
			return defaultValue;
		}
	}
	return defaultValue;
}

bool Config::getBool(const std::string& key, bool defaultValue) const {
	auto it = configMap.find(key);
	if (it != configMap.end()) {
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
	configMap[key] = value;
}

void Config::setInt(const std::string& key, int value) {
	configMap[key] = std::to_string(value);
}

void Config::setDouble(const std::string& key, double value) {
	configMap[key] = std::to_string(value);
}

void Config::setBool(const std::string& key, bool value) {
	configMap[key] = value ? "true" : "false";
}

bool Config::has(const std::string& key) const {
	return configMap.find(key) != configMap.end();
}

std::string Config::getConfigDir() const {
	return configDir;
}

std::string Config::getDataDir() const {
	return dataDir;
}

std::string Config::getRecipesDir() const {
	return recipesDir;
}

std::string Config::getLogFile() const {
	return logFile;
}

std::vector<std::string> Config::splitKey(const std::string& key) const {
	std::vector<std::string> parts;
	std::stringstream ss(key);
	std::string part;
	while (std::getline(ss, part, '.')) {
		parts.push_back(part);
	}
	return parts;
}

} // namespace Emiglio
