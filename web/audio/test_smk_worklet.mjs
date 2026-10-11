// Exercise the real worklet off-device at both browser output sample rates.
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import vm from 'node:vm';
const module=await WebAssembly.compile(readFileSync(new URL('./smk-engine.wasm',import.meta.url)));
const source=readFileSync(new URL('./smk-worklet.js',import.meta.url),'utf8');
for(const rate of [44100,48000]){
  let Processor;const messages=[];
  class Base {constructor(){this.port={postMessage:d=>messages.push(d)};}}
  vm.runInNewContext(source,{AudioWorkletProcessor:Base,WebAssembly,Uint8Array,Int32Array,Uint32Array,sampleRate:rate,registerProcessor:(_,C)=>Processor=C});
  const p=new Processor({processorOptions:{module}}),out=[new Float32Array(128),new Float32Array(128)];
  const send=data=>p.port.onmessage({data});
  // The first gesture may arrive before the browser starts audio processing.
  send({type:'visible',value:true});send({type:'panel',kind:2,control:0,value:100});
  let peak=0;
  for(let b=0;b<Math.ceil(rate*3/128);b++){
    p.process([],[out]);for(const channel of out)for(const v of channel){assert(Number.isFinite(v));peak=Math.max(peak,Math.abs(v));}
  }
  assert(peak>.001&&peak<=1);assert(messages.some(m=>m.type==='screen'&&m.pixels.byteLength===115200));
  send({type:'release'});send({type:'snapshot',id:1});const snapshot=messages.find(m=>m.id===1);
  assert(snapshot.buffer.byteLength>1000000);send({type:'restore',id:2,buffer:snapshot.buffer});assert(messages.find(m=>m.id===2).restored);
  send({type:'restore',id:3,buffer:new ArrayBuffer(2)});assert.match(messages.find(m=>m.id===3).error,/Invalid session size/);
  send({type:'command',id:4,bytes:[125,70,76,1]});assert.match(new TextDecoder().decode(messages.find(m=>m.id===4).bytes),/SMK37 BROWSER/);
  console.log(`PASS: ${rate} Hz worklet audio, LCD/status, command transport, release, snapshot/restore and rejected malformed import; peak ${peak.toFixed(3)}`);
}
