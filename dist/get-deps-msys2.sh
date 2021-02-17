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

  # Return code to indicate if any error occurred during install
  local failure=
  # Try to install MinGW package for appropriate target (i686 or x86_64)
  # --needed for no error if already installed, --noconfirm for non-interactive mode
  if pacman --sync --noconfirm --needed "mingw-w64-$target-$name"; then
    failure=0
    log "Successfully installed $name."
  else
    failure=1
    error "Error : Unable to install $name."
  fi

  return $failure
}


## Determine target type with first script arg : 32 for i686, 64 for x86_64
mode="$1"

# $target will be used to determine name of installed MinGW packages
if [ "$mode" == 32 ] || [ "$mode" == "i686" ]; then
  target="i686"
elif [ "$mode" == 64 ] || [ "$mode" == "x86_64" ]; then
  target="x86_64"
else # Parameter mode is required by the script
  error "Error : Unable to get target type for mode \"$1\"."
  exit 1
fi


failure= # Set if any of install try actually fails

tryPacmanGet boost "$target" || failure=1
tryPacmanGet spdlog "$target" || failure=1
tryPacmanGet nlohmann-json "$target" || failure=1
tryPacmanGet lua "$target" || failure=1
tryPacmanGet sol2 "$target" || failure=1

if [ $failure ]; then # If any error occurred...
  exit 1 # ...the script hasn't complete successfully
fi
