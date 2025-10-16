#ifndef JSONPARSER_H
#define JSONPARSER_H

#include <string>
#include <memory>
#include <cstdint>

namespace Emiglio {

// Wrapper around RapidJSON for easier usage
class JsonParser {
public:
	JsonParser();
	~JsonParser();

	// Parse JSON from string
	bool parse(const std::string& jsonString);

	// Parse JSON from file
	bool parseFile(const std::string& filePath);

	// Get string value by key path (e.g., "exchange.apiKey")
	std::string getString(const std::string& keyPath, const std::string& defaultValue = "") const;

	// Get integer value
	int getInt(const std::string& keyPath, int defaultValue = 0) const;

	// Get int64 value (for large numbers like timestamps)
	int64_t getInt64(const std::string& keyPath, int64_t defaultValue = 0) const;

	// Get double value
	double getDouble(const std::string& keyPath, double defaultValue = 0.0) const;

	// Get boolean value
	bool getBool(const std::string& keyPath, bool defaultValue = false) const;

	// Check if key exists
	bool has(const std::string& keyPath) const;

	// Check if key is an array
	bool isArray(const std::string& keyPath) const;

	// Get array size
	size_t getArraySize(const std::string& keyPath) const;

	// Get string from array at index
	std::string getArrayString(const std::string& keyPath, size_t index, const std::string& defaultValue = "") const;

	// Get int from array at index
	int getArrayInt(const std::string& keyPath, size_t index, int defaultValue = 0) const;

	// Get int64 from array at index
	int64_t getArrayInt64(const std::string& keyPath, size_t index, int64_t defaultValue = 0) const;

	// Get double from array at index
	double getArrayDouble(const std::string& keyPath, size_t index, double defaultValue = 0.0) const;

	// Get size of nested array (array within array)
	size_t getNestedArraySize(const std::string& keyPath, size_t index) const;

	// Get double from nested array [outerIndex][innerIndex]
	double getNestedArrayDouble(const std::string& keyPath, size_t outerIndex, size_t innerIndex, double defaultValue = 0.0) const;

	// Get int64 from nested array [outerIndex][innerIndex]
	int64_t getNestedArrayInt64(const std::string& keyPath, size_t outerIndex, size_t innerIndex, int64_t defaultValue = 0) const;

	// Get string from nested array [outerIndex][innerIndex]
	std::string getNestedArrayString(const std::string& keyPath, size_t outerIndex, size_t innerIndex, const std::string& defaultValue = "") const;

	// Get value from object within array [index].field
	std::string getArrayObjectString(const std::string& keyPath, size_t index, const std::string& field, const std::string& defaultValue = "") const;
	double getArrayObjectDouble(const std::string& keyPath, size_t index, const std::string& field, double defaultValue = 0.0) const;
	int64_t getArrayObjectInt64(const std::string& keyPath, size_t index, const std::string& field, int64_t defaultValue = 0) const;

	// Get error message if parsing failed
	std::string getError() const;

	// Convert to JSON string
	std::string toString(bool pretty = true) const;

private:
	class Impl;
	std::unique_ptr<Impl> pImpl;
};

} // namespace Emiglio

#endif // JSONPARSER_H
