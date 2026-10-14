#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Link an offline SMK integration image, never a flash/update package.

JIELI_TOOLCHAIN=... python tools/build_smk37.py
The raw .bin is not directly installable. Private packaging is a separate step.
Uses the conservative existing AC79 linker allocation, not the FM-1 package
builder/storage geometry. Outputs are isolated under build/smk37-integration.
"""
import argparse
import hashlib
from pathlib import Path
from smk37_backup import inspect_backup
import json
import re
import subprocess
import sys
import zlib
import build


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--stock-backups', nargs=2, type=Path)
    parser.add_argument('--stock-package', type=Path)
    parser.add_argument('--public-release', action='store_true', help='Build portable model-gated firmware without owner backup fingerprints; no flashing')
    parser.add_argument('--enable-saves', action='store_true', help='private Test10 candidate; requires validated stock evidence and separate save-recovery kit')
    parser.add_argument('--enable-usb-update', action='store_true', help='private RAM-updater candidate; each built artifact requires validation')
    parser.add_argument('--usb-test-number',type=int,default=14,choices=range(11,100))
    parser.add_argument('--enable-presets',action='store_true',help='opt-in eight-slot preset sectors; validates both stock reads')
    parser.add_argument('--enable-usb-audio', action='store_true', help='experimental UAC1 recording; requires device timing/host validation')
    parser.add_argument('--enable-samples', action='store_true', help='private sample storage candidate; explicit device initialization required')
    parser.add_argument('--enable-trs', action='store_true', help='TRS MIDI OUT driver; restores explicitly saved preference')
    parser.add_argument('--panel-mirror', action='store_true', help='local device-master panel candidate; requires hardware validation')
    args = parser.parse_args()
    if args.public_release:
        if args.stock_backups or args.stock_package:
            parser.error('--public-release does not consume private backups or vendor packages')
        if args.usb_test_number==14:args.usb_test_number=24
        args.enable_saves=args.enable_presets=args.enable_usb_update=True
        args.enable_usb_audio=args.enable_samples=args.enable_trs=True
    if args.enable_trs and not args.enable_usb_audio:
        parser.error('--enable-trs requires --enable-usb-audio (2 kHz service timer)')
    if args.enable_presets and not args.enable_saves:
        parser.error('--enable-presets requires --enable-saves')
    if args.enable_samples and not args.enable_presets:
        parser.error('--enable-samples requires --enable-presets and validated stock backups')
    if args.enable_usb_update and not args.enable_saves:
        parser.error('--enable-usb-update requires --enable-saves')
    if bool(args.stock_backups) != bool(args.stock_package):
        parser.error('--stock-backups and --stock-package must be supplied together')
    if args.enable_saves and not args.stock_backups and not args.public_release:
        parser.error('--enable-saves requires validated stock backups and package')
    evidence = None
    if args.stock_backups:
        try:
            evidence = inspect_backup(*(p.read_bytes() for p in args.stock_backups),
                                      args.stock_package.read_bytes())
        except (OSError, ValueError) as exc:
            parser.error(str(exc))
    if args.enable_presets and not args.public_release:
        for backup in args.stock_backups:
            raw=backup.read_bytes()
            for sector in (0xf3000,0xfe000):
                if raw[sector:sector+4096] != bytes([255])*4096:
                    parser.error(f'preset allocation is not blank in {backup.name} at {sector:#x}')
    out = build.SRC / "build" / (("smk37-webupdate" if args.usb_test_number==11 else f"smk37-webupdate-test{args.usb_test_number}") if args.enable_usb_update else "smk37-integration")
    if args.panel_mirror: out = build.SRC / 'build' / 'smk37-panel-mirror-candidate'
    out.mkdir(parents=True, exist_ok=True)
    build.GEN = out / "gen"
    build.GEN.mkdir(parents=True, exist_ok=True)
    trim_crc = zlib.crc32(args.stock_backups[0].read_bytes()[0xc1000:0xc1000+556]) if evidence else 0
    release_version = re.search(r'SMK37_RELEASE_VERSION "([0-9.]+)"',
                                (build.FW/'hal/smk37_version.h').read_text())[1]
    update_version = f'Felucca SMK {release_version} Beta' if args.public_release else f'SMK37 TEST{args.usb_test_number:02d} USB'
    if args.panel_mirror: update_version='Felucca SMK 0.9.27 Beta'
    sample_crcs=[0,0,0]
    if args.enable_samples and not args.public_release:
        raw=args.stock_backups[0].read_bytes()
        if raw[0xbf000:0xc0000]!=bytes([255])*4096:
            parser.error('sample ownership journal is not blank in the validated stock backup')
        sample_crcs=[zlib.crc32(raw[a:a+0x14000]) for a in (0x9d000,0xc2000,0xd6000)]
    (build.GEN/'smk37_unit.h').write_text(
        '/* Generated build policy; public builds contain no owner fingerprints. */\n'
        f'#define SMK37_PUBLIC_BUILD {int(args.public_release)}\n'
        f'#define SMK37_UNIT_TRIM_CRC 0x{trim_crc:08x}u\n'
        '#define SMK37_UNIT_SAMPLE_CRCS {'+','.join(f'0x{c:08x}u' for c in sample_crcs)+'}\n'
        f'#define SMK37_UNIT_HEAD_CRC 0x{0x3a04ad23 if args.public_release else zlib.crc32(args.stock_backups[0].read_bytes()[:0x4000]) if evidence else 0:08x}u\n'
        f'#define SMK37_UPDATE_PRODUCT "SMK-37_9{args.usb_test_number:02d}"\n'
        f'#define SMK37_UPDATE_VERSION "{update_version}"\n')
    if args.enable_usb_update:
        from build_smk37_loader import build_loader
        build_loader(out, build.GEN, f"SMK-37_9{args.usb_test_number:02d}")
    for generator, header in (("gen_tables.py", "felucca_tables.h"),
                              ("gen_fm6_patches.py", "felucca_fm6.h"),
                              ("gen_samples.py", "felucca_samples.h"),
                              ("gen_ui_palettes.py", "ui_palettes.h"),
                              ("gen_aa_keycaps.py", "ui_keycaps.h")):
        subprocess.run([sys.executable, str(build.SRC / "tools" / generator),
                        str(build.GEN / header)], check=True, cwd=build.SRC)
    subprocess.run([sys.executable, str(build.SRC / "tools" / "gen_aa_font.py"),
                    str(build.GEN / "ui_fonts.h"), "--preset", "inter-tight"],
                   check=True, cwd=build.SRC)
    flags = [*build.CFLAGS, f"-DSMK37_ENABLE_SAMPLES={int(args.enable_samples)}", f"-DSMK37_ENABLE_USB_AUDIO={int(args.enable_usb_audio)}", "-Ifirmware/hal", "-Ifirmware/src", f"-I{build.GEN.relative_to(build.SRC)}", f"-DSMK37_ENABLE_PRESETS={int(args.enable_presets)}", f"-DSMK37_ENABLE_SAVES={int(args.enable_saves)}", f"-DSMK37_ENABLE_USB_UPDATE={int(args.enable_usb_update)}"]
    flags += [f'-DSMK37_ENABLE_TRS={int(args.enable_trs)}', f'-DSMK37_EXPANDED_PRESETS={int(args.public_release)}']
    flags += [f'-DSMK37_REMOTE_PANEL={int(args.panel_mirror)}']
    build.tc_all(
        ("cc", *flags, "-c", build.FW / "src" / "smk37_app.c", "-o", out / "app.o"),
        ("cc", f"-DSMK37_ENABLE_USB_AUDIO={int(args.enable_usb_audio)}", "-c", build.FW / "smk37_crt0.S", "-o", out / "crt0.o"),
        ("cc", "-c", build.FW / "hal" / "fm1_vec.S", "-o", out / "vec.o"),
    )
    elf = out / "smk37-integration.elf"
    # This allocation is a hypothesis based on shared CPU family + reserved
    # boot/vector addresses. The manifest explicitly does not certify RAM.
    build.tc("pi32v2/bin/ld", "-T", build.FW / "app.ld",
             out / "crt0.o", out / "vec.o", out / "app.o", "-o", elf)
    symbols = build.tc("common/bin/objdump", "-t", elf)
    sections = build.tc("common/bin/objdump", "-h", elf)
    disassembly = build.tc("common/bin/objdump", "-d", elf)
    (out / "smk37-integration.dis").write_text(disassembly)
    (out / "sections.txt").write_text(sections)
    (out / "symbols.txt").write_text(symbols)
    def sym(name):
        return int(re.search(r"^([0-9a-f]+) .*\s" + name + r"$", symbols, re.M)[1], 16)
    image = bytearray()
    for section, address in ((".text", build.APP_XIP), (".ram_text", sym("_rt_load")),
                             (".data", sym("_data_load"))):
        blob = out / (section[1:] + ".bin")
        blob.unlink(missing_ok=True)
        build.tc("common/bin/objcopy", "-O", "binary", "-j", section, elf, blob)
        raw = blob.read_bytes() if blob.exists() else b""
        if not raw:
            continue
        offset = address - build.APP_XIP
        if offset < len(image): raise SystemExit("overlapping image sections")
        image.extend(b"\xff" * (offset - len(image)))
        image.extend(raw)
    image.extend(b"\xff" * (-len(image) % 4))
    if sym("_start") != build.APP_XIP or image[:4] != bytes.fromhex("04818000"):
        raise SystemExit("invalid entry stub")
    # Keep inside the stricter FM-1 allocation as well as stock SMK app extent.
    if len(image) > 0x8DFBC: raise SystemExit("image exceeds conservative XIP allocation")
    for forbidden in ("fm1_flash", "ota_session", "uart_midi_init", "fm1_adc_init", "fm1_input_init"):
        if re.search(r"\b" + forbidden + r"\w*$", symbols, re.M):
            raise SystemExit("wrong-board/flash routine linked: " + forbidden)
    # Test09 may read flash, but neither write entry point may touch hardware.
    for routine in ('fl_erase4k_ram', 'fl_prog_ram'):
        body = disassembly.split(routine + ':', 1)[1].split('\n\n', 1)[0]
        instructions = [line.split('\t')[-1].strip() for line in body.splitlines() if re.match(r'\s*[0-9a-f]+:', line)]
        if not args.enable_saves and instructions != ['r0 = -1', 'rts']:
            raise SystemExit('Test09 flash write gate is not a rejection stub: ' + routine)
        if args.enable_saves and instructions == ['r0 = -1', 'rts']:
            raise SystemExit('Save-enabled candidate still rejects all writes: ' + routine)
    for routine in ('fl_read_ram', 'fl_jedec_ram'):
        if not 0x1c00000 <= sym(routine) < 0x1c08000:
            raise SystemExit('Flash read helper is not in reserved RAM: ' + routine)
    ram_dis = build.tc('common/bin/objdump', '-d', '-j', '.ram_text', elf)
    if re.search(r'\bcall\b', ram_dis):
        raise SystemExit('Flash RAM routines contain a call requiring review')
    name = "smk37-integration-NOT-FOR-INSTALL.bin"
    (out / name).write_bytes(image)
    manifest = {
        "board": "SMK-37 Pro & Elite" if args.public_release else "SMK-37 Pro", "installable": False,
        "public_build": args.public_release,
        "remote_panel_support": args.panel_mirror,
        "remote_panel_protocol": 4 if args.panel_mirror else 0,
        "remote_panel_hardware_validated": False,
        "remote_panel_framebuffer_bytes": 28800 if args.panel_mirror else 0,
        "purpose": "offline linked application integration",
        "image": name, "bytes": len(image), "sha256": hashlib.sha256(image).hexdigest(),
        "entry": hex(sym("_start")), "bss_end": hex(sym("_bss_end")),
        "pool_end": hex(sym("_pool_end")), "flash_write_support": args.enable_saves, "preset_storage_support": args.enable_presets, "preset_sectors": (["0xb1000..0xb7000", "0xf3000", "0xfe000"] if args.public_release else ["0xf3000", "0xfe000"]) if args.enable_presets else [], "preset_slots": 32 if args.public_release else 8, "usb_update_support": args.enable_usb_update,
        "usb_update_hardware_validated": False,
        "usb_audio_support": args.enable_usb_audio,
        "usb_audio_hardware_validated": False,
        "sample_storage_support": args.enable_samples,
        "sample_storage_requires_explicit_initialization": args.enable_samples,
        "sample_storage_hardware_validated": False,
        "sample_regions": ["0x9d000..0xb1000", "0xc2000..0xd6000", "0xd6000..0xea000"] if args.enable_samples else [],
        "sample_ownership_journal": "0xbf000..0xc0000" if args.enable_samples else None,
        "sample_write_gate": "unit/stock CRCs; explicit resumable initialization; committed ownership; non-contiguous slot bounds; no boot erase" if args.enable_samples else "disabled",
        "trs_midi_output_support": args.enable_trs,
        "trs_midi_output_default": "Saved preference; OFF for new/legacy settings",
        "trs_midi_output_hardware_validated": False,
        "usb_audio_rates": [44100, 48000] if args.enable_usb_audio else [],
        "update_product": f"SMK-37_9{args.usb_test_number:02d}", "update_version": update_version,
        "upstream_revision": "a4d7ca2e5d52a4c35e2468401f00d883cadbda03",
        "upstream_version": "1.5.1",
        "prior_unit_evidence": "Test12: owner-confirmed USB update/restart and saved-project retention; earlier tests cover controls, sound, sustain and editor. This new artifact is not device-validated.",
        "flash_write_gate": (("projects 0xEB000..0xF3000; " + ("presets 0xB1000..0xB7000 (blank/owned A/B only), legacy 0xF3000..0xF4000 and 0xFE000..0xFF000; " if args.enable_presets else "") + "call-free RAM routines; runtime JEDEC/trim/allocation checks") if args.enable_saves else "erase/program entry points disassemble to return -1; read routines are call-free RAM code"),
        "unverified": [f"Test{args.usb_test_number:02d} startup, audio/control timing and USB update on hardware",
                       "new effects, automation, MIDI learn and metronome on hardware",
                       "Test12-to-FUN10 project migration on hardware",
                       "long-run audio/control timing", "unit calibration backup"],
    }
    if evidence:
        manifest['stock_backup_evidence'] = evidence
        manifest['unverified'].remove('unit calibration backup')
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps(manifest, indent=2))
    return 0


if __name__ == "__main__":
    sys.exit(main())
