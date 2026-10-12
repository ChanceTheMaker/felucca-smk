# Automatic SMK installation and resume

Ported from ChanceTheMaker/sloop-smk commit 069fe4c (installer only). Firmware binaries, package format, flash-write bounds, calibration handling and release manifests are unchanged.

The automatic route accepts verified stock Pro/Elite 016, an exact official loader, or the portable SMK updater (SMK-37 / ota-SMK-37 with version 922 or later). Existing Felucca, Sloop and Melodee identities use the portable package, never the stock image. Discovery sends identity queries one output at a time while listening on every input; matching port names are not required. Multiple matching inputs/identities stop installation. Port open/close calls are bounded and discovery-owned ports are released.

An Elite loader reporting ota-FM-1 requires an explicit on-page confirmation that it belongs to the connected Elite and every actual FM-1 is disconnected. This is not technical model proof. Pro cannot use this exception. Cancel, timeout and failed staging never start application programming. Failed application transfers are not retried automatically.

Host tests cover normal stock installation, already-running exact/generic/portable loaders, same-attempt handoff, cancellation, unrelated/wrong models, old private versions, transfer failures, return-identity mismatches, differing port names, busy/hung ports and cleanup. Cross-firmware tests cover Felucca 922, Sloop 936 and Melodee 940. No physical flashing is performed by these tests. Physical Elite handoff and cross-firmware installations still need device confirmation.

The public Felucca installer uses this flow with its existing 0.9.22 release.
