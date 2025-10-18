#include "CredentialManager.h"
#include "Logger.h"
#include <sqlite3.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <cstring>
#include <vector>
#include <sstream>
#include <iomanip>
#include <unistd.h>  // For gethostname()

namespace Emiglio {

// Implementation details
class CredentialManager::Impl {
public:
	sqlite3* db;
	std::string lastError;
	bool initialized;

	// Encryption key derived from machine-specific data
	unsigned char encryptionKey[32]; // 256 bits for AES-256
	bool keyInitialized;

	Impl() : db(nullptr), initialized(false), keyInitialized(false) {
		memset(encryptionKey, 0, sizeof(encryptionKey));
	}

	~Impl() {
		if (db) {
			sqlite3_close(db);
		}
		// Clear sensitive data
		memset(encryptionKey, 0, sizeof(encryptionKey));
	}

	// Generate encryption key from machine-specific data
	bool initializeEncryptionKey() {
		if (keyInitialized) {
			return true;
		}

		// In a production system, this would use:
		// - Hardware UUID/serial number
		// - TPM (Trusted Platform Module) if available
		// - User password hash
		// For now, we'll use a combination of database path and a fixed salt
		// This provides basic obfuscation, not military-grade security

		const char* salt = "Emiglio-Trading-Bot-v1.0-Salt-2025";
		std::string keyMaterial = salt;

		// Add some system-specific data (hostname would be better but not portable)
		char hostname[256];
		if (gethostname(hostname, sizeof(hostname)) == 0) {
			keyMaterial += hostname;
		}

		// Derive key using SHA-256
		SHA256(reinterpret_cast<const unsigned char*>(keyMaterial.c_str()),
		       keyMaterial.length(),
		       encryptionKey);

		keyInitialized = true;
		return true;
	}

	// Encrypt data using AES-256-CBC
	bool encrypt(const std::string& plaintext, std::string& ciphertext, std::string& iv) {
		if (!keyInitialized) {
			lastError = "Encryption key not initialized";
			return false;
		}

		// Generate random IV (Initialization Vector)
		unsigned char ivBytes[EVP_MAX_IV_LENGTH];
		if (!RAND_bytes(ivBytes, EVP_MAX_IV_LENGTH)) {
			lastError = "Failed to generate IV";
			return false;
		}

		// Convert IV to hex string for storage
		std::stringstream ss;
		for (int i = 0; i < EVP_MAX_IV_LENGTH; i++) {
			ss << std::hex << std::setw(2) << std::setfill('0') << (int)ivBytes[i];
		}
		iv = ss.str();

		// Encrypt
		EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
		if (!ctx) {
			lastError = "Failed to create cipher context";
			return false;
		}

		// Initialize encryption
		if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, encryptionKey, ivBytes) != 1) {
			EVP_CIPHER_CTX_free(ctx);
			lastError = "Failed to initialize encryption";
			return false;
		}

		// Allocate output buffer (input size + block size for padding)
		std::vector<unsigned char> ciphertextBytes(plaintext.length() + EVP_CIPHER_block_size(EVP_aes_256_cbc()));
		int len = 0;
		int ciphertextLen = 0;

		// Encrypt
		if (EVP_EncryptUpdate(ctx, ciphertextBytes.data(), &len,
		                     reinterpret_cast<const unsigned char*>(plaintext.c_str()),
		                     plaintext.length()) != 1) {
			EVP_CIPHER_CTX_free(ctx);
			lastError = "Encryption failed";
			return false;
		}
		ciphertextLen = len;

		// Finalize
		if (EVP_EncryptFinal_ex(ctx, ciphertextBytes.data() + len, &len) != 1) {
			EVP_CIPHER_CTX_free(ctx);
			lastError = "Encryption finalization failed";
			return false;
		}
		ciphertextLen += len;

		EVP_CIPHER_CTX_free(ctx);

		// Convert to hex string for storage
		std::stringstream cipherSs;
		for (int i = 0; i < ciphertextLen; i++) {
			cipherSs << std::hex << std::setw(2) << std::setfill('0') << (int)ciphertextBytes[i];
		}
		ciphertext = cipherSs.str();

