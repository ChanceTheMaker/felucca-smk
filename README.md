# Felucca SMK

Felucca SMK by Chance Roth (@ChanceTheMaker), based on Felucca by Leo Kuroshita
and Hügelton Instruments. This branch integrates **Felucca 1.5.1 into the SMK-37
Pro & Elite project**, based on upstream `a4d7ca2`.

The standalone project lives at
[ChanceTheMaker/felucca-smk](https://github.com/ChanceTheMaker/felucca-smk).
Start with [SMK development and dependency setup](docs/smk37/DEVELOPMENT.md).
The original Felucca history, licenses and credits are preserved.

**Current product version: Felucca SMK 0.9.21.** Website labels and the next
device splash use this version. Existing private Test19–Test21 packages retain
their original identities; this naming change does not rebuild, publish or flash
them. The browser-only preview is at `http://localhost:8773/`.

**Private hardware test history:** The owner reported Test18 worked
well on the SMK. Test18 added grouped navigation, independent
display/control settings, presets/favorites and naming, FM6 operator editing,
sound undo, sustained-note monitoring, and upstream rolling-digit animation.
See [Test18 controls, compatibility and test notes](docs/smk37/TEST18.md).
**Test19 is staged privately** with additional panel/Studio settings, factory
phrases and optional USB recording. The local installer now serves Test19;
Test19's USB recording path has been exercised on the device. See [Test19 implementation and remaining work](docs/smk37/TEST19.md)
and the [FM-1/SMK CPU, RAM and flash comparison](docs/smk37/HARDWARE_COMPARISON.md).
The next private [Test20 build](docs/smk37/TEST20.md) adds the master-volume
odometer, saved-preset phrases, step preview and Studio tuning. It is prepared
separately; the installer still serves Test19. Test19 has recorded sound through
Windows at requested 44.1/48 kHz, including 90 seconds at each rate; native-rate checks remain.
The separate [Test21 candidate](docs/smk37/TEST21.md) includes those Test20 changes,
three explicitly initialized sample slots, persistent manual slice markers,
opt-in physical MIDI output and additional Studio settings. It is a private
hardware-test candidate, not a public release; Test19 stays on port 8770.
It retains the Test14 features below. The owner's Pro passed Test12's
normal-USB update, controls, sound, sustain and project-save checks. Test14 adds
upstream 1.5.1 synthesis/effects, sequencing details, automation and MIDI control,
plus song sections, sequence tools/undo, eight device presets and FM-1-style
bank-1 controls. See [Test14 controls and testing notes](docs/smk37/TEST14.md)
and [upstream integration details](docs/smk37/UPSTREAM_1.5.1.md).

The private installer package remains matched to the backed-up Pro test device.
It is not a stock-first-install image or a public firmware release. Public-site
publishing and hardware flashing are separate from this source integration.
Experimental Bluetooth changes remain excluded.

Development follows the [upstream workflow](docs/UPSTREAM_WORKFLOW.md): receive
updates in focused PRs, keep Salt differences explicit, and prepare selected
contributions on clean upstream branches. See [upstream status](docs/UPSTREAM_STATUS.md)
for the reviewed revision, retained features, and contribution ledger. The
[feature inventory](docs/FEATURE_INVENTORY.md) documents Salt additions and
contribution choices; [upstream 1.0.1 review](docs/UPSTREAM_1.0.1_REVIEW.md)
explains the new lighting and sequence-scale features.

## Studio and website

The Studio offers connected-device editing, a browser Studio, and an SMK
simulator. The browser runtime builds the native SMK synthesis, display, editor,
sequencer and storage code as WebAssembly. Projects, presets and sample slots
in browser mode use virtual storage, with session export/import and local saving.
The standalone simulator reproduces the 37-key panel, pads, knobs, faders,
wheels, buttons and lighting. It supports key glissando, model finishes,
fullscreen and a replayable tutorial. Browser audio may require the first user
gesture before playback is permitted.

The website retains themes, light/dark modes, the oscilloscope, grouped movable
Sound controls, a docked keyboard and eight interface languages. Its SMK hero
cycles through Pro and Elite finishes and links to Studio and Simulator.
The prebuilt browser runtime is included; no device or vendor firmware is needed
to try it locally. See the [setup guide](docs/smk37/DEVELOPMENT.md).

The editor follows upstream's protocol: **SONG is command 33; preferences and
favorites are commands 34–38**. Capability detection supports upstream 1.0 and
Salt's display extensions. The unshipped experimental Salt command-33 layout
is intentionally not supported.

Candidate validation and the remaining device checks are recorded in
[the integration report](docs/SALT-1.0-CANDIDATE.md).

[![License: GPL-3.0-only](https://img.shields.io/badge/license-GPL--3.0--only-blue.svg)](LICENSE)
[![Sponsor](https://img.shields.io/badge/Sponsor-ea4aaa?logo=githubsponsors&logoColor=white)](https://github.com/sponsors/hugelton)

![Felucca 1.0](docs/felucca-1.0.png)

Felucca SMK adapts the multi-engine Felucca synthesizer to the M-VAVE SMK-37
Pro & Elite. Use Chrome or Edge with a USB data cable for the staged SMK updater.
Stock firmware and device-specific backups are not distributed here. Report issues in
[the SMK issue tracker](https://github.com/ChanceTheMaker/felucca-smk/issues).

- Local SMK installer/Studio/simulator: `python tools/serve_smk37.py --port 8773`
- SMK setup: [development guide](docs/smk37/DEVELOPMENT.md)
- Related FM-1 website (separate project): [Felucca Salt](https://chancethemaker.github.io/Felucca/webapp/installer/)
- Upstream FM-1 build reference: [BUILDING.md](BUILDING.md)

## Upstream FM-1 reference

The following inherited feature and control reference describes the FM-1.
For SMK control mapping, storage sizes and hardware-test results, use the
linked Test18–Test21 notes above. For example, SMK currently has eight user
preset slots rather than the FM-1's 32. Do not install an FM-1 package on an SMK.

### Features

- **Thirteen engines** (below), each with its own factory presets
- **Four tracks**, one synth part each with its own engine and sound (drums are the DRUM engine or
  the SAMPLE engine's GM kit); 8 voices shared between them. ALGORITHM selects the track on every page
- **Sequencer:** 64 steps per track with chords, ties, accent, slide and per-step chance; a piano
  roll of the steps; a drum grid (white keys = steps, black keys = lanes); motion recording of knob
  moves; live loop recording with overdub; divisions from 1/32 to 4 bars; loading a sound never
  touches your patterns
- **Songs:** chain patterns A–D
- **Chord keys:** one finger plays an in-key chord (triads or sevenths of the scale, or fixed chord
  shapes), with voicings; on the keys, MIDI in, recording and the arpeggiator
- **Arpeggiator** with REPEAT and a beat LED, 16 scales with a white-key mode, glide,
  MONO / LEGATO / UNISON
- **Modulation matrix:** 4 slots per track, MIDI controllers as sources
- **Effects:** distortion and the SLICER per track; chorus, delay and reverb sends (the reverb as
  ROOM or SPRING); master limiter
- **FX layer:** hold FX for repeat, reverse, filter sweeps, tape stop, freeze and a harmonizer
  (OCT UP / OCT DN with shimmer), and mutes on the black keys
- **Quick layers:** hold FX, GLO, SCL or EDIT for shortcuts on the keys and knobs; one-step undo
  (SAVE held); REC on every page; OCT+ confirms, OCT- goes back
- **Presets:** factory presets, 32 user preset slots and 4 projects, named on the device;
  projects from every earlier version load
- **Screen:** flat UI with Inter Tight and Fukiai icons, 21 palettes including grayscale and high contrast,
  regular/bold text, and the Salt MIDI monitor
- **LEDs:** DIM idle glow or INV lighting (MENU > LEDS), green PLAY while running,
  and selected-track sequencer/ARP note feedback on the keys.
- **USB:** class-compliant MIDI in and out, and a 44.1 kHz stereo audio input ("Felucca") that
  records the master output on the computer, no driver needed
- **MIDI:** USB and TRS MIDI in; channels 1–4 play tracks 1–4 (other channels the selected track),
  and the keys send on the track's channel; pitch bend, sustain, panic; clock from internal, USB or TRS
- **Web:** editor for every parameter (with a 6-operator FM patch editor), step grid, mixer,
  preset library, sample upload and recording with trim; full backup and restore; return to the
  official firmware

## Controls

![FM-1 controls with Felucca 1.0](docs/panel.jpg)

- **SELECT** sets the BPM, **MASTER** the volume, **ALGORITHM** picks the track (T1–T4) and
  **PRESETS** its sound. **KNOB 1–4** edit the four columns of the page
- FX, SCL, ENV, LFO, EDIT, GLO, SAVE, ARP and SEQ open their pages; press again for the next page.
  HOME returns home
- **Held:** FX, GLO, SCL and EDIT open their quick layers; SAVE is undo, HOME the menu, SEQ the song
- PLAY starts and stops all four tracks; REC arms the selected track, on every page
- OCT− / OCT+ shift the octave (both: reset). On action pages, in dialogs and the menu, OCT+ does it
  and OCT− goes back
- Save a sound: stop, tap SAVE, pick a slot with KNOB 1, then OCT+ and OCT+ again (name it with the keys)

## Engines

In the order the device lists them:

- **ANALOG**: virtual analog; two oscillators, noise, drive, resonant low-pass filter
- **FM6**: classic 6-operator FM (Dexed-based): 32 algorithms, a full patch per track edited in the
  web editor, macros on the device, an algorithm chart on screen
- **PHASE**: phase distortion (ported from CrispyZebra)
- **LOFI**: chiptune; pulse, triangle, saw, noise and a 4-bit wave RAM, stepped envelope, sweep, arpeggio
- **SAMPLE**: multisampled instruments, a GM percussion set and 3 user sample slots
- **VOICE**: formant oscillator, sung vowels
- **TRIO**: 3 oscillators with ring modulation and sync, multimode filter
- **WHEEL**: tonewheel-style organ; drawbar registrations, percussion, key click, drive, rotary speaker
- **GRAIN**: granular textures from the built-in samples or a user slot
- **PHYS**: physical models: modal resonators, strings, struck membranes, sympathetic strings
- **NOISE**: noise from analog to digital: colours, crackle, shift-register and metallic tones
- **SLICE**: a drum break or your own sample cut into slices, one per key; set the slices by hand
  on the SLICES page
- **DRUM**: an 8-lane kit of Felucca's own drum voices on the General MIDI key map

The DIGITAL engine of 0.9 has been replaced by FM6: projects and presets with DIGITAL sounds load
as FM6 sounds converted from them.

**SLICER** (FX page, every track): a tempo-synced 16-step gate or stutter, with 16 patterns.

## Scale keyboard

On the **SCL** page, set **QNT** to WHITE to play the selected scale using only the
white keys (SNAP keeps every key and rounds it down to the scale). C4 plays **ROOT**; consecutive white keys play consecutive scale notes
above and below it. Black keys are silent, including during live recording and
step entry. **TRN** transposes the resulting notes; the octave buttons shift them
by full octaves. Set QNT to OFF for the normal chromatic keyboard. QNT SEQ snaps
the keys like SNAP and maps sequenced notes to the current ROOT / SCALE during
playback without changing stored steps. Drum kits are never quantized.

Available scales: chromatic (CHR), major (MAJ), natural minor (MIN), Dorian (DOR),
Mixolydian (MIX), major pentatonic (PEN), minor pentatonic (MPEN), harmonic minor
(HARM), Phrygian (PHRY), Lydian (LYD), Locrian (LOC), ascending melodic minor (MEL),
minor blues (BLUES), whole tone (WHOLE), half-whole diminished (DIMHW), and
whole-half diminished (DIMWH). Scales with other than seven notes continue across
the white keys without repeating notes; their roots need not fall on every C key.
Drum kits and incoming MIDI keep their own note mapping.

Press **SCL** again for the **CHORD** page: CHRD picks the chord keys (OFF, the scale's triads or
sevenths, or a fixed shape) and VOIC the voicing.

## Layout

| Path | What |
| --- | --- |
| `firmware/` | firmware sources: `src/` app, `hal/` hardware layer, `loader/` update loader |
| `tools/` | build script, generators, package maker, installer and sample uploader |
| `assets/` | UI font, icon names, CC0 instrument samples |
| `web/` | web installer and editor sources |
| `tests/` | tests that run on the build machine |
| `LICENSES/` | licence texts of the bundled font, icons, ported DSP and SDK files |

## Support

If Felucca is useful to you, [sponsoring on GitHub](https://github.com/sponsors/hugelton) or a donation
on [itch.io](https://hugelton.itch.io/felucca) helps keep its development going.

Pull requests are welcome, and so are ideas and requests: post them in
[Discussions](https://github.com/hugelton/Felucca/discussions) or on X ([@kurogedelic](https://x.com/kurogedelic)).

## Credits

- **[Hügelton Instruments](https://hugelton.com)** (Leo Kuroshita, [@kurogedelic](https://github.com/kurogedelic)):
  Felucca itself; the PHASE engine's waveforms (a C port of the oscillator of
  [CrispyZebra](https://github.com/hugelton/CrispyZebra), GPL-3.0); the DRUM voices; the Hügelton Sample
  Pack (the drum samples, GPL-3.0-only, not CC0); the [Fukiai](https://github.com/hugelton/Fukiai) icon
  font ([MIT](LICENSES/MIT-Fukiai.txt))
- Font: [Inter Tight](https://github.com/rsms/inter-tight) by The Inter Project Authors, [SIL OFL 1.1](LICENSES/OFL-InterTight.txt)
- Samples: [Versilian Studios](https://versilian-studios.com/) [VSCO-2 Community Edition](https://github.com/sgossner/VSCO-2-CE) and [VCSL](https://github.com/sgossner/VCSL), CC0 1.0 ([attribution](assets/samples-cc0/ATTRIBUTION.txt))
- VOICE engine: after [klattsch](https://github.com/tgies/klattsch) by Tony Gies (MIT); formant data from Klatt (1980) and Hillenbrand et al. (1995)
- PHYS engine: models ported from [DaisySP](https://github.com/electro-smith/DaisySP) by Electrosmith and Emilie Gillet ([MIT](LICENSES/MIT-DaisySP.txt)) and from Emilie Gillet's [eurorack](https://github.com/pichenettes/eurorack) code ([MIT](LICENSES/MIT-Rings.txt))
- FM6 engine: msfa from [Dexed](https://github.com/asb2m10/dexed) by Google Inc. and Pascal Gauthier ([Apache-2.0](LICENSES/Apache-2.0-msfa.txt))
- Package format and boot files: [JieLi AC79 SDK](https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK) ([Apache-2.0](LICENSES/Apache-2.0.txt); three of its files are in every package, none in this tree)
- Contributions: [keremimo](https://github.com/keremimo) (white-key scales, #2), [ChanceTheMaker](https://github.com/ChanceTheMaker)
  (TRS MIDI, bend, sustain and clock, palettes, favourites, editor display settings: #8, #10, #11, #12),
  [andreahaku](https://github.com/andreahaku) (sample recording and trim, #29; SLICE manual slices and tests, #27, #22)
- Salt fork and browser Studio: Chance Roth ([@ChanceTheMaker](https://github.com/ChanceTheMaker));
  content-driven TRS receive-buffer fix: [keremimo](https://github.com/keremimo)

## Licence

Free software: [GPL-3.0-only](LICENSE), the Hügelton Sample Pack included. The bundled font and the
ported DSP keep their own licences ([LICENSES/](LICENSES/)); details in [LICENSING.md](LICENSING.md).

M-VAVE and FM-1 are trademarks of their respective owners. Felucca is not affiliated with or endorsed by them.

Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
