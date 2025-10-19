#!/bin/bash

# Comprehensive Recipe Comparison Script for Emiglio Trading System
# Compares all recipes on ETH and EUR pairs with 1h and 1d timeframes
# Period: Last 365 days

set -e

RECIPES_DIR="../recipes"
OUTPUT_DIR="../backtest_comparison_$(date +%Y%m%d_%H%M%S)"

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Note: This script creates a comprehensive comparison framework
# The actual backtest execution would require EmilioUI or a backtest engine

echo "================================================================================================="
echo "EMIGLIO RECIPE COMPARISON FRAMEWORK"
echo "================================================================================================="
echo "Output Directory: $OUTPUT_DIR"
echo ""

# Test configurations
declare -A TEST_CONFIGS
TEST_CONFIGS=(
    ["ETHUSDT_1h"]="ETHUSDT|1h|Ethereum hourly"
    ["ETHUSDT_1d"]="ETHUSDT|1d|Ethereum daily"
    ["EURUSDT_1h"]="EURUSDT|1h|Euro/USDT hourly"
    ["EURUSDT_1d"]="EURUSDT|1d|Euro/USDT daily"
    ["ETHEUR_1h"]="ETHEUR|1h|ETH/EUR hourly"
    ["ETHEUR_1d"]="ETHEUR|1d|ETH/EUR daily"
)

