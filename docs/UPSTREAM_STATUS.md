# Felucca SMK 0.9.23 update — 2026-10-11

Focused port from Sloop SMK 7dc90af: TRS persistence and tracked Note Off cleanup.
Upstream remains Felucca 1.5.1 at a4d7ca2e5d52a4c35e2468401f00d883cadbda03.
Adds guarded 32-slot preset migration and fixes the erase-driver output pointer.
Browser runtime rebuilt. No upstream PR; this work is specific to our SMK fork.
[Release evidence and hardware limitations](smk37/RELEASE_0.9.23.md).

## Elite loader handoff capture v2 — 2026-10-11

Contributor stock diagnostic evidence confirms matching Elite -Private input/output
names and a SMK-37 Elite_016 reply within about 2 ms on macOS. It does not capture
the RAM loader. Stock pairing works in that log; the loader failure remains undiagnosed.

The existing MIDI connection-check page now has a separate, explicit stock-loader
test. It verifies an allowlisted official Pro/Elite 016 file and its ota.bin hash,
requires the matching stock identity, and stages only the official header and
RAM-loader ranges. The diagnostic image contains no application/configuration
payload. It refuses out-of-range reads and never sends the second START that
begins flash programming. Normal installer model/pairing gates are unchanged.

After the stage-1 completion it observes all MIDI inputs and sends identity
queries to SMK outputs and newly appearing outputs, independently of port names.
Pre-existing unrelated outputs such as IAC are excluded. Logs include port IDs,
names, states, identity packets and request addresses, stay in the browser, and
are saved manually. The UI explains loader mode, a possible black screen, and
power-cycling after capture. No automatic operation occurs on page load.

Validation: host tests cover stage-1 bounds, invalid requests, one START only,
abort, timeout, disconnect, changed IDs/names, identity-only capture, and cleanup.
Both pinned official files pass optional full mocked handoffs and SHA-256 checks;
non-loader bytes in diagnostic packages are zero. Existing identity/update/public
installer tests pass. No hardware was opened, queried or flashed for this change.
Actual Elite/macOS handoff testing remains the purpose of the published tool.
Published firmware 0.9.22 and its hashes are unchanged; this is a website-only PR.

## Elite stock-loader handoff investigation ? 2026-10-11

Issue ChanceTheMaker/felucca-smk#4 reports stage-1 completion followed by an
undetected official Elite 016 loader on macOS/Chrome; owner reports return to
stock after power-cycle. Alternate identity and unequal MIDI port names are
hypotheses, not observed root causes. Do not loosen the installer's model gate.

Both allowlisted official Pro/Elite 016 files contain the same 19,969-byte
`ota.bin`: SHA-256 `ef1f2db89b011fa53e71df363f0122de0ea896618c53fb53e8da676322004541`.
Plaintext inspection did not establish its runtime identity.

Branch `diag/elite-loader-handoff` adds local read-only MIDI diagnostics and
installer logging of port inventories, unavailable pairs, open failures and
accepted/rejected identities. Diagnostics sends only HS_QUERY to the selected
output and observes identity replies on all inputs, independent of name pairing.
It cannot stage a loader, start an update or write flash. Logs stay local until
manually saved/shared. Existing updater pairing/model filters remain unchanged.
No firmware changes or device operations. Host diagnostics/public updater tests
pass. The owner tested the diagnostic on Windows Chrome: matching Felucca ports,
SMK-37_931 identity within 5 ms, both ports closed afterward. Actual Elite/macOS
loader evidence is still needed. Diagnostic website publication authorized; no
issue reply sent. Published firmware and updater acceptance gates unchanged.

Next evidence: after the reported handoff failure, close the installer without
power-cycling, open MIDI connection check in Chrome, enable SysEx access, select
the loader/SMK output and check identity. Save the log. If no MIDI ports appear,
record that result and the USB device description from macOS System Information.
A Windows/current-firmware probe checks the tool but cannot validate Elite OTA.

# Upstream integration status

## October 11 public SMK beta 0.9.22

Prepared the portable public application and Pro/Elite 016 browser installer.
No upstream import: baseline remains Felucca 1.5.1. Unit-bound calibration
fingerprints are replaced by model/layout and each instrument's own trim
validation. See [release evidence](smk37/PUBLIC_0.9.22.md) for exact tests,
retained limitations and the pending stock-hardware first-install check.
Public beta publication is authorized; no connected hardware was flashed.
No upstream contribution is included in this release.

Last reviewed: **2026-10-10**. Update this record when importing or offering a
feature; follow [UPSTREAM_WORKFLOW.md](UPSTREAM_WORKFLOW.md).

## October 10 SMK Test20 continuation