		return true;
	}

	// Decrypt data using AES-256-CBC
	bool decrypt(const std::string& ciphertext, const std::string& iv, std::string& plaintext) {
		if (!keyInitialized) {
			lastError = "Encryption key not initialized";
			return false;
		}

		// Convert hex IV to bytes
		if (iv.length() != EVP_MAX_IV_LENGTH * 2) {
			lastError = "Invalid IV length";
			return false;
		}

		unsigned char ivBytes[EVP_MAX_IV_LENGTH];
		for (int i = 0; i < EVP_MAX_IV_LENGTH; i++) {
			std::string byteString = iv.substr(i * 2, 2);
			ivBytes[i] = (unsigned char)strtol(byteString.c_str(), nullptr, 16);
		}

		// Convert hex ciphertext to bytes
		if (ciphertext.length() % 2 != 0) {
			lastError = "Invalid ciphertext length";
			return false;
		}

		std::vector<unsigned char> ciphertextBytes;
		for (size_t i = 0; i < ciphertext.length(); i += 2) {
			std::string byteString = ciphertext.substr(i, 2);
			ciphertextBytes.push_back((unsigned char)strtol(byteString.c_str(), nullptr, 16));
		}

		// Decrypt
		EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
		if (!ctx) {
			lastError = "Failed to create cipher context";
			return false;
		}

		// Initialize decryption
		if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, encryptionKey, ivBytes) != 1) {
			EVP_CIPHER_CTX_free(ctx);
			lastError = "Failed to initialize decryption";
			return false;
		}

		// Allocate output buffer
		std::vector<unsigned char> plaintextBytes(ciphertextBytes.size() + EVP_CIPHER_block_size(EVP_aes_256_cbc()));
		int len = 0;
		int plaintextLen = 0;

		// Decrypt
		if (EVP_DecryptUpdate(ctx, plaintextBytes.data(), &len,
		                     ciphertextBytes.data(), ciphertextBytes.size()) != 1) {
			EVP_CIPHER_CTX_free(ctx);
			lastError = "Decryption failed";
			return false;
		}
		plaintextLen = len;

		// Finalize
		if (EVP_DecryptFinal_ex(ctx, plaintextBytes.data() + len, &len) != 1) {
			EVP_CIPHER_CTX_free(ctx);
			lastError = "Decryption finalization failed";
			return false;
		}
		plaintextLen += len;

		EVP_CIPHER_CTX_free(ctx);

		// Convert to string
		plaintext = std::string(reinterpret_cast<char*>(plaintextBytes.data()), plaintextLen);

		return true;
	}

	// Create credentials table if it doesn't exist
	bool createTable() {
		const char* sql = R"(
			CREATE TABLE IF NOT EXISTS credentials (
				exchange TEXT PRIMARY KEY,
				api_key_encrypted TEXT NOT NULL,
				api_key_iv TEXT NOT NULL,
				api_secret_encrypted TEXT NOT NULL,
				api_secret_iv TEXT NOT NULL,
				created_at INTEGER NOT NULL,
				updated_at INTEGER NOT NULL
			);
		)";

		char* errMsg = nullptr;
		int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
		if (rc != SQLITE_OK) {
			lastError = "Failed to create credentials table: ";
			lastError += errMsg ? errMsg : "Unknown error";
			sqlite3_free(errMsg);
			return false;
		}

		return true;
	}
};

CredentialManager::CredentialManager()
	: pImpl(std::make_unique<Impl>()) {
}

CredentialManager::~CredentialManager() {
}

bool CredentialManager::init(const std::string& dbPath) {
	if (pImpl->initialized) {
		LOG_WARNING("CredentialManager already initialized");
		return true;
	}

	// Open database
	int rc = sqlite3_open(dbPath.c_str(), &pImpl->db);
	if (rc != SQLITE_OK) {
		pImpl->lastError = "Failed to open database: ";
		pImpl->lastError += sqlite3_errmsg(pImpl->db);
		LOG_ERROR(pImpl->lastError);
		return false;
	}

	// Initialize encryption key
	if (!pImpl->initializeEncryptionKey()) {
		LOG_ERROR("Failed to initialize encryption key");
		return false;
	}

	// Create table
	if (!pImpl->createTable()) {
		LOG_ERROR(pImpl->lastError);
		return false;
	}

	pImpl->initialized = true;
	LOG_INFO("CredentialManager initialized successfully");
	return true;
}

bool CredentialManager::saveCredentials(const std::string& exchange,
                                       const std::string& apiKey,
                                       const std::string& apiSecret) {
	if (!pImpl->initialized) {
		pImpl->lastError = "CredentialManager not initialized";
		return false;
	}

	// Encrypt API key
	std::string encryptedKey, keyIv;
	if (!pImpl->encrypt(apiKey, encryptedKey, keyIv)) {
		LOG_ERROR("Failed to encrypt API key: " + pImpl->lastError);
		return false;
	}

	// Encrypt API secret
	std::string encryptedSecret, secretIv;
	if (!pImpl->encrypt(apiSecret, encryptedSecret, secretIv)) {
		LOG_ERROR("Failed to encrypt API secret: " + pImpl->lastError);
		return false;
	}

	// Store in database
	const char* sql = R"(
		INSERT OR REPLACE INTO credentials
		(exchange, api_key_encrypted, api_key_iv, api_secret_encrypted, api_secret_iv, created_at, updated_at)
		VALUES (?, ?, ?, ?, ?, ?, ?);
	)";

	sqlite3_stmt* stmt;
	int rc = sqlite3_prepare_v2(pImpl->db, sql, -1, &stmt, nullptr);
	if (rc != SQLITE_OK) {
		pImpl->lastError = "Failed to prepare statement: ";
		pImpl->lastError += sqlite3_errmsg(pImpl->db);
		LOG_ERROR(pImpl->lastError);
		return false;
	}

	time_t now = time(nullptr);
	sqlite3_bind_text(stmt, 1, exchange.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, encryptedKey.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 3, keyIv.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 4, encryptedSecret.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 5, secretIv.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int64(stmt, 6, now);
	sqlite3_bind_int64(stmt, 7, now);

	rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	if (rc != SQLITE_DONE) {
		pImpl->lastError = "Failed to save credentials: ";
		pImpl->lastError += sqlite3_errmsg(pImpl->db);
		LOG_ERROR(pImpl->lastError);
		return false;
	}

	LOG_INFO("Credentials saved successfully for exchange: " + exchange);
	return true;
}

