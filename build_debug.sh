#!/usr/bin/env bash

set +e

if [[ ! -f "./autogen.sh" ]]; then
    echo "Run from project root (folder with autogen.sh)"
    exit 1
fi

CPP_DEFS="-DDEBUG " #-DINPUT_DEBUG"

#for some reason this sometimes fails
make clean distclean \
    || /bin/true

#scrub any left over object files
find . -name '*.o' -type f -delete
find src/ -name "xournal" -type f -executable -delete

BASIC_DEBUG_FLAGS="-g -O0"

#set cflags to -g -O0 if it's unset or empty
#leave it as-is if it's nonempty
CFLAGS="${CFLAGS:?$BASIC_DEBUG_FLAGS}" ./autogen.sh --enable-maintainer-mode

NPROC=$(nproc)
make "-j$NPROC"