Master-volume odometer, saved-preset phrases, step audition and Studio tuning
are built and host-tested. Larger preset storage has an atomic backend and
conversion tests; the runtime flash gate still permits only existing storage.
Stock export/restore roundtrips privately. MIDI TX queue tests pass and the
stock UART1 TX DMA path has been located; pin/mux verification remains.
Test19 successfully produced two short stereo Windows recordings. See
[Test20 progress and limits](smk37/TEST20.md). Test20 is packaged privately,
not flashed or published; localhost remains on Test19.

## October 10 SMK Test19 continuation

Documented the hardware comparison with configured clocks distinguished from
runtime measurements. Added factory phrase selection/confirmation, sequence
timing undo, further panel settings and upstream MENU_DESC/SET descriptors in
Studio, plus the main-loop manual-slice adapter. Optional USB recording now
builds with the shared UAC implementation. Target, application host, renderer,
C-to-JS menu and UAC tests pass. See [Test19](smk37/TEST19.md).

Expanded storage is an allocation/migration plan, not enabled storage. Physical
MIDI routing remains under investigation; the documented SMK jack is OUT.
Private Test19 package prepared, with no device flash or publication. The
local installer was subsequently staged to Test19 at the owner's request;
the served package hash was verified. No Test19 flash yet. The owner reported
Test18 worked well. Upstream revision remains 1.5.1 `a4d7ca2`.

## October 10 SMK Test18 workflow and FM-1 fidelity

Owner reviewed Test17's display on hardware and reported it looks good. Test18
retains upstream 1.5.1 (`a4d7ca2`) and adds SMK hold shortcuts/grouped navigation,
settings/favorites in existing preset sectors, device naming, FM6 operator pages,
sound undo, note/event monitor and upstream rolling digits. Commands 34-38 match
the existing Salt editor protocol; no new command IDs or sample allocations.
The slice page shows the real waveform/markers; manual edits require user-sample
storage, which remains separate. Full FM-1 menu/layer parity is not claimed.
See [Test18](smk37/TEST18.md) for the migration, limits, tests and artifact hashes.
No flashing, commit, push, public publication or upstream submission this round.

## October 10 SMK Test17 missing page content

Shared FM-1 sound/instrument graphs now populate the SMK Scale, Chord, FX,
Slicer, modulation and engine centers. SMK sequence/storage/performance views
show their actual state. PATCH/Home, PARA/Save and BT/Edit shortcuts are included.
The local hero cycles red, white, Elite, black, pink, charcoal with crossfades.
Host, renderer, target and updater checks pass. See [Test17](smk37/TEST17.md)
for scope, remaining UI differences, hashes and physical checks. The local
installer staged the private Test17 candidate; no flash, upstream submission,
commit, push or public deployment.

## October 10 SMK Test16 branding and installer

The local SMK installer reuses the live fork site's button, gallery and hover-preview
styles (gh-pages deployment 79019a6). Original SVG art shows the Pro and Elite
finishes with the correct control count. All 24 Studio theme/mode thumbnails are
fresh SMK Browser-mode captures. The firmware boot line is now centered Felucca
SMK with the real version in smaller type at the bottom. No added startup delay.
Test16 passes the target build, combined host suite and updater regression tests.
The local server stages the private Test16 package; no device flash or publication.
See [Test16 details and artifact identity](smk37/TEST16.md).

## October 10 SMK Test15 device display

Test15 replaces the simplified sound-page list with FM-1-style cards, a compact
header, track overview and graphical panels. The ADSR/LFO/scope drawing is shared
with upstream; the SMK navigation remains board-specific. Host renderer and
combined SMK regressions pass, as does the original FM-1 UI suite. The target
build passes its bounds. See [Test15 scope, evidence and hashes](smk37/TEST15.md).
Staged at localhost:8770 with served package hash verified; no automatic flash,
push or publication.

## October 10 SMK Test14 completion

On the same 1.5.1 integration branch, Test14 adds per-track song chains,
upstream sequence tools with one-level pattern undo/redo, eight device preset
slots with FM6 patches, and the owner's confirmed FM-1-style first knob bank.
The two preset sectors are separate from the four existing project slots and
are enabled only with private stock evidence and runtime allocation checks.
See [Test14 controls, storage, tests and artifact hashes](smk37/TEST14.md).
The candidate is built locally; no flash, push or public release. At the owner's
request the local installer was staged with Test14 (superseded by Test15 above), with its served package hash checked. Browser and firmware support are tested with host
fixtures; the Pro device checks come next.

## October 10 SMK upstream 1.5.1 integration (Test13)

