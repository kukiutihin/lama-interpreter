#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

GREEN='\033[1;32m'
RED='\033[1;31m'
BOLD='\033[1m'
RESET='\033[0m'

cleanup() {
  rm -rf "$ROOT_DIR/build"
}
trap cleanup EXIT

cmake -S "$ROOT_DIR" -B "$ROOT_DIR/build"
cmake --build "$ROOT_DIR/build" --target lamastat

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
