#!/bin/bash


## Escape consts
RESET="\033[m"
BRIGHT_RED="\033[91m"


help=
local=
mingw=

## Iterate args looking for :
# --local : Install path set to dist/install
# --mingw : Enable "MinGW Makefiles" generator
for arg in "$@"; do
  if [ "$arg" ]; then
    if [ "$arg" == "--help" ]; then
      help=1
    elif [ "$arg" == "--local" ]; then
      local=1
    elif [ "$arg" == "--mingw" ]; then
      mingw=1
    else
      echo -e "${BRIGHT_RED}Unknown argument \"$arg\".${RESET}"
      exit 1
    fi
  fi
done


# Help message mode
if [ "$help" ]; then
  echo "Usage: $0 [option]..."
  echo
  echo "Options are :"
  echo "    --help  : Print this message"
  echo "    --local : Set install path to dist/install"
  echo "    --mingw : Set build generator to \"MinGW Makefiles\", required on MinGW"
  echo

  exit 0
fi


# Special case for MinGW environment
if [ "$mingw" ]; then
  generator="MinGW Makefiles"
else
  generator="Unix Makefiles"
fi

# Use default install path if local isn't set
if [ "$local" ]; then
  install_prefix="-DCMAKE_INSTALL_PREFIX=../dist/install"
else
  install_prefix=""
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
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_SYSTEM_PREFIX_PATH="../dist/install" $install_prefix \
  -DCMAKE_AR="$ar_exec" -DCMAKE_RANLIB="$ranlib_exec" -G"$generator" .. && \
cmake --build . -- "-j$(nproc)" && \
cd .. || \
echo -e "${BRIGHT_RED}Error occurred during build script, build might be incomplete.${RESET}"
