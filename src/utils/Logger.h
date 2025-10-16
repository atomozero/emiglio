#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <sstream>
#include <iostream>

namespace Emiglio {

enum class LogLevel {
	DEBUG = 0,
	INFO = 1,
	WARNING = 2,
	ERROR = 3,
	CRITICAL = 4
};

class Logger {
public:
	static Logger& getInstance();

	// Initialize logger with file path
	void init(const std::string& logFilePath, LogLevel minLevel = LogLevel::INFO);

	// Log methods
	void debug(const std::string& message);
	void info(const std::string& message);
	void warning(const std::string& message);
	void error(const std::string& message);
	void critical(const std::string& message);

	// Generic log with level
	void log(LogLevel level, const std::string& message);

	// Set minimum log level
	void setLogLevel(LogLevel level);

	// Flush logs to disk
	void flush();

	// Close log file
	void close();

	// Delete copy constructor and assignment operator
	Logger(const Logger&) = delete;
	Logger& operator=(const Logger&) = delete;

private:
	Logger();
	~Logger();

	std::string getCurrentTimestamp();
	std::string levelToString(LogLevel level);
	void writeLog(LogLevel level, const std::string& message);

	std::ofstream logFile;
	LogLevel minLogLevel;
	std::mutex logMutex;
	bool initialized;
};

// Convenience macros
#define LOG_DEBUG(msg) Emiglio::Logger::getInstance().debug(msg)
#define LOG_INFO(msg) Emiglio::Logger::getInstance().info(msg)
#define LOG_WARNING(msg) Emiglio::Logger::getInstance().warning(msg)
#define LOG_ERROR(msg) Emiglio::Logger::getInstance().error(msg)
#define LOG_CRITICAL(msg) Emiglio::Logger::getInstance().critical(msg)

} // namespace Emiglio

#endif // LOGGER_H
