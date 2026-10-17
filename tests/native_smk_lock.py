"""Verify cross-process exclusion without opening any MIDI ports."""
import os,subprocess,sys,tempfile
from pathlib import Path
if os.name!='nt':
    print('SKIP: Windows file locking test');raise SystemExit(0)
import msvcrt
root=Path(__file__).resolve().parents[1]
with open(Path(tempfile.gettempdir())/'fm1-smk-native-usb.lock','a+b') as lock:
    lock.seek(0)
    if not lock.read(1):lock.write(b'0');lock.flush()
    lock.seek(0);msvcrt.locking(lock.fileno(),msvcrt.LK_NBLCK,1)
    script="""
import sys
sys.path.insert(0,'tools')
from native_smk import Bridge
bridge=Bridge()
def forbidden(*args):raise AssertionError('USB stage must not run while another helper holds the lock')
bridge._child=forbidden
job={'id':'lock-regression-no-usb','state':'running','logs':[]}
bridge._run(job,'connect')
assert job['state']=='error' and 'Another local SMK operation' in job['error'],job
"""
    subprocess.run([sys.executable,'-c',script],cwd=root,check=True,timeout=10)
print('PASS: another process holding the shared lock prevents any USB stage.')
