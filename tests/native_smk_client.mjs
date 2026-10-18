import assert from 'node:assert/strict';
Object.defineProperty(globalThis,'navigator',{value:{platform:'Win32'},configurable:true});
globalThis.location={hostname:'chancethemaker.github.io'};
globalThis.document={dispatchEvent(){}};
globalThis.CustomEvent=class{constructor(type,init){this.type=type;this.detail=init.detail;}};
const {hasNativeSmk,runNativeSmk}=await import('../web/native-smk.js');
const token='a'.repeat(43);let calls=[];
const capability=async(url)=>{calls.push(url);return new Response(JSON.stringify({available:true,models:['smk-pro'],protocol:1,token,app:'felucca-smk',firmware:'SMK-37_928'}));};
assert.equal(await hasNativeSmk('smk-pro',capability),true);assert.equal(calls[0],'http://127.0.0.1:8893/api/smk/capabilities');
for(const response of [()=>Promise.reject(Error('denied')),async()=>new Response('',{status:404}),async()=>new Response(JSON.stringify({available:true,models:['smk-pro']})),async()=>new Response(JSON.stringify({available:false,protocol:1,token,app:'felucca-smk',firmware:'SMK-37_928'}))])await assert.rejects(hasNativeSmk('smk-pro',response),/browser MIDI updater was not started/);
await assert.rejects(hasNativeSmk('smk-pro',async()=>new Response(JSON.stringify({available:true,models:['smk-pro'],protocol:1,token,app:'wrong-site',firmware:'SMK-37_969'}))),/browser MIDI updater was not started/);
await hasNativeSmk('smk-pro',capability);
assert.equal(await runNativeSmk('install',()=>{},async(url,options)=>{
 assert(url.startsWith('http://127.0.0.1:8893/api/smk/'));
 if(options.method==='POST'){assert.equal(options.headers['X-Felucca-Token'],token);assert.deepEqual(JSON.parse(options.body),{action:'install',model:'smk-pro',app:'felucca-smk'});return new Response(JSON.stringify({id:'fake-job'}));}
 return new Response(JSON.stringify({state:'done',identity:'SMK-37_928',logs:[],phase:'reboot'}));
}),'SMK-37_928');
assert.equal(await hasNativeSmk('fm1',()=>{throw Error('Unexpected probe');}),false);
navigator.platform='MacIntel';assert.equal(await hasNativeSmk('smk-pro',()=>{throw Error('Unexpected probe');}),false);
location.hostname='localhost';assert.equal(await hasNativeSmk('smk-pro',capability),true);
console.log('PASS: hosted Windows helper, token forwarding, absent/old/denied helper fails closed, local support and FM-1/non-Windows isolation.');
