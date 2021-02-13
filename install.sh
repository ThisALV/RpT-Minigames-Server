#!/bin/bash


## Escape consts
RESET="\033[m"
BRIGHT_RED="\033[91m"
BRIGHT_GREEN="\033[92m"


if cd build && cmake --install . && cd ..; then
  echo -e "${BRIGHT_GREEN}Successfully installed.${RESET}"
else
  echo -e "${BRIGHT_RED}Error occurred during install script, install might be incomplete.${RESET}"
fi
