# Felucca SMK 0.9.23 Beta

## Changes

- Port TRS output preference persistence and tracked-note shutdown from Sloop SMK
  commit 7dc90af. First-time/legacy settings default OFF. Changes save while idle.
- Send CC64/120/123 and explicit Note Off for transmitted notes before UART stop.
  This cannot clean up a receiver after its cable has been disconnected.
- Expand user presets to U01–U32, including each slot's FM6 OWN patch, name,
  phrase and favorite. Studio discovers capacity from the device.
- Fix flash erase elapsed-time output: pass a valid pointer to the RAM driver.
- Remove the old Open Studio URL restriction that forced all tabs back to Sound.
- Rebuild Studio/simulator with the same 32-slot storage and browser-session support.

## Allocation and migration

Expanded A/B banks occupy 0xB1000–0xB3FFF and 0xB4000–0xB6FFF.
Each sector's final 256 bytes remain erased for the vendor boot scanner.
The original banks at 0xF3000 and 0xFE000 are retained during expansion.
The 2,564-byte and 3,116-byte legacy payloads migrate in RAM without boot writes.
The first user save writes and verifies the inactive expanded copy, then commits
its CRC-protected header last. Existing projects, samples, boot and calibration
regions are outside this allocation. Unknown expanded data keeps capacity at
8 and is not erased. Interrupted initial migration falls back to the old bank;
there is deliberately no automatic format/retry over unidentified data.

Downgrading to 0.9.22 exposes the retained legacy bank, not later U01–U32 edits.
Export important presets before changing firmware families or downgrading.

## Validation

- Full tools/test_smk37_application.sh host suite, with AddressSanitizer.
- Test23 exercises all 32 slots; eight-slot migration with FM6/favorites;
  47 erase/program interruption points and every partial commit-header length;
  unknown-region fallback; protected regions and erased sector tails;
  browser command bounds and TRS ON/OFF persistence.
- Browser WASM engine and 44.1/48 kHz worklet tests, including U32 session restore.
- Pro016 and Elite016 package reconstruction, protected byte comparison,
  checksums and corruption rejection.
- Upstream remains Felucca 1.5.1, a4d7ca2e5d52a4c35e2468401f00d883cadbda03.

No device was flashed in this implementation round. Hardware installation,
U32 restart retention, original preset migration and connected-receiver cleanup
still need checks on this artifact; host tests do not establish those results.

Application: 415,940 bytes.
SHA-256: 9263b5924b747d87a475cd095b1a44dde7faa3e0d69407eb14099f5e01dbeee6
Identity: SMK-37_923. Application entry 0x02000120.
BSS end 0x01C186FC; pool end 0x01C6CDD8 (within linker limits).
