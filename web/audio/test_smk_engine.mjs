// Native simulator regression. No USB, MIDI devices, or physical storage.
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import vm from 'node:vm';
const {instance:{exports:e}}=await WebAssembly.instantiate(readFileSync(new URL('./smk-engine.wasm',import.meta.url)));
e.synth_init();const mem=new Uint8Array(e.memory.buffer);
function command(...args){const b=[125,70,76,...args];mem.set(b,e.fw_request());const n=e.fw_command(b.length);assert(n>0,`command ${args} replied`);return mem.slice(e.fw_reply(),e.fw_reply()+n);}
assert.match(new TextDecoder().decode(command(1)),/FELUCCA SMK37 BROWSER/);
const html=readFileSync(new URL('../editor.html',import.meta.url),'utf8');
const proto=html.slice(html.indexOf('/*PROTO-BEGIN*/'),html.indexOf('/*PROTO-END*/'));
const {parse,CMD,req,SMP,buildSlot}=vm.runInNewContext(proto+';({parse,CMD,req,SMP,buildSlot})',{console,setTimeout,clearTimeout});
const decode=(...args)=>parse[args[0]](command(...args).slice(5,-1),info);
let info=parse[CMD.INFO](command(1).slice(5,-1));
assert.equal(info.ntrk,4);assert.equal(info.pcount,111);
for(let eng=0;eng<info.nengines;eng++){
  decode(CMD.PRESET,eng,0);
  assert.equal(decode(CMD.NAMES,eng).titles.length,2);
  for(let p=0;p<info.pcount;p++){const d=decode(CMD.DESC,0,p);assert(d.min<=d.max);}
}
decode(CMD.PRESET,0,0);
assert.equal(decode(CMD.SMP_INFO).nslots,3);
assert.equal(decode(CMD.DUMP).p.length,111);
const {hdr,data}=buildSlot('WEB TEST',[{s:Int16Array.from({length:3000},(_,i)=>Math.round(8000*Math.sin(i/7))),root:60}]);
const request=([cmd,args])=>decode(cmd,...args);
assert.equal(request(req.smpBegin(1)).rc,0);
for(let off=0;off<data.length;off+=256)assert.equal(request(req.smpWrite(1,SMP.DATA_OFF+off,data.subarray(off,off+256))).rc,0);
assert.equal(request(req.smpEnd(1,hdr)).rc,0);assert.equal(decode(CMD.SMP_INFO).slots[1].name,'WEB TEST');
const before=command(2,0,1);command(3,0,1,42,64);assert.equal(command(2,0,1)[7],42);
const snapshot=mem.slice(e.fw_snapshot(),e.fw_snapshot()+e.fw_snapshot_size());
command(3,0,1,9,64);mem.set(snapshot,e.fw_snapshot());assert.equal(e.fw_restore(),0);assert.equal(command(2,0,1)[7],42);
assert.equal(decode(CMD.SMP_INFO).slots[1].name,'WEB TEST');
command(3,0,1,before[7],before[8]);
command(9,1,0);assert.equal(command(9,2,0)[7],1);
for(let b=0;b<10;b++)e.synth_render();
e.fw_event(2,0,110);let peak=0;for(let b=0;b<150;b++){const p=e.synth_render();for(const v of new Int32Array(e.memory.buffer,p,64))peak=Math.max(peak,Math.abs(v));}assert(peak>0,'keyboard produces audio');
e.fw_release();assert.equal(new Uint32Array(e.memory.buffer,e.fw_status(),12)[1],0);
let screen=e.fw_screen();assert(mem.slice(screen,screen+115200).some(v=>v),'native LCD renders');
for(let b=0;b<18;b++){e.fw_event(0,b,1);e.fw_event(0,b,0);e.fw_screen();}
e.fw_release();
e.fw_event(4,0,1023);assert.equal(new Uint32Array(e.memory.buffer,e.fw_status(),16)[12],1023);
assert.equal(e.fw_command(0),0);console.log('PASS: all native engine descriptors parsed, three sample slots, SET/GET, snapshot, project storage, key audio, release, LCD, faders and 18 buttons; peak',peak);
