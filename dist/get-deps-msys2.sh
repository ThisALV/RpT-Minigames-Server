#!/bin/bash


# Load utils shared across get-deps scripts
source get-deps-common.sh

## Retrieve and install package from pacman
# Args:
# $1: Name of installed package
# $2: MinGW target version (i686 / x86_64)
function tryPacmanGet() {
    local name="$1"
    local target="$2"

    echoPackageInstall "$name"

    if pacman --sync --noconfirm --needed "mingw-w64-$target-$name"; then
      log "Successfully installed $name."
    else
      log "Error : Unable to install $name."
    fi
}


## Determine target type with first script arg : 32 for i686, 64 for x86_64
mode="$1"

if [ "$mode" == 32 ] || [ "$mode" == "i686" ]; then
  target="i686"
elif [ "$mode" == 64 ] || [ "$mode" == "x86_64" ]; then
  target="x86_64"
else
  log "Error : Unable to get target type for mode \"$1\"."
  exit 1
fi

tryPacmanGet boost "$target"
tryPacmanGet spdlog "$target"
tryPacmanGet nlohmann-json "$target"
tryPacmanGet lua "$target"
tryPacmanGet sol2 "$target"
