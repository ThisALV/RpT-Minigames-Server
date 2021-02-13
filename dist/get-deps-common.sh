## Shell source for common constants and utils functions shared across get-deps scripts


## Escape consts
RESET="\033[m"
YELLOW="\033[33m"
BRIGHT_GREEN="\033[92m"
BRIGHT_WHITE="\033[97m"


## Print script log message as white text
# Args:
# $1: Message to print
function log() {
  echo -e "${BRIGHT_GREEN}${1}${RESET}"
}
