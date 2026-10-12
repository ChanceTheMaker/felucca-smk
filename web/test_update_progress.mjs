// SPDX-License-Identifier: GPL-3.0-only
// Offline: no MIDI access, connected instruments, or network required.
import assert from 'node:assert/strict';
import {createTransferCounter,progressView,createProgress,reportProgress} from './update-progress.js?v=progress1';
const count=createTransferCounter(1024);
assert.deepEqual(count(512,256),{bytes:256,total:1024});
assert.equal(count(512,256).bytes,256);
assert.equal(count(640,256).bytes,384);
assert.equal(count(0,512).bytes,896);
assert.equal(count(896,128).bytes,1024);
assert.equal(count(1020,5),null);assert.equal(count(-1,10),null);
assert.equal(progressView({phase:'transfer',bytes:1024,total:1024}).value,99);
assert.equal(progressView({phase:'reboot'}).value,null);
assert.equal(progressView({phase:'done'}).value,100);
// Minimal DOM verifies the operation lifecycle and that late events cannot
// turn errors or unconfirmed restarts into completed installations.
class Element {
 constructor(){this.dataset={};this.attrs={};this.children=[];this.textContent='';}
 setAttribute(k,v){this.attrs[k]=v;} removeAttribute(k){delete this.attrs[k];if(k==='value')this._value=undefined;}
 append(...nodes){this.children.push(...nodes);} insertAdjacentElement(_,box){this.box=box;}
 set value(v){this._value=v;this.attrs.value=String(v);} get value(){return this._value;}
 remove(){this.removed=true;}
}
const doc=new EventTarget();doc.head=new Element();doc.createElement=()=>new Element();doc.getElementById=()=>null;
globalThis.document=doc;
globalThis.CustomEvent??=class extends Event{constructor(type,{detail}){super(type);this.detail=detail;}};
const anchor=new Element(),ui=createProgress(anchor),box=anchor.box,bar=box.children[1];
assert.equal(box.hidden,true);ui.start('prepare');ui.ready();assert.equal(bar.value,0);
ui.start();reportProgress({phase:'transfer',bytes:512,total:1024});assert.equal(bar.value,50);
ui.fail();assert.equal(box.dataset.state,'error');assert.equal(bar.value,50);
reportProgress({phase:'done'});assert.equal(bar.value,50);
ui.start();reportProgress({phase:'transfer',bytes:1024,total:1024});ui.unconfirmed();assert.equal(bar.value,99);
reportProgress({phase:'done'});assert.equal(bar.value,99);
ui.start();reportProgress({phase:'reboot'});assert.equal(bar.value,undefined);ui.done();assert.equal(bar.value,100);
ui.reset();assert.equal(box.hidden,true);ui.destroy();assert.equal(box.removed,true);
// Exercise the real transport with valid encoded read packets. Retries do not
// inflate progress, and a send failure cannot count bytes or confirm a reboot.
const events=[];doc.addEventListener('firmware-transfer-progress',e=>events.push(e.detail));
for(const path of ['./fm1ota.js', './restore-stock/transport.js']){
 const {Updater}=await import(path+'?v=progress1');
 const pack=data=>{let a=0,n=0,out=[];for(const b of data){a|=b<<n;n+=8;while(n>=7){out.push(a&127);a>>>=7;n-=7;}}if(n)out.push(a&127);return out;};
 const request=(addr,len)=>{const b=[0,0x59,0x30,8,0,0,0,addr&255,(addr>>>8)&255,(addr>>>16)&255,addr>>>24,len&255,len>>>8,0];b.push(~b.slice(6).reduce((a,b)=>a+b,0)&255);return [0xf0,...pack(b),0xf7];};
 const image=Uint8Array.from({length:1024},(_,i)=>i&255),u=new Updater({});
 let packets,sent,callbacks;
 const link={lost:false,async read(){return packets.shift()||null;},send(b){sent.push(b);return true;}};
 const run=async(end,list)=>{events.length=0;sent=[];callbacks=[];packets=list;return u.serve(link,image,end,-1,n=>callbacks.push(n));};
 const result=await run(0xf0000000,[request(0,512),request(0,512),request(512,512),request(0xf0000000,0)]);
 assert.equal(result.finished,true);assert.deepEqual(callbacks,[1,2,3]);
 assert.deepEqual(events.filter(e=>e.phase==='transfer').map(e=>e.bytes),[0,512,512,1024]);
 assert.equal(events.at(-1).phase,'reboot');assert(!events.some(e=>e.phase==='done'));
 assert.deepEqual(sent[0],sent[1]);
 await run(0xe0000000,[request(0,32),request(0xe0000000,0)]);
 assert(events.every(e=>e.phase==='loader'));
 const stopped=await run(0xf0000000,[request(0,256)]);assert.equal(stopped.finished,false);assert.equal(events.at(-1).bytes,256);
 events.length=0;packets=[request(0,512)];link.send=()=>false;
 assert.equal((await u.serve(link,image,0xf0000000,-1)).lost,true);assert.equal(events.at(-1).bytes,0);
 packets=[request(1020,8)];await assert.rejects(u.serve(link,image,0xf0000000,-1),/bad request/);
 console.log('PASS real transport progress:',path);
}
// A presentation listener cannot interrupt transport when the DOM is unavailable.
globalThis.document={dispatchEvent(){throw Error('UI failure');}};
assert.doesNotThrow(()=>reportProgress({phase:'transfer',bytes:1,total:2}));
delete globalThis.document;
console.log('PASS unique-byte counting, completion gate, errors, reset and UI isolation.');
