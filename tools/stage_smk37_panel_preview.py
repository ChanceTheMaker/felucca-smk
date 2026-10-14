#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Stage the opt-in panel candidate under build/ for loopback testing only.

Does not access MIDI, flash a device, or modify the public release files.
"""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
candidate = ROOT / 'build/smk37-panel-mirror-candidate'
out = Path(sys.argv[1]).resolve() if len(sys.argv)>1 else ROOT / 'build/panel-preview/site-rect'
m = json.loads((candidate / 'manifest.json').read_text(encoding='utf-8'))
app = (candidate / m['image']).read_bytes()
if (not m.get('remote_panel_support') or m.get('update_product') != 'SMK-37_927'
        or m.get('update_version') != 'Felucca SMK 0.9.27 Beta'
        or len(app) != m['bytes'] or hashlib.sha256(app).hexdigest() != m['sha256']):
    raise SystemExit('Build and verify the 0.9.27 panel candidate first.')
subprocess.run([sys.executable, str(ROOT / 'tools/make_smk_site.py'), str(out)], check=True)
release = json.loads((out / 'firmware/public-release.json').read_text(encoding='utf-8'))
(out / 'firmware' / release['app']).unlink()
release.update(version=m['update_version'], product=m['update_product'],
               app='felucca-smk-0.9.27-app.bin', bytes=m['bytes'], sha256=m['sha256'],
               hardware_validation='Local panel-mirror candidate. Host tested; device timing and mirroring test pending.')
(out / 'firmware' / release['app']).write_bytes(app)
(out / 'firmware/public-release.json').write_text(json.dumps(release, indent=2)+'\n', encoding='utf-8')
for name in ('smk37-installer.js', 'smk-version.js', 'index.html'):
    p = out / name
    s = p.read_text(encoding='utf-8')
    if name == 'index.html':
        import re
        s = re.sub(r'(<body\b[^>]*>)', r'\1<aside style="padding:16px;text-align:center;background:#ffe1a1;color:#251a07">LOCAL TEST BUILD: 0.9.27 changed-area screen synchronization. Not yet tested on hardware. Installation starts only when you confirm and click Install.</aside>', s, count=1)
    p.write_text(s, encoding='utf-8')
print('Local candidate staged:', out)
print('Serve on loopback only; public release files are unchanged.')