Branch: `integrate/smk37-upstream-20261010`, based on the tested Test12 source
`36875e41f236d243c984fd31839b1a864b4642eb`.
Upstream pinned at `a4d7ca2e5d52a4c35e2468401f00d883cadbda03` (1.5.1);
previous imported baseline `20c275e39f75fa820978032efaceddfc5283c8cb` (1.0.1).
This is a source-delta import, not an upstream merge. Imported paths: `firmware`,
`tests`, `tools`, `web`, `assets`, `LICENSES`, `LICENSING.md`, `BUILDING.md`.

See [the complete integration record](smk37/UPSTREAM_1.5.1.md) for SMK-specific
adaptations, hardware controls, save migration, test evidence and limitations.
No upstream contribution, public deployment, or Test13 device flash has occurred.
The local installer continues to stage Test12 until the new candidate is selected
for hardware testing. Product headings use SMK-37 Pro & Elite; the current private
binary is the Pro test-device package.

## October 10 SMK private beta and direct USB updater

Test12 normal-USB installation, restart and saved-project retention were
confirmed by the owner on the SMK-37 Pro. Test11 adapter bootstrap had full
flash readback verification; Test12 changed four application sectors, exercising
USB programming. Controls, sound, sustain, project persistence and two-way web
editing have also passed owner checks. Host driver/synth regressions, protected
flash/update tests and browser updater checks pass. See
[USB_INSTALLER.md](smk37/USB_INSTALLER.md) and
[CONTROL_TRACE.md](smk37/CONTROL_TRACE.md) for the evidence and its limits.

This branch contains source, tests and the local SMK landing page/installer.
Private stock backups, calibration and generated firmware packages stay outside
Git. It is prepared for the owner's fork; no upstream submission or public-site
publication is part of this checkpoint. Public packaging, stock first-install,
power-loss recovery and wider-device validation remain unfinished. The RAM
updater still requires the adapter after power loss during programming.

The dated entries below preserve earlier development stages and are superseded
by this checkpoint where hardware validation is now recorded.

## October 8 SMK sound interface and startup checks

The latest local build adds [sound controls](smk37/SOUND_CONTROLS.md): four-track,
engine and factory-preset selection, 23 editable sound pages and input
diagnostics. Encoder transitions use a separate bounded queue consumed by the
audio ISR; the physical encoder ordering/direction/detents remain unverified.
Startup and later preset loads now use the same complete preset initialization.
Tests exercise every selectable engine/factory preset with active voices,
held-key sound changes, track ownership, parameter bounds and navigation.
The real-synth AddressSanitizer suite, FM-1 chord/MIDI regressions and all 16
SMK driver suites pass. The processor build completes successfully.

Latest image: **338200 bytes**, BSS end `0x01C1550C`, pool end `0x01C6FA70`,
SHA256 `f9cce8a8fc54b18de01a2d003931df8e2ea1e726002f0252e69de5c3c96004dd`.
It remains analysis-only with no flash writer; no installation, publication or
device access occurred. This supersedes the earlier diagnostic images below.
Physical verification, calibration, remaining panel functions, persistence and
a verified recovery/install path remain unfinished.

Recovery follow-up is recorded in [RECOVERY_RESEARCH.md](smk37/RECOVERY_RESEARCH.md).
The official tool manual clarifies reset/handshake timing; battery-backed power
means USB cycling alone cannot be assumed to reset the board. The community
reader lists WL82 support as unknown, so its generic read command is not a
verified backup route. No recovery commands were executed; binary unchanged.

External ADC transport is now linked into the diagnostic application. SPI2
owns conversion polling and mux updates; ADC IRQ is disabled. The screen
rotates raw electrical inputs 01..24, and bounded conversion failure disables
ADC while digital scanning continues. Sixteen driver suites pass with
ASan/UBSan, including the joint ADC/scanner test. The processor build completes
without the initializer warning found during the first compile. Current image:
334088 bytes, BSS end `0x01C150EC`, pool end `0x01C6FA70`, SHA256
`f8f1c9fa462ce355aec4d23c16c7e46905008bcbcedff633d773c9059eaaa924`.
The manifest now explicitly lists analog mapping/settling/calibration as
unverified. This supersedes the helper-only state below. No device access,
flashing, push or publication occurred.

Analog follow-up: local SDK definitions identify ADC inputs PA8/PC9/PA10 and
the special `0x11900` bit as internal analog-test enable. Added an offline
external-input scheduler covering all 24 channel/mux pairs with three scanner
completions before conversion and explicit result ownership. All 15 driver
suites pass under ASan/UBSan. The helper remains unconnected to hardware; it
does not reproduce stock's internal-monitor path or pad sample cadence.
The application binary is unchanged from the digital-control build below.

