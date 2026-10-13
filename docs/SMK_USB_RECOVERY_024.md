# Felucca SMK 0.9.24 USB updater repair

Selected public release after owner-authorized Pro hardware validation on
October 12, 2026 (October 13 UTC). Runtime: `SMK-37_924`.

The public loader now accepts valid calibration or a wholly erased 4 KiB
USRTRIM sector, rejects partial corruption, and checks the entire sector after
updating. Application write bounds remain `0x4000..0x9d000`; no calibration is
created or written. This focused port comes from Doom `da621ca` via Sloop b9.
Existing synthesis, controls, storage layout, author credits and licenses remain.
No upstream contribution was submitted. Private backups and generated recovery
images remain under ignored build directories and are excluded from publication.

An instrument still running an older loader that rejects blank calibration can
require a one-time recovery-adapter repair. Publishing these builds does not
change an already-installed old loader. Old release binaries remain immutable.
Elite hardware validation remains pending.

Source baseline: Felucca SMK `a5a1098`; public website changes through `26fece6`
are retained. Musical upstream stays at Felucca 1.5.1
`a4d7ca2e5d52a4c35e2468401f00d883cadbda03`.

Application: 416,004 bytes, SHA-256
`10b32af3e02adb2ac2b8ea4c621dd865333fe5090fdb373cb590d04f549d7581`.

Two fresh independent full-device reads matched the installed 0.9.23 application
and showed a wholly erased calibration sector. Adapter repair changed 45
application sectors; full readback preserved bootloader and all device data.
The owner confirmed Felucca booted and played. A normal-USB reinstall completed
2,450 requests and returned 924. Felucca then installed Melodee dc8 successfully
over normal USB with application readback and the exact 942 returned identity.

Native compilation, sanitizer-backed public/private updater tests, package
simulation, calibration/store/migration bounds, expanded preset and interrupted
commit regressions passed. Separate physical expanded-preset/TRS checks remain
pending. Same-version reinstalls skip unchanged sectors; the outgoing Melodee
installation separately exercised changed application programming.
