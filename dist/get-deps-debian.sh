#!/bin/bash

## Return codes :
# 1 : A package hasn't been installed correctly
# 2 : A fatal error occurred


# Load utils shared across get-deps scripts
source get-deps-common.sh


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

  local dep_full_name="$author/$name" # Full and unique name of dep used for logging
  local download_dir="download/$author" # Downloaded archive location
  local dest_dir="$extract_dir/$author" # Extracted archive content location
  local archive="$name-$branch_name.tar.gz" # Name of downloaded archive
  local sources_url="https://github.com/$author/$name/archive/$branch_name.tar.gz" # Download URL

  log "Create download directory $download_dir..." && \
  mkdir -p "$download_dir" && \
  log "Download sources from $sources_url..." && \
  wget -O "$download_dir/$archive" "$sources_url" && \
  log "Create destination directory $dest_dir..." && \
  mkdir -p "$dest_dir" && \
  log "Extract downloaded sources to $dest_dir..." && \
  tar -xzf "$download_dir/$archive" -C "$dest_dir" && \
  log "Extracting done for $dep_full_name." || \
  error "Error : unable to extract sources for $dep_full_name."
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
    log "Successfully get $name."
  else
    failure=1
    error "Error : unable to get $name." >&2
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
  log "Successfully get $dep_full_name." && \
  failure=0 || \
  error "Error : unable to get $dep_full_name."

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
  log "Successfully get $dep_full_name." || \
  error "Error : unable to get $dep_full_name."

  log "Back to $caller_dir..."
  cd "$caller_dir" || exit 2 # Script cannot continue normally if working dir cannot be restored

  return $failure
}


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

tryAptGet libboost-all-dev || failure=1
tryAptGet liblua5.3-dev || failure=1
tryHeaderOnlyGet nlohmann json master || failure=1
tryHeaderOnlyGet ThePhD sol2 main || failure=1
trySourceGet gabime spdlog v1.x spdlog-1.x || failure=1

if [ $failure ]; then # If any error occurred...
  exit 1 # ...the script hasn't complete successfully
fi