Offline analog tracing now identifies ADC IRQ 24, its three-SPI-completion
selection handshake, shared shift-word mux bits 3..5, and channel banks 1/8/2.
The primary/secondary schedule and unresolved secondary 6..7 path are recorded
in CONTROL_TRACE.md. This resolves scanner/ADC coordination needed for the
remaining controls; analog hardware is still disabled. No new binary or device
operation was needed for this trace.

Digital-control follow-up: the integrated scanner decodes all 18 button
contacts and eight quadrature encoder pairs for display diagnostics. Button
debounce, encoder initialization, bidirectional cycles and invalid-transition
resynchronization pass offline tests (14 driver suites with ASan/UBSan).
Physical names/direction, detent scaling and timing are not inferred from these
tests, and no sound-control assignment is made yet. Current image: 333408 bytes,
BSS end `0x01C1508C`, pool end `0x01C6FA70`, SHA256
`277f05b4063a29443187ccc57ba05bf82412cdcdc98d97be8dcddb8ef26fdcc7`.
This supersedes the startup-only images below. No flashing or publication.

Further startup review found that clearing BSS erased the early reset-source
snapshot. Startup now retains the pre-watchdog reset/watchdog values across
initialization and explicitly initializes the early P33 timeout counter. It
also copies the linker RAM-text range before peripheral startup and issues
`csync`; that section is currently empty. Thirteen driver suites pass with
ASan/UBSan, including startup copy/zero boundaries and empty sections. Processor
disassembly confirms inline initialization loops, synchronization, and restored
diagnostic values. The current offline image is 332720 bytes, SHA256
`a9b9b4e4579912336360dc2eff9ce4238023733c67c3af53ccd105b39de60815`;
BSS/pool endpoints are unchanged from the keyboard-handoff build below.
No hardware execution or recovery guarantee follows from these checks.

The follow-up keyboard handoff preserves full contact durations in a bounded
scanner-to-audio event queue. A loss latches until the consumer discards the
incomplete history and releases all local keys; the display counts these loss
episodes. Fixed velocity remains deliberate until unit calibration and timing
are verified. Twelve driver suites pass with ASan/UBSan, including 100 complete
37-key press/release cycles, counter wrap, capacity and loss recovery. The real
synth test also checks all-local-key release and retrigger after a lost burst.
The rebuilt offline image is 332636 bytes, BSS end `0x01C1505C`, SHA256
`68b756411d862e5225ce598001205da3407fb2efc412e54c168fdb6775110809`.
This supersedes the image hash below. No device access or flashing was performed
in this follow-up; the owner explicitly requested continued offline work.

The offline SMK application now records startup stages in retained RAM. Two
failed warm-start attempts request the stock software-update mailbox path
before starting peripherals. Thirty seconds of operation with recent scanner
and audio progress clears the pending-start flag; intentional USB update entry
does not count as a crash. Cold power-on and invalid retained state reset the
counter. The diagnostic screen includes the previous boot stage. This policy
is host-tested; SRAM retention, P33 reset-source behavior and successful updater
entry are still unverified on this device. It is not a verified hardware
recovery mechanism.

SMK startup also establishes TIMER4's full period and oscillator /1 divider
explicitly instead of accepting a possibly incompatible inherited timer.
Eleven driver suites pass with ASan/UBSan, including the new boot policy and
timer-inheritance tests. The updated processor image links successfully:
332476 bytes, BSS end `0x01C14E5C`, pool end `0x01C6FA70`, SHA256
`1fcd965c5ad659da29dea3261628c37afbe144fe1d1aae1f86c3d228d1d51aea`.
HAL boundary and whitespace checks pass. No flash package is generated.

Read-only Windows inspection found no present device matching the previously
observed SMK VID or RP2040/Seeed recovery-adapter identifiers, and MIDI input
enumeration returned an empty list. The owner has been asked to connect the
stock SMK and confirm recovery-part availability. Hardware validation and a
unit calibration backup cannot be established from the offline image/tests.
No device commands, firmware writes, pushes or publication occurred.

Subsequently the owner reconnected the keyboard and confirmed the recovery
parts have not arrived. Windows now enumerates all three SMK MIDI inputs;
the new USB snapshot reports `4353:B64B`, audio `00` and MIDI `03`, all `OK`.
This removes the disconnected-device obstacle, but does not establish forced
recovery or a calibration backup. The difference from the Pro016 compiled
PID template is recorded in the interface/arrival notes; no flash-selection
rule is inferred from either PID. No outgoing device commands were sent.

## October 7 SMK arrival analysis

