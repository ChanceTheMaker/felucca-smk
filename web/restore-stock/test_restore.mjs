// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import fs from 'node:fs/promises';
import {webcrypto} from 'node:crypto';
import {prepareStock,restoreStock} from './core.js?v=progress1';
import {Updater} from './transport.js?v=progress1';
import {smkLogicalImage} from './package-smk.js';
import {crc32,HEADER,LO,HI} from './format.js';
globalThis.crypto??=webcrypto;
const mirror='https://raw.githubusercontent.com/jonathaslacerda/smk-37-pro-docs/ab85d7340163ba9e171dc9396e31c034829bbc02/firmware/';
async function file(env,url){if(process.env[env])return new Uint8Array(await fs.readFile(process.env[env]));const r=await fetch(url);assert.ok(r.ok);return new Uint8Array(await r.arrayBuffer());}
const fixtures=[['SMK-37 Pro',await file('PRO_STOCK_FILE',mirror+'smk37pro/SMK-37_Pro_016.fwsc')],['SMK-37 Elite',await file('ELITE_STOCK_FILE',mirror+'smk37elite/SMK-37_Elite_016.fwsc')]];
let cfg,phase,sends,closed,serves,selected;
const id=(model,version)=>({model,version,text:model+'_'+String(version).padStart(3,'0')});
const link=()=>({send(){++sends;return true;},async close(){++closed;}});
Updater.prototype.findAny=async filter=>filter(cfg.source)?{id:cfg.source,link:link()}:null;
Updater.prototype.waitForAny=async filter=>{
  if(cfg.noLoader&&phase===0)return null;
  const next=phase===0?id('ota-'+cfg.source.model,cfg.oldLoader?1:cfg.source.version):(cfg.wrongReturn?id(selected.family==='smk'?'SMK-37':'FM-1',999):id(selected.model,selected.family==='smk'?16:15));
  assert.ok(filter(next),'Expected identity selected');return {id:next,link:link()};
};
Updater.prototype.serve=async function(l,bytes,finish){assert.deepEqual(bytes,selected.bytes||selected.image);serves.push(finish);phase=finish===0xe0000000?0:1;return {finished:!cfg.fail};};
async function flow(pkg,name,options,expected,error){
  selected=pkg;cfg=options;phase=sends=closed=0;serves=[];
  const run=()=>restoreStock({},pkg,()=>{},{confirmModel:()=>cfg.cancel?false:true});
  if(error)await assert.rejects(run,error);else await run();
  assert.deepEqual(serves,expected);if(!expected.length)assert.equal(sends,options.started||0);
  if(cfg.source.model===pkg.model||cfg.source.model.includes('SMK')||cfg.source.model.includes('FM-1'))assert.ok(closed||sends===0);
  console.log('PASS',pkg.model,name);
}
for(const [model,stock] of fixtures){
  const pkg=await prepareStock(stock,model),bytes=pkg.bytes,h=new DataView(bytes.buffer);
  assert.equal(bytes.length,HEADER+HI-LO);assert.equal(h.getUint32(20,true),LO);assert.equal(h.getUint32(24,true),HI);
  assert.equal(new TextDecoder().decode(bytes.slice(48,58)),'SMK-37_016');assert.equal(pkg.product,model+'_016');
  assert.equal(crc32(bytes.subarray(HEADER)),h.getUint32(40,true));
  for(let s=0;s<(HI-LO)/4096;s++)assert.equal(crc32(bytes.subarray(HEADER+s*4096,HEADER+(s+1)*4096)),h.getUint32(128+s*4,true));
  const raw=smkLogicalImage(stock).raw,off=Buffer.from(raw).indexOf(bytes.subarray(HEADER));assert.ok(off>=LO,'Payload is exact official ciphertext');
  if(process.env.RESTORE_TEST_OUT){
    const out=process.env.RESTORE_TEST_OUT;await fs.mkdir(out,{recursive:true});
    const flash=Buffer.alloc(0x100000,0xa5);flash.set(raw.subarray(off-LO,off-LO+HI));flash.fill(0x5a,LO,HI);
    for(let k=0;k<37;k++)for(let j=0;j<5;j++)flash.writeUInt16LE(5+j*200,0xc1000+168+k*10+j*2);
    flash.set(Buffer.from('trimdat\0'),0xc1000+548);
    const prefix=model.endsWith('Pro')?'pro':'elite';await fs.writeFile(out+'/'+prefix+'-flash.bin',flash);await fs.writeFile(out+'/'+prefix+'-stock.smkupdate',bytes);
  }
  const wrong=model.endsWith('Pro')?'SMK-37 Elite':'SMK-37 Pro';await assert.rejects(prepareStock(stock,wrong),/does not match/);
  const corrupt=stock.slice();corrupt[corrupt.length-1]^=1;await assert.rejects(prepareStock(corrupt,model),/does not match/);
  for(const version of [922,936,941])await flow(pkg,'custom '+version,{source:id('SMK-37',version)},[0xf0000000]);
  await flow(pkg,'Doom handoff',{source:id('Doom-SMK-37',8)},[0xf0000000]);
  await flow(pkg,'Doom active loader',{source:id('ota-Doom-SMK-37',8)},[0xf0000000]);
  await flow(pkg,'SMK active loader',{source:id('ota-SMK-37',937)},[0xf0000000]);
  await flow(pkg,'old source refused',{source:id('SMK-37',911)},[],/cannot receive/);
  await flow(pkg,'old Doom refused',{source:id('Doom-SMK-37',6)},[],/cannot receive/);
  await flow(pkg,'old handoff loader refused',{source:id('SMK-37',937),oldLoader:true,started:1},[],/not compatible/);
  await flow(pkg,'wrong family refused',{source:id('Doom-FM-1',8)},[],/No matching/);
  await flow(pkg,'cancelled',{source:id('SMK-37',937),cancel:true},[],/cancelled/);
  await flow(pkg,'already stock no writes',{source:id(model,16)},[]);
  await flow(pkg,'missing loader',{source:id('SMK-37',937),noLoader:true,started:1},[],/expected RAM loader/);
  await flow(pkg,'interrupted write no retry',{source:id('SMK-37',937),fail:true},[0xf0000000],/interrupted/);
  await flow(pkg,'wrong return not success',{source:id('SMK-37',937),wrongReturn:true},[0xf0000000],/Stock boot was not confirmed/);
  pkg.bytes[HEADER]^=1;await assert.rejects(restoreStock({},pkg),/changed/);
}
if(process.env.FM1_STOCK_FILE){
 const pkg=await prepareStock(new Uint8Array(await fs.readFile(process.env.FM1_STOCK_FILE)),'FM-1');
 await flow(pkg,'running synth staging',{source:id('FM-1',914)},[0xe0000000,0xf0000000]);
 await flow(pkg,'Doom handoff',{source:id('Doom-FM-1',8)},[0xf0000000]);
 await flow(pkg,'Doom active loader',{source:id('ota-Doom-FM-1',8)},[0xf0000000]);
 await flow(pkg,'already V15',{source:id('FM-1',15)},[]);
 await flow(pkg,'SMK refused',{source:id('Doom-SMK-37',8)},[],/No matching/);
 await flow(pkg,'wrong returned version',{source:id('Doom-FM-1',8),wrongReturn:true},[0xf0000000],/Stock boot was not confirmed/);
}
await assert.rejects(restoreStock({},{}),/Prepare a checksum/);
console.log('Official package and restore-flow tests passed. No MIDI devices opened.');
