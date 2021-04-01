#!/bin/bash

## Return codes :
# 1 : A package hasn't been installed correctly
# 2 : A fatal error occurred


# Load utils shared across get-deps scripts
source get-deps-common.sh

## Download and extract sources archive from given URL
# Args:
# $1: URL to fetch sources archive from
# $2: Downloaded sources archive local directory
# $3: Downloaded archive local filename
# $4: Downloaded archive compression format
# $5: Sources archive extraction directory
function extractSourceFrom() {
  local sources_url="$1"
  local download_dir="$2"
  local archive_filename="$3"
  local archive_compression="$4" # Passed as option to tar command for archive decompression
  local extraction_dir="$5"

  log "Create download directory $download_dir..." && \
  mkdir -p "$download_dir" && \
  log "Download sources from $sources_url..." && \
  wget -O "$download_dir/$archive_filename" "$sources_url" && \
  log "Create destination directory" && \
  mkdir -p "$extraction_dir" && \
  log "Extract downloaded sources to $dest_dir..." && \
  tar xf "--$archive_compression" -C "$extraction_dir" && \
  log "Extraction done to $dest_dir." || \
  error "Error: unable to extract sources from $sources_url."
}


## Download and extract GitHub sources archive
# Args:
# $1: Author of extracted package
# $2: Name of extracted package
# $3: Name of fetched remote branch
# $4: Sources archive extraction destination
function extractGitHubSource() {
  local author="$1"
  local name="$2"
  local branch_name="$3"
  local extract_dir="$4"

  local dest_dir="$extract_dir/$author" # Extracted archive content location
  local archive_filename="$name-$branch_name.tar.gz" # Name of downloaded archive
  local sources_url="https://github.com/$author/$name/archive/$branch_name.tar.gz" # Download from URL using gzip compression

  extractSourceFrom "$sources_url" "download/$author" "$archive_filename" gzip "$extract_dir/$author"
}


## Retrieve boost sources, build given libraries and install files
# Args:
# $1: Major version number
# $2: Minor version number
# $3: Patch version number
# $4: Comma separated list for libraries to be built separately
function tryBoostSourceGet() {
  echoPackageInstall "Boost"

  local major="$1"
  local minor="$2"
  local patch="$3"
  local build_required_for="$4" # Will be passed to --with-libraries bootstrap option

  local caller_dir # Saved working_directory for later restore as it will change during script execution
  caller_dir="$(pwd)" # Must be absolute for working-directory restore

  local version="$major.$minor.$patch" # Formatted version library number
  local archive_filename="boost_${major}_${minor}_${patch}.tar.bz2" # Archive which will be fetched and downloaded using bzip2 compression
  local sources_url="https://dl.bintray.com/boostorg/release/${version}/source/${archive_filename}" # Full sources archive URL

  local deps_dir="build" # Built dependencies parent directory
  local build_dir="$dest_dir/boostorg" # Where Boost will be bootstrapped and built
  local install_dir="install" # Where headers and binaries will be installed

  # Return code to indicate if any error occurred during install
  # Set to 0 at the end of install operations if successfully done
  local failure=1

  log "Create deps build directory at $deps_dir..." && \
  mkdir -p "$dest_dir" && \
  extractSourceFrom "$sources_url" "download/boostorg" "$archive_filename" bzip2 "$build_dir" && \
  log "Go to Boost root directory $build_dir..." && \
  cd "$build_dir" && \
  log "Bootstrapping Boost project to install at $install_dir and build libraries $build_required_for..." && \
  ./bootstrap.sh "--prefix=$install_dir" "--with-libraries=$build_required_for" && \
  log "Installing built Boost packages..." && \
  ./b2 install && \
  log "Successfully got Boost packages." && \
  failure=0 || \
  error "Error: unable to get Boost packages."

  log "Back to $caller_dir..."
  cd "$caller_dir" || exit 2

  return $failure
}


## Retrieve and install package from apt-get
# Args:
# $1: Name of installed package
function tryAptGet() {
  local name="$1"

  echoPackageInstall "$name"

  # Return code to indicate if any error occurred during install
  local failure=
  # Try to install package from apt-get, -y for non-interactive mode
  if apt-get install -y "$name"; then
    failure=0
    log "Successfully got $name."
  else
    failure=1
    error "Error: unable to get $name." >&2
  fi

  return $failure
}

## Retrieve and install header-only package from GitHub
# Args:
# $1: Author of extracted package
# $2: Name of extracted package
# $3: Name of fetched remote branch
function tryHeaderOnlyGet() {
  local author="$1"
  local name="$2"
  local branch_name="$3"

  local dep_full_name="$author/$name" # Full and unique name of dep used for logging
  echoPackageInstall "$dep_full_name"

  local caller_dir # Saved working_directory for later restore as it will change during script execution
  caller_dir="$(pwd)" # Must be absolute for working-directory restore

  local install_dir="$caller_dir/install" # Must be absolute for CMake configuration
  local deps_dir="build" # CMake project parent directory
  local project_dir="$deps_dir/$author/$name-$branch_name" # CMake project fully-qualified directory

  # Return code to indicate if any error occurred during install
  # Set to 0 at the end of install operations if successfully done
  local failure=1

  log "Create deps build directory at $deps_dir..." && \
  mkdir -p $deps_dir && \
  log "Extract $dep_full_name sources..." && \
  extractGitHubSource "$author" "$name" "$branch_name" "$deps_dir" && \
  log "Go to $dep_full_name project directory $project_dir..." && \
  cd "$project_dir" && \
  log "Configure project..." && \
  cmake -DCMAKE_BUILD_TYPE=Release "-DCMAKE_INSTALL_PREFIX=$install_dir" . && \
  log "Install project to $install_dir..." && \
  cmake --install . && \
  log "Successfully got $dep_full_name." && \
  failure=0 || \
  error "Error: unable to get $dep_full_name."

  log "Back to $caller_dir..."
  cd "$caller_dir" || exit 2 # Script cannot continue normally if working dir cannot be restored

  return $failure
}

