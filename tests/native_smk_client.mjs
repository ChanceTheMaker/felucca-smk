import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
const appHash=JSON.parse(readFileSync(new URL('../web/firmware/public-release.json',import.meta.url))).sha256;
Object.defineProperty(globalThis,'navigator',{value:{platform:'Win32'},configurable:true});
globalThis.location={hostname:'chancethemaker.github.io'};
globalThis.document={dispatchEvent(){}};
globalThis.CustomEvent=class{constructor(type,init){this.type=type;this.detail=init.detail;}};
const {hasNativeSmk:checkNativeSmk,runNativeSmk}=await import('../web/native-smk.js');
const hasNativeSmk=(model,fetcher,action='install')=>checkNativeSmk(model,fetcher,action,appHash);
const token='a'.repeat(43);let calls=[];
const capability=async(url)=>{calls.push(url);return new Response(JSON.stringify({available:true,models:['smk-pro'],protocol:1,token,app:'felucca-smk',application_sha256:appHash,firmware:'SMK-37_954'}));};
assert.equal(await hasNativeSmk('smk-pro',capability),true);assert.equal(calls[0],'http://127.0.0.1:8893/api/smk/capabilities');
for(const response of [()=>Promise.reject(Error('denied')),async()=>new Response('',{status:404}),async()=>new Response(JSON.stringify({available:true,models:['smk-pro']})),async()=>new Response(JSON.stringify({available:false,protocol:1,token,app:'felucca-smk',application_sha256:appHash,firmware:'SMK-37_954'}))])await assert.rejects(hasNativeSmk('smk-pro',response),/browser MIDI updater was not started/);
await assert.rejects(hasNativeSmk('smk-pro',async()=>new Response(JSON.stringify({available:true,models:['smk-pro'],protocol:1,token,app:'wrong-site',firmware:'SMK-37_969'}))),/browser MIDI updater was not started/);
await hasNativeSmk('smk-pro',capability);
assert.equal(await runNativeSmk('install',()=>{},async(url,options)=>{
 assert(url.startsWith('http://127.0.0.1:8893/api/smk/'));
 if(options.method==='POST'){assert.equal(options.headers['X-Felucca-Token'],token);assert.deepEqual(JSON.parse(options.body),{action:'install',model:'smk-pro',app:'felucca-smk'});return new Response(JSON.stringify({id:'fake-job'}));}
 return new Response(JSON.stringify({state:'done',identity:'SMK-37_954',logs:[],phase:'reboot'}));
}),'SMK-37_954');
// Restore compatibility is independent of the custom release installed by a helper.
for(const firmware of ['SMK-37_938','SMK-37_949','SMK-37_954','unknown-custom-name',undefined]){
 const legacy=async()=>new Response(JSON.stringify({available:true,models:['smk-pro'],protocol:1,token,app:'felucca-smk',firmware}));
 await assert.rejects(hasNativeSmk('smk-pro',legacy),/does not have the firmware selected/);
 assert.equal(await hasNativeSmk('smk-pro',legacy,'restore'),true);
 await assert.rejects(runNativeSmk('install',()=>{},()=>{throw Error('Must not send');}),/Verify the selected/);
 assert.equal(await runNativeSmk('restore',()=>{},async(url,options)=>{
  if(options.method==='POST'){
   assert.equal(JSON.parse(options.body).action,'restore');assert.equal(options.headers['X-Felucca-Token'],token);
   return new Response(JSON.stringify({id:'restore-job'}));
  }
  return new Response(JSON.stringify({state:'done',identity:'SMK-37 Pro_016',logs:[],phase:'reboot'}));
 }),'SMK-37 Pro_016');
}
const future=extra=>async()=>new Response(JSON.stringify({available:true,models:['smk-pro'],protocol:1,token,app:'felucca-smk',firmware:'SMK-37_999',...extra}));
assert.equal(await hasNativeSmk('smk-pro',future({restore_product:'SMK-37 Pro_016'}),'restore'),true);
assert.equal(await hasNativeSmk('smk-pro',future({}),'restore'),true);
for(const extra of [{restore_product:'SMK-37 Elite_016'},{app:'doom',restore_product:'SMK-37 Pro_016'},{protocol:2,restore_product:'SMK-37 Pro_016'}])
 await assert.rejects(hasNativeSmk('smk-pro',future(extra),'restore'),/browser MIDI updater was not started/);
await assert.rejects(runNativeSmk('restore',()=>{},()=>{throw Error('Must not send');}),/Verify the selected/);
// Install binds to the chosen package bytes, not a displayed firmware name.
assert.equal(await checkNativeSmk('smk-pro',future({application_sha256:appHash}),'install',appHash),true);
await assert.rejects(checkNativeSmk('smk-pro',future({application_sha256:'0'.repeat(64)}),'install',appHash),/does not have the firmware selected/);
assert.equal(await hasNativeSmk('fm1',()=>{throw Error('Unexpected probe');}),false);
navigator.platform='MacIntel';assert.equal(await hasNativeSmk('smk-pro',()=>{throw Error('Unexpected probe');}),false);
location.hostname='localhost';assert.equal(await hasNativeSmk('smk-pro',capability),true);
console.log('PASS: hosted Windows helper, token forwarding, action-specific versions, compatible legacy restore, absent/wrong/denied helper fails closed, local support and FM-1/non-Windows isolation.');

assert.equal(await checkNativeSmk('smk-pro',capability,'connect'),true);
await assert.rejects(runNativeSmk('restore',()=>{},()=>{throw Error('No request expected')}),/Verify the selected/);

let manifestRequests=0;
const selectedFetcher=async url=>{
 if(String(url).includes('/firmware/')){manifestRequests++;return new Response(readFileSync(new URL("../web/firmware/public-release.json",import.meta.url)));}
 return capability(url);
};
assert.equal(await checkNativeSmk('smk-pro',selectedFetcher,'install'),true);
assert.equal(manifestRequests,1);
manifestRequests=0;assert.equal(await checkNativeSmk('smk-pro',selectedFetcher,'restore'),true);assert.equal(manifestRequests,0);
