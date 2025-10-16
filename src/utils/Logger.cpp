#include "Logger.h"
#include <iomanip>
#include <ctime>

namespace Emiglio {

Logger::Logger()
	: minLogLevel(LogLevel::INFO)
	, initialized(false) {
}

Logger::~Logger() {
	close();
}

Logger& Logger::getInstance() {
	static Logger instance;
	return instance;
}

void Logger::init(const std::string& logFilePath, LogLevel minLevel) {
	std::lock_guard<std::mutex> lock(logMutex);

	if (initialized) {
		logFile.close();
	}

	logFile.open(logFilePath, std::ios::app);
	if (!logFile.is_open()) {
		std::cerr << "Failed to open log file: " << logFilePath << std::endl;
		return;
	}

	minLogLevel = minLevel;
	initialized = true;

	// Log initialization (no lock needed, already locked)
	std::string logEntry = getCurrentTimestamp() + " " +
	                       levelToString(LogLevel::INFO) + " " +
	                       "=== Emiglio Logger Initialized ===";
	if (logFile.is_open()) {
		logFile << logEntry << std::endl;
	}
}

void Logger::debug(const std::string& message) {
	log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
	log(LogLevel::INFO, message);
}

void Logger::warning(const std::string& message) {
	log(LogLevel::WARNING, message);
}

void Logger::error(const std::string& message) {
	log(LogLevel::ERROR, message);
}

void Logger::critical(const std::string& message) {
	log(LogLevel::CRITICAL, message);
}

void Logger::log(LogLevel level, const std::string& message) {
	if (level >= minLogLevel) {
		writeLog(level, message);
	}
}

void Logger::setLogLevel(LogLevel level) {
	std::lock_guard<std::mutex> lock(logMutex);
	minLogLevel = level;
}

void Logger::flush() {
	std::lock_guard<std::mutex> lock(logMutex);
	if (logFile.is_open()) {
		logFile.flush();
	}
}

void Logger::close() {
	std::lock_guard<std::mutex> lock(logMutex);
	if (initialized && logFile.is_open()) {
		// Log close message (no lock needed, already locked)
		std::string logEntry = getCurrentTimestamp() + " " +
		                       levelToString(LogLevel::INFO) + " " +
		                       "=== Emiglio Logger Closed ===";
		logFile << logEntry << std::endl;
		logFile.close();
		initialized = false;
	}
}

std::string Logger::getCurrentTimestamp() {
	auto now = std::chrono::system_clock::now();
	auto time = std::chrono::system_clock::to_time_t(now);
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
		now.time_since_epoch()) % 1000;

	std::stringstream ss;
	ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
	ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
	return ss.str();
}

std::string Logger::levelToString(LogLevel level) {
	switch (level) {
		case LogLevel::DEBUG:    return "[DEBUG]   ";
		case LogLevel::INFO:     return "[INFO]    ";
		case LogLevel::WARNING:  return "[WARNING] ";
		case LogLevel::ERROR:    return "[ERROR]   ";
		case LogLevel::CRITICAL: return "[CRITICAL]";
		default:                 return "[UNKNOWN] ";
	}
}

void Logger::writeLog(LogLevel level, const std::string& message) {
	std::lock_guard<std::mutex> lock(logMutex);

	std::string logEntry = getCurrentTimestamp() + " " +
	                       levelToString(level) + " " +
	                       message;

	// Write to file
	if (initialized && logFile.is_open()) {
		logFile << logEntry << std::endl;
	}

	// If no log file, write INFO and above to stdout
	if (!initialized || !logFile.is_open()) {
		std::cout << logEntry << std::endl;
	}

	// Also write to console for WARNING and above
	if (level >= LogLevel::WARNING) {
		std::cerr << logEntry << std::endl;
	}
}

} // namespace Emiglio
