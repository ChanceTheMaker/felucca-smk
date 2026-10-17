"""Exercise helper HTTP boundaries without starting any device operation."""
import json,subprocess,sys,time,urllib.request,urllib.error,socket
from pathlib import Path
root=Path(__file__).resolve().parents[1]
with socket.socket() as s:s.bind(('127.0.0.1',0));port=s.getsockname()[1]
base=f'http://127.0.0.1:{port}';origin='https://chancethemaker.github.io'
def request(method='GET',path='/api/smk/capabilities',headers=None,body=None):
 req=urllib.request.Request(base+path,data=body,headers=headers or {},method=method)
 try:return urllib.request.urlopen(req,timeout=3)
 except urllib.error.HTTPError as e:return e
for hosted in [False,True]:
 child=subprocess.Popen([sys.executable,'tools/serve_native_smk.py','--port',str(port)]+(['--allow-hosted'] if hosted else []),cwd=root,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
 try:
  for _ in range(30):
   try:r=request();break
   except OSError:time.sleep(.1)
  assert r.status==200
  assert request(headers={'Origin':'https://untrusted.invalid'}).status==403
  assert request(headers={'Host':'untrusted.invalid'}).status==403
  r=request(headers={'Origin':origin});assert r.status==(200 if hosted else 403)
  if not hosted:continue
  assert r.headers['Access-Control-Allow-Origin']==origin
  cap=json.load(r);assert cap['protocol']==1 and len(cap['token'])>=32
  assert request('OPTIONS',headers={'Origin':origin,'Access-Control-Request-Method':'POST','Access-Control-Request-Headers':'content-type,x-felucca-token'}).status==204
  for headers in [{'Origin':origin},{'Origin':origin,'X-Felucca-Token':'wrong'},{'Origin':'null','X-Felucca-Token':cap['token']}]:
   assert request('POST','/api/smk/jobs',headers,b'{}').status==403
  assert cap['app']=='felucca-smk' and cap['firmware']=='SMK-37_927'
  h={'Origin':origin,'X-Felucca-Token':cap['token'],'Content-Type':'application/json'}
  assert request('POST','/api/smk/jobs',h,b'{"action":"shell","model":"smk-pro","app":"felucca-smk"}').status==400
  assert request('POST','/api/smk/jobs',h,b'{"action":"install","model":"fm1","app":"felucca-smk"}').status==400
  assert request('POST','/api/smk/jobs',h,b'{"action":"install","model":"smk-pro","app":"felucca-smk","firmware":"evil"}').status==400
  assert request('POST','/api/smk/jobs',h,b'{"action":"install","model":"smk-pro","app":"wrong-site"}').status==400
 finally:child.terminate();child.wait(timeout=5)
print('PASS: opt-in hosted access, exact Host/Origin, preflight, token and fixed-action boundaries. No device operation started.')
