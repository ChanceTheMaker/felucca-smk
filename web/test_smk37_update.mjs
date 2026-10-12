// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {webcrypto} from 'node:crypto';
import {parsePackage,crc32,HEADER,LO,HI,installSmk} from './smk37-update.js?v=progress1';
import {Updater} from './fm1ota.js?v=progress1';
globalThis.crypto??=webcrypto;
const bytes=new Uint8Array(HEADER+HI-LO),v=new DataView(bytes.buffer),enc=new TextEncoder();
bytes.set(enc.encode('SMK37-SALT-USB1'));
for(const [off,x] of [[16,1],[20,LO],[24,HI],[28,HI-LO]])v.setUint32(off,x,true);
bytes.set(enc.encode('SMK-37_911'),48);bytes.set(enc.encode('SMK37 TEST11 USB'),80);
v.setUint32(40,crc32(bytes.subarray(HEADER)),true);
for(let s=0;s<(HI-LO)/4096;s++)v.setUint32(128+4*s,crc32(bytes.subarray(HEADER+s*4096,HEADER+(s+1)*4096)),true);
v.setUint32(44,crc32(bytes.subarray(0,HEADER)),true);
const hash=async x=>Buffer.from(await crypto.subtle.digest('SHA-256',x)).toString('hex');
const pkg=await parsePackage(bytes,await hash(bytes));assert.equal(pkg.product,'SMK-37_911');
await assert.rejects(parsePackage(bytes,'0'.repeat(64)),/reviewed/);
const bad=bytes.slice();bad[HEADER+10]^=1;await assert.rejects(parsePackage(bad,await hash(bad)),/checksum/);
await assert.rejects(parsePackage(new Uint8Array(0x100000),'x'),/size/);
let sent=0,closed=0,serve=0;
const link=()=>({send:()=>{sent++;return true},close:()=>closed++});
const original={};for(const n of ['findAny','waitForAny','serve'])original[n]=Updater.prototype[n];
try{
 Updater.prototype.findAny=async function(filter){assert.equal(filter({model:'FM-1'}),false);assert.equal(filter({model:'ota-FM-1'}),false);return null;};
 await assert.rejects(installSmk({},pkg),/No updater-enabled/);assert.equal(sent,0);
 Updater.prototype.findAny=async()=>({id:{model:'SMK-37'},link:link()});
 let waiting=0;
 Updater.prototype.waitForAny=async filter=>{waiting++;const id=waiting===1?{model:'ota-SMK-37'}:{model:'SMK-37',text:pkg.product};assert.ok(filter(id));return {id,link:link()};};
 Updater.prototype.serve=async(l,b,end)=>{serve++;assert.equal(b,bytes);assert.equal(end,0xf0000000);return {finished:true};};
 await installSmk({},pkg);assert.equal(sent,2);assert.equal(serve,1);assert.equal(closed,3);
 Updater.prototype.findAny=async()=>({id:{model:'ota-SMK-37'},link:link()});
 Updater.prototype.serve=async()=>({finished:false});
 await assert.rejects(installSmk({},pkg),/recovery adapter/);assert.equal(closed,4);
}finally{Object.assign(Updater.prototype,original);}
console.log('SMK web updater: package integrity, wrong model isolation, bootstrap, success, disconnect and port cleanup passed');
