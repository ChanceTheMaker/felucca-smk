// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {probeMidiIdentity,identityEvidence,midiInventory} from './midi-diagnostics.js';
import {Updater,pack7} from './fm1ota.js?v=progress1';
const QUERY=[0xf0,0,0x32,0x45,0,0,0,0x40,0x7f,0xf7];
const body=new Uint8Array(34);body.set([0,0x59,0x11]);body.set(new TextEncoder().encode('ota-SMK-37_016'),6);
const packet=[0xf0,...pack7(body),0xf7];
class Port extends EventTarget {
  constructor(type,name,id){super();Object.assign(this,{type,name,id,state:'connected',connection:'closed',manufacturer:'Test',sent:[],closed:0});}
  async open(){this.connection='open';}
  async close(){this.connection='closed';this.closed++;}
  send(bytes){this.sent.push(bytes);queueMicrotask(()=>this.respond?.());}
  receive(data){const event=new Event('midimessage');event.data=data;this.dispatchEvent(event);this.onmidimessage?.(event);}
}
const input=new Port('input','SMK-37 Elite MIDI IN','in'), output=new Port('output','USB MIDI OUT','out');
const access={inputs:new Map([['in',input]]),outputs:new Map([['out',output]])};
output.respond=()=>{input.receive([0x90,60,100]);input.receive(packet);};
const logs=[];
const found=await probeMidiIdentity(access,'out',{timeoutMs:5,log:s=>logs.push(s)});
assert.equal(found.length,1);assert.equal(found[0].model,'ota-SMK-37');
assert.equal(found[0].input,input.name);assert.equal(found[0].output,output.name);
assert.deepEqual(output.sent,[QUERY],'only identity query sent; no update/START/flash commands');
assert.equal(input.closed,1);assert.equal(output.closed,1);
assert.equal(midiInventory(access).length,2);
const logged=logs.length;input.receive(packet);assert.equal(logs.length,logged,'listener removed');
assert.equal(identityEvidence([0x90,60,100]),null);
assert.equal(identityEvidence([0xf0,1,2,3,0xf7]),null);
await input.open();await output.open();output.respond=()=>{};
assert.equal((await probeMidiIdentity(access,'out',{timeoutMs:5})).length,0);
assert.equal(input.connection,'open','pre-existing input remains open');assert.equal(output.connection,'open');
output.send=()=>{throw Error('unplugged');};
await assert.rejects(probeMidiIdentity(access,'out',{timeoutMs:5}),/unplugged/);
const after=logs.length;input.receive(packet);assert.equal(logs.length,after);
// Added updater logs must not weaken its existing pairing or identity gate.
const reports=[];let starts=0;
output.send=bytes=>{assert.deepEqual(bytes,QUERY);starts++;queueMicrotask(()=>input.receive(packet));};
const updater=new Updater(access,s=>reports.push(s),/smk|usb/i);
assert.equal(await updater.find(()=>true),null);
assert.equal(starts,0);assert(reports.some(s=>s.includes('No same-name output')));
output.name=input.name;
assert.equal(await updater.find(id=>id.model==='ota-SMK-37 Elite'),null);
assert(reports.some(s=>s.includes('ota-SMK-37_016')&&s.includes('not the expected identity')));
const accepted=await updater.find(id=>id.model==='ota-SMK-37');assert(accepted);accepted.link.close();
console.log('PASS: differently named ports diagnosed, identity-only sends, no-reply/error cleanup, existing ports preserved, updater gates unchanged');
