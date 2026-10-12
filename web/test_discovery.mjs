// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {Updater,pack7} from './fm1ota.js?v=smk-flow-r1';
const query=[0xf0,0x00,0x32,0x45,0x00,0x00,0x00,0x40,0x7f,0xf7];
function reply(text){const d=new Uint8Array(34);d.set([0,0x59,0x11]);d.set(new TextEncoder().encode(text),6);return [0xf0,...pack7(d),0xf7];}
class Port extends EventTarget{
 constructor(id,name,type){super();Object.assign(this,{id,name,type,state:'connected',connection:'closed',opens:0,closes:0,listeners:0,sends:[]});}
 async open(){this.opens++;if(this.fail)throw Error('busy');this.connection='open';return this;}
 async close(){this.closes++;this.connection='closed';return this;}
 addEventListener(t,f){this.listeners++;super.addEventListener(t,f);}
 removeEventListener(t,f){this.listeners--;super.removeEventListener(t,f);}
 send(bytes){assert.deepEqual([...bytes],query,'Discovery must never send START or payload');this.sends.push(bytes);this.respond?.();}
 receive(text){const ev=new Event('midimessage');ev.data=reply(text);this.dispatchEvent(ev);}
}
const input=new Port('i','Elite input','input'),output=new Port('o','Different loader output','output'),other=new Port('x','IAC','output');
const access={inputs:new Map([['i',input]]),outputs:new Map([['x',other],['o',output]])};
output.respond=()=>input.receive('ota-SMK-37 Elite_016');
const up=new Updater(access,()=>{}),filter=id=>id.model==='ota-SMK-37 Elite';
let dev=await up.findAny(filter,{probeMs:1});assert.equal(dev.link.input,input);assert.equal(dev.link.output,output);assert.equal(input.listeners,0);assert.equal(other.connection,'closed');await dev.link.close();assert.equal(input.connection,'closed');assert.equal(output.connection,'closed');assert.equal(input.onmidimessage,null);
console.log('PASS differently named input/output matched by reply, identity only, ports released');
output.respond=()=>input.receive('FM-1_900');assert.equal(await up.findAny(filter,{probeMs:1}),null);assert.equal(input.connection,'closed');assert.equal(input.listeners,0);
console.log('PASS unrelated FM-1 never selected');
const second=new Port('i2','second Elite','input');access.inputs.set('i2',second);output.respond=()=>{input.receive('ota-SMK-37 Elite_016');second.receive('ota-SMK-37 Elite_016');};
await assert.rejects(()=>up.findAny(filter,{probeMs:1}),/More than one/);for(const p of [input,output,other,second])assert.equal(p.connection,'closed');assert.equal(second.listeners,0);
console.log('PASS multiple responding devices stop discovery and release ports');
access.inputs.delete('i2');input.fail=true;assert.equal(await up.findAny(filter,{probeMs:1}),null);assert.equal(input.listeners,0);
console.log('PASS busy input cannot be selected');
input.fail=false;input.connection='open';output.respond=()=>input.receive('FM-1_900');assert.equal(await up.findAny(filter,{probeMs:1}),null);assert.equal(input.connection,'open');
console.log('PASS pre-existing open port retained');
assert.equal(await up.waitForAny(filter,0),null);
console.log('PASS bounded discovery timeout');

input.connection='closed';input.open=()=>new Promise(()=>{});
assert.equal(await up.findAny(filter,{probeMs:1,openMs:5}),null);
assert.equal(input.listeners,0);
console.log('PASS hung port open times out instead of freezing installer');
