#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Stage the SMK website for Pages; never copy private device/build files."""
import argparse
import shutil
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
                                source.name in ('index.html', 'editor.html', 'simulator.html')):
            shutil.copy2(source, out / source.name)
    for name in ('fonts', 'screenshots'):
        shutil.copytree(web / name, out / name)
    (out / 'audio').mkdir()
    for source in (web / 'audio').iterdir():
        if source.is_file() and source.suffix in ('.js', '.wasm', '.json'):
            shutil.copy2(source, out / 'audio' / source.name)
    for name in ('LICENSE', 'LICENSING.md'):
        shutil.copy2(ROOT / name, out / name)
    shutil.copytree(ROOT / 'LICENSES', out / 'LICENSES')
    (out / '.nojekyll').write_text('')
    print(f'SMK site: {out}')

if __name__ == '__main__':
    main()
