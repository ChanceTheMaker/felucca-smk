# SPDX-License-Identifier: GPL-3.0-only
"""Loopback-only SMK Pro installer. USB stages run outside the browser process."""
from pathlib import Path
import hashlib,json,os,re,shutil,subprocess,sys,threading,time,uuid,tempfile

ROOT=Path(__file__).resolve().parents[1]
WORK=ROOT/'build/native-smk'

class Bridge:
    def __init__(self,owner=False):
        self.owner=owner
        self.jobs={}
        self.lock=threading.Lock()
        self.available=os.name=='nt' and bool(shutil.which('node')) and (ROOT/'build/python-deps/mido').is_dir() and (ROOT/'build/smk37/stock-pro.fwsc').is_file()

    def start(self,action):
        if not self.available:raise ValueError('Local SMK USB service is unavailable')
        if action not in ('connect','install','restore'):raise ValueError('Invalid action')
        with self.lock:
            if any(j['state']=='running' for j in self.jobs.values()):raise ValueError('An SMK operation is already running')
            job={'id':uuid.uuid4().hex,'state':'running','phase':'connect' if action=='connect' else 'prepare','logs':[]}
            self.jobs[job['id']]=job
        threading.Thread(target=self._run,args=(job,action),daemon=True).start()
        return job['id']

    def status(self,key):
        with self.lock:return json.loads(json.dumps(self.jobs.get(key)))

    def _event(self,job,event):
        with self.lock:
            if event.get('log'):job['logs']=(job['logs']+[event['log']])[-200:]
            for key in ('phase','bytes','total','identity'):
                if key in event:job[key]=event[key]

    def _child(self,job,stage,directory,expected=''):
        command=[sys.executable,str(Path(__file__)),stage,str(directory),expected]
        child=subprocess.Popen(command,cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
        # Native stages also enforce their own protocol deadlines. The watchdog
        # stops a stuck driver process without power-cycling the waiting loader.
        timer=threading.Timer(330,child.kill);timer.start();result=None
        try:
            for line in child.stdout:
                event=json.loads(line);self._event(job,event)
                if 'ok' in event:result=event
            code=child.wait();error=child.stderr.read()
            if code or not result or not result['ok']:raise RuntimeError((result or {}).get('error') or error[-1000:] or 'USB stage stopped. Keep the SMK powered and retry.')
            return result
        finally:timer.cancel()

    def _run(self,job,action):
        device_lock=None
        try:
            WORK.mkdir(parents=True,exist_ok=True)
            # All three app helpers share this OS lock, including separate checkouts.
            # A competing site must not open MIDI while this update is running.
            import msvcrt
            device_lock=open(Path(tempfile.gettempdir())/'fm1-smk-native-usb.lock','a+b');device_lock.seek(0)
            # Windows can lock beyond EOF; do not read a byte another helper locked.
            try:msvcrt.locking(device_lock.fileno(),msvcrt.LK_NBLCK,1)
            except OSError:raise RuntimeError('Another local SMK operation is running. Wait for it to finish.')
            directory=WORK/job['id']
            if action=='connect':
                result=self._child(job,'probe',directory)
            else:
                self._event(job,{'log':'Verifying the selected SMK Pro firmware.','phase':'prepare'})
                subprocess.run(['node',str(ROOT/'tools/native_smk_package.mjs'),action,'owner' if self.owner else 'public',str(directory)],cwd=ROOT,check=True,capture_output=True,text=True,timeout=30,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
                manifest=json.loads((directory/'verified.json').read_text())
                first=self._child(job,'begin',directory)
                if first['handoff']:time.sleep(1)
                self._child(job,'write',directory,first['loader'])
                self._event(job,{'log':'Transfer verified. Checking the restarted firmware identity...','phase':'reboot'})
                time.sleep(1)
                result=self._child(job,'verify',directory,manifest['expected'])
                self._event(job,{'log':'Installed and restart verified: '+result['identity']})
            with self.lock:job.update(state='done',identity=result['identity'])
        except Exception as exc:
            message=str(exc)
            if isinstance(exc,subprocess.CalledProcessError):message=exc.stderr[-1200:]
            self._event(job,{'log':message})
            with self.lock:job.update(state='error',error=message)
        finally:
            if device_lock:device_lock.close()

def emit(**event):print(json.dumps(event),flush=True)

def worker(stage,directory,expected):
    sys.path[:0]=[str(ROOT/'build/python-deps'),str(ROOT/'tools')]
    import native_smk_wire as wire
    wire.PORT_RE=re.compile(r'^(?!MIDIIN).*(?:SMK|GameDude|NESynth|Felucca|Felucca|Melodee|USB-Midi|Doom)',re.I)
    up=wire.Updater(wire.MidoBackend())
    allowed=lambda i:i.model in ('SMK-37 Pro','ota-SMK-37 Pro','SMK-37','ota-SMK-37','Doom-SMK-37','ota-Doom-SMK-37')
    if stage in ('probe','verify'):
        dev=up.wait_for(lambda i:allowed(i) and not (stage=='verify' and i.loader),35 if stage=='verify' else 6)
        if not dev:raise RuntimeError('SMK Pro did not reply. Keep it powered, check USB and close other MIDI apps.')
        text=dev.id.text;dev.link.close()
        if stage=='verify' and text!=expected:raise RuntimeError('Unexpected restart: '+text+'; expected '+expected)
        emit(ok=True,identity=text,log='Connected: '+text);return
    manifest=json.loads((directory/'verified.json').read_text())
    if manifest['model']!='SMK-37 Pro' or manifest['expected'] not in ('SMK-37_927','SMK-37 Pro_016'):raise RuntimeError('Invalid verified package')
    def payload(kind):
        data=(directory/(kind+'.bin')).read_bytes();meta=manifest[kind]
        if len(data)!=meta['bytes'] or hashlib.sha256(data).hexdigest()!=meta['sha256']:raise RuntimeError('Firmware changed after verification')
        return data
    def serve(link,data,finish):
        writing=finish==wire.FINISH_WRITE
        emit(phase='transfer' if writing else 'loader',bytes=0,total=len(data))
        seen=bytearray(len(data));count=0;last=time.monotonic();reported=last;end=last+(280 if writing else 25)
        while time.monotonic()<end:
            pkt=link.read(.5)
            if not pkt:
                if link.lost or time.monotonic()-last>15:raise RuntimeError('USB transfer interrupted. Keep the SMK powered and retry; no automatic retry was made.')
                continue
            request=wire.parse_request(pkt)
            if not request:continue
            flags,address,size=request;last=time.monotonic()
            if address in (wire.FINISH_CHECK,wire.FINISH_WRITE):
                if not link.send(wire.response(address,b'success\0')):raise RuntimeError('Finish acknowledgement failed')
                if address==finish:return
                continue
            if size>512 or address+size>len(data):raise RuntimeError('Device requested bytes outside the verified package')
            time.sleep(.01)
            if not link.send(wire.response(address,data[address:address+size],flags)):raise RuntimeError('USB response failed')
            count+=seen[address:address+size].count(0);seen[address:address+size]=b'\1'*size
            if writing and last-reported>.25:emit(phase='transfer',bytes=count,total=len(data));reported=last
        raise RuntimeError('USB transfer timed out. Keep the SMK powered and retry.')
    if stage=='begin':
        dev=up.wait_for(allowed,8)
        if not dev:raise RuntimeError('No SMK Pro replied. No programming started.')
        emit(identity=dev.id.text,log='Connected: '+dev.id.text)
        if dev.id.loader:
            loader=dev.id.text;dev.link.close();emit(ok=True,loader=loader,handoff=False);return
        if dev.id.model=='SMK-37 Pro':
            if dev.id.version!=16:raise RuntimeError('First installation requires SMK Pro 016')
            data=payload('image');emit(log='Preparing the official SMK loader. Keep power connected.',phase='loader')
            if not dev.link.send(wire.UPGRADE):raise RuntimeError('Loader start failed')
            serve(dev.link,data,wire.FINISH_CHECK)
        else:
            if dev.id.version<(7 if dev.id.model.startswith('Doom') else 922):raise RuntimeError('This older updater requires migration')
            emit(phase='loader',log='Entering the SMK USB loader.')
            if not dev.link.send(wire.UPGRADE):raise RuntimeError('Loader start failed')
        emit(ok=True,loader='ota-'+dev.id.text,handoff=True)
    elif stage=='write':
        dev=up.wait_for(lambda i:allowed(i) and i.loader and i.text==expected,25)
        if not dev:raise RuntimeError('The expected SMK loader did not appear. Keep it powered and retry.')
        data=payload('image' if dev.id.model=='ota-SMK-37 Pro' else 'packet')
        emit(log='Installing '+manifest['expected']+'. Keep power and USB connected.')
        if not dev.link.send(wire.UPGRADE):raise RuntimeError('Programming start failed')
        serve(dev.link,data,wire.FINISH_WRITE);emit(ok=True,phase='reboot')
    else:raise RuntimeError('Invalid USB stage')
    # RtMidi handle destructors race USB removal on Windows. End this short-lived
    # transport process; rediscovery happens in a fresh process after re-enumeration.

if __name__=='__main__':
    try:worker(sys.argv[1],Path(sys.argv[2]),sys.argv[3] if len(sys.argv)>3 else '');code=0
    except Exception as exc:emit(ok=False,error=str(exc));code=1
    sys.stdout.flush();sys.stderr.flush();os._exit(code)
