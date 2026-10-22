#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/smk37-tests
[ "$#" -gt 0 ] || set -- sound14 sound15 insert reverb click perform drum fm6bank fun10 ratchet step15 detail motion midi_control project editor settings theme ui seq155 fold155 ui155 v156 v16
for name in "$@"; do
  echo "== $name"
  cc -std=gnu99 -O1 -w -Ibuild/gen -Ifirmware/src "tests/${name}_test.c" -lm -o "build/smk37-tests/upstream-$name"
  "build/smk37-tests/upstream-$name" > "build/smk37-tests/upstream-$name.log" 2>&1 || { tail -n 30 "build/smk37-tests/upstream-$name.log"; exit 1; }
  tail -n 1 "build/smk37-tests/upstream-$name.log"
done
