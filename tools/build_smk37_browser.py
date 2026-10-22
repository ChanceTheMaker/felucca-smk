"""Build the hardware-free SMK Studio/simulator runtime (no device access)."""
import argparse
import hashlib
import json
import importlib.util
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]

def default_zig():
    """Use an explicit override, pip-installed Zig, or this checkout's tools."""
    if os.environ.get('ZIG'):
        return os.environ['ZIG']
    name = 'zig.exe' if os.name == 'nt' else 'zig'
    package = importlib.util.find_spec('ziglang')
    if package and package.origin:
        candidate = Path(package.origin).parent / name
        if candidate.is_file():
            return str(candidate)
    local = ROOT / 'build/wasm-tools/ziglang' / name
    return str(local) if local.is_file() else shutil.which('zig')

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--zig', default=default_zig(), help='Zig executable (tested with 0.13.0); otherwise ZIG, ziglang, or PATH')
p.add_argument('--skip-generate', action='store_true', help='Reuse build/gen from a checked SMK build')
a = p.parse_args()
if not (ROOT/"firmware/engines/src/felucca_engine.h").is_file():
    p.error("Initialize the pinned engines: git submodule update --init firmware/engines")
if not a.zig:
    p.error('Zig not found. Install tools/smk-browser-requirements.txt or pass --zig PATH.')
gen = ROOT / 'build/gen'
if not a.skip_generate:
    from PIL import features
    if not features.check('raqm'):
        p.error('Pillow needs libraqm for display fonts. Use the Linux/WSL setup in docs/smk37/DEVELOPMENT.md.')
    gen.mkdir(parents=True, exist_ok=True)
    for script, header in [('gen_tables.py','felucca_tables.h'), ('gen_samples.py','felucca_samples.h'),
                           ('gen_fm6_patches.py','felucca_fm6.h'), ('gen_ui_palettes.py','ui_palettes.h'),
                           ('gen_aa_keycaps.py','ui_keycaps.h')]:
        subprocess.run([sys.executable, str(ROOT/'tools'/script), str(gen/header)], check=True)
    subprocess.run([sys.executable, str(ROOT/'tools/gen_aa_font.py'), str(gen/'ui_fonts.h'), '--preset', 'inter-tight'], check=True)
exports = ['synth_init','synth_midi','synth_render','fw_event','fw_release','fw_screen','fw_status',
           'fw_request','fw_reply','fw_command','fw_changed','fw_snapshot','fw_snapshot_size','fw_restore']
out = ROOT/'web/audio/smk-engine.wasm'
subprocess.run([a.zig, 'cc', '-target','wasm32-freestanding','-O2','-fno-builtin','-fwrapv','-nostdlib',
                '-Wl,--no-entry','-Wl,-z,stack-size=0x80000','-Wl,--initial-memory=0x1000000','-Wl,--max-memory=0x1000000',
                *['-Wl,--export='+e for e in exports], '-I'+str(ROOT/'web/audio/include'),
                '-I'+str(gen), '-I'+str(ROOT/'firmware/hal'), str(ROOT/'web/audio/smk-engine.c'), '-o',str(out)], check=True)
revision=hashlib.sha256(out.read_bytes()).hexdigest()
(ROOT/'web/audio/smk-build.json').write_text(json.dumps({'schema':1,'sha256':revision,'bytes':out.stat().st_size})+'\n')
print('Built SMK simulator:', revision, out.stat().st_size)
