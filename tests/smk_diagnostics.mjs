import assert from 'node:assert/strict';
import {parseSmkStatus,smkStatusError} from '../web/smk-diagnostics.js';
import {Updater as InstallUpdater} from '../web/fm1ota.js';
import {Updater as RestoreUpdater} from '../web/restore-stock/transport.js';
function packet(code=14,writing=0){
  const body=[83,77,75,85,1,code,writing,0,0x14,0x60,0xeb,0];
  const raw=[0,0x59,0x7e,12,0,0,...body,~body.reduce((a,b)=>a+b,0)&255];
  const out=[0xf0];let acc=0,bits=0;
  for(const v of raw){acc|=v<<bits;bits+=8;while(bits>=7){out.push(acc&127);acc>>>=7;bits-=7;}}
  if(bits)out.push(acc&127);out.push(0xf7);return new Uint8Array(out);
}
assert.deepEqual(parseSmkStatus(packet()),{code:14,writing:false,flashId:0xeb6014});
assert.match(smkStatusError(parseSmkStatus(packet())).message,/EB6014.*before any application erase/);
assert.match(smkStatusError(parseSmkStatus(packet(9,1))).message,/Keep power and USB connected/);
const corrupt=packet();corrupt[8]^=1;assert.equal(parseSmkStatus(corrupt),null);
for(let n=0;n<packet().length;n++)assert.equal(parseSmkStatus(packet().slice(0,n)),null);
for(const p of [packet(0),packet(16),packet(14,2),null])assert.equal(parseSmkStatus(p),null);
for(const Transport of [InstallUpdater,RestoreUpdater]){
  let reads=0,sends=0;
  const link={read:async()=>{assert.equal(++reads,1);return packet();},send:()=>{++sends;}};
  await assert.rejects(new Transport({},()=>{}).serve(link,new Uint8Array(1024),0xf0000000,1000),/SMK loader error 14.*EB6014/);
  assert.equal(sends,0);
}
console.log('SMK diagnostics: valid failures, protected-state advice and malformed messages passed');
