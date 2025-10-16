#include "JsonParser.h"
#include "Logger.h"
#include <fstream>
#include <sstream>

// RapidJSON includes
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/error/en.h"

using namespace rapidjson;

namespace Emiglio {

// Real RapidJSON implementation
class JsonParser::Impl {
public:
	Document doc;
	std::string errorMsg;
	bool valid;

	Impl() : valid(false) {}

	// Navigate to a value using key path (supports nested keys like "exchange.apiKey")
	const Value* navigate(const std::string& keyPath) const {
		if (!valid) {
			return nullptr;
		}

		// Empty path means root document
		if (keyPath.empty()) {
			return &doc;
		}

		// Root must be object for path navigation
		if (!doc.IsObject()) {
			return nullptr;
		}

		const Value* current = &doc;

		// Split keyPath by '.'
		std::string path = keyPath;
		size_t pos = 0;
		while ((pos = path.find('.')) != std::string::npos) {
			std::string key = path.substr(0, pos);
			if (!current->IsObject() || !current->HasMember(key.c_str())) {
				return nullptr;
			}
			current = &(*current)[key.c_str()];
			path = path.substr(pos + 1);
		}

		// Final key
		if (!current->IsObject() || !current->HasMember(path.c_str())) {
			return nullptr;
		}

		return &(*current)[path.c_str()];
	}
};

JsonParser::JsonParser()
	: pImpl(std::make_unique<Impl>()) {
}

JsonParser::~JsonParser() {
}

bool JsonParser::parse(const std::string& jsonString) {
	pImpl->doc.Parse(jsonString.c_str());

	if (pImpl->doc.HasParseError()) {
		pImpl->errorMsg = GetParseError_En(pImpl->doc.GetParseError());
		pImpl->errorMsg += " (offset: " + std::to_string(pImpl->doc.GetErrorOffset()) + ")";
		pImpl->valid = false;
		LOG_ERROR("Failed to parse JSON: " + pImpl->errorMsg);
		return false;
	}

	pImpl->valid = true;
	LOG_DEBUG("JSON parsed successfully");
	return true;
}

bool JsonParser::parseFile(const std::string& filePath) {
	std::ifstream file(filePath);
	if (!file.is_open()) {
		pImpl->errorMsg = "Failed to open file: " + filePath;
		pImpl->valid = false;
		LOG_ERROR(pImpl->errorMsg);
		return false;
	}

	std::stringstream buffer;
	buffer << file.rdbuf();
	return parse(buffer.str());
}

std::string JsonParser::getString(const std::string& keyPath, const std::string& defaultValue) const {
	const Value* val = pImpl->navigate(keyPath);
	if (val && val->IsString()) {
		return val->GetString();
	}
	return defaultValue;
}

int JsonParser::getInt(const std::string& keyPath, int defaultValue) const {
	const Value* val = pImpl->navigate(keyPath);
	if (val && val->IsInt()) {
		return val->GetInt();
	}
	return defaultValue;
}

int64_t JsonParser::getInt64(const std::string& keyPath, int64_t defaultValue) const {
	const Value* val = pImpl->navigate(keyPath);
	if (val) {
		if (val->IsInt64()) {
			return val->GetInt64();
		} else if (val->IsUint64()) {
			return static_cast<int64_t>(val->GetUint64());
		} else if (val->IsInt()) {
			return static_cast<int64_t>(val->GetInt());
		} else if (val->IsUint()) {
			return static_cast<int64_t>(val->GetUint());
		}
	}
	return defaultValue;
}

double JsonParser::getDouble(const std::string& keyPath, double defaultValue) const {
	const Value* val = pImpl->navigate(keyPath);
	if (val) {
		if (val->IsDouble()) {
			return val->GetDouble();
		} else if (val->IsInt()) {
			return static_cast<double>(val->GetInt());
		} else if (val->IsInt64()) {
			return static_cast<double>(val->GetInt64());
		} else if (val->IsString()) {
			// Try to parse string as double (Binance sometimes returns numbers as strings)
			try {
				return std::stod(val->GetString());
			} catch (...) {
				return defaultValue;
			}
		}
	}
	return defaultValue;
}

bool JsonParser::getBool(const std::string& keyPath, bool defaultValue) const {
	const Value* val = pImpl->navigate(keyPath);
	if (val && val->IsBool()) {
		return val->GetBool();
	}
	return defaultValue;
}

bool JsonParser::has(const std::string& keyPath) const {
	return pImpl->navigate(keyPath) != nullptr;
}

bool JsonParser::isArray(const std::string& keyPath) const {
	const Value* val = pImpl->navigate(keyPath);
	return val && val->IsArray();
}

size_t JsonParser::getArraySize(const std::string& keyPath) const {
	const Value* val = pImpl->navigate(keyPath);
	if (val && val->IsArray()) {
		return val->Size();
	}
	return 0;
}

std::string JsonParser::getArrayString(const std::string& keyPath, size_t index, const std::string& defaultValue) const {
	const Value* val = pImpl->navigate(keyPath);
	if (val && val->IsArray() && index < val->Size()) {
		const Value& element = (*val)[static_cast<rapidjson::SizeType>(index)];
		if (element.IsString()) {
			return element.GetString();
		}
	}
	return defaultValue;
}

int JsonParser::getArrayInt(const std::string& keyPath, size_t index, int defaultValue) const {
	const Value* val = pImpl->navigate(keyPath);
	if (val && val->IsArray() && index < val->Size()) {
		const Value& element = (*val)[static_cast<rapidjson::SizeType>(index)];
		if (element.IsInt()) {
			return element.GetInt();
		}
	}
	return defaultValue;
}

int64_t JsonParser::getArrayInt64(const std::string& keyPath, size_t index, int64_t defaultValue) const {
	const Value* val = pImpl->navigate(keyPath);
	if (val && val->IsArray() && index < val->Size()) {
		const Value& element = (*val)[static_cast<rapidjson::SizeType>(index)];
		if (element.IsInt64()) {
			return element.GetInt64();
		} else if (element.IsUint64()) {
			return static_cast<int64_t>(element.GetUint64());
		} else if (element.IsInt()) {
			return static_cast<int64_t>(element.GetInt());
		}
	}
	return defaultValue;
}

double JsonParser::getArrayDouble(const std::string& keyPath, size_t index, double defaultValue) const {
	const Value* val = pImpl->navigate(keyPath);
	if (val && val->IsArray() && index < val->Size()) {
		const Value& element = (*val)[static_cast<rapidjson::SizeType>(index)];
		if (element.IsDouble()) {
			return element.GetDouble();
		} else if (element.IsInt()) {
			return static_cast<double>(element.GetInt());
		} else if (element.IsInt64()) {
			return static_cast<double>(element.GetInt64());
		} else if (element.IsString()) {
			// Binance often returns numbers as strings
			try {
				return std::stod(element.GetString());
			} catch (...) {
				return defaultValue;
			}
		}
	}
	return defaultValue;
}

size_t JsonParser::getNestedArraySize(const std::string& keyPath, size_t index) const {
	const Value* val = pImpl->navigate(keyPath);
	if (val && val->IsArray() && index < val->Size()) {
		const Value& element = (*val)[static_cast<rapidjson::SizeType>(index)];
		if (element.IsArray()) {
			return element.Size();
		}
	}
	return 0;
}

double JsonParser::getNestedArrayDouble(const std::string& keyPath, size_t outerIndex, size_t innerIndex, double defaultValue) const {
	const Value* val = pImpl->navigate(keyPath);
	if (val && val->IsArray() && outerIndex < val->Size()) {
		const Value& outerElement = (*val)[static_cast<rapidjson::SizeType>(outerIndex)];
		if (outerElement.IsArray() && innerIndex < outerElement.Size()) {
			const Value& innerElement = outerElement[static_cast<rapidjson::SizeType>(innerIndex)];
			if (innerElement.IsDouble()) {
				return innerElement.GetDouble();
			} else if (innerElement.IsInt()) {
				return static_cast<double>(innerElement.GetInt());
			} else if (innerElement.IsInt64()) {
				return static_cast<double>(innerElement.GetInt64());
			} else if (innerElement.IsString()) {
				try {
					return std::stod(innerElement.GetString());
				} catch (...) {
					return defaultValue;
				}
			}
		}
	}
	return defaultValue;
}

int64_t JsonParser::getNestedArrayInt64(const std::string& keyPath, size_t outerIndex, size_t innerIndex, int64_t defaultValue) const {
	const Value* val = pImpl->navigate(keyPath);
	if (val && val->IsArray() && outerIndex < val->Size()) {
		const Value& outerElement = (*val)[static_cast<rapidjson::SizeType>(outerIndex)];
		if (outerElement.IsArray() && innerIndex < outerElement.Size()) {
			const Value& innerElement = outerElement[static_cast<rapidjson::SizeType>(innerIndex)];
			if (innerElement.IsInt64()) {
				return innerElement.GetInt64();
			} else if (innerElement.IsUint64()) {
				return static_cast<int64_t>(innerElement.GetUint64());
			} else if (innerElement.IsInt()) {
				return static_cast<int64_t>(innerElement.GetInt());
			}
		}
	}
	return defaultValue;
}

std::string JsonParser::getNestedArrayString(const std::string& keyPath, size_t outerIndex, size_t innerIndex, const std::string& defaultValue) const {
	const Value* val = pImpl->navigate(keyPath);
	if (val && val->IsArray() && outerIndex < val->Size()) {
		const Value& outerElement = (*val)[static_cast<rapidjson::SizeType>(outerIndex)];
		if (outerElement.IsArray() && innerIndex < outerElement.Size()) {
			const Value& innerElement = outerElement[static_cast<rapidjson::SizeType>(innerIndex)];
			if (innerElement.IsString()) {
				return innerElement.GetString();
			}
		}
	}
	return defaultValue;
}

std::string JsonParser::getArrayObjectString(const std::string& keyPath, size_t index, const std::string& field, const std::string& defaultValue) const {
	const Value* val = pImpl->navigate(keyPath);
	if (val && val->IsArray() && index < val->Size()) {
		const Value& element = (*val)[static_cast<rapidjson::SizeType>(index)];
		if (element.IsObject() && element.HasMember(field.c_str())) {
			const Value& fieldVal = element[field.c_str()];
			if (fieldVal.IsString()) {
				return fieldVal.GetString();
			}
		}
	}
	return defaultValue;
}

double JsonParser::getArrayObjectDouble(const std::string& keyPath, size_t index, const std::string& field, double defaultValue) const {
	const Value* val = pImpl->navigate(keyPath);
	if (val && val->IsArray() && index < val->Size()) {
		const Value& element = (*val)[static_cast<rapidjson::SizeType>(index)];
		if (element.IsObject() && element.HasMember(field.c_str())) {
			const Value& fieldVal = element[field.c_str()];
			if (fieldVal.IsDouble()) {
				return fieldVal.GetDouble();
			} else if (fieldVal.IsInt()) {
				return static_cast<double>(fieldVal.GetInt());
			} else if (fieldVal.IsInt64()) {
				return static_cast<double>(fieldVal.GetInt64());
			} else if (fieldVal.IsString()) {
				try {
					return std::stod(fieldVal.GetString());
				} catch (...) {
					return defaultValue;
				}
			}
		}
	}
	return defaultValue;
}

int64_t JsonParser::getArrayObjectInt64(const std::string& keyPath, size_t index, const std::string& field, int64_t defaultValue) const {
	const Value* val = pImpl->navigate(keyPath);
	if (val && val->IsArray() && index < val->Size()) {
		const Value& element = (*val)[static_cast<rapidjson::SizeType>(index)];
		if (element.IsObject() && element.HasMember(field.c_str())) {
			const Value& fieldVal = element[field.c_str()];
			if (fieldVal.IsInt64()) {
				return fieldVal.GetInt64();
			} else if (fieldVal.IsUint64()) {
				return static_cast<int64_t>(fieldVal.GetUint64());
			} else if (fieldVal.IsInt()) {
				return static_cast<int64_t>(fieldVal.GetInt());
			}
		}
	}
	return defaultValue;
}

std::string JsonParser::getError() const {
	return pImpl->errorMsg;
}

std::string JsonParser::toString(bool pretty) const {
	if (!pImpl->valid) {
		return "{}";
	}

	StringBuffer buffer;
	if (pretty) {
		PrettyWriter<StringBuffer> writer(buffer);
		pImpl->doc.Accept(writer);
	} else {
		Writer<StringBuffer> writer(buffer);
		pImpl->doc.Accept(writer);
	}

	return buffer.GetString();
}

} // namespace Emiglio
