# Flash ID compatibility and stock recovery

Candidate 0.9.28 addresses Felucca issue [#17](https://github.com/ChanceTheMaker/felucca-smk/issues/17#issuecomment-6249932641).
Baseline: `1e4bb38c18d06f77757029f1ba4876bfc0856cf6`; musical upstream remains
Felucca 1.5.1, `a4d7ca2e5d52a4c35e2468401f00d883cadbda03`.

The loader rejected every JEDEC ID except `856014` before requesting a package.
The preset-storage probe had the same restriction. The contributor measured
`EB6014` and reports restoring original Pro 016 through the existing ROM USB
interface. Both Felucca checks now use one exact allowlist containing those
two IDs. No SPI commands, timing, memory maps, boot/calibration writes, storage
ownership checks or package formats were changed. All other IDs remain blocked.

Issue #12 may have this cause, but its generic failure and storage warning do
not establish a flash ID or prove calibration damage. Do not overwrite or
invent calibration in response to that warning.

## Detailed failures

On a failed update attempt, the RAM loader sends an additional checksummed OTA
message with command `7E` and this 12-byte body:

`"SMKU" | version=1 | positive error code | write-attempted=0/1 | reserved=0 | JEDEC-ID-u32le`

The existing `00 59` OTA envelope, pack7 encoding and checksum are unchanged.
Identity (`11`), data requests (`30`) and completion handshakes are unchanged.
Older hosts ignore the added message. Updated browser install, browser stock
restore and native Windows paths report it immediately. The write flag is set
before the first erase attempt; a failure then tells the owner to keep power
and USB connected. It describes this attempt, not previous interrupted updates.

## Already-installed rejecting loaders

A new website or package cannot repair an old loader that rejects the chip
before accepting that package. The contributor's alternate USB route bypasses
that loader. It makes two matching backups and restores only differing
application sectors in `[0x4000,0x9D000)`, retaining boot and the complete tail.

Reviewed attachment:
`smk37-pro-stock-recovery-0.1.0.zip`, SHA-256
`081e71a6a39cbad2903d2602d0f4bbd25e4ab77f4ac5c57b187ab5c1d3b797e4`.
Its 23 synthetic/mock tests passed locally. Its author reports one successful
underlying recovery, but the generalized ZIP program is not hardware validated.
It is scoped to Windows, Pro, published Felucca 0.9.27 and EB6014. This change
links to that report; it does not broaden or silently run the recovery tool.
No vendor firmware, helper binaries or private device dumps are redistributed.

If the application no longer boots and USB recovery cannot be entered, this
software route cannot guarantee a return to stock; external recovery hardware
may still be required.

## Validation and release boundary

The private/public updater suites cover successful writes, no-op updates,
unknown chips, bad/corrupted packages, changed host data, interrupted writes,
and preserved boot/tail/calibration. The shared policy is exercised for both
accepted IDs and rejects unrelated same-capacity IDs. Python and JavaScript
diagnostic tests cover valid failures, malformed messages and checksums.

Candidate application SHA-256:
`b53614f3420d80bdc7ad31cf5b1d20ed35b4063fba4d64b70b1226c6bc498ab9`.
Product: `SMK-37_928`; size: 420,796 bytes. RAM loader size: 8,392 bytes.
Build: `tools/build_smk37.py --public-release --panel-mirror --usb-test-number 28`.
Build checks verified application bounds and RAM-only flash routines.

On October 18, the owner's Pro installed the candidate from Sloop 938 and
restarted as `SMK-37_928`. A deliberately invalid header was rejected before
erase, returning error 2 and measured ID `856014`. The same loader subsequently
restored original Pro 016, completed application readback, and the restarted
device replied `SMK-37 Pro_016`. UI/sound confirmation is separate. EB6014 and Elite candidate validation remain
pending; an 856014 test does not establish either. Keep public 0.9.27 artifacts
unchanged until the affected-variant validation supports promotion.

Sibling project rollout and unrelated DAW-control work are outside this
narrowed task. No upstream contribution or issue reply has been submitted.
