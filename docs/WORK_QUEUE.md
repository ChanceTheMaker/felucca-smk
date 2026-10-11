# Felucca Salt work queue

Deferred requests for a future round. Listing an item here does not add it to
the current implementation or publish.

## Website

- [x] **Dedicated SMK simulator page (local, 2026-10-11).**
  `web/simulator.html` reuses the native browser engine and simulator controls,
  with a top-navigation link from the homepage and Studio. Includes audio,
  session import/export, shared browser storage, finish swatches, tutorial and
  fullscreen. Normal navigation between the running browser Studio and the
  simulator saves the session before leaving. Existing embedded simulator mode
  remains available. Preview: `http://localhost:8773/simulator.html`.
  Native runtime remains based on the SMK branch at `4831471`, with upstream
  Felucca 1.5.1 pinned at `a4d7ca2e5d52a4c35e2468401f00d883cadbda03`.
  Web protocol regression suite and JavaScript syntax checks pass; local Chrome
  startup, display and navigation checked. No device firmware or publication
  changed. New simulator-specific labels currently remain in English.

- [x] **Make the header logo link to the homepage.** Implemented locally after
  the owner brought it into scope on 2026-10-05. The “Felucca [Salt]” logo on both
  pages links to the installer homepage, with its size/styling preserved and
  visible keyboard focus. The owner authorized merging into the fork's main;
  live deployment is separate.

## SMK firmware

- [ ] **Investigate browser-audio regression retry.** The commit-time run against
  localhost:8771 ended with a timeout in its error-path Playwright screenshot,
  masking the original error. Recover the original failure and rerun; do not
  infer a browser-audio pass or a specific product defect from this result.

- [ ] **TRS OFF leaves a held FM-1 note sounding (Test21).** On 2026-10-10 the
  owner confirmed that switching TRS OUT OFF with the cable connected does not
  stop a held receiver note. Chords, sustain and pitch otherwise worked; mod
  remains inconclusive. Root cause unknown. The owner explicitly deferred repair
  to continue sample/storage testing. No shutdown fix is included. See
  [Test21 hardware results](smk37/TEST21.md#hardware-follow-up).

- [ ] **Review saving TRS OUT across power cycles.** The owner confirmed Test21
  restarts with TRS OFF and requested recording this for later. This is the
  current intentional startup behavior, not evidence of a failed settings write.
  Decide and implement the desired persistence in a later round.

- [ ] **Finish user-sample persistence validation.** USR1 test upload and playback
  were reported successful, but the post-restart check was inconclusive while
  testing the FM-1 receiver. Reconnect Studio to inspect USR1, select SAMPLE / USR1
  and listen directly through the SMK headphone output after a power cycle.
  TRS sends MIDI events, not the uploaded sample audio. Do not mark persistence
  passed or sample data lost without that check. Deferred at the owner's request.
