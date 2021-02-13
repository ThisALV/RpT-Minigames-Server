#!/bin/bash


## Escape consts
RESET="\033[m"
BRIGHT_RED="\033[91m"


# Special case for MinGW environment
if [ "$1" == "mingw" ]; then
  generator="MinGW Makefiles"
else
  generator="Unix Makefiles"
fi

# Default value for archiver used by CMake
if [ ! "$AR" ]; then
  AR="ar"
fi

# Default value for ranlib executable used by CMake
if [ ! "$RANLIB" ]; then
  RANLIB="ranlib"
fi

ar_exec="$(which "$AR")"
ranlib_exec="$(which "$RANLIB")"


mkdir -p build && \
cd build && \
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_SYSTEM_PREFIX_PATH="../dist/install" \
  -DCMAKE_AR="$ar_exec" -DCMAKE_RANLIB="$ranlib_exec" .. && \
cmake --build . -- "-j$(nproc)" && \
cd .. || \
echo -e "${BRIGHT_RED}Error occurred during build script, build might be incomplete.${RESET}"
