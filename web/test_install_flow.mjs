// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {Updater} from './fm1ota.js?v=smk-flow-r1';
import {installStockSmk,installSmk} from './smk37-update.js';
let cfg,sends,serves,confirms,phase,closed;
const link=()=>({send(){sends++;return true;},async close(){closed++;}});
const mk=(model,version=16)=>({model,version,text:model+'_'+String(version).padStart(3,'0')});
const pkg={format:2,image:new Uint8Array(64),bytes:new Uint8Array(96),stockModel:'SMK-37 Elite',product:'SMK-37_922',version:'Felucca SMK 0.9.22 Beta'};
Updater.prototype.findAny=async function(filter){
 const ids=[mk('FM-1'),mk('unrelated'),...(cfg.start==='stock'?[mk(pkg.stockModel)]:cfg.start==='exact'?[mk('ota-'+pkg.stockModel)]:cfg.start==='generic'?[mk('ota-FM-1')]:cfg.start==='wrong'?[mk('SMK-37 Pro')]:cfg.start==='melodee'?[mk('SMK-37',cfg.sourceVersion||940)]:cfg.start==='portable-loader'?[mk('ota-SMK-37',940)]:cfg.start==='doom'?[mk('Doom-SMK-37',8)]:cfg.start==='doom-loader'?[mk('ota-Doom-SMK-37',8)]:cfg.start==='old-doom'?[mk('Doom-SMK-37',6)]:cfg.start==='doom-fm1'?[mk('Doom-FM-1',8)]:cfg.start==='private'?[mk('SMK-37',911)]:[])];
 for(const id of ids){if(filter(id))return {id,name:cfg.start==='melodee'?(cfg.sourceName||'Melodee'):'test MIDI',link:link()};}
 return null;
};
Updater.prototype.waitForAny=async function(filter){
 if(phase===1&&cfg.noLoader)return null;
 const id=phase===0?(cfg.start==='doom'?mk('ota-Doom-SMK-37',8):mk('ota-SMK-37',940)):phase===1?mk(cfg.genericHandoff?'ota-FM-1':'ota-'+pkg.stockModel):{model:'SMK-37',text:cfg.wrongReturn?'SMK-37_931':pkg.product};
 assert.ok(filter(id));return {id,name:'renamed loader output',link:link()};
};
Updater.prototype.serve=async function(l,image,finish){assert.equal(image,cfg.portable?pkg.bytes:pkg.image);serves.push(finish);phase=finish===0xe0000000?1:2;return {finished:!(cfg.failWrite&&phase===2)&&!(cfg.failStage&&phase===1)};};
async function test(name,options,expected,error){
 cfg=options;sends=confirms=phase=closed=0;serves=[];pkg.stockModel=options.model||'SMK-37 Elite';
 const ask=async evidence=>{confirms++;assert.equal(evidence.model,'SMK-37 Elite');assert.match(evidence.identity,/^ota-FM-1/);return !!cfg.confirm;};
 const act=()=>cfg.manual?installSmk({},pkg):installStockSmk({},pkg,()=>{},{confirmGeneric:cfg.noCallback?undefined:ask});
 if(error)await assert.rejects(act,error);else await act();
 assert.deepEqual(serves,expected);assert.equal(sends,options.sends??expected.length);assert.equal(confirms,options.confirms||0);
 if(expected.length||options.start==='generic')assert.ok(closed);
 console.log('PASS '+name);
}
await test('exact loader resumes without stage one',{start:'exact'},[0xf0000000]);
await test('normal stock completes in one flow',{start:'stock'},[0xe0000000,0xf0000000]);
await test('generic handoff completes in same attempt after confirmation',{start:'stock',genericHandoff:true,confirm:true,confirms:1},[0xe0000000,0xf0000000]);
await test('generic handoff cancellation never starts flash stage',{start:'stock',genericHandoff:true,confirms:1},[0xe0000000],/paused/);
await test('missing loader stops before stage two',{start:'stock',noLoader:true},[0xe0000000],/has not replied/);
await test('generic loader needs explicit callback',{start:'generic',noCallback:true},[],/paused/);
await test('generic loader cancelled',{start:'generic',confirms:1},[],/paused/);
await test('generic Elite already active confirms and resumes',{start:'generic',confirm:true,confirms:1},[0xf0000000]);
await test('generic fallback refused for Pro',{start:'generic',confirm:true,model:'SMK-37 Pro'},[],/does not match/);
await test('no SMK does not send START',{start:'missing'},[],/No matching SMK/);
await test('wrong stock model refused',{start:'wrong'},[],/No matching SMK/);
await test('incomplete stage one never starts application',{start:'stock',failStage:true},[0xe0000000],/preparation did not complete/);
await test('write failure is not retried or reported successful',{start:'exact',failWrite:true},[0xf0000000],/did not finish/);
await test('wrong returned firmware rejected',{start:'exact',wrongReturn:true},[0xf0000000],/Expected/);
await test('Melodee 940 automatically uses portable package, not official image',{start:'melodee',portable:true,sends:2},[0xf0000000]);
await test('portable loader automatically resumes without stock staging',{start:'portable-loader',portable:true},[0xf0000000]);
await test('Melodee manual Update mode uses portable package',{start:'melodee',portable:true,manual:true,sends:2},[0xf0000000]);
await test('old private updater refused without START',{start:'private'},[],/older private updater/);
await test('Felucca 922 automatically uses portable package',{start:'melodee',sourceVersion:922,sourceName:'Felucca',portable:true,sends:2},[0xf0000000]);
await test('Sloop 936 automatically uses portable package',{start:'melodee',sourceVersion:936,sourceName:'Sloop',portable:true,sends:2},[0xf0000000]);
await test('Doom SMK uses the portable package without stock staging',{start:'doom',portable:true,sends:2},[0xf0000000]);
await test('Doom loader resumes directly',{start:'doom-loader',portable:true},[0xf0000000]);
await test('Doom manual update uses its own loader identity',{start:'doom',portable:true,manual:true,sends:2},[0xf0000000]);
await test('old Doom refused before START',{start:'old-doom'},[],/older private updater/);
await test('Doom FM-1 never selected for an SMK update',{start:'doom-fm1'},[],/No matching SMK/);
await test('Doom update still requires exact returned Felucca identity',{start:'doom',portable:true,wrongReturn:true,sends:2},[0xf0000000],/Unexpected returned version/);
console.log('No physical MIDI ports were opened.');
