#!/bin/bash


## Escape consts
RESET="\033[m"
BRIGHT_RED="\033[91m"


clear=
help=
local=
mingw=
debug_features=

## Iterate args looking for :
# --clear : Total remove of build/ directory
# --help : Explain command usage
# --local : Install path set to dist/install
# --mingw : Enable "MinGW Makefiles" generator and $MSYSTEM_PREFIX system install path
# --debug-features : Enable CMake targets for unit testing and doc generation, even if build type is Release
for arg in "$@"; do
  if [ "$arg" ]; then
    if [ "$arg" == "--clear" ]; then
      clear=1
    elif [ "$arg" == "--help" ]; then
      help=1
    elif [ "$arg" == "--local" ]; then
      local=1
    elif [ "$arg" == "--mingw" ]; then
      mingw=1
    elif [ "$arg" == "--debug-features" ]; then
      debug_features=1
    else
      echo -e "${BRIGHT_RED}Unknown argument \"$arg\".${RESET}"
      exit 1
    fi
  fi
done


# Clear project CMake dir mode
if [ "$clear" ]; then
  rm -r -f build/
  exit 0
fi


# Help message mode
if [ "$help" ]; then
  echo "Usage: $0 [option]..."
  echo
  echo "Options are :"
  echo "    --clear          : Clear build directory build/"
  echo "    --help           : Print this message"
  echo "    --local          : Set install path to dist/install"
  echo "    --mingw          : Set build generator to \"MinGW Makefiles\" and system"
  echo "                       prefix path to $MSYSTEM_PREFIX. Required on MinGW."
  echo "    --debug-features : Enable CMake targets for unit testing and doc generation,"
  echo "                       even if this command use Release build."
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
  if [ "$mingw" ]; then # Better to use msystem environment as install directory for non-local MinGW installation
    install_prefix="-DCMAKE_INSTALL_PREFIX=$MSYSTEM_PREFIX"
  else # If using any other environment, keep default system install path
    install_prefix=""
  fi
fi

# Set CMake RPT_FORCE_DEBUG_FEATURES depending on --debug-features command option
if [ "$debug_features" ]; then
  debug_features_option="-DRPT_FORCE_DEBUG_FEATURES=1"
else
  debug_features_option="-DRPT_FORCE_DEBUG_FEATURES=0"
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


successfull=

mkdir -p build && \
cd build && \
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_SYSTEM_PREFIX_PATH="../dist/install" $install_prefix \
  -DCMAKE_AR="$ar_exec" -DCMAKE_RANLIB="$ranlib_exec" -G"$generator" $debug_features_option .. && \
cmake --build . -- "-j$(nproc)" && \
cd .. && \
successfull=1

if [ ! $successfull ]; then
  echo -e "${BRIGHT_RED}Error occurred during build script, build might be incomplete.${RESET}"
  exit 2
fi

