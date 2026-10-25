#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
export DEVKITPRO="${DEVKITPRO:-/opt/devkitpro}"
export DEVKITA64="${DEVKITA64:-$DEVKITPRO/devkitA64}"
export PATH="$DEVKITA64/bin:$DEVKITPRO/tools/bin:$PATH"
if [ ! -f "$DEVKITPRO/libnx/switch_rules" ]; then
  echo "Missing devkitPro Switch SDK. Follow the README to install switch-dev."
  exit 1
fi
if [ ! -f "$DEVKITPRO/portlibs/switch/include/curl/curl.h" ]; then
  echo "Missing switch-curl. Run: sudo dkp-pacman -S switch-curl switch-zlib switch-libopus"
  exit 1
fi
python3 vendor/rtc/build_native.py "$DEVKITPRO"
make -j2
mkdir -p sdcard/switch/SwitchTalk
cp SwitchTalk.nro sdcard/switch/SwitchTalk/SwitchTalk.nro
echo "Built! Copy sdcard/switch/SwitchTalk/ to SD:/switch/SwitchTalk/"