## Retrieve, build and install package from Github
# Args:
# $1: Author of extracted package
# $2: Name of extracted package
# $3: Name of fetched remote branch
# $4: (Optional) Custom name for project directory inside build/
function trySourceGet() {
  local author="$1"
  local name="$2"
  local branch_name="$3"
  local special_project_dir="$4"

  local dep_full_name="$author/$name" # Full and unique name of dep used for logging
  echoPackageInstall "$dep_full_name"

  local caller_dir # Saved working_directory for later restore as it will change during script execution
  caller_dir="$(pwd)" # Must be absolute for working-directory restore

  local install_dir="$caller_dir/install" # Must be absolute for CMake configuration
  local deps_dir="build" # CMake project parent directory
  local project_dir="$deps_dir/$author/$name-$branch_name" # CMake project fully-qualified directory

  if [ "$special_project_dir" ]; then
    build_dir="$deps_dir/$author/$special_project_dir/build"
  else
    build_dir="$deps_dir/$author/$name-$branch_name/build"
  fi

  # Return code to indicate if any error occurred during install
  # Set to 0 at the end of install operations if successfully done
  local failure=1

  log "Create deps build directory at $deps_dir..." && \
  mkdir -p "$deps_dir" && \
  extractGitHubSource "$author" "$name" "$branch_name" "$deps_dir"
  log "Create $dep_full_name build directory at $build_dir..."
  mkdir -p "$build_dir" && \
  log "Go to $dep_full_name project directory $project_dir..." && \
  cd "$build_dir" && \
  log "Configure project..." && \
  cmake -DCMAKE_BUILD_TYPE=Release "-DCMAKE_INSTALL_PREFIX=$install_dir" "-DCMAKE_AR=$(which "$AR")" "-DCMAKE_RANLIB=$(which "$RANLIB")" .. && \
  log "Build project..." && \
  cmake --build . -- "-j$(nproc)" && \
  log "Install project to $install_dir..." && \
  cmake --install . && \
  log "Successfully got $dep_full_name." && \
  failure=0 || \
  error "Error: unable to get $dep_full_name."

  log "Back to $caller_dir..."
  cd "$caller_dir" || exit 2 # Script cannot continue normally if working dir cannot be restored

  return $failure
}


# List available libboost-dev packages on APT, then parse versions number, then finally retrieves latest (first) version number
apt_boost_latest="$(apt-cache madison libboost-dev | sed -E "s/.+\| (.+) \| .*/\\1/g" | head -n1)"

if dpkg --compare-versions "$apt_boost_latest" ">=" "1.66"; then # Boost.Beast available since 1.66
  use_apt_boost=1
else
  use_apt_boost=
fi

# Default value for archiver used by CMake
if [ ! "$AR" ]; then
  AR="ar"
fi

# Default value for ranlib executable used by CMake
if [ ! "$RANLIB" ]; then
  RANLIB="ranlib"
fi

echo -e "$BRIGHT_WHITE"
echo "==================== Toolchain summary ====================="
echo "CC=$(which "$CC"), CXX=$(which "$CXX"), LDflags=$LDFLAGS"
echo "Archiver=$(which "$AR"), Ranlib=$(which "$RANLIB")"
echo "============================================================"
echo -e "$RESET"


failure= # Set if any of install try actually fails

# Boost MPI dependency libxnvctrl0 is broken for non-updated 20.04 ubuntu distributions
# Because of this, and for performance reasons, only required Boost modules are downloaded/installed

# As dependencies MIGHT be available even if get-deps script failed, installation failure should NOT
# interrupts script

if [ use_apt_boost ]; then # If Boost apt package latest version contains Boost.Beast, then use apt-get
  # Header-only files
  tryAptGet libboost-dev || failure=1
  # Binary files
  tryAptGet libboost-filesystem-dev || failure=1
  tryAptGet libboost-test-dev || failure=1
else # Otherwise, build it from sources
  tryBoostSourceGet 1 66 0 "filesystem,test"
fi

tryAptGet liblua5.3-dev || failure=1
tryHeaderOnlyGet nlohmann json master || failure=1
tryHeaderOnlyGet ThePhD sol2 main || failure=1
trySourceGet gabime spdlog v1.x spdlog-1.x || failure=1

if [ $failure ]; then # If any error occurred...
  error "Error: some dependencies haven't been installed successfully by this script, build.sh might fails."
  exit 1 # ...the script hasn't complete successfully
fi

