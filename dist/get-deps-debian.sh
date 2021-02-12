#!/bin/bash


function echoPackageInstall() {
  name="$1"

  echo
  echo "============================================================"
  echo "Installing $name..."
  echo "============================================================"
  echo
}

function extractGitHubSource() {
  author="$1"
  name="$2"
  branch_name="$3"
  extract_dir="$4"

  download_dir="download/$author"
  dest_dir="$extract_dir/$author"
  archive="$name-$branch_name.tar.gz"

  mkdir -p "$download_dir" && \
  wget -O "$download_dir/$archive" "https://github.com/$author/$name/archive/$branch_name.tar.gz" && \
  mkdir -p "$dest_dir" && \
  tar -xzf "$download_dir/$archive" -C "$dest_dir"
}


function tryAptGet() {
  name="$1"

  echoPackageInstall "$name"

  if apt-get install -y "$name"; then
    echo "Successfully get $name."
  else
    echo "Error : unable to get $name." >&2
  fi
}

function tryHeaderOnlyGet() {
  author="$1"
  name="$2"
  branch_name="$3"

  dep_full_name="$author/$name"
  echoPackageInstall "$dep_full_name"

  caller_dir="$(pwd)" # Must be absolute for working-directory restore
  install_dir="$caller_dir/install" # Must be absolute for CMake configuration
  deps_dir="build"

  mkdir -p $deps_dir && \
  extractGitHubSource "$author" "$name" "$branch_name" "$deps_dir" && \
  cd "$deps_dir/$author/$name-$branch_name" && \
  cmake "-DCMAKE_INSTALL_PREFIX=$install_dir" . && \
  cmake --install . && \
  echo "Successfully get $dep_full_name." || \
  echo "Error : unable to get $dep_full_name."

  cd "$caller_dir" || exit 1
}

function trySourceGet() {
  author="$1"
  name="$2"
  branch_name="$3"

  dep_full_name="$author/$name"
  echoPackageInstall "$dep_full_name"

  caller_dir="$(pwd)"
  install_dir="$caller_dir/install"
  deps_dir="build"
  build_dir="$deps_dir/$author/$name-$branch_name/build"

  mkdir -p "$deps_dir" && \
  extractGitHubSource "$author" "$name" "$branch_name" "$deps_dir"
  mkdir -p "$build_dir" && \
  cd "$build_dir" && \
  cmake "-DCMAKE_INSTALL_PREFIX=$install_dir" "-DCMAKE_AR=$(which "$AR")" "-DCMAKE_RANLIB=$(which "$RANLIB")" .. && \
  cmake --build . -- "-j$(nproc)" && \
  cmake --install . && \
  echo "Successfully get $dep_full_name." || \
  echo "Error : unable to get $dep_full_name."

  cd "$caller_dir" || exit 1
}


tryAptGet libboost-all-dev
tryAptGet liblua5.3-dev
tryHeaderOnlyGet nlohmann json master
tryHeaderOnlyGet ThePhD sol2 main
trySourceGet gabime spdlog v1.x
