"""Compile the actual USB string descriptors and check their host-visible names.
No MIDI/device access. Uses cc, or WSL cc on Windows.
"""
import json,os,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def linux(p):return subprocess.check_output(['wsl','-e','wslpath','-a',p.as_posix()],text=True).strip()
for i,case in enumerate(json.loads((ROOT/'tests/usb_product_names.json').read_text())):
 source=(ROOT/case['source']).read_text(encoding='utf-8')
 start=source.index('static const uint8_t STR0');end=source.index('static int get_desc',start)
 fragment=source[start:end]
 expected=json.dumps(case['name'])
 code='#include <stdint.h>\n#include <string.h>\n#include <assert.h>\n'+''.join('#define '+k+' '+str(v)+'\n' for k,v in case['defines'].items())+fragment+'\nint main(void){const char *name='+expected+';assert(STR2[0]==sizeof STR2);assert(STR2[1]==3);assert(sizeof STR2==2+2*strlen(name));for(unsigned i=0;i<strlen(name);i++){assert(STR2[2+2*i]==(uint8_t)name[i]);assert(STR2[3+2*i]==0);}return 0;}\n'
 out=ROOT/'build/usb-name-tests';out.mkdir(parents=True,exist_ok=True);c=out/f'name{i}.c';exe=out/f'name{i}'
 c.write_text(code,encoding='utf-8')
 if os.name=='nt':cmd=['wsl','-e','cc','-std=c11','-Wall','-Werror','-Wno-unused-const-variable',linux(c),'-o',linux(exe)];run=['wsl','-e',linux(exe)]
 else:cmd=['cc','-std=c11','-Wall','-Werror','-Wno-unused-const-variable',str(c),'-o',str(exe)];run=[str(exe)]
 subprocess.run(cmd,check=True);subprocess.run(run,check=True)
 print('PASS '+case['name'],flush=True)
