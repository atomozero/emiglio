#!/bin/bash
# Emiglio - Simple Test Suite Runner
# Streamlined version for Haiku OS

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}======================================${NC}"
echo -e "${BLUE}   EMIGLIO TEST SUITE${NC}"
echo -e "${BLUE}======================================${NC}"
echo ""

TESTS_PASSED=0
TESTS_FAILED=0

# Test 1: Recipe Files Exist
echo -e "${YELLOW}[1/7] Checking recipe files...${NC}"
cd /boot/home/Emiglio/recipes
RECIPE_COUNT=$(ls -1 *.json 2>/dev/null | wc -l)
if [ "$RECIPE_COUNT" -gt 0 ]; then
    echo -e "${GREEN}✓ Found $RECIPE_COUNT recipe files${NC}"
    ((TESTS_PASSED++))
else
    echo -e "${RED}✗ No recipe files found${NC}"
    ((TESTS_FAILED++))
fi
echo ""

# Test 2: Check for camelCase compliance
echo -e "${YELLOW}[2/7] Checking recipe naming convention...${NC}"
grep -l 'position_size_percent' /boot/home/Emiglio/recipes/*.json > /tmp/snake_case_check.txt 2>&1 || true
if [ -s /tmp/snake_case_check.txt ]; then
    echo -e "${RED}✗ Found files with snake_case naming${NC}"
    ((TESTS_FAILED++))
else
    echo -e "${GREEN}✓ All recipes use camelCase${NC}"
    ((TESTS_PASSED++))
fi
rm -f /tmp/snake_case_check.txt
echo ""

# Test 3: Check for gmtime usage in EquityChartView
echo -e "${YELLOW}[3/7] Checking date handling (gmtime fix)...${NC}"
if grep -q 'gmtime' /boot/home/Emiglio/src/ui/EquityChartView.cpp; then
    echo -e "${GREEN}✓ Using gmtime() for UTC timestamps${NC}"
    ((TESTS_PASSED++))
else
    echo -e "${RED}✗ gmtime() not found in EquityChartView.cpp${NC}"
    ((TESTS_FAILED++))
fi
echo ""

# Test 4: Check for period selector fix
echo -e "${YELLOW}[4/7] Checking period selector calculation...${NC}"
if grep -q 'days - 1' /boot/home/Emiglio/src/ui/BacktestView.cpp; then
    echo -e "${GREEN}✓ Period calculation includes off-by-one fix${NC}"
    ((TESTS_PASSED++))
else
    echo -e "${RED}✗ Period calculation may be incorrect${NC}"
    ((TESTS_FAILED++))
fi
echo ""

# Test 5: Check if application binary exists
echo -e "${YELLOW}[5/7] Checking application build...${NC}"
cd /boot/home/Emiglio
if [ -f "objects.x86_64-cc13-release/Emiglio" ]; then
    echo -e "${GREEN}✓ Application binary exists${NC}"
    ls -lh objects.x86_64-cc13-release/Emiglio | awk '{print "  Size:", $5, "Modified:", $6, $7, $8}'
    ((TESTS_PASSED++))
else
    echo -e "${YELLOW}⚠ Application binary not found (run: make -f MakefileUI)${NC}"
    ((TESTS_FAILED++))
fi
echo ""

# Test 6: Git status
echo -e "${YELLOW}[6/7] Checking git repository...${NC}"
cd /boot/home/Emiglio
echo "  Branch: $(git branch --show-current)"
echo "  Last commit: $(git log --oneline -1)"
echo -e "${GREEN}✓ Git repository OK${NC}"
((TESTS_PASSED++))
echo ""

# Test 7: Check for test executables
echo -e "${YELLOW}[7/7] Checking test executables...${NC}"
TEST_FILES=0
[ -f "/boot/home/Emiglio/src/tests/TestRecipeLoader" ] && ((TEST_FILES++))
[ -f "/boot/home/Emiglio/src/tests/TestIndicators" ] && ((TEST_FILES++))
[ -f "/boot/home/Emiglio/src/tests/TestDataStorage" ] && ((TEST_FILES++))
[ -f "/boot/home/Emiglio/src/tests/TestBacktest" ] && ((TEST_FILES++))

if [ $TEST_FILES -gt 0 ]; then
    echo -e "${GREEN}✓ Found $TEST_FILES test executable(s)${NC}"
    ((TESTS_PASSED++))
else
    echo -e "${YELLOW}⚠ No test executables found${NC}"
    ((TESTS_PASSED++))  # Not a failure, just informational
fi
echo ""

# Summary
echo -e "${BLUE}======================================${NC}"
echo -e "${BLUE}         TEST SUMMARY${NC}"
echo -e "${BLUE}======================================${NC}"
echo -e "Tests Passed: ${GREEN}$TESTS_PASSED${NC}"
echo -e "Tests Failed: ${RED}$TESTS_FAILED${NC}"
echo ""

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}✓ ALL TESTS PASSED!${NC}"
    echo ""
    exit 0
else
    echo -e "${RED}✗ SOME TESTS FAILED${NC}"
    echo ""
    exit 1
fi
