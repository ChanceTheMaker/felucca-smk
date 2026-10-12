# Restore original firmware

The installer links to `restore-stock/`, a separate explicit restore flow. No
operation runs on page load. SMK Pro/Elite use the pinned official 016 files;
FM-1 uses the exact official V15 file selected locally. Full SHA-256 and layout
validation happen before any device command. The selected physical model is
confirmed because custom SMK USB identities do not distinguish Pro from Elite.

SMK restore packages contain the original encrypted application bytes in
0x4000..0x9d000, within the existing portable loader limits. The generic transport
label SMK-37_016 satisfies existing loader headers; success requires the actual
stock Pro/Elite_016 identity after restart. The bootloader and calibration are
outside the write range. Custom music can be incompatible or overwritten once
stock runs, so preserving flash during installation is not a backup guarantee.
FM-1 uses its validated official package and existing FM-1 loader flow.

Compatible sources: public SMK updaters 922+, Doom SMK updater 007+, and supported
FM-1 synth / Doom loaders. Older private builds are refused. Cancellation,
missing/wrong loaders, incorrect returned identities and interrupted transfers
are not reported as success. No automatic write retry or firmware change.

Validation 2026-10-12: exact Pro/Elite official fixtures, all application sectors,
header/sector CRCs, wrong models, corrupt files, both Doom identities, source
version gates, cancellation and return identity tests pass. The native SMK
loader restored all 153 application sectors in host tests while retaining the
head, calibration and tail. The owner restored the physical Pro from Sloop
2.6.0-b8-dc1 to official Pro 016 successfully. Elite and FM-1 physical restore
were not repeated in this round. FM-1 V15 mock-flow checks passed with the actual
699,956-byte official fixture. No vendor firmware is committed or hosted here.

Run the restore tests with Node: `node web/restore-stock/test_restore.mjs`.
PRO_STOCK_FILE and ELITE_STOCK_FILE can point at local official files; otherwise
the pinned mirror is used. FM1_STOCK_FILE enables the V15 fixture cases.