# List all recipes
echo "Available recipes:"
RECIPES=()
for recipe_file in "$RECIPES_DIR"/*.json; do
    if [ -f "$recipe_file" ] && [ "$(basename "$recipe_file")" != "README.json" ]; then
        recipe_name=$(basename "$recipe_file" .json)
        RECIPES+=("$recipe_name")
        echo "  - $recipe_name"
    fi
done

echo ""
echo "Total recipes: ${#RECIPES[@]}"
echo "Test configurations: ${#TEST_CONFIGS[@]}"
echo "Total test combinations: $((${#RECIPES[@]} * ${#TEST_CONFIGS[@]}))"
echo ""

# Create recipe analysis summary
ANALYSIS_FILE="$OUTPUT_DIR/recipe_analysis.txt"

{
    echo "EMIGLIO RECIPE ANALYSIS"
    echo "Generated: $(date)"
    echo "================================================================================================="
    echo ""

    for recipe_name in "${RECIPES[@]}"; do
        recipe_file="$RECIPES_DIR/${recipe_name}.json"

        echo "────────────────────────────────────────────────────────────────────────────────────────────"
        echo "RECIPE: $recipe_name"
        echo "────────────────────────────────────────────────────────────────────────────────────────────"

        # Extract key information using python3
        python3 <<PYTHON_END
import json
import sys

try:
    with open('$recipe_file', 'r') as f:
        recipe = json.load(f)

    print(f"Name: {recipe.get('name', 'N/A')}")
    print(f"Description: {recipe.get('description', 'N/A')}")
    print()
    
    market = recipe.get('market', {})
    print(f"Original Symbol: {market.get('symbol', 'N/A')}")
    print(f"Original Timeframe: {market.get('timeframe', 'N/A')}")
    print()
    
    capital = recipe.get('capital', {})
    print(f"Initial Capital: \${capital.get('initial', 'N/A')}")
    print(f"Position Size: {capital.get('positionSizePercent', 'N/A')}%")
    print()
    
    risk = recipe.get('risk', {})
    print(f"Stop Loss: {risk.get('stopLossPercent', 'N/A')}%")
    print(f"Take Profit: {risk.get('takeProfitPercent', 'N/A')}%")
    print(f"Max Daily Loss: {risk.get('maxDailyLossPercent', 'N/A')}%")
    print(f"Max Open Positions: {risk.get('maxOpenPositions', 'N/A')}")
    print()
    
    indicators = recipe.get('indicators', [])
    print(f"Indicators ({len(indicators)}):")
    for ind in indicators:
        ind_name = ind.get('name', 'unknown')
        period = ind.get('period', 'N/A')
        print(f"  - {ind_name} (period: {period})")
    print()
    
    entry = recipe.get('entryConditions', {})
    print(f"Entry Logic: {entry.get('logic', 'N/A')}")
    print(f"Entry Rules: {len(entry.get('rules', []))}")
    
    exit_cond = recipe.get('exitConditions', {})
    print(f"Exit Logic: {exit_cond.get('logic', 'N/A')}")
    print(f"Exit Rules: {len(exit_cond.get('rules', []))}")

except Exception as e:
    print(f"Error analyzing recipe: {e}", file=sys.stderr)
    sys.exit(1)
PYTHON_END

        echo ""
    done

    echo "================================================================================================="
} > "$ANALYSIS_FILE"

echo "✓ Recipe analysis saved to: $ANALYSIS_FILE"
echo ""

# Create comparison matrix CSV
MATRIX_FILE="$OUTPUT_DIR/comparison_matrix.csv"

{
    echo -n "Recipe,"
    for config_key in "${!TEST_CONFIGS[@]}"; do
        echo -n "$config_key,"
    done
    echo "Average_Return,Best_Config"

    for recipe_name in "${RECIPES[@]}"; do
        echo -n "$recipe_name,"
        for config_key in "${!TEST_CONFIGS[@]}"; do
            echo -n "PENDING,"
        done
        echo "N/A,N/A"
    done
} > "$MATRIX_FILE"

echo "✓ Comparison matrix template created: $MATRIX_FILE"
echo ""

# Create detailed test plan
PLAN_FILE="$OUTPUT_DIR/test_plan.md"

{
    cat <<'MARKDOWN_END'
# Emiglio Recipe Comparison Test Plan

## Objective
Compare all available trading recipes across multiple market pairs and timeframes to identify:
1. Best performing strategy overall
2. Best strategy per symbol/timeframe combination
3. Optimization opportunities for each recipe

## Test Period
- Duration: 365 days (1 year)
- End Date: Current date
- Start Date: 365 days ago

## Test Matrix

### Symbols
1. **ETHUSDT** - Ethereum vs US Dollar (Tether)
2. **EURUSDT** - Euro vs US Dollar (Tether)
3. **ETHEUR** - Ethereum vs Euro

### Timeframes
1. **1h** - Hourly candles (8,760 candles/year)
2. **1d** - Daily candles (365 candles/year)

### Recipes
MARKDOWN_END

    for recipe_name in "${RECIPES[@]}"; do
        recipe_file="$RECIPES_DIR/${recipe_name}.json"
        description=$(python3 -c "import json; print(json.load(open('$recipe_file')).get('description', 'No description'))")
        echo "$((++counter)). **${recipe_name}** - $description"
    done

    cat <<'MARKDOWN_END'

## Metrics to Compare

### Performance Metrics
- **Total Return (%)** - Overall profit/loss percentage
- **Sharpe Ratio** - Risk-adjusted return
- **Max Drawdown (%)** - Largest peak-to-trough decline
- **Win Rate (%)** - Percentage of winning trades
- **Profit Factor** - Gross profit / gross loss

### Activity Metrics
- **Total Trades** - Number of trades executed
- **Average Trade Duration** - Time held per position
- **Trade Frequency** - Trades per day/week

### Risk Metrics
- **Volatility** - Standard deviation of returns
- **Maximum Loss** - Largest single losing trade
- **Risk of Ruin** - Probability of losing all capital

## Execution Plan

### Phase 1: Data Collection
- Download historical data for all symbols and timeframes
- Verify data completeness and quality

### Phase 2: Backtest Execution
- Run each recipe on each symbol/timeframe combination
- Total tests: [recipe count] × 6 configurations = [total] tests

### Phase 3: Analysis
- Aggregate results
- Rank recipes by performance
- Identify optimal parameters

### Phase 4: Optimization
- Suggest improvements for each recipe
- Test optimized versions

## Notes
- All tests use the same historical period for fair comparison
- Commission and slippage should be included
- Results should account for realistic trading conditions
MARKDOWN_END

} > "$PLAN_FILE"

echo "✓ Test plan created: $PLAN_FILE"
echo ""

# Display summary
cat "$ANALYSIS_FILE"

echo ""
echo "================================================================================================="
echo "NEXT STEPS"
echo "================================================================================================="
echo ""
echo "1. Review the recipe analysis: $ANALYSIS_FILE"
echo "2. Check the test plan: $PLAN_FILE"
echo "3. To run actual backtests, you need:"
echo "   - EmilioUI application with backtest functionality"
echo "   - OR a dedicated backtest engine"
echo "   - Historical data for all symbols (ETHUSDT, EURUSDT, ETHEUR)"
echo ""
echo "4. The comparison matrix is ready at: $MATRIX_FILE"
echo "   Once backtests complete, populate this file with results"
echo ""
echo "================================================================================================="

