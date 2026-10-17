#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Prepare the Felucca Windows USB helper; never opens MIDI or flashes a device."""
import hashlib,os,shutil,subprocess,sys,urllib.request
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
    if os.name!='nt' or not shutil.which('node'):
        raise SystemExit('Windows, Python 3 and Node.js 22 or newer are required.')
    target=ROOT/'build/smk37/stock-pro.fwsc'
    expected='2d98ce72530e71d617384963423820536f094501299e2cfec0a8d3c3fb6bcfb0'
    if not target.is_file() or hashlib.sha256(target.read_bytes()).hexdigest()!=expected:
        url='https://raw.githubusercontent.com/jonathaslacerda/smk-37-pro-docs/ab85d7340163ba9e171dc9396e31c034829bbc02/firmware/smk37pro/SMK-37_Pro_016.fwsc'
        with urllib.request.urlopen(url,timeout=45) as response:data=response.read(32*1024*1024+1)
        if hashlib.sha256(data).hexdigest()!=expected:raise SystemExit('Official SMK Pro package checksum mismatch.')
        target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
    subprocess.run([sys.executable,'-m','pip','install','--target',str(ROOT/'build/python-deps'),'mido==1.3.3','python-rtmidi==1.5.8'],check=True)
    subprocess.run(['node',str(ROOT/'tools/native_smk_package.mjs'),'install','public',str(ROOT/'build/native-setup-check')],cwd=ROOT,check=True)
    print('Ready. Run: python tools/serve_native_smk.py --allow-hosted')
if __name__=='__main__':main()
