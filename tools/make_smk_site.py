#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Stage the SMK website for Pages; never copy private device/build files."""
import argparse
import shutil
import json
import hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('out', type=Path)
    args = parser.parse_args()
    out = args.out.resolve()
    if out.exists():
        parser.error('Output must be a new directory (no destructive cleanup).')
    out.mkdir(parents=True)
    web = ROOT / 'web'
    for source in web.iterdir():
        if source.is_file() and (source.suffix in ('.js', '.css', '.ttf', '.txt') or
                                source.name in ('index.html', 'editor.html', 'simulator.html', 'midi-diagnostics.html')):
            shutil.copy2(source, out / source.name)
    for name in ('fonts', 'screenshots'):
        shutil.copytree(web / name, out / name)
    (out / 'audio').mkdir()
    for source in (web / 'audio').iterdir():
        if source.is_file() and source.suffix in ('.js', '.wasm', '.json'):
            shutil.copy2(source, out / 'audio' / source.name)
    for name in ('LICENSE', 'LICENSING.md'):
        shutil.copy2(ROOT / name, out / name)
    (out / 'restore-stock').mkdir()
    for asset in (web / 'restore-stock').iterdir():
        if asset.suffix in ('.html', '.css', '.js'):
            shutil.copy2(asset, out / 'restore-stock' / asset.name)
    # Explicit release allowlist: never stage private candidates or vendor files.
    release_path=web/'firmware/public-release.json'
    if release_path.exists():
        release=json.loads(release_path.read_text())
        if release.get('public') is not True or release.get('app')!='felucca-smk-0.9.27-app.bin':
            raise ValueError('Unsupported public release manifest')
        app=web/'firmware'/release['app']
        data=app.read_bytes()
        if len(data)!=release['bytes'] or hashlib.sha256(data).hexdigest()!=release['sha256']:
            raise ValueError('Public application digest mismatch')
        (out/'firmware').mkdir()
        shutil.copy2(release_path,out/'firmware/public-release.json')
        shutil.copy2(app,out/'firmware'/app.name)
    shutil.copytree(ROOT / 'LICENSES', out / 'LICENSES')
    (out / '.nojekyll').write_text('')
    print(f'SMK site: {out}')

if __name__ == '__main__':
    main()
