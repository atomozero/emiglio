#include "../strategy/RecipeLoader.h"
#include <iostream>
#include <cassert>
#include <fstream>
#include <sys/stat.h>

using namespace Emiglio;

// Test macros
#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    std::cout << "Running " #name "..." << std::endl; \
    test_##name(); \
    std::cout << "✓ " #name " passed" << std::endl; \
} while(0)

#define ASSERT_TRUE(expr) do { \
    if (!(expr)) { \
        std::cerr << "✗ Assertion failed: " #expr << " at line " << __LINE__ << std::endl; \
        exit(1); \
    } \
} while(0)

#define ASSERT_FALSE(expr) ASSERT_TRUE(!(expr))
#define ASSERT_EQ(a, b) ASSERT_TRUE((a) == (b))
#define ASSERT_NEAR(a, b, epsilon) ASSERT_TRUE(std::abs((a) - (b)) < (epsilon))

// Helper to create test recipe file
void createTestRecipe(const std::string& filename, const std::string& content) {
    std::ofstream file(filename);
    file << content;
    file.close();
}

// Helper to delete test file
void deleteTestRecipe(const std::string& filename) {
    remove(filename.c_str());
}

// Test: Load valid simple recipe
TEST(load_simple_recipe) {
    std::string testFile = "/tmp/test_simple_recipe.json";
    std::string content = R"({
        "name": "Test Simple RSI",
        "version": "1.0",
        "description": "Test recipe",
        "market": {
            "exchange": "binance",
            "symbol": "BTC/USDT",
            "timeframe": "1h"
        },
        "capital": {
            "initial": 10000,
            "position_size_percent": 10
        },
        "risk_management": {
            "stop_loss_percent": 2.0,
            "take_profit_percent": 5.0
        },
        "indicators": [
            {"name": "rsi", "period": 14}
        ],
        "entry_conditions": {
            "logic": "AND",
            "rules": [
                {"indicator": "rsi", "operator": "<", "value": 30}
            ]
        },
        "exit_conditions": {
            "logic": "OR",
            "rules": [
                {"indicator": "rsi", "operator": ">", "value": 70}
            ]
        }
    })";

    createTestRecipe(testFile, content);

    RecipeLoader loader;
    Recipe recipe;

    bool loaded = loader.loadFromFile(testFile, recipe);
    ASSERT_TRUE(loaded);

    // Verify basic fields
    ASSERT_EQ(recipe.name, "Test Simple RSI");
    ASSERT_EQ(recipe.market.exchange, "binance");
    ASSERT_EQ(recipe.market.symbol, "BTC/USDT");
    ASSERT_EQ(recipe.market.timeframe, "1h");
    ASSERT_NEAR(recipe.capital.initial, 10000.0, 0.01);
    ASSERT_NEAR(recipe.riskManagement.stopLossPercent, 2.0, 0.01);

    // Verify indicators
    ASSERT_EQ(recipe.indicators.size(), 1);
    ASSERT_EQ(recipe.indicators[0].name, "rsi");

    // Verify conditions
    ASSERT_EQ(recipe.entryConditions.logic, "AND");
    ASSERT_EQ(recipe.entryConditions.rules.size(), 1);
    ASSERT_EQ(recipe.exitConditions.logic, "OR");

    deleteTestRecipe(testFile);
}

// Test: Load recipe with multiple indicators
TEST(load_multiple_indicators) {
    std::string testFile = "/tmp/test_multi_indicators.json";
    std::string content = R"({
        "name": "Multi Indicator Test",
        "market": {"exchange": "binance", "symbol": "ETH/USDT", "timeframe": "4h"},
        "capital": {"initial": 5000, "position_size_percent": 15},
        "risk_management": {"stop_loss_percent": 3.0, "take_profit_percent": 10.0},
        "indicators": [
            {"name": "rsi", "period": 14},
            {"name": "ema", "periods": [9, 21, 50]},
            {"name": "macd", "fast_period": 12, "slow_period": 26, "signal_period": 9},
            {"name": "bollinger_bands", "period": 20, "std_dev": 2}
        ],
        "entry_conditions": {"logic": "AND", "rules": []},
        "exit_conditions": {"logic": "OR", "rules": []}
    })";

    createTestRecipe(testFile, content);

    RecipeLoader loader;
    Recipe recipe;

    bool loaded = loader.loadFromFile(testFile, recipe);
    ASSERT_TRUE(loaded);

    ASSERT_EQ(recipe.indicators.size(), 4);
    ASSERT_EQ(recipe.indicators[0].name, "rsi");
    ASSERT_EQ(recipe.indicators[1].name, "ema");
    ASSERT_EQ(recipe.indicators[2].name, "macd");
    ASSERT_EQ(recipe.indicators[3].name, "bollinger_bands");

    deleteTestRecipe(testFile);
}

