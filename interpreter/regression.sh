#!/bin/bash

LAMAC=lamac
ROOT_DIR="$(pwd)"

GREEN='\033[1;32m'
RED='\033[1;31m'
BOLD='\033[1m'
RESET='\033[0m'

cleanup() {
  cd "$ROOT_DIR" || exit
  rm -rf "$ROOT_DIR/tests"
  make -s clean
}
trap cleanup EXIT

make -s app
mkdir -p "$ROOT_DIR/tests"
cd "$ROOT_DIR/tests" || exit

run_suite() {
  local title="$1"
  local pattern="$2"

  echo
  echo -e "${BOLD}${title}${RESET}"

  for test in $pattern; do
    [[ -f "$test" ]] || continue

    local name
    name=$(basename "$test" .lama)
    local dir
    dir=$(dirname "$test")
    local input="$dir/$name.input"

    "$LAMAC" -b "$test" 2>/dev/null || continue

    local expected
    if [[ -f "$input" ]]; then
      expected=$("$LAMAC" -i "$test" < "$input" 2>&1) || continue
    else
      expected=$("$LAMAC" -i "$test" < /dev/null 2>&1) || continue
    fi

    local actual
    if [[ -f "$input" ]]; then
      actual=$("$ROOT_DIR/app" "$name.bc" < "$input" 2>&1)
    else
      actual=$("$ROOT_DIR/app" "$name.bc" < /dev/null 2>&1)
    fi

    echo -en "${BOLD}TEST: ${name}${RESET} "

    if [[ "$actual" == "$expected" ]]; then
      echo -e "${GREEN}PASSED${RESET}"
    else
      echo -e "${RED}FAILED${RESET}"
      echo "---- expected ----"
      echo "$expected"
      echo "----- actual -----"
      echo "$actual"
      echo -e "------------------\n"
    fi
  done
}

run_suite "REGRESSION:" "$ROOT_DIR/Lama/regression/test*.lama"
run_suite "REGRESSION LONG:" "$ROOT_DIR/Lama/regression_long/expressions/generated*.lama"
run_suite "REGRESSION LONG DEEP: --" "$ROOT_DIR/Lama/regression_long/deep-expressions/generated*.lama"
