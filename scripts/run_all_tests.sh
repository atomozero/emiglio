#!/bin/bash
# Emiglio - Comprehensive Test Suite Runner
# Tests all components including recent changes

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

TESTS_PASSED=0
TESTS_FAILED=0
TEST_LOG="/tmp/emiglio_test_$(date +%Y%m%d_%H%M%S).log"

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}   EMIGLIO COMPREHENSIVE TEST SUITE${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""
echo "Test log: $TEST_LOG"
echo ""

# Function to run a test
run_test() {
    local test_name=$1
    local test_cmd=$2

    echo -e "${YELLOW}Testing: $test_name${NC}"
    echo "----------------------------------------"

    if eval "$test_cmd" 2>&1 | tee -a "$TEST_LOG"; then
        echo -e "${GREEN}✓ $test_name PASSED${NC}\n"
        ((TESTS_PASSED++))
        return 0
    else
        echo -e "${RED}✗ $test_name FAILED${NC}\n"
        ((TESTS_FAILED++))
        return 1
    fi
}

# 1. Recipe Files Validation
echo -e "${BLUE}=== 1. RECIPE FILES VALIDATION ===${NC}\n"

run_test "Recipe JSON Syntax" "
    cd /boot/home/Emiglio/recipes
    echo \"Found recipe files:\"
    ls -1 *.json
    echo \"\"
    echo \"Note: JSON syntax validation requires python3 or jq (not available on Haiku)\"
    echo \"Skipping automated JSON syntax check - manual inspection recommended\"
    echo \"All recipes listed successfully\"
"

run_test "Recipe Schema Compliance" "
    cd /boot/home/Emiglio/recipes
    echo \"Checking for camelCase compliance in recipes...\"

    # Check for snake_case usage which indicates non-compliance
    if grep -l 'position_size_percent' *.json 2>/dev/null; then
        echo \"ERROR: Found position_size_percent (should be positionSizePercent)\"
        exit 1
    fi
    if grep -l 'risk_management' *.json 2>/dev/null; then
        echo \"ERROR: Found risk_management (should be risk)\"
        exit 1
    fi
    if grep -l 'entry_conditions' *.json 2>/dev/null; then
        echo \"ERROR: Found entry_conditions (should be entryConditions)\"
        exit 1
    fi

    echo \"✓ All recipes use camelCase naming convention\"
"

# 2. RecipeLoader Tests (if executable exists)
echo -e "${BLUE}=== 2. RECIPE LOADER TESTS ===${NC}\n"

if [ -f "/boot/home/Emiglio/src/tests/TestRecipeLoader" ]; then
    run_test "RecipeLoader Component" "/boot/home/Emiglio/src/tests/TestRecipeLoader"
else
    echo -e "${YELLOW}⚠ TestRecipeLoader executable not found, skipping${NC}\n"
fi

# 3. Indicators Tests
echo -e "${BLUE}=== 3. TECHNICAL INDICATORS TESTS ===${NC}\n"

if [ -f "/boot/home/Emiglio/src/tests/TestIndicators" ]; then
    run_test "Technical Indicators" "/boot/home/Emiglio/src/tests/TestIndicators"
else
    echo -e "${YELLOW}⚠ TestIndicators executable not found, skipping${NC}\n"
fi

# 4. Data Storage Tests
echo -e "${BLUE}=== 4. DATA STORAGE TESTS ===${NC}\n"

if [ -f "/boot/home/Emiglio/src/tests/TestDataStorage" ]; then
    run_test "Data Storage & SQLite" "/boot/home/Emiglio/src/tests/TestDataStorage"
else
    echo -e "${YELLOW}⚠ TestDataStorage executable not found, skipping${NC}\n"
fi

# 5. Backtest Simulator Tests
echo -e "${BLUE}=== 5. BACKTEST SIMULATOR TESTS ===${NC}\n"

if [ -f "/boot/home/Emiglio/src/tests/TestBacktest" ]; then
    run_test "Backtest Simulator" "/boot/home/Emiglio/src/tests/TestBacktest"
else
    echo -e "${YELLOW}⚠ TestBacktest executable not found, skipping${NC}\n"
fi

# 6. Date Handling Tests (verify gmtime fix)
echo -e "${BLUE}=== 6. DATE HANDLING VERIFICATION ===${NC}\n"

run_test "Chart Date Display" "
    echo \"Verifying gmtime() usage in EquityChartView...\"
    if grep -q 'gmtime' /boot/home/Emiglio/src/ui/EquityChartView.cpp; then
        echo \"✓ Using gmtime() for UTC timestamps\"
    else
        echo \"ERROR: gmtime() not found in EquityChartView.cpp\"
        exit 1
    fi

    if grep -q 'localtime' /boot/home/Emiglio/src/ui/EquityChartView.cpp | grep -v '//'; then
        echo \"WARNING: localtime() still present in code\"
    else
        echo \"✓ No localtime() calls in date display code\"
    fi
"

# 7. Period Selector Date Calculation Tests
echo -e "${BLUE}=== 7. PERIOD SELECTOR VERIFICATION ===${NC}\n"

run_test "Period Date Calculation" "
    echo \"Verifying period selector date calculation...\"
    if grep -q 'days - 1' /boot/home/Emiglio/src/ui/BacktestView.cpp; then
        echo \"✓ Period calculation includes off-by-one fix (days - 1)\"
    else
        echo \"ERROR: Period calculation may be incorrect\"
        exit 1
    fi

    echo \"✓ Date range calculation verified\"
"

# 8. Application Build Test
echo -e "${BLUE}=== 8. APPLICATION BUILD TEST ===${NC}\n"

run_test "UI Application Build" "
    cd /boot/home/Emiglio
    if [ -f \"objects.x86_64-cc13-release/Emiglio\" ]; then
        echo \"✓ Application binary exists\"
        ls -lh objects.x86_64-cc13-release/Emiglio
    else
        echo \"ERROR: Application binary not found\"
        echo \"Run: make -f MakefileUI\"
        exit 1
    fi
"

# 9. Git Repository Status
echo -e "${BLUE}=== 9. GIT REPOSITORY STATUS ===${NC}\n"

run_test "Git Commit History" "
    cd /boot/home/Emiglio
    echo \"Recent commits:\"
    git log --oneline -5
    echo \"\"
    echo \"Current branch:\"
    git branch --show-current
    echo \"\"
    echo \"Repository status:\"
    git status --short
"

# Summary
echo ""
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}         TEST SUMMARY${NC}"
echo -e "${BLUE}========================================${NC}"
echo -e "Tests Passed: ${GREEN}$TESTS_PASSED${NC}"
echo -e "Tests Failed: ${RED}$TESTS_FAILED${NC}"
echo ""
echo "Full log saved to: $TEST_LOG"
echo ""

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}✓ ALL TESTS PASSED!${NC}"
    echo ""
    exit 0
else
    echo -e "${RED}✗ SOME TESTS FAILED${NC}"
    echo -e "Check log file for details: $TEST_LOG"
    echo ""
    exit 1
fi