Local branch `feat/smk37-arrival-analysis` extends the interface-research stack.
[Arrival results](smk37/ARRIVAL_RESULTS.md) record receive-only stock MIDI tests,
owner-reported successful Pro 016 installation and audible synthesis, photographed
settings, and port/bank distinctions. [Startup analysis](smk37/STARTUP_ANALYSIS.md)
records the decoded Pro/FM-1 entry-path comparison and source boundaries needing
an SMK implementation. No upstream revision changed.

The follow-up adds a [board-photo inventory](smk37/BOARD_PHOTOS.md) and a
hash-pinned offline startup/RAM mapping tool. All 52 SMK preparation tests pass;
the mapped disassembly identifies documented scanner/display candidates,
including GPIO reads and a 16-entry selection table. These are static findings,
not hardware-validated pin assignments;
the standard SMK packaging target still refuses installation-package generation.
Further tracing identifies a 37-pair contact consumer, eight quadrature pairs,
and timing/response logic. `tools/smk37_controls.py` extracts the pinned tables
offline. Physical ordering still needs hardware verification; normal note and
MIDI velocity conversion have now been traced and implemented independently.
Follow-up tracing adds 18 distinct button contacts, ADC ordering leads, response
defaults and a 548-byte persistent trim-block path containing key response data.
The owner reports stock operation is fine after reassembly. No device writes
were performed; physical storage location and recovery remain open.
The first isolated firmware components now decode the 108 digital contacts and
convert calibrated key response to velocity/note values. Native C tests pass
with address/undefined-behavior sanitizers; a freestanding pi32v2 compilation
emits 1822 bytes of code and 308 bytes of constants, with no unresolved
external symbols. The read-only trim decoder now validates all 37 key curves
and converts contact duration using the supplied calibration; tests use a
synthetic fixture, not an actual unit backup. The stock trim file is identified
as `mnt/sdfile/app/usrtrim`; its physical address conversion is still being
traced. The dual-contact debounce/timing component also passes host and target
compilation checks. Stock key processing is triggered by a complete 16-slot
SPI2 scan via software IRQ 2; its physical cadence remains unmeasured.
These components are now connected to the offline application's SPI2 scanner;
that scanner has not been executed on the physical device.
The separate SMK ALNK0 audio transport is implemented from the stock channel-3
setup and existing credited GPL transport. Its register simulation passes;
physical audio and clock configuration remain unverified. The separate SPI1
LCD transport adds bounded transfers and timeout cleanup, with no backlight
assumption. Seven C suites pass, including panel startup/clear sequencing and
failure injection at every transfer. Panel callbacks are now connected to the
separate offline SMK application entry; visible output has not been checked.
Further stock tracing confirms boot information at 0x01C7FD50, vectors at
0x01C7FE00 and software update-entry mailbox 0x01C7FD80 with the
`usb_update_mode` marker and P33 reset. Forced hardware recovery is still
unverified; the owner has been asked whether the recovery parts have arrived.
There is no custom hardware execution, installable firmware build, flash, push
or PR submission. This is an intermediate implementation milestone, not completion
of the owner's requested SMK firmware.
Raw captures and vendor binary/disassembly artifacts remain local and ignored.

The application integration now links `firmware/src/smk37_app.c` using the
real Felucca DSP/sequencer, shared AC79 CPU/USB routines, and separate SMK
audio/display transports. `tools/build_smk37.py` produced a 332008-byte image
at entry `0x02000120`, BSS end `0x01C14E4C`, pool end `0x01C6FA70`.
Its manifest explicitly marks it non-installable. No package/loader is built;
the application contains no flash writer, FM-1 ADC/scanner, TRS UART or radio.
The linker allocation remains a conservative hypothesis, not measured SMK RAM.
Shared local-note arrays now have a board-supplied size (FM-1 default 27), and
key note-on accepts velocity. The separate 37-key event adapter passes its
ASan integration test with actual rendered audio, including upper keys,
velocity, duplicate-note ownership and release after track/note changes.
The stock-derived SPI2 scanner now feeds debounced key events through a bounded
IRQ queue to the audio consumer. Bring-up uses explicitly fixed velocity 100;
factory defaults are not substituted for unit calibration. Queue overflow
releases local notes rather than retaining a lost release. Boot startup copies
the stock SPL handoff fields to the reserved boot-info area before taking over
vectors. A diagnostic screen identifies the build and shows scan/audio/late
counters. Full SMK control menus are still pending.
Nine driver suites pass with ASan/UBSan; the real-synth test passes with ASan;
existing FM-1 chord and MIDI-control/clock integration suites pass unchanged.
All 52 Python SMK tests and the HAL register-access boundary check pass.
Build SHA256: `6e0bdd955a107e0f571df25791399a6e419e35c4aed001ceb0ff3ba1ef971e8b`.

Forced recovery, board wiring, calibration preservation and available RAM remain
unverified. The earlier pre-arrival notes below describe the preceding rounds.