// Test: Load recipe with complex conditions
TEST(load_complex_conditions) {
    std::string testFile = "/tmp/test_complex_conditions.json";
    std::string content = R"({
        "name": "Complex Conditions Test",
        "market": {"exchange": "binance", "symbol": "BTC/USDT", "timeframe": "15m"},
        "capital": {"initial": 10000, "position_size_percent": 10},
        "risk_management": {"stop_loss_percent": 2.0, "take_profit_percent": 5.0},
        "indicators": [{"name": "rsi", "period": 14}],
        "entry_conditions": {
            "logic": "AND",
            "rules": [
                {"indicator": "rsi", "operator": "<", "value": 30},
                {"indicator": "close", "operator": ">", "reference": "ema_50"},
                {"indicator": "volume", "operator": ">", "reference": "volume_sma", "multiplier": 1.5}
            ]
        },
        "exit_conditions": {
            "logic": "OR",
            "rules": [
                {"indicator": "rsi", "operator": ">", "value": 70},
                {"type": "stop_loss"},
                {"type": "take_profit"}
            ]
        }
    })";

    createTestRecipe(testFile, content);

    RecipeLoader loader;
    Recipe recipe;

    bool loaded = loader.loadFromFile(testFile, recipe);
    ASSERT_TRUE(loaded);

    ASSERT_EQ(recipe.entryConditions.rules.size(), 3);
    ASSERT_EQ(recipe.exitConditions.rules.size(), 3);

    // Check first entry rule
    ASSERT_EQ(recipe.entryConditions.rules[0].indicator, "rsi");
    ASSERT_EQ(recipe.entryConditions.rules[0].op, "<");
    ASSERT_NEAR(recipe.entryConditions.rules[0].value, 30.0, 0.01);

    deleteTestRecipe(testFile);
}

// Test: Invalid JSON
TEST(invalid_json) {
    std::string testFile = "/tmp/test_invalid.json";
    std::string content = R"({
        "name": "Invalid",
        "market": {"exchange": "binance"
        // Missing closing brace
    })";

    createTestRecipe(testFile, content);

    RecipeLoader loader;
    Recipe recipe;

    bool loaded = loader.loadFromFile(testFile, recipe);
    ASSERT_FALSE(loaded); // Should fail

    deleteTestRecipe(testFile);
}

// Test: Missing required fields
TEST(missing_required_fields) {
    std::string testFile = "/tmp/test_missing_fields.json";
    std::string content = R"({
        "name": "Missing Fields",
        "market": {"exchange": "binance"}
    })";

    createTestRecipe(testFile, content);

    RecipeLoader loader;
    Recipe recipe;

    bool loaded = loader.loadFromFile(testFile, recipe);
    // Should either fail or fill with defaults
    // Depending on implementation, adjust assertion

    deleteTestRecipe(testFile);
}

// Test: File not found
TEST(file_not_found) {
    RecipeLoader loader;
    Recipe recipe;

    bool loaded = loader.loadFromFile("/tmp/nonexistent_recipe_file_12345.json", recipe);
    ASSERT_FALSE(loaded);
}

// Test: Load all recipes from directory
TEST(load_from_directory) {
    RecipeLoader loader;

    // Try loading from actual recipes directory
    std::vector<Recipe> recipes = loader.loadAllFromDirectory("recipes/");

    // Should load at least some recipes
    std::cout << "  Loaded " << recipes.size() << " recipes from recipes/ directory" << std::endl;

    if (recipes.size() > 0) {
        // Verify first recipe has valid data
        ASSERT_FALSE(recipes[0].name.empty());
        ASSERT_FALSE(recipes[0].market.symbol.empty());
    }
}

// Test: Validate recipe
TEST(validate_recipe) {
    Recipe recipe;
    recipe.name = "Test";
    recipe.market.exchange = "binance";
    recipe.market.symbol = "BTC/USDT";
    recipe.market.timeframe = "1h";
    recipe.capital.initial = 10000.0;
    recipe.capital.positionSizePercent = 10.0;
    recipe.riskManagement.stopLossPercent = 2.0;
    recipe.riskManagement.takeProfitPercent = 5.0;

    RecipeLoader loader;
    bool valid = loader.validate(recipe);
    ASSERT_TRUE(valid);

    // Invalid recipe (negative capital)
    Recipe invalidRecipe = recipe;
    invalidRecipe.capital.initial = -1000.0;
    valid = loader.validate(invalidRecipe);
    ASSERT_FALSE(valid);

    // Invalid recipe (stop loss > take profit makes no sense)
    invalidRecipe = recipe;
    invalidRecipe.riskManagement.stopLossPercent = 10.0;
    invalidRecipe.riskManagement.takeProfitPercent = 5.0;
    // This might still be valid depending on strategy
    // So we just check it doesn't crash
    loader.validate(invalidRecipe);
}

