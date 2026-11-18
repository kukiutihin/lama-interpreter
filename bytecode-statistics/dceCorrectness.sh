#!/bin/bash

set -e

SCRIPT_DIR="$(pwd)"
ROOT_DIR="${SCRIPT_DIR}/.."

GREEN='\033[1;32m'
RED='\033[1;31m'
BOLD='\033[1m'
RESET='\033[0m'

cleanup() {
  rm -rf "$SCRIPT_DIR/build"
}

cmake -S "$ROOT_DIR" -B "$ROOT_DIR/build"
cmake --build "$ROOT_DIR/build" --target lamastat

trap cleanup EXIT 

LAMASTAT="$ROOT_DIR/build/bytecode-statistics/lamastat"

out_sort=$("$LAMASTAT" "$ROOT_DIR/example/Sort.bc" 2>&1)
out_dead=$("$LAMASTAT" "$ROOT_DIR/example/SortWithDead.bc" 2>&1)

if [[ "$out_sort" == "$out_dead" ]]; then
  echo -e "${BOLD}${GREEN}CORRECT${RESET}"
  exit 0
else
  echo -e "${BOLD}${RED}INCORRECT${RESET}"
  echo "---- Sort.bc ----"
  echo "$out_sort"
  echo "---- SortWithDead.bc ----"
  echo "$out_dead"
  echo "-------------------------"
  exit 1
fi
