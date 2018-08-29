#!/usr/bin/env bash

find src/  -regextype egrep \
    -regex '.*\.(h|c)' \
    -exec sed -E -i 's/^#include\s+<gtk\/gtk.h>\s*$/#include <gtk-2.0\/gtk\/gtk.h>/' {} \;
