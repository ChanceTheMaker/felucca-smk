#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/smk37-tests
python3 tests/test_smk37_storage_backup.py
python3 -c 'import sys; sys.path.insert(0, "tools"); import build; build.generate()'
cc -std=c99 -O1 -w -fsanitize=address -Ibuild/gen -Ifirmware/hal \
    tests/smk37_synth_test.c -o build/smk37-tests/synth
build/smk37-tests/synth
cc -std=c99 -O1 -w -fsanitize=address -Ibuild/gen -Ifirmware/hal \
    tests/smk37_buttons_test.c -o build/smk37-tests/buttons
build/smk37-tests/buttons
cc -std=c99 -O1 -w -fsanitize=address -Ibuild/gen -Ifirmware/hal \
    tests/smk37_features_test.c -o build/smk37-tests/features
build/smk37-tests/features
build/smk37-tests/features --wire > build/smk37-tests/editor-wire.jsonl
if command -v node >/dev/null 2>&1; then
    node web/test_smk37_wire.mjs build/smk37-tests/editor-wire.jsonl
elif command -v node.exe >/dev/null 2>&1; then
    node.exe web/test_smk37_wire.mjs build/smk37-tests/editor-wire.jsonl
else
    echo 'Node is required for the SMK firmware/editor integration check' >&2
    exit 1
fi
cc -std=c99 -O1 -w tests/smk37_sysex_test.c -o build/smk37-tests/sysex
build/smk37-tests/sysex
# The board-sized key arrays and velocity-aware note-on are shared with FM-1.
# Exercise its existing chord and expressive MIDI regression suites unchanged.
for component in chord midi_control midi_uart; do
    cc -O1 -w -Ibuild/gen -Ifirmware/src "tests/${component}_test.c" \
        -lm -o "build/smk37-tests/${component}"
    "build/smk37-tests/${component}" > "build/smk37-tests/${component}.log"
    tail -n 2 "build/smk37-tests/${component}.log"
done

# Real Test12 save migration and the SMK adapters for upstream 1.5.1.
for component in smk37_upstream smk37_store smk37_object_store smk37_samples smk37_sample_draw smk37_midi_tx smk37_midi_output smk37_completion smk37_draw smk37_workflow smk37_test19 smk37_test20 smk37_test21 smk37_test23; do
    cc -std=c99 -O1 -w -fsanitize=address -Ibuild/gen -Ifirmware/hal \
        "tests/${component}_test.c" -o "build/smk37-tests/${component}"
    "build/smk37-tests/${component}"
done

build/smk37-tests/smk37_completion --wire > build/smk37-tests/completion-wire.jsonl
if command -v node >/dev/null 2>&1; then
    node web/test_smk37_completion.mjs
else
    node.exe web/test_smk37_completion.mjs
fi

build/smk37-tests/smk37_test19 --wire > build/smk37-tests/test19-wire.jsonl
if command -v node >/dev/null 2>&1; then
    node web/test_smk37_menu.mjs
else
    node.exe web/test_smk37_menu.mjs
fi

build/smk37-tests/smk37_workflow --wire > build/smk37-tests/workflow-wire.jsonl
if command -v node >/dev/null 2>&1; then
    node web/test_smk37_preferences.mjs
else
    node.exe web/test_smk37_preferences.mjs
fi

build/smk37-tests/smk37_samples --wire > build/smk37-tests/samples-wire.jsonl
if command -v node >/dev/null 2>&1; then
    node web/test_smk37_samples.mjs
else
    node.exe web/test_smk37_samples.mjs
fi