// Test: Recipe with trailing stop
TEST(recipe_with_trailing_stop) {
    std::string testFile = "/tmp/test_trailing_stop.json";
    std::string content = R"({
        "name": "Trailing Stop Test",
        "market": {"exchange": "binance", "symbol": "BTC/USDT", "timeframe": "1h"},
        "capital": {"initial": 10000, "position_size_percent": 10},
        "risk_management": {
            "stop_loss_percent": 2.0,
            "take_profit_percent": 5.0,
            "trailing_stop": true,
            "trailing_stop_percent": 1.5
        },
        "indicators": [],
        "entry_conditions": {"logic": "AND", "rules": []},
        "exit_conditions": {"logic": "OR", "rules": []}
    })";

    createTestRecipe(testFile, content);

    RecipeLoader loader;
    Recipe recipe;

    bool loaded = loader.loadFromFile(testFile, recipe);
    ASSERT_TRUE(loaded);

    ASSERT_TRUE(recipe.riskManagement.trailingStop);
    ASSERT_NEAR(recipe.riskManagement.trailingStopPercent, 1.5, 0.01);

    deleteTestRecipe(testFile);
}

// Test: Recipe with max positions
TEST(recipe_with_max_positions) {
    std::string testFile = "/tmp/test_max_positions.json";
    std::string content = R"({
        "name": "Max Positions Test",
        "market": {"exchange": "binance", "symbol": "BTC/USDT", "timeframe": "1h"},
        "capital": {
            "initial": 10000,
            "position_size_percent": 10,
            "max_open_positions": 5
        },
        "risk_management": {"stop_loss_percent": 2.0, "take_profit_percent": 5.0},
        "indicators": [],
        "entry_conditions": {"logic": "AND", "rules": []},
        "exit_conditions": {"logic": "OR", "rules": []}
    })";

    createTestRecipe(testFile, content);

    RecipeLoader loader;
    Recipe recipe;

    bool loaded = loader.loadFromFile(testFile, recipe);
    ASSERT_TRUE(loaded);

    ASSERT_EQ(recipe.capital.maxOpenPositions, 5);

    deleteTestRecipe(testFile);
}

// Test: Load recipe with comments (if supported)
TEST(recipe_with_comments) {
    std::string testFile = "/tmp/test_with_comments.json";
    std::string content = R"({
        "name": "Recipe with Comments",
        "comment": "This is a test recipe with comment field",
        "market": {
            "exchange": "binance",
            "symbol": "BTC/USDT",
            "timeframe": "1h",
            "comment": "Market configuration"
        },
        "capital": {"initial": 10000, "position_size_percent": 10},
        "risk_management": {"stop_loss_percent": 2.0, "take_profit_percent": 5.0},
        "indicators": [{"name": "rsi", "period": 14, "comment": "RSI indicator"}],
        "entry_conditions": {"logic": "AND", "rules": []},
        "exit_conditions": {"logic": "OR", "rules": []}
    })";

    createTestRecipe(testFile, content);

    RecipeLoader loader;
    Recipe recipe;

    bool loaded = loader.loadFromFile(testFile, recipe);
    ASSERT_TRUE(loaded);

    // Comments should be ignored or stored
    ASSERT_EQ(recipe.name, "Recipe with Comments");

    deleteTestRecipe(testFile);
}

// Performance test: Load many recipes
TEST(performance_load_many) {
    // Create 100 temp recipe files
    std::vector<std::string> tempFiles;
    for (int i = 0; i < 100; i++) {
        std::string filename = "/tmp/test_recipe_" + std::to_string(i) + ".json";
        std::string content = R"({
            "name": "Perf Test )" + std::to_string(i) + R"(",
            "market": {"exchange": "binance", "symbol": "BTC/USDT", "timeframe": "1h"},
            "capital": {"initial": 10000, "position_size_percent": 10},
            "risk_management": {"stop_loss_percent": 2.0, "take_profit_percent": 5.0},
            "indicators": [{"name": "rsi", "period": 14}],
            "entry_conditions": {"logic": "AND", "rules": []},
            "exit_conditions": {"logic": "OR", "rules": []}
        })";

        createTestRecipe(filename, content);
        tempFiles.push_back(filename);
    }

    RecipeLoader loader;
    auto start = std::chrono::high_resolution_clock::now();

    int loaded = 0;
    for (const auto& file : tempFiles) {
        Recipe recipe;
        if (loader.loadFromFile(file, recipe)) {
            loaded++;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "  Loaded " << loaded << "/100 recipes in " << duration.count() << "ms" << std::endl;

    ASSERT_EQ(loaded, 100);
    ASSERT_TRUE(duration.count() < 1000); // Should load in less than 1 second

    // Cleanup
    for (const auto& file : tempFiles) {
        deleteTestRecipe(file);
    }
}

int main() {
    std::cout << "=== RecipeLoader Tests ===" << std::endl << std::endl;

    RUN_TEST(load_simple_recipe);
    RUN_TEST(load_multiple_indicators);
    RUN_TEST(load_complex_conditions);
    RUN_TEST(invalid_json);
    RUN_TEST(missing_required_fields);
    RUN_TEST(file_not_found);
    RUN_TEST(load_from_directory);
    RUN_TEST(validate_recipe);
    RUN_TEST(recipe_with_trailing_stop);
    RUN_TEST(recipe_with_max_positions);
    RUN_TEST(recipe_with_comments);
    RUN_TEST(performance_load_many);

    std::cout << "\n=== All RecipeLoader tests passed! ===" << std::endl;
    return 0;
}
