# Felucca 1.6.1 integration - SMK candidate 0.9.29

This is an isolated, unpublished candidate on `integrate/felucca-1.6.1-smk`.
The published installer remains on the hardware-tested 0.9.28 application.
No device was flashed in this integration round. Older saves are not a release
requirement, as requested; current project and preference saving are tested.

## Source and scope

Fork base: `4a6b332` (published SMK recovery build). Musical baseline:
`a4d7ca2e5d52a4c35e2468401f00d883cadbda03` (1.5.1). Imported upstream target:
`a53ecf428c29c780108fd70a31b71a32a6b5c313` (v1.6.1).
This is an attributed source-delta import, not a merge of upstream ancestry.

Imported firmware, tools/build.py, table generation, host tests, editor descriptors,
protocol documentation, web/emu and licensing. Retained SMK control, MIDI, storage,
USB, remote-panel and display adapters. The existing site and installer are retained.
No Bluetooth code, new flash IDs, or unrelated queued features are included.

The upstream MIT Felucca-Engines submodule is pinned at
`c46e0086f69bdc26df1cd144dca9e683cc4531dd`. Initialize it with
`git -c core.autocrlf=false submodule update --init firmware/engines`. Source distributions must include
its contents, not only the Git submodule pointer; its license and author credits
remain intact.

Additional narrow upstream fix: `63f88ec18b71c6ed4371d03d6f04dd93f4193ad3`:
`<stdint.h>` for the Linux drum stress test and the zero-tempo division guard.
The missing header was reproduced while running the shared host suite.
No other post-1.6.1 changes were imported.

## SMK controls

- STEP DETAIL: K5 moves through chance percentages, then 1:2 through 4:4,
  FILL and !FILL conditions. Studio offers a separate Condition field.
- SEQ TOOLS adds EUCLID, DOUBLE, HALF and HOLD FILL. Select HOLD FILL and
  hold `>` to activate it; releasing it clears Fill even after leaving the page.
  Loss of a remote-panel connection releases its Fill hold while preserving a
  physical hold.
- EUCLID: K5 hits, K6 steps, K7 rotation, K8 drum lane; `<` cancels and `>` applies.
  It is also in the sequencer navigation group. Leaving the editor applies the
  preview. Double/half and Euclid retain pattern undo.
- MIDI CHANNELS: K5-K8 set tracks 1-4, channels 1-16, OMNI or OFF. Choose TRACK
  under MIDI IN to use them. These values are saved with the project, and are
  exposed through upstream MENU IDs 26-29 in Studio.
- Studio Settings adds custom palette preview, save, remove and cancel. All 23
  built-in Salt palettes and bold-font/monitor controls remain. CUSTOM appears
  only while stored. Commands 80/81 and the condition trailer use upstream IDs.
- Sound updates include PHASE folding, added drum kits, SEQ+T quantization and
  velocity/time modulation destinations. Existing 37-key and pad ownership,
  velocity, sustain, MIDI/TRS, sample slots, presets and remote screen remain.

The upstream opt-in second-core startup is FM-1-specific. This SMK candidate
keeps `FELUCCA_CPU2=0`, using the validated serial audio path. It does not claim
SMK dual-core support. The separate FM-1 Salt repository is unchanged.

## Build and memory

```
python3 tools/build_smk37.py --public-release --panel-mirror --usb-test-number 29
```

The full-feature build initially exceeded POOL by 672 bytes. Its two new static
sequencer scratch buffers (Euclid source steps and double/half automation plan)
now use spare ordinary RAM on SMK. Region boundaries, stacks and protected
flash ranges are unchanged. Remote screen support is retained. Both buffers
are initialized by the normal BSS startup path; no heap allocation was added.

The raw application is not directly installable. The existing public package
builder verifies its identity/hash and combines it with the appropriate official
Pro or Elite image. Vendor-containing packages stay in ignored local build
output and must not be published as source.

## Validation

Real hardware startup, audio load, controls and custom to original round trip
remain required before promoting this firmware to the public installer.
No upstream contribution or public release was submitted in this round.

### Artifact identity

- Product: `SMK-37_929`; version: `Felucca SMK 0.9.29 Beta`.
- Application: **429348 bytes**, SHA-256 `f3b040f765be378ce05e319943a08f20c6d382b74cc1485d259abf83f0636415`.
- BSS ends at `0x1c1929c` (3,428 bytes below its fixed boundary).
- POOL ends at `0x1c73e5c` (420 bytes below the stack guard).
- Remote-panel protocol 4, 28,800-byte framebuffer; USB audio, TRS,
  32 presets, project storage and user sample storage enabled.

### Completed checks

- Final hardware link: application and RAM bounds, entry instruction, call-free
  RAM flash routines and compiled write gates verified.
