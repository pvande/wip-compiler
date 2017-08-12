#!/bin/bash

tests="$(dirname "$0")"

results=$(mktemp -d "${TMPDIR:-/tmp/}$(basename 0).XXXXXXXXXXXX")

for test in $(find "$tests/compilation" -not -type d | sort); do
  mkdir -p "$results/$test"
  rmdir "$results/$test"

  echo "Running test $test"

  $1 "$test" > "$results/$test.stderr" 2>/dev/null || {
    cat "$results/$test.stderr"
    echo ""
    echo "Failed test $test"
    echo "Test output can be found in $results."
    exit 1
  }
done

for test in $(find "$tests/errors" -name "*.xxx" -and -not -type d | sort); do
  mkdir -p "$results/$test"
  rmdir "$results/$test"

  echo "Running test $test"

  $1 "$test" > "$results/$test.stderr" 2>/dev/null
  # $1 "$test" > "$test.stderr" 2>/dev/null
  status=$?

  diff "$results/$test.stderr" "$test.stderr" || {
    cat "$results/$test.stderr"
    echo ""
    echo "Failed test $test"
    echo "Test output can be found in $results."
    exit 1
  }
done
