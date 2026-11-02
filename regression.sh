#!/bin/bash

LAMAC=lamac
ROOT_DIR="$(pwd)"
DIR="$ROOT_DIR/Lama/regression"

GREEN='\033[1;32m'
RED='\033[1;31m'
BOLD='\033[1m'
RESET='\033[0m'

make -s app
mkdir tests/
cd tests

for test in "$DIR"/test*.lama; do
  name=$(basename "$test" .lama)
  input="$DIR/$name.input"

  "$LAMAC" "$test" -b 2>/dev/null
  exitCode=$?

  if [[ $exitCode -ne 0 ]]; then 
    continue
  fi

  expected=$("$LAMAC" -i "$test" < "$input" 2>&1)
  exitCode=$?

  if [[ $exitCode -ne 0 ]]; then 
    continue
  fi

  actual=$(../app "$name.bc" < "$input" 2>&1) 

  echo -en "${BOLD}TEST: ${name}${RESET} "

  if [[ "$actual" == "$expected" ]]; then
    echo -e "${GREEN}PASSED${RESET}\n"
  else
    echo -e "${RED}FAILED${RESET}"
    echo "---- expected ----"
    echo "$expected"
    echo "----- actual -----"
    echo "$actual"
    echo -e "------------------\n"
  fi
done

cd ..
rm -f -rf tests
make -s clean
echo $total
