#!/bin/bash

tests="$(dirname "$0")"

for test in $(find "$tests/compilation" -not -type d | sort); do
  $1 "$test"
done
