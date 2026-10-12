// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {pack7,unpack7} from './fm1ota.js?v=progress1';
import {QUERY} from './midi-diagnostics.js?v=elite-handoff2';
import {buildStockHandoffPackage,smkLogicalImage} from './smk37-package.js?v=elite-handoff2';
import {serveLoaderOnly,captureHandoff,diagnoseStockHandoff} from './smk37-handoff-diagnostics.js?v=elite-handoff2';
const START=[0xf0,0x22,0x24,0x35,0x7f,0xf7];
const tick=()=>new Promise(r=>setTimeout(r,0));
const identity=text=>{const body=new Uint8Array(34);body.set([0,0x59,0x11]);body.set(new TextEncoder().encode(text),6);return [0xf0,...pack7(body),0xf7];};
function request(addr,len=0,fl=0){const body=[0,0x59,0x30,8,0,0,fl,addr&255,(addr>>>8)&255,(addr>>>16)&255,(addr>>>24)&255,len&255,(len>>>8)&255,(len>>>16)&255];body.push(~body.slice(6).reduce((a,b)=>a+b,0)&255);return [0xf0,...pack7(body),0xf7];}
const responseAddress=bytes=>{const u=unpack7(bytes.slice(1,-1));return (u[7]|u[8]<<8|u[9]<<16|u[10]<<24)>>>0;};
class Port extends EventTarget{
  constructor(type,id,name=id){super();Object.assign(this,{type,id,name,state:'connected',connection:'closed',manufacturer:'Test',sent:[],closed:0});}
  async open(){this.connection='open';}
  async close(){this.connection='closed';this.closed++;}
  send(data){const bytes=Array.from(data);this.sent.push(bytes);this.respond?.(bytes);}
  receive(data){const e=new Event('midimessage');e.data=data;this.dispatchEvent(e);}
}
class Access extends EventTarget{constructor(){super();this.inputs=new Map();this.outputs=new Map();}change(){this.dispatchEvent(new Event('statechange'));}}
function pair(){const a=new Access(),i=new Port('input','stock-in','Stock input'),o=new Port('output','stock-out','SMK-37 Elite -Private');a.inputs.set(i.id,i);a.outputs.set(o.id,o);return {a,i,o};}
const pkg={image:Uint8Array.from({length:4096},(_,i)=>i&255),ranges:[[0,1024],[2048,2560]]};
// Simulated stage-1 requests cannot escape the verified header/loader ranges.
{
  const {i,o}=pair();const reads=[request(0,512),request(2048,512),request(0xe0000000)];
  o.respond=()=>{if(reads.length)queueMicrotask(()=>i.receive(reads.shift()));};
  const result=await serveLoaderOnly(i,o,pkg,{idleMs:100});assert.equal(result.served,2);
  assert.deepEqual(o.sent[0],START);assert.equal(o.sent.filter(x=>JSON.stringify(x)===JSON.stringify(START)).length,1);
  assert.deepEqual(o.sent.slice(1).map(responseAddress),[0,2048,0xe0000000]);
  const sent=o.sent.length;i.receive(request(0,10));await tick();assert.equal(o.sent.length,sent,'listener removed');
}
for(const [addr,len,pattern] of [[1024,10,/outside/],[1020,8,/outside/],[2048,513,/outside/],[0,0,/outside/],[0xf0000000,0,/flash-stage/],[0xe0000000,0,/before any/]]){
  const {i,o}=pair();o.respond=()=>queueMicrotask(()=>i.receive(request(addr,len)));
  await assert.rejects(serveLoaderOnly(i,o,pkg,{idleMs:100}),pattern);assert.equal(o.sent.length,1,'blocked request has no response');
}
{
  const {i,o}=pair(),abort=new AbortController();abort.abort();
  await assert.rejects(serveLoaderOnly(i,o,pkg,{signal:abort.signal}),/stopped/);assert.equal(o.sent.length,0);
  await assert.rejects(serveLoaderOnly(i,o,pkg,{idleMs:1}),/stopped requesting/);
}
{
  const {i,o}=pair();o.respond=()=>{i.state='disconnected';};
  await assert.rejects(serveLoaderOnly(i,o,pkg),/disconnected/);assert.equal(o.sent.length,1);
}
// Capture follows new port IDs and unequal names. It sends only identity queries.
{
  const {a,i,o}=pair(),logs=[];await i.open();
  const capture=captureHandoff(a,{log:x=>logs.push(x)});await capture.ready();
  const nextIn=new Port('input','new-in','Different loader input'),nextOut=new Port('output','new-out','Loader output');
  a.inputs.set(nextIn.id,nextIn);a.outputs.set(nextOut.id,nextOut);a.change();
  nextOut.respond=()=>{nextIn.receive([0x90,60,100]);nextIn.receive(identity('ota-SMK-37_016'));};
  await capture.query(nextOut);assert.deepEqual(nextOut.sent,[Array.from(QUERY)]);
  assert(logs.some(x=>x.includes('Handoff identity reply:')&&x.includes('ota-SMK-37_016')&&x.includes('new-in')));
  assert(!logs.some(x=>x.includes('90 3c 64')));
  await capture.stop();assert.equal(i.connection,'open');assert.equal(nextIn.connection,'closed');assert.equal(nextOut.connection,'closed');
  const count=logs.length;nextIn.receive(identity('SMK-37_016'));a.change();assert.equal(logs.length,count,'all capture listeners removed');
}
await assert.rejects(diagnoseStockHandoff(new Access(),'x',pkg),/hash-verified/);
await assert.rejects(buildStockHandoffPackage(new Uint8Array(16)),/unmodified official/);
console.log('PASS: stage-1 allowlist, single START, flash/cross-boundary/size refusal, abort/timeout/disconnect, re-enumeration, unequal names, identity-only capture, port cleanup');
// Optional original vendor fixtures stay outside source control.
for(const path of process.argv.slice(2)){
  const stock=new Uint8Array(readFileSync(path)),verified=await buildStockHandoffPackage(stock);
  const logical=smkLogicalImage(stock).raw;
  for(let j=0;j<verified.image.length;j++)assert.equal(verified.image[j],verified.ranges.some(([lo,hi])=>j>=lo&&j<hi)?logical[j]:0);
  assert.equal(verified.ranges[1][1]-verified.ranges[1][0],19969);
  const corrupted=stock.slice();corrupted[corrupted.length-1]^=1;await assert.rejects(buildStockHandoffPackage(corrupted),/unmodified official/);
  const {a,i,o}=pair(),logs=[],iac=new Port('output','iac','Driver IAC MPC MMC');a.outputs.set(iac.id,iac);
  const requests=[];for(const [lo,hi] of verified.ranges)for(let addr=lo;addr<hi;addr+=512)requests.push(request(addr,Math.min(512,hi-addr)));
  requests.push(request(0xe0000000));
  const nextIn=new Port('input','ota-in','Loader INPUT'),nextOut=new Port('output','ota-out','Loader OUTPUT');
  nextOut.respond=bytes=>{assert.deepEqual(bytes,Array.from(QUERY));queueMicrotask(()=>nextIn.receive(identity('ota-SMK-37_016')));};
  o.respond=bytes=>{
    if(JSON.stringify(bytes)===JSON.stringify(QUERY)){queueMicrotask(()=>i.receive(identity(verified.stockModel+'_016')));return;}
    if(JSON.stringify(bytes)!==JSON.stringify(START)&&responseAddress(bytes)===0xe0000000){
      i.state=o.state='disconnected';a.inputs.set(nextIn.id,nextIn);a.outputs.set(nextOut.id,nextOut);a.change();return;
    }
    if(requests.length)queueMicrotask(()=>i.receive(requests.shift()));
  };
  await diagnoseStockHandoff(a,o.id,verified,{probeMs:1,observeMs:30,queryMs:2,log:x=>logs.push(x)});
  assert.equal(o.sent.filter(x=>JSON.stringify(x)===JSON.stringify(START)).length,1);assert.equal(iac.sent.length,0);
  assert(nextOut.sent.length>0);assert(logs.some(x=>x.includes('Handoff identity reply:')&&x.includes('ota-SMK-37_016')));
  assert.equal(nextIn.connection,'closed');assert.equal(nextOut.connection,'closed');
  for(const bytes of o.sent.slice(2)){
    const addr=responseAddress(bytes);if(addr===0xe0000000)continue;
    const u=unpack7(bytes.slice(1,-1)),len=u[11]|u[12]<<8;assert(verified.ranges.some(([lo,hi])=>addr>=lo&&addr+len<=hi));
  }
  // A stock model mismatch sends only the identity query, never START.
  const bad=pair();bad.o.respond=()=>bad.i.receive(identity('SMK-37_922'));
  await assert.rejects(diagnoseStockHandoff(bad.a,bad.o.id,verified,{probeMs:1}),/Expected exactly one/);assert.deepEqual(bad.o.sent,[Array.from(QUERY)]);
  console.log('PASS: verified '+verified.stockModel+' fixture, OTA-only payload, complete mock handoff, unrelated IAC excluded, alternate identity logged, model gate');
}
