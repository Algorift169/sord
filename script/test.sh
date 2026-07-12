#!/usr/bin/env bash

# Exit immediately if any command fails
set -e

# Get the root directory of the project
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

echo "=== Building Sord project and all unit tests ==="
cmake -S . -B build
cmake --build build

echo ""
echo "=== Running all tests one by one ==="
echo ""

# List of tests to run
TESTS=(
    "test_document_title"
    "test_document_insert_char"
    "test_document_insert_newline"
    "test_document_backspace"
    "test_document_move_cursor"
    "test_page_basics"
    "test_page_manager_basics"
    "test_page_manager_add_remove"
    "test_page_manager_reorder"
    "test_page_layout"
    "test_save_manager_path"
    "test_save_manager_save"
    "test_editor_input"
    "test_pdf_exporter"
)

PASSED_COUNT=0
FAILED_COUNT=0
FAILED_TESTS=()

for test_name in "${TESTS[@]}"; do
    test_bin="./build/${test_name}"
    
    if [ ! -f "$test_bin" ]; then
        echo -e "\e[31m[ERROR] Test binary not found: ${test_bin}\e[0m"
        FAILED_COUNT=$((FAILED_COUNT + 1))
        FAILED_TESTS+=("$test_name")
        continue
    fi

    echo -n "Running ${test_name}... "
    if "$test_bin" > /dev/null 2>&1; then
        echo -e "\e[32mPASSED\e[0m"
        PASSED_COUNT=$((PASSED_COUNT + 1))
    else
        echo -e "\e[31mFAILED\e[0m"
        FAILED_COUNT=$((FAILED_COUNT + 1))
        FAILED_TESTS+=("$test_name")
    fi
done

echo ""
echo "=== Test Summary ==="
echo "Total Tests Run: $((${PASSED_COUNT} + ${FAILED_COUNT}))"
echo -e "Passed: \e[32m${PASSED_COUNT}\e[0m"
echo -e "Failed: \e[31m${FAILED_COUNT}\e[0m"

if [ ${FAILED_COUNT} -gt 0 ]; then
    echo "Failed tests:"
    for failed in "${FAILED_TESTS[@]}"; do
        echo "  - $failed"
    done
    exit 1
else
    echo "All tests passed successfully!"
    exit 0
fi
