#!/usr/bin/env bash

set +e

LONG_DEBUG_FLAGS="-g -O0 -Wall -Wextra -Wstrict-prototypes -Wundef -Wcast-qual -Wconversion -Wformat=2 -Wshadow -ftrapv -Wuninitialized -Winit-self -fsanitize=address -Wcast-align -Wwrite-strings"

CFLAGS="$LONG_DEBUG_FLAGS" ./build_debug.sh
