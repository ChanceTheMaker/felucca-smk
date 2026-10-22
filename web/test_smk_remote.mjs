// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {readFileSync,mkdirSync} from 'node:fs';
import {resolve,extname} from 'node:path';
import {decodeRemoteState,decodeRemoteTile,decodeRemoteDirty,decodeRemoteBatch,createSMKRemotePanel} from './smk-remote-panel.js';
const fixture=JSON.parse(readFileSync('build/smk37-tests/remote-wire.json','utf8'));
const state=decodeRemoteState(fixture.state);
assert.equal(state.keysLo,1);assert.equal(state.keysHi,16);assert.equal(state.heldPads,4);
assert.equal(state.padColor,2);assert.equal(state.buttons,1<<12);
for(let i=0;i<60;i++){const t=decodeRemoteTile(fixture.tiles[i]);assert.equal(t.row,i*4);assert.equal(t.rgba.length,3840);}
for(let i=0;i<60;i++)assert.deepEqual(decodeRemoteTile(fixture.compactTiles[i]),decodeRemoteTile(fixture.tiles[i]));
assert.deepEqual(decodeRemoteDirty(fixture.dirty),[]);
const batched=fixture.batches.flatMap(a=>decodeRemoteBatch(a).tiles);
assert.equal(batched.length,60);assert.equal(new Set(batched.map(t=>t.row)).size,60);
for(const t of batched)assert.deepEqual(t,decodeRemoteTile(fixture.tiles[t.row/4]));
const rectInitial=fixture.rectBatches.flatMap(a=>decodeRemoteBatch(a).tiles);
assert.equal(rectInitial.length,60);
for(const t of rectInitial)assert.deepEqual(t,decodeRemoteTile(fixture.tiles[t.row/4]));
const framebuffer=new Uint8ClampedArray(240*240*4);
const paint=t=>{const w=t.width||240;for(let y=0;y<4;y++)framebuffer.set(t.rgba.subarray(y*w*4,(y+1)*w*4),((t.row+y)*240+(t.x||0))*4);};
rectInitial.forEach(paint);
for(const batch of fixture.rectChanges){const decoded=decodeRemoteBatch(batch);assert.equal(decoded.pending.length,0);assert.equal(decoded.tiles.length,26);assert(decoded.tiles.every(t=>t.width<=6));decoded.tiles.forEach(paint);}
for(const a of fixture.afterRectTiles){const t=decodeRemoteTile(a);assert.deepEqual(framebuffer.slice(t.row*240*4,(t.row+4)*240*4),t.rgba);}
const badRect=fixture.rectBatches[0].slice();badRect[106]=0;assert.throws(()=>decodeRemoteBatch(badRect));
console.log('SMK rectangle patches exactly match final device pixels, including untouched surroundings');
assert.throws(()=>decodeRemoteBatch(fixture.batches[0].slice(0,-1)));
const edgeDirty=fixture.dirty.slice();edgeDirty[5]=1;edgeDirty[13]=8;assert.deepEqual(decodeRemoteDirty(edgeDirty),[0,59]);
const brokenRun=fixture.compactTiles.find(a=>a[6]===1).slice();brokenRun[55]=0;assert.throws(()=>decodeRemoteTile(brokenRun));
assert.throws(()=>decodeRemoteTile(fixture.tiles[0].slice(1)));
// Input request ordering: even a rapid release must never overtake its press.
const sent=[];const remote=createSMKRemotePanel({connection:()=>({id:1,supported:true}),request:async args=>{sent.push(args);await new Promise(r=>setTimeout(r,1));return [...args.slice(0,5),0];}});
remote.panel(2,1,100);remote.panel(2,1,0);remote.panel(0,12,1);remote.panel(0,12,0);
await new Promise(r=>setTimeout(r,40));assert.deepEqual(sent.map(x=>x.slice(5)),[[2,1,100,64],[2,1,0,64],[0,12,1,64],[0,12,0,64]]);
console.log('SMK remote wire decoding and input FIFO passed');
// Screen traffic must yield to input without starving during repeated gestures.
globalThis.document={hidden:false};
const traffic=[];let unblockTile,unblockInput,firstTile=true,firstInput=true;
const scheduled=createSMKRemotePanel({connection:()=>({id:2,supported:true}),request:async(args,options)=>{
 traffic.push({op:args[3],options});
 if(args[3]===2&&firstTile){firstTile=false;await new Promise(r=>{unblockTile=r;});}
 if(args[3]===1&&firstInput){firstInput=false;await new Promise(r=>{unblockInput=r;});}
 const reply=args[3]===0?fixture.state.slice():args[3]===2?fixture.tiles[args[5]].slice():[...args.slice(0,5),0];
 reply[4]=args[4];return reply;
}});
scheduled.visible(true);
while(!unblockTile)await new Promise(r=>setTimeout(r,2));
scheduled.panel(2,0,100);scheduled.panel(2,0,0);
unblockTile();await new Promise(r=>setTimeout(r,25));
assert.equal(traffic.filter(x=>x.op===2).length,1,'No additional screen requests while inputs are pending');
unblockInput();
const started=performance.now();
const gestures=setInterval(()=>scheduled.panel(1,0,1),30);
try{
 while(traffic.filter(x=>x.op===2).length<61){
  assert(performance.now()-started<2000,'Screen must progress during continuous gestures');
  await new Promise(r=>setTimeout(r,5));
 }
 console.log(`SMK synthetic immediate USB replies: full screen in ${Math.round(performance.now()-started)}ms during repeated input`);
 assert(traffic.filter(x=>x.op===1).every(x=>x.options.front),'Inputs have queue priority');
 assert(traffic.filter(x=>x.op===0).length>=2,'Device state still updates during screen streaming');
}finally{clearInterval(gestures);scheduled.visible(false);await new Promise(r=>setTimeout(r,10));delete globalThis.document;}
console.log('SMK input priority and non-starving screen polling passed');
// After a full initial snapshot, an unchanged screen transfers no pixel data.
globalThis.document={hidden:false};const deltas=[];let changed=false;
const deltaPanel=createSMKRemotePanel({connection:()=>({id:3,supported:true,delta:true}),request:async args=>{
 deltas.push(args);let reply;
 if(args[3]===0)reply=fixture.state.slice();
 else if(args[3]===3){reply=fixture.dirty.slice();if(changed)reply[7]=1;}
 else if(args[3]===4){reply=fixture.compactTiles[args[5]].slice();if(args[5]===14)changed=false;}
 else reply=[...args.slice(0,5),0];
 reply[4]=args[4];return reply;
}});
deltaPanel.visible(true);
while(deltas.filter(a=>a[3]===4).length<60)await new Promise(r=>setTimeout(r,5));
await new Promise(r=>setTimeout(r,80));assert.equal(deltas.filter(a=>a[3]===4).length,60);
changed=true;
while(deltas.filter(a=>a[3]===4).length<61)await new Promise(r=>setTimeout(r,5));
await new Promise(r=>setTimeout(r,80));
assert.deepEqual(deltas.filter(a=>a[3]===4).slice(60).map(a=>a[5]),[14]);
deltaPanel.visible(false);await new Promise(r=>setTimeout(r,10));delete globalThis.document;
console.log('SMK delta screen: unchanged frames send zero tiles; one changed strip sends one tile');
// Realistic 50ms per request: the full screen needs eight replies, including
// all state readbacks, rather than sixty strip replies plus separate state.
globalThis.document={hidden:false};let batchIndex=0,batchDrawn=0,batchStates=0;
const batchStart=performance.now();
const batchPanel=createSMKRemotePanel({connection:()=>({id:4,supported:true,delta:true,batch:true}),request:async args=>{
 if(args[3]===1)return [...args.slice(0,5),0];
 assert.equal(args[3],5);await new Promise(r=>setTimeout(r,50));
 const reply=fixture.batches[batchIndex++].slice();reply[4]=args[4];return reply;
}});
batchPanel.listeners.add(e=>{if(e.remote?.ready)batchStates++;if(e.tile&&++batchDrawn===60)batchPanel.visible(false);});
batchPanel.visible(true);
while(batchDrawn<60){assert(performance.now()-batchStart<4000);await new Promise(r=>setTimeout(r,5));}
assert.equal(batchIndex,fixture.batches.length);assert.equal(batchStates,batchIndex);
await new Promise(r=>setTimeout(r,10));delete globalThis.document;
console.log(`SMK simulated 50ms USB latency: full screen plus state in ${Math.round(performance.now()-batchStart)}ms, ${batchIndex} requests`);
const {chromium}=await import(process.env.PLAYWRIGHT_MODULE || 'playwright');
const core=readFileSync('build/smk37-tests/editor-wire.jsonl','utf8').trim().split('\n').map(JSON.parse);
const extra=readFileSync('build/smk37-tests/completion-wire.jsonl','utf8').trim().split('\n').map(JSON.parse);
const workflow=readFileSync('build/smk37-tests/workflow-wire.jsonl','utf8').trim().split('\n').map(JSON.parse);
const frames=[...workflow,...extra.filter(f=>!workflow.some(e=>e[4]===f[4])),...core.filter(f=>![...extra,...workflow].some(e=>e[4]===f[4]))];
const browser=await chromium.launch({channel:'chrome',headless:true});
const page=await browser.newPage({viewport:{width:1500,height:1050}}),errors=[];
page.setDefaultTimeout(20000);page.on('pageerror',e=>{errors.push(e.message);console.error('PAGE:',e.message);});
// Serve the exact local checkout to the test browser; no external dependencies.
await page.route('**/*',async route=>{
 const u=new URL(route.request().url());if(!['localhost','127.0.0.1'].includes(u.hostname))return route.abort();
 const file=resolve('web','.'+decodeURIComponent(u.pathname));
 if(!file.startsWith(resolve('web')+'/')&&!file.startsWith(resolve('web')+'\\'))return route.abort();
 try{await route.fulfill({status:200,body:readFileSync(file),contentType:({'.html':'text/html; charset=utf-8','.js':'text/javascript; charset=utf-8','.css':'text/css; charset=utf-8','.wasm':'application/wasm','.json':'application/json'})[extname(file)]||'application/octet-stream'});}catch{await route.fulfill({status:404,body:''});}
});
await page.addInitScript(({frames,fixture})=>{
 localStorage.setItem('felucca.smk.deviceTutorial.v1','done');
 localStorage.setItem('felucca.web.soundSourceTipDismissed','1');
 const input={id:'smk-in',name:'Felucca',type:'input',state:'connected',open:async()=>{},close:async()=>{}},sent=[];
 window.panelFixture=fixture;window.smkSent=sent;window.panelSupported=true;window.panelVersion=4;window.panelBatchIndex=0;
 const output={id:'smk-out',name:'Felucca',type:'output',state:'connected',open:async()=>{},close:async()=>{},send(bytes){
  if(bytes[0]!==240)return;
  const cmd=bytes[4],a=Array.from(bytes).slice(5,-1);sent.push([cmd,...a]);
  let reply=frames.find(f=>f[4]===cmd && (cmd===5?f[5]===a[0]&&f[6]===a[1]:[10,28,29,31,72].includes(cmd)?f[5]===a[0]:true));
  if(cmd===1){const info=window.panelSupported?fixture.info.slice():fixture.info.slice(0,-3);if(window.panelSupported)info[info.length-1]=window.panelVersion;reply=[240,125,70,76,cmd,...info,247];}
  if(cmd===126&&[5,6].includes(a[3])){
    let data;
    const batches=a[3]===6?fixture.rectBatches:fixture.batches;
    if(window.panelBatchIndex<batches.length)data=batches[window.panelBatchIndex++].slice();
    else {data=batches.at(-1).slice(0,104);data.fill(0,94,104);}
    data[4]=a[4];reply=[240,125,70,76,cmd,...data,247];
  }else if(cmd===126){let data=a[3]===0?fixture.state.slice():a[3]===2?fixture.tiles[a[5]].slice():a[3]===3?fixture.dirty.slice():a[3]===4?fixture.compactTiles[a[5]].slice():[...a.slice(0,5),0];data[4]=a[4];reply=[240,125,70,76,cmd,...data,247];}
  if(cmd===80)reply=[240,125,70,76,cmd,...fixture.palette,247];
  if(cmd===72)reply=[240,125,70,76,cmd,...fixture.menus[a[0]],247];
  if(cmd===37)reply=[240,125,70,76,cmd,0,...a,...Array(a[3]).fill(0),247];
  if(cmd===6){reply=frames.find(f=>f[4]===6).slice();reply[5]=a[0];}
  if(cmd===9)reply=[240,125,70,76,cmd,...a,0,247];
  if(cmd===22)reply=[240,125,70,76,cmd,3,247];
  if(cmd===25)reply=[240,125,70,76,cmd,247];
  if(cmd===64)reply=[240,125,70,76,cmd,a[0],0,0,0,0,0,1,247];
  if(cmd===79)reply=[240,125,70,76,cmd,3,247];
  if(reply)queueMicrotask(()=>input.onmidimessage?.({data:new Uint8Array(reply)}));
 }};
 navigator.requestMIDIAccess=async()=>({inputs:new Map([[input.id,input]]),outputs:new Map([[output.id,output]])});
},{frames,fixture});
try{
 await page.goto((process.env.SITE_URL||'http://localhost:8820')+'/editor.html',{waitUntil:'commit'});console.log('Document received');
 await page.waitForFunction(()=>document.querySelector('#connect')?.textContent.includes('Connect'));console.log('Editor ready');
 const consent=page.locator('.analytics-banner [data-choice=denied]');if(await consent.isVisible())await consent.click();
 await page.getByRole('button',{name:'Connect',exact:true}).click();console.log('Connect clicked');
 await page.waitForFunction(()=>document.querySelector('.sim-live-status')?.textContent.includes('Device controls'));
 assert.equal(await page.locator('#smk-device .sim-key[aria-pressed=true]').count(),2);
 assert.equal(await page.locator('#smk-device .sim-pad.lit').count(),1);
 assert.equal(await page.locator('#smk-device .sim-pad.lit').getAttribute('data-pad'),'2');
 assert.equal(await page.locator('#smk-device .sim-button.lit').getAttribute('aria-label'),'PATCH / HOME');
 assert.equal(await page.locator('#smk-device select').count(),0);
 const dial=page.locator('#smk-device .sim-dial').first();
 await dial.focus();await dial.press('ArrowUp');
 assert.equal(await dial.evaluate(d=>d.style.getPropertyValue('--angle')),'8deg');
 await dial.press('ArrowDown');
 assert.equal(await dial.evaluate(d=>d.style.getPropertyValue('--angle')),'0deg');
 const bounds=await dial.boundingBox();
 await page.mouse.move(bounds.x+bounds.width/2,bounds.y+bounds.height/2);
 await page.mouse.down();await page.mouse.move(bounds.x+bounds.width/2,bounds.y+bounds.height/2-20);await page.mouse.up();
 assert.equal(await dial.evaluate(d=>d.style.getPropertyValue('--angle')),'32deg');
 await page.waitForFunction(()=>window.smkSent.some(a=>a[0]===126&&a[4]===1&&a[6]===1&&a[7]===0&&a[8]===4));
 await page.locator('#smk-device .sim-pad[data-pad="0"]').click();
 await page.waitForFunction(()=>window.smkSent.some(a=>a[0]===126&&a[4]===1&&a[6]===3&&a[7]===0&&a[8]===100));
 // A click must not invent a pad light or replace the physical held-key readback.
 assert.equal(await page.locator('#smk-device .sim-pad.lit').getAttribute('data-pad'),'2');
 await page.waitForFunction(()=>window.panelBatchIndex>=window.panelFixture.rectBatches.length);
 mkdirSync('build/panel-preview',{recursive:true});
 await page.screenshot({path:'build/panel-preview/device-mirror-fixture.png',fullPage:true});
 const screenToggle=page.locator('#smk-device .sim-screen-button');
 await screenToggle.click();assert.equal(await screenToggle.getAttribute('title'),'Full panel');
 assert(await page.locator('#smk-device .sim-bottom').isHidden());
 assert(await page.locator('#smk-device .sim-right').isHidden());
 assert((await page.locator('#smk-device canvas').boundingBox()).width>=700);
 await page.screenshot({path:'build/panel-preview/device-screen-only.png',fullPage:true});
 await page.setViewportSize({width:390,height:844});
 const smallScreen=await page.locator('#smk-device canvas').boundingBox();
 assert(smallScreen.width>200&&smallScreen.x>=0&&smallScreen.x+smallScreen.width<=390,'Screen-only view fits a narrow touch screen');
 await page.setViewportSize({width:1500,height:1050});
 await screenToggle.click();assert(await page.locator('#smk-device .sim-bottom').isVisible());
 await page.getByRole('button',{name:'Device Studio',exact:true}).click();
 assert(await page.locator('#smk-device').isHidden());
 await page.getByRole('button',{name:'Live Panel',exact:true}).click();
 await page.getByRole('button',{name:'Disconnect',exact:true}).click();
 await page.evaluate(()=>{window.panelVersion=1;window.smkSent.length=0;});
 await page.getByRole('button',{name:'Connect',exact:true}).click();
 await page.waitForFunction(()=>window.smkSent.some(a=>a[0]===126&&a[4]===2));
 assert.equal(await page.evaluate(()=>window.smkSent.some(a=>a[0]===126&&[3,4,5,6].includes(a[4]))),false);
 await page.getByRole('button',{name:'Disconnect',exact:true}).click();
 await page.evaluate(()=>window.panelSupported=false);
 await page.getByRole('button',{name:'Connect',exact:true}).click();console.log('Connect clicked');
 await page.waitForFunction(()=>document.querySelector('.sim-live-status')?.textContent.includes('firmware required'));
 assert(await page.locator('#smk-device .sim-pad').first().isDisabled());
 await page.getByRole('button',{name:'Browser Studio',exact:true}).click();
 await page.waitForFunction(()=>document.querySelector('#version')?.textContent.includes('BROWSER'));
 await page.getByRole('button',{name:'Simulator',exact:true}).click();
 assert(await page.locator('#smk-simulator').isVisible());
 assert(await page.locator('#smk-device').isHidden());
 assert.deepEqual(errors,[]);console.log('SMK browser: device screen, physical keys/LEDs, input routing, view switching and old-firmware gate passed');
}catch(error){console.log(await page.evaluate(()=>({status:document.querySelector('#status')?.textContent,panel:document.querySelector('.sim-live-status')?.textContent,version:document.querySelector('#version')?.textContent,sent:window.smkSent.slice(-8)})));throw error;}finally{await browser.close();}
