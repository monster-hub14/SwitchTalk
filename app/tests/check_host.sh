#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
cc -std=c11 -Wall -Wextra -Werror -Iinclude source/json.c tests/test_json.c -o /tmp/switchtalk_json_test
/tmp/switchtalk_json_test
cc -std=c11 -Wall -Wextra -Werror -Iinclude $(pkg-config --cflags libcurl) -c source/talk.c -o /tmp/switchtalk_talk.o
echo 'Host code checks passed (this does NOT build a Switch .nro).'
