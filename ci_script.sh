#!/bin/bash

set -x -e -v

for board in d1_mini nano32 ; do
    echo "- Building for $board"
    env PLATFORMIO_CI_SRC=examples/basic/ \
        pio ci --board=$board --lib="."
    env PLATFORMIO_CI_SRC=examples/debug \
        pio ci --board=$board --lib="."
    env PLATFORMIO_CI_SRC=examples/time \
        pio ci --board=$board --lib="."
done

echo "- Running host tests"

cd test/
pio test