bool CredentialManager::loadCredentials(const std::string& exchange,
                                       std::string& apiKey,
                                       std::string& apiSecret) {
	if (!pImpl->initialized) {
		pImpl->lastError = "CredentialManager not initialized";
		return false;
	}

	const char* sql = "SELECT api_key_encrypted, api_key_iv, api_secret_encrypted, api_secret_iv FROM credentials WHERE exchange = ?;";

	sqlite3_stmt* stmt;
	int rc = sqlite3_prepare_v2(pImpl->db, sql, -1, &stmt, nullptr);
	if (rc != SQLITE_OK) {
		pImpl->lastError = "Failed to prepare statement: ";
		pImpl->lastError += sqlite3_errmsg(pImpl->db);
		LOG_ERROR(pImpl->lastError);
		return false;
	}

	sqlite3_bind_text(stmt, 1, exchange.c_str(), -1, SQLITE_TRANSIENT);

	rc = sqlite3_step(stmt);
	if (rc == SQLITE_ROW) {
		// Get encrypted data
		std::string encryptedKey = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
		std::string keyIv = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
		std::string encryptedSecret = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
		std::string secretIv = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));

		sqlite3_finalize(stmt);

		// Decrypt API key
		if (!pImpl->decrypt(encryptedKey, keyIv, apiKey)) {
			LOG_ERROR("Failed to decrypt API key: " + pImpl->lastError);
			return false;
		}

		// Decrypt API secret
		if (!pImpl->decrypt(encryptedSecret, secretIv, apiSecret)) {
			LOG_ERROR("Failed to decrypt API secret: " + pImpl->lastError);
			return false;
		}

		LOG_INFO("Credentials loaded successfully for exchange: " + exchange);
		return true;
	} else {
		sqlite3_finalize(stmt);
		pImpl->lastError = "No credentials found for exchange: " + exchange;
		return false;
	}
}

bool CredentialManager::hasCredentials(const std::string& exchange) {
	if (!pImpl->initialized) {
		return false;
	}

	const char* sql = "SELECT COUNT(*) FROM credentials WHERE exchange = ?;";

	sqlite3_stmt* stmt;
	int rc = sqlite3_prepare_v2(pImpl->db, sql, -1, &stmt, nullptr);
	if (rc != SQLITE_OK) {
		return false;
	}

	sqlite3_bind_text(stmt, 1, exchange.c_str(), -1, SQLITE_TRANSIENT);

	rc = sqlite3_step(stmt);
	bool hasKeys = false;
	if (rc == SQLITE_ROW) {
		int count = sqlite3_column_int(stmt, 0);
		hasKeys = (count > 0);
	}

	sqlite3_finalize(stmt);
	return hasKeys;
}

bool CredentialManager::deleteCredentials(const std::string& exchange) {
	if (!pImpl->initialized) {
		pImpl->lastError = "CredentialManager not initialized";
		return false;
	}

	const char* sql = "DELETE FROM credentials WHERE exchange = ?;";

	sqlite3_stmt* stmt;
	int rc = sqlite3_prepare_v2(pImpl->db, sql, -1, &stmt, nullptr);
	if (rc != SQLITE_OK) {
		pImpl->lastError = "Failed to prepare statement: ";
		pImpl->lastError += sqlite3_errmsg(pImpl->db);
		return false;
	}

	sqlite3_bind_text(stmt, 1, exchange.c_str(), -1, SQLITE_TRANSIENT);

	rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	if (rc != SQLITE_DONE) {
		pImpl->lastError = "Failed to delete credentials: ";
		pImpl->lastError += sqlite3_errmsg(pImpl->db);
		LOG_ERROR(pImpl->lastError);
		return false;
	}

	LOG_INFO("Credentials deleted for exchange: " + exchange);
	return true;
}

std::string CredentialManager::getLastError() const {
	return pImpl->lastError;
}

} // namespace Emiglio
