#!/usr/bin/env bash

LAMAC=lamac
ROOT="$(pwd)"
TEST="$ROOT/Lama/performance/Sort.lama"

make -s app
mkdir -p "$ROOT/perf"
cd "$ROOT/perf"

name=$(basename "$TEST" .lama)

"$LAMAC" "$TEST" -b

start1=$(date +%s%N)
"$LAMAC" -i "$TEST" > /dev/null 2>&1
end1=$(date +%s%N)

elapsed_ns1=$((end1 - start1))
elapsed_sec1=$(awk "BEGIN {printf \"%.3f\", $elapsed_ns1 / 1000000000}")

start2=$(date +%s%N)
"$ROOT/app" "$name.bc" > /dev/null 2>&1
end2=$(date +%s%N)

elapsed_ns2=$((end2 - start2))
elapsed_sec2=$(awk "BEGIN {printf \"%.3f\", $elapsed_ns2 / 1000000000}")

cd "$ROOT"
rm -rf "$ROOT/perf"
make clean

echo
echo "lamac -i: ${elapsed_sec1}s"
echo "lamaSkib: ${elapsed_sec2}s"

if [ "$elapsed_ns1" -lt "$elapsed_ns2" ]; then
  ratio=$(awk "BEGIN {printf \"%.2f\", $elapsed_ns2 / $elapsed_ns1}")
  echo "WINNER: lamac -i (${ratio}x faster)"
elif [ "$elapsed_ns2" -lt "$elapsed_ns1" ]; then
  ratio=$(awk "BEGIN {printf \"%.2f\", $elapsed_ns1 / $elapsed_ns2}")
  echo "WINNER: lamaSkib (${ratio}x faster)"
else
  echo "WINNER: TIE"
fi

echo
