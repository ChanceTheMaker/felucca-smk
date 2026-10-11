# Felucca SMK 0.9.22 public beta

Upstream baseline: Felucca 1.5.1,
`a4d7ca2e5d52a4c35e2468401f00d883cadbda03`. The public application is built
from this repository with `tools/build_smk37.py --public-release`.
Bluetooth remains excluded. No hardware was flashed during release preparation.

## Installation and packaging

The website publishes the GPL application and its SHA-256 manifest. Owners
select their original Pro 016 or Elite 016 `.fwsc` file; packaging runs locally
in the browser. Vendor firmware, private backups and calibration are not hosted.

Both official files are pinned by SHA-256. Pro has 36 wrapper marker blocks;
Elite has 20. The Pro markers overlap the start of flash.bin, so package parsing
and serialization must use logical offsets rather than a constant physical
offset. The new implementation handles this and revalidates the result.

First install requires the selected model's stock 016 identity and uses its
unaltered official OTA loader. Only app bytes and their checksums change in the
vendor flash payload. Every other vendor payload remains identical.
The browser verifies the returned Felucca `SMK-37_922` identity before success.

Subsequent public updates use format 2 and the application-only Felucca RAM
loader. It pins the shared boot-header CRC, validates the current instrument's
trim record, and preserves that instrument's calibration. The writable interval
remains `0x4000..0x9d000`; protected head and calibration CRCs are rechecked
after programming. Format 1 remains accepted for the existing private workflow.
Private Test11–Test21 loaders cannot accept format 2 and are refused before
START. Private previews remain accessible through `/?private=1` on loopback
only, with an explicitly staged package.

## Checks performed

- Jieli application and RAM loader built; entry, memory allocation, image size,
  RAM flash routines and exclusion of wrong-board routines checked by builder.
- Both official 016 packages: CRC/container round trips, unchanged boot region,
  unchanged non-flash payloads, rejection of modified input/app, and resulting
  format-2 package parsed successfully.
- ASan/UBSan loader tests: successful/no-op updates, three synthetic calibration
  records, malformed trim, wrong boot header/chip, corrupt input, preflight
  failure, changed host data, programming failure and retry.
- Both generated packages ran through the C loader against synthetic full-flash
  fixtures: 152 changed application sectors each; protected tail unchanged.
- Browser transport mocks: wrong model/version, first-stage failure, missing
  loader, write failure, wrong returned identity and port cleanup.
- Existing USB updater, USB MIDI input, analytics, browser engine, and 44.1/48 kHz
  worklet regression tests passed.
- Homepage loaded in Chrome with 0.9.22 release metadata, file selector and
  install-path selector; Install stays disabled before package selection/consent.

The public stock first-install path has not yet been confirmed on hardware.
Earlier development firmware was exercised on the Pro through Test21; those
reports do not establish installation success on other instruments. The owner
approved public beta publication with this limitation stated at installation.
Failed or interrupted installation can require hardware recovery.

TRS output still starts OFF after restart. Release notes/pedal before disabling
TRS; held receiver notes may otherwise remain on. Sample persistence after
restart and longer USB audio testing remain follow-ups.

## Reproduce checks

```sh
python3 tools/build_smk37.py --public-release --usb-test-number 22
sh tools/test_smk37_public.sh
node web/test_smk37_update.mjs
node web/test_smk37_public.mjs
# Optional local official-file fixtures (never commit them):
node web/test_smk37_public.mjs /path/pro016.fwsc /path/elite016.fwsc /absolute/path/build/smk37-webupdate-test22/manifest.json
build/public-tests/public build/public-tests/pro-synthetic-flash.bin build/public-tests/pro.smkupdate
build/public-tests/public build/public-tests/elite-synthetic-flash.bin build/public-tests/elite.smkupdate
```

The raw application `.bin` is not a complete device image and must not be
flashed directly. Use the installer to combine it with the correct official file.
