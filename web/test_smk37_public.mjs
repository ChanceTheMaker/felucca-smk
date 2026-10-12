// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {readFileSync,writeFileSync,mkdirSync} from 'node:fs';
import {resolve,dirname} from 'node:path';
import {buildSmkPublicPackage,smkLogicalImage} from './smk37-package.js';
import {parsePackage,installStockSmk,installSmk,LO,HI} from './smk37-update.js';
import {Updater} from './fm1ota.js?v=smk-flow-r1';
import {sha256hex} from './fm1pkg.js';

// No MIDI hardware is opened. Exercise transport decisions with simulated links.
const saved=Object.fromEntries(['findAny','waitForAny','serve'].map(k=>[k,Updater.prototype[k]]));
let sends=0,closes=0,serves=0,phase=0,scenario='success';
const link=()=>({send(){sends++;return true;},close(){closes++;}});
const pkg={format:2,image:new Uint8Array(64),stockModel:'SMK-37 Pro',product:'SMK-37_922',version:'Felucca SMK 0.9.22 Beta'};
try{
  Updater.prototype.findAny=async function(filter){
    for(const id of [{model:'FM-1',version:16},{model:'SMK-37 Elite',version:16},{model:'SMK-37 Pro',version:15}])assert.equal(filter(id),false);
    const id={model:pkg.stockModel,version:16,text:pkg.stockModel+'_016'};
    assert.ok(filter(id));return scenario==='missing'?null:{id,link:link()};
  };
  Updater.prototype.waitForAny=async function(filter){
    phase++;if(scenario==='no-loader')return null;
    const id=phase===1?{model:'ota-'+pkg.stockModel}:{model:'SMK-37',text:scenario==='wrong-return'?'SMK-37_921':pkg.product};
    assert.ok(filter(id));return {id,link:link()};
  };
  Updater.prototype.serve=async function(l,image,end){
    serves++;assert.equal(image,pkg.image);assert.equal(end,serves===1?0xe0000000:0xf0000000);
    return {finished:!(scenario==='first-failure'&&serves===1)&&!(scenario==='write-failure'&&serves===2)};
  };
  for(const [caseName,pattern] of [['success',null],['missing',/Connect/],['first-failure',/preparation/],['no-loader',/has not replied/],['write-failure',/did not finish/],['wrong-return',/Expected/]]){
    scenario=caseName;sends=closes=serves=phase=0;
    if(pattern)await assert.rejects(installStockSmk({},pkg),pattern);else await installStockSmk({},pkg);
    if(caseName==='missing')assert.equal(sends,0);
    if(['first-failure','no-loader'].includes(caseName))assert.equal(serves,1);
    if(caseName==='success'){assert.equal(sends,2);assert.equal(closes,3);}
  }
  sends=0;Updater.prototype.findAny=async()=>({id:{model:'SMK-37',version:921},link:link()});
  await assert.rejects(installSmk({},pkg),/older private/);assert.equal(sends,0);
}finally{Object.assign(Updater.prototype,saved);}
console.log('Public installer: exact model/version, stage failures, identity mismatch, cleanup and legacy-loader refusal passed');

// Vendor files are optional local fixtures, never checked into the repository.
if(process.argv.length>2){
  const [pro,elite,manifestPath]=process.argv.slice(2);
  const m=JSON.parse(readFileSync(manifestPath));
  const app=new Uint8Array(readFileSync(resolve(dirname(manifestPath),m.image)));
  const release={public:true,product:m.update_product,version:'Felucca SMK 0.9.22 Beta',bytes:m.bytes,sha256:m.sha256};
  mkdirSync('build/public-tests',{recursive:true});
  for(const [name,path] of [['pro',pro],['elite',elite]]){
    const stock=new Uint8Array(readFileSync(path));const original=stock.slice();
    const result=await buildSmkPublicPackage(stock,app,release);
    assert.deepEqual(stock,original);
    const parsed=await parsePackage(result.bytes,result.sha);assert.equal(parsed.format,2);assert.equal(parsed.product,release.product);
    const before=smkLogicalImage(stock).raw,after=result.image;
    assert.equal(before.length,after.length);
    // Every vendor payload outside flash.bin remains exactly unchanged.
    assert.deepEqual(after.subarray(1024+HI),before.subarray(1024+HI));
    assert.deepEqual(after.subarray(1024,1024+LO),before.subarray(1024,1024+LO));
    const bad=stock.slice();bad[2000]^=1;await assert.rejects(buildSmkPublicPackage(bad,app,release),/unmodified official/);
    const badApp=app.slice();badApp[99]^=1;await assert.rejects(buildSmkPublicPackage(stock,badApp,release),/verification/);
    const flash=new Uint8Array(0x100000).fill(0xa5);flash.set(before.subarray(1024,1024+HI));
    const points=[5,640,800,920,980];
    for(let k=0;k<37;k++)for(let j=0;j<5;j++){const p=0xc1000+168+k*10+j*2;flash[p]=points[j]&255;flash[p+1]=points[j]>>8;}
    flash.set(new TextEncoder().encode('trimdat\0'),0xc1000+548);
    writeFileSync(`build/public-tests/${name}-synthetic-flash.bin`,flash);
    writeFileSync(`build/public-tests/${name}.smkupdate`,result.bytes);
    console.log(`${result.stockModel}: container/CRC round trip, protected bytes, native package, corruption rejection passed (${await sha256hex(app)})`);
  }
}