- SMK host suite: all 37 keys, pad/key ownership, velocity/sustain, scanner and
  buttons, MIDI/TRS, all sound pages, LCD bounds, preferences, presets, projects,
  samples, interrupted commits and real C-to-Studio protocol replies.
- New 1.6.1 adapter tests: condition encoder, momentary Fill, project routing and
  condition persistence, double/half undo, Euclid cancel/apply, invalid palette
  rejection and palette save/reload. Zero-tempo guard checked explicitly.
- Remote panel: physical/browser ownership, lease release, Fill disconnect with
  physical hold preserved, compressed screen batches, pixel equality and browser
  view/input/LED switching.
- Upstream seq155, fold155, ui155, v156, v16, editor, MIDI and project tests.
- Shared sound suite: 98 golden renders unchanged in serial/default/host-threaded
  variants, 108 standalone engine renders unchanged, 720 engine-switch sequences
  unchanged, synth/drum/noise stability, modulation/effects and voice routing.
- ASan/UBSan: randomized drum editing during synth/SONG playback; 20,000 editor
  fuzz cases, 5,000 project cases and 1,000 sample cases completed.
- Public/private SMK updater and flash-profile tests passed. Both official Pro
  and Elite fixtures produced verified candidate packages; protected bytes,
  CRCs, corruption rejection and synthetic native transfers checked. Vendor
  fixtures/packages remain in ignored build output.
- Studio in isolated headless Chrome using actual SMK WASM behind simulated MIDI:
  condition editing, track channel changes, custom palette save/remove and
  desktop/mobile rendering passed without script errors. Simulator/worklet tests
  passed at 44.1/48 kHz. All eight locales cover 516 matching keys.

Logs are under ignored `build/`: `application-tests.log`, `upstream-tests.log`,
`recovery-tests.log`, `package-tests.log`, `shared-host-tests.log`,
`shared-host-tail.log`, `engine-rerun.log`, `remote-browser-tests.log`.
The shared suite was resumed after the upstream stdint fix; its later aggregate
reported failure from Windows CRLF conversion of reference files and the engine
shell script. All three affected checks passed on rerun with upstream LF bytes;
no golden hashes changed. `.gitattributes` now keeps root golden files LF, and
engine checkout instructions disable CRLF conversion. The original failure logs
are retained, rather than rewritten as successful runs.

### Deliberate validation limits

`SMK_HOST_ONLY=1` omits FM-1 package/loader and FM-1 disassembly budget checks;
SMK has separate passing package/updater checks. The old Emscripten FM-1 emulator
was not built (emcc unavailable); the actual SMK Zig/WASM runtime was built and
tested. Host instruction counting is unavailable on this Linux host; no claim of
hardware audio timing or CPU budget follows from host renders. Physical Pro and
Elite installation, power cycling, sound, calibration and stock restore for this
exact application are not yet verified. Published firmware/helper pins remain
0.9.28. No backward-save hardware promise is made.


### Owner SMK Pro validation ? 2026-10-21

Candidate app SHA-256 `f3b040f765be378ce05e319943a08f20c6d382b74cc1485d259abf83f0636415`
installed over the native Windows transport from Sloop `SMK-37_954` and
restarted as `SMK-37_929`. The owner confirmed screen, controls and sound.
Real-device editor checks passed: condition set/readback, save/load in previously
empty project slot 1, custom-palette query and remote-panel state. After the
owner switched the unit off for five seconds and back on, loading slot 1
recovered the saved FILL condition. This test project remains in slot 1.
Original firmware restore then completed and the restarted unit replied
`SMK-37 Pro_016`. Local evidence: `build/hardware-929/hardware.jsonl`,
`features.json`, and `power-cycle.json`. Native transport was used; this is
not a browser installer or Chrome disconnect validation. Elite, FM-1,
long-run timing and detailed physical testing of every new tool remain untested.
Nothing was published. A separate naming correction is being prepared and
must not be conflated with this tested application hash.

## Public 0.9.30 release validation ? 2026-10-21

The final build is 429376 bytes, SHA-256 `e8ff418cb1eba18b92dd43b22eca08d8bd6e1e5165596c2775c08d749cf5b488`. It reports `Felucca SMK 0.9.30 Beta` and `SMK-37_930`. On the owner Pro it installed and restarted, Windows MIDI enumeration reported `Felucca SMK-37`, the existing conditional-step project loaded with FILL preserved, palette/live-panel queries replied, and generated MIDI notes produced non-silent stereo USB audio. Native restore completed and restart identity was `SMK-37 Pro_016`.

This follows the 0.9.29 controls/sound/save/cold-restart validation above. Cold power-cycle on final 0.9.30, physical Chrome disconnect, Elite/EB6014 and long-run validation were not repeated. Windows audio endpoint labels may remain cached as Felucca. No USB VID/PID, protected flash ranges or radio code changed. Private package/audio/log evidence stays in build/hardware-930; no vendor packages or private device data are published.
