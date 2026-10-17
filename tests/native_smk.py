"""Native transport checks using a fake wire; never accesses MIDI hardware."""
import contextlib,hashlib,importlib.util,io,json,sys,tempfile,types,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('native_smk',ROOT/'tools/native_smk.py')
native=importlib.util.module_from_spec(spec);spec.loader.exec_module(native)
class NativeSmkTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory();self.directory=Path(self.tmp.name)
        self.image=b'I'*64;self.packet=b'P'*64
        manifest={'model':'SMK-37 Pro','expected':'SMK-37_927'}
        for kind,data in [('image',self.image),('packet',self.packet)]:
            (self.directory/(kind+'.bin')).write_bytes(data);manifest[kind]={'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}
        (self.directory/'verified.json').write_text(json.dumps(manifest))
        self.sent=[];self.requests=[];self.closed=0;self.identity('SMK-37 Pro_016')
        case=self
        class Link:
            lost=True
            def send(self,data):case.sent.append(data);return True
            def read(self,timeout):return case.requests.pop(0) if case.requests else None
            def close(self):case.closed+=1
        class Updater:
            def __init__(self,backend):pass
            def wait_for(self,want,seconds):return types.SimpleNamespace(id=case.id,link=Link()) if want(case.id) else None
        self.previous=sys.modules.get('native_smk_wire')
        sys.modules['native_smk_wire']=types.SimpleNamespace(Updater=Updater,MidoBackend=object,UPGRADE='START',FINISH_CHECK=0xe0000000,FINISH_WRITE=0xf0000000,parse_request=lambda p:p,response=lambda a,d,f=0:(a,d,f))
    def tearDown(self):
        if self.previous is None:sys.modules.pop('native_smk_wire',None)
        else:sys.modules['native_smk_wire']=self.previous
        self.tmp.cleanup()
    def identity(self,text):
        model,version=text.rsplit('_',1);self.id=types.SimpleNamespace(text=text,model=model,version=int(version),loader=model.startswith('ota-'))
    def run_stage(self,stage,expected=''):
        with contextlib.redirect_stdout(io.StringIO()) as output:native.worker(stage,self.directory,expected)
        return [json.loads(line) for line in output.getvalue().splitlines()]
    def test_other_instrument_never_started(self):
        self.identity('FM-1_938')
        with self.assertRaisesRegex(RuntimeError,'No SMK'):self.run_stage('begin')
        self.assertEqual(self.sent,[])
    def test_corruption_never_started(self):
        (self.directory/'image.bin').write_bytes(b'bad')
        with self.assertRaisesRegex(RuntimeError,'changed'):self.run_stage('begin')
        self.assertEqual(self.sent,[])
    def test_stock_handoff(self):
        self.requests=[(0,0,16),(0,0xe0000000,8)]
        rows=self.run_stage('begin');self.assertEqual(rows[-1]['loader'],'ota-SMK-37 Pro_016')
        self.assertEqual(self.sent[1],(0,b'I'*16,0));self.assertEqual(self.closed,0)
    def test_custom_handoff_and_old_version(self):
        self.identity('SMK-37_927');self.assertTrue(self.run_stage('begin')[-1]['handoff']);self.assertEqual(self.sent,['START'])
        self.sent=[];self.identity('SMK-37_921')
        with self.assertRaisesRegex(RuntimeError,'migration'):self.run_stage('begin')
        self.assertEqual(self.sent,[])
    def test_loader_resume_and_exact_selection(self):
        self.identity('ota-SMK-37 Pro_016');self.assertFalse(self.run_stage('begin')[-1]['handoff']);self.assertEqual(self.sent,[])
        with self.assertRaisesRegex(RuntimeError,'expected SMK loader'):self.run_stage('write','ota-SMK-37_927')
        self.assertEqual(self.sent,[])
    def test_both_transfer_formats(self):
        for identity,byte in [('ota-SMK-37 Pro_016',b'I'),('ota-SMK-37_927',b'P')]:
            self.identity(identity);self.sent=[];self.requests=[(0,0,16),(0,0xf0000000,8)]
            self.assertTrue(self.run_stage('write',identity)[-1]['ok']);self.assertEqual(self.sent[1],(0,byte*16,0))
    def test_bounds_and_missing_data_stop(self):
        self.identity('ota-SMK-37_927');self.requests=[(0,60,16)]
        with self.assertRaisesRegex(RuntimeError,'outside'):self.run_stage('write',self.id.text)
        self.assertEqual(self.sent,['START'])
        self.sent=[]
        with self.assertRaisesRegex(RuntimeError,'interrupted'):self.run_stage('write',self.id.text)
        self.assertEqual(self.sent,['START'])
    def test_exact_restart(self):
        self.identity('SMK-37_927');self.assertEqual(self.run_stage('verify','SMK-37_927')[-1]['identity'],self.id.text)
        with self.assertRaisesRegex(RuntimeError,'Unexpected restart'):self.run_stage('verify','SMK-37 Pro_016')
if __name__=='__main__':unittest.main()