## Earlier preparation rounds

SMK-37 preparation begins separately from RC1 at fork commit
`7b1164abd2322c279faf9bb57ae22283466b9f53`. The shared Pro/Elite target is
reserved, with hardware/recovery prerequisites documented in
[SMK37_PREPARATION.md](SMK37_PREPARATION.md). The default FM-1 build remains
unchanged. This round imports no firmware, has no SMK hardware validation,
and does not promote RC1 or submit work upstream.

Preparation validation: 19 offline tests cover reserved-target behavior,
container corruption/bounds, and receive-only MIDI recording/analysis. Thirteen
pinned stock-package samples were inventoried; Pro/Elite v16 share the OTA
loader but have different flash payloads and product markers. The existing
FM-1 installer simulation passes; its built-package JavaScript comparison and
official V15 restore are skipped in the new worktree because those artifacts
are absent. The isolated MIDI environment uses mido 1.3.3/python-rtmidi 1.5.8;
dependency checks and input enumeration pass, with no device connected. Physical
recovery, scanning, audio and calibration behavior remain untested.

The preparation stack is published as fork draft PRs
[target and plan #39](https://github.com/ChanceTheMaker/Felucca/pull/39),
[package inspection #40](https://github.com/ChanceTheMaker/Felucca/pull/40), and
[arrival diagnostics #41](https://github.com/ChanceTheMaker/Felucca/pull/41).
They are stacked in that order on the RC1 candidate branch, not merged to main.

Follow-on [draft PR #42](https://github.com/ChanceTheMaker/Felucca/pull/42),
`feat/smk37-flash-layout`, adds offline JLFS/application inspection and
boot-configuration decoding. All 13 archived packages pass the implemented
checks; 11 additional tests bring the preparation suite to 30. Pro/Elite v16
share boot settings and region descriptors, but their decoded application
files differ. FM-1's main store address falls inside the SMK stock application
span, so the reserved-target build guard remains in place. See
[flash and boot evidence](smk37/FLASH_LAYOUT.md). No firmware is built or flashed.

[Draft PR #44](https://github.com/ChanceTheMaker/Felucca/pull/44),
`feat/smk37-arrival-evidence`, adds read-only Windows USB snapshots, offline
before/after comparisons and a local arrival-report template. Instance paths
are hashed before saving. All 37 preparation tests pass, including seven USB
tests; actual Windows enumeration saved a 26-node host baseline locally without
an SMK attached. This does not validate SMK interfaces, recovery or any board
pinout. Stock-manual bank/latch distinctions are added to the capture guide;
forced firmware recovery remains unverified.

[Draft PR #46](https://github.com/ChanceTheMaker/Felucca/pull/46),
`feat/smk37-interface-research`, extends that stack with an offline USB descriptor
and diagnostic-string scanner. All 13 stock packages were inspected; Pro/Elite
v16 have different candidate PIDs, while earlier versions and other models
share IDs. These are compiled-in candidates, not live enumeration. Seven new
tests bring the preparation suite to 44 passing tests. See
[interface evidence](smk37/INTERFACES.md). No peripheral pinout or recovery
sequence is established, and the SMK build guard remains in place.

The separate [Sloop review PR #45](https://github.com/ChanceTheMaker/Felucca/pull/45)
documents the FM-1 fork comparison and proposed backup/installer switching
features from the RC1 base. It imports no Sloop firmware into this SMK stack.

The separately requested saxophone options are documented and queued in
[draft PR #43](https://github.com/ChanceTheMaker/Felucca/pull/43), based directly
on RC1 rather than the SMK stack. It proposes sampled-tenor improvements,
expression, a browser reed prototype and measured hardware integration; it
contains no new engine implementation.

Installer follow-up on `feat/rc1-installer-retry`: failed discovery, denied MIDI
permission, interrupted updates and stopped resumes show Try Again below the
output. Retrying requests MIDI access and rediscovers the device, hides stale
actions and locks installation controls. Success offers Edit in Studio. Official
recovery retries preserve the selected package and repeat backup/confirmation
checks. Eight-language labels and desktop/mobile simulated-device checks cover
both Salt14 and RC1. This website-only follow-up does not rebuild firmware or
change the draft RC1 release assets. The owner authorized merging the companion
[PR #38](https://github.com/ChanceTheMaker/Felucca/pull/38) into the candidate
branch while [PR #37](https://github.com/ChanceTheMaker/Felucca/pull/37) publishes
the same website feature with Salt14. Candidate firmware promotion remains separate.

## 1.0.1 candidate update

Pinned source: upstream v1.0.1, `20c275e39f75fa820978032efaceddfc5283c8cb`,
compared with the v1.0 baseline below. This is a selective source import,
split into lighting, scale quantization, and candidate build/version PRs.
Lighting is [PR #34](https://github.com/ChanceTheMaker/Felucca/pull/34), followed
by [QNT SEQ #35](https://github.com/ChanceTheMaker/Felucca/pull/35). The final
version/build record is on `feat/1.0.1-rc1-candidate`. These extend draft PR #28.

The lighting import adds DIM/INV preferences, idle glow, green PLAY indication,
and selected-track sequencer/ARP note LEDs. Salt font, monitor, HOLD and palette
preferences remain independent; their combined persistence is regression tested.
The hardware menu scrolls its ninth row into view, retaining readable fonts and
fixed footer controls. Input, settings, backup, UI behavior and renderer checks
cover the combined implementation. No MIDI receive or Bluetooth code is imported.

The scale import adds QNT SEQ to firmware and Studio's mock descriptors. It snaps
melodic sequence playback to the current root/scale without rewriting stored
steps, deduplicates collapsed pitches, and releases the mapped notes correctly.
Drum lanes and engines with their own key maps remain unchanged. Upstream scale
and UI regression coverage travels with the import; the browser DSP is rebuilt
from the combined source before packaging this candidate.

Validation: complete host suite; 87 unchanged golden renders; target budgets;
115 UI screens across 21 palettes; combined LED/font/monitor/HOLD round trips;
Studio descriptors/protocol, package/update and backup simulations; rebuilt WASM
audio; eight languages/479 keys; analytics isolation. Candidate artifact identity:
`1.0.1-salt1rc1`, `FM-1_910`, 609,849 bytes. See the candidate bundle's
`VALIDATION.txt` and `SHA256SUMS` for source identity and artifact checksums.

The [feature inventory](FEATURE_INVENTORY.md) records 40 feature groups and
proposed upstream contribution boundaries; [1.0.1 review](UPSTREAM_1.0.1_REVIEW.md)
records the pinned update. No new upstream PR is authorized or submitted.

Website follow-up: Install opens the initially collapsed Install Progress
accordion, scrolls to its output, and confirmed
success reveals an Edit in Studio link. Desktop/mobile, resumed success, failure,
retry and translations are tested with a simulated updater. The owner authorized
porting the UI round to the fork's main, separately from this RC1 stack. Website PRs #30–33 were merged into fork main `5874362` and published as
`gh-pages` deployment `f370b0c`, retaining Salt14. The earlier RC1 draft assets
remain unchanged; this update has a distinct candidate package.
The header logo also links home from both pages, with its typography preserved
and keyboard navigation checked on desktop and mobile. The compact hero preview
stays clear of both buttons. Section links and Install Progress scroll smoothly,
respecting reduced-motion preferences. These UI features are separate fork PRs.

## Baseline and release state

- Integrated upstream: [hugelton/Felucca v1.0.1](https://github.com/hugelton/Felucca/tree/v1.0.1),
  commit `20c275e39f75fa820978032efaceddfc5283c8cb`.
- Prior integrated baseline: v1.0, `727f272015da26eb2d0291bd652eba28ff57cb37`.
- Earlier upstream reference: `1e838e1` (0.9 plus white-key scale changes).
- Published Salt firmware: **0.9-salt14**. Current website updates retain it.
- Integration candidate: **1.0.1-salt1rc1**, hardware validation pending.
- Foundation: [PR 16](https://github.com/ChanceTheMaker/Felucca/pull/16), a pinned
  source import, not a merge of upstream Git ancestry. The baseline above is
  therefore the authoritative comparison point for the next upstream sync.
- Remaining candidate stack: TRS [#17](https://github.com/ChanceTheMaker/Felucca/pull/17),
  display [#18](https://github.com/ChanceTheMaker/Felucca/pull/18), Studio
  [#24](https://github.com/ChanceTheMaker/Felucca/pull/24), installer
  [#25](https://github.com/ChanceTheMaker/Felucca/pull/25), browser DSP
  [#26](https://github.com/ChanceTheMaker/Felucca/pull/26), regressions
  [#27](https://github.com/ChanceTheMaker/Felucca/pull/27), and final website/docs
  [#28](https://github.com/ChanceTheMaker/Felucca/pull/28). Keep these draft until
  candidate hardware validation is complete. Website PRs #15 and #19–23 are
  already merged; the live installer still serves Salt14.

## Features and retained differences

| Area | Current decision | Upstream contribution state |
| --- | --- | --- |
| 1.0 engines, FM6, songs, motion, modulation, quick layers, USB audio | Import upstream implementation and migrations | Upstream source; credit its authors |
| TRS receive ring | Retain content-driven reads with upstream overflow/timestamp handling; fork PR 17 | Potential focused contribution; check upstream before proposing |
| Palettes, fonts and MIDI monitor | Retain 21 palettes, regular/bold font and Events/Notes monitor; fork PR 18 | Additional Salt behavior; split any proposed contribution by feature |
| MIDI, palettes, favorites already credited in upstream README | Reuse current upstream behavior and test Salt extensions | Do not resubmit already adopted work |
| Protocol | SONG 33, preferences/favorites 34–38; capability detection | Follow upstream allocation; experimental Salt command 33 retired |
| Editor and installer | Current protocol, FM6/song/motion, sample trim/recording, backup-first recovery | Upstream functionality integrated into Salt interface |
| Browser synth | Rebuild from 1.0.1 DSP; 13 selectable engines, 69 presets, imported FM6 sounds | Fork feature; a contribution needs an independent upstream proposal |
| Website, themes, branding, analytics | Maintain independently in the fork | Not selected for upstream submission |
| Bluetooth experiment | Excluded from integration and release | Not selected for upstream submission |

The complete firmware host suite passes on the candidate, including 87 golden
renders and target disassembly budgets. Browser DSP and editor/backup tests pass.
Real-device installation and MIDI validation remain pending; optional DaisySP
reference checks and actual vendor V15 restoration were not run. Detailed build
evidence accompanies the candidate PRs and release artifacts.

## Contribution ledger

Private SMK follow-up: [Test21](smk37/TEST21.md) continues the pinned Felucca
1.5.1 adapter with initialized sample storage, upstream sample/slice formats,
stock-derived TRS output and three additional MENU settings. Host/build/Studio
checks pass. Owner testing confirmed TRS notes/chords, sustain and pitch into an
FM-1, plus sample slot initialization, upload and reported playback. TRS shutdown
with a held note failed; mod and sample restart persistence remain inconclusive.
Deferred follow-ups are in WORK_QUEUE.md. Test19 completed two 90-second stereo
USB captures. The owner authorized committing and pushing the accumulated
Test15-21 work to the fork integration branch; no upstream PR or public release
is included. Private stock backups and unit-specific firmware packages stay local.
The only new shared-core hooks are an optional MIDI-output tap and a configurable
slice-slot address; both retain the default FM-1 behavior.

No new upstream PR has been authorized or submitted during this round. Preparing
and documenting candidates does not imply upstream acceptance. For each future
contribution, append the following information:

| Feature | Upstream base SHA | Fork branch or PR | Upstream issue or PR | State and next action | Evidence |
| --- | --- | --- | --- | --- | --- |
| Content-driven TRS receive ring | Recheck current main | `fix/1.0-trs-ring`, fork PR 17 | None submitted in this round | Candidate for a focused proposal | Host parser/ring tests; combined hardware validation pending |

After upstream merges a contribution, record the merge SHA and the Salt PR that
removes or reconciles the duplicate implementation.

## Private SMK Test09 work (October 10)

Continues the existing SMK worktree base; no upstream import, PR, push or website
publication in this round. Test08's eight knob directions and performance checks
are recorded in smk37/CONTROL_TRACE.md. Test09 uses existing SysEx command IDs
for sound/track/step editing with OTA disabled, adds generic velocity curves and
RAM step/pad banks, and prepares four project slots with runtime writes blocked.
Pedal mapping and runtime flash reads require the next hardware pass. Details,
limitations and save-enablement prerequisites are in smk37/SOUND_CONTROLS.md.

Test09 hardware follow-up: owner confirmed velocity response, pedal ADC23, runtime read probe, bidirectional editor, lit step editing/playback and pad bank2. Test10 SAVE is a private sustain/persistence candidate with separate save-only adapter recovery preparation. Tests pass; adapter hardware verification, new-image approval and physical save/load/reboot tests remain pending. No push, PR or publication.

## Installer readiness fix (2026-10-11)

Upstream remains Felucca 1.5.1 (`a4d7ca2e5d52a4c35e2468401f00d883cadbda03`).
Website-only change: automatic Pro/Elite package preparation, cache revision r4,
and local Test21 migration selection. The published 0.9.22 application is unchanged.
Both models reached ready status in Chrome; updater regression tests passed.
The owner installed the private migration successfully with verified readback and
returned identity SMK-37_922. Stock first-install hardware confirmation remains
pending. Private packages stay outside Git and Pages. Fork-only contribution.

## Website family navigation — 2026-10-11

Added footer links among SMKLab, Felucca SMK, Sloop SMK and Melodee SMK on the homepage, Studio and simulator. Website-only change; published firmware and protocols remain unchanged. Local link/markup checks pass. No upstream contribution is intended for this branding/navigation change.
