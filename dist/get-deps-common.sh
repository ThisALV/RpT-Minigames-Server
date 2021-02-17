## Shell source for common constants and utils functions shared across get-deps scripts


## Escape consts
RESET="\033[m"
YELLOW="\033[33m"
BRIGHT_RED="\033[91m"
BRIGHT_GREEN="\033[92m"
BRIGHT_WHITE="\033[97m"


## Print script log message as green text
# Args:
# $1: Message to print
function log() {
  echo -e "${BRIGHT_GREEN}${1}${RESET}"
}

## Print error log message as red text
# Args:
# $1: Error message to print
function error() {
  echo -e "${BRIGHT_RED}${1}${RESET}"
}

## Print packages installation separator
# Args:
# $1: Full name of installed package
function echoPackageInstall() {
  local name="$1"

  echo -e "$YELLOW"
  echo "============================================================"
  echo "Installing $name..."
  echo "============================================================"
  echo -e "$RESET"
}
