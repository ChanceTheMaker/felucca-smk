#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/public-tests
for mode in private public; do
    flags=
    if [ "$mode" = public ]; then flags=-DTEST_PUBLIC; fi
    cc -std=c99 -O1 -g -fsanitize=address,undefined $flags -Ifirmware/hal tests/smk37_update_test.c -o build/public-tests/$mode
    build/public-tests/$mode
done
cc -std=c99 -O1 -g -fsanitize=address,undefined -Ifirmware/hal tests/smk37_flash_profile_test.c -o build/public-tests/flash-profile
build/public-tests/flash-profile
