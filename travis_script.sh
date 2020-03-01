#!/bin/bash

set -x -e -v

env PLATFORMIO_CI_SRC=examples/basic/ \
    pio ci --board=d1_mini --board=nano32 --lib="."
env PLATFORMIO_CI_SRC=examples/debug \
    pio ci --board=d1_mini --board=nano32 --lib="."
env PLATFORMIO_CI_SRC=examples/time \
    pio ci --board=d1_mini --board=nano32 --lib="."
