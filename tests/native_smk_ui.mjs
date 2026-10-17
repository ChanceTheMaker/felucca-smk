// Browser regression with simulated USB jobs; never opens physical MIDI.
import assert from 'node:assert/strict';
import fs from 'node:fs/promises';
import path from 'node:path';
const {chromium}=await import(process.env.PLAYWRIGHT_MODULE||'playwright');
const base='https://chancethemaker.github.io/felucca-smk/';
const helper='http://127.0.0.1:8893',token='t'.repeat(43);
const types={'.js':'text/javascript','.html':'text/html','.css':'text/css','.json':'application/json','.svg':'image/svg+xml'};
const browser=await chromium.launch({channel:'chrome',headless:true});
try{
 const context=await browser.newContext({permissions:['local-network-access']});
 const page=await context.newPage();page.setDefaultTimeout(30000);let mode='ready',probes=0,actions=[],errors=[];
 page.on('pageerror',e=>errors.push(e.stack));page.on('dialog',d=>d.accept());
 await page.addInitScript(()=>{
  if(window===window.top)window.midiCalls=0;
  Object.defineProperty(navigator,'platform',{value:'Win32'});
  Object.defineProperty(navigator,'userAgentData',{value:{platform:'Windows'}});
  Object.defineProperty(navigator,'requestMIDIAccess',{value:async()=>{
   window.top.midiCalls++;
   if(!window.top.emptyMidi)throw Error('Simulated MIDI denial');
   const port=()=>({close:async()=>{window.top.closedPorts++;}});
   return {inputs:new Map([['i',port()]]),outputs:new Map([['o',port()]])};
  }});
 });
 await page.route(base+'**',async route=>{
  let rel=decodeURIComponent(new URL(route.request().url()).pathname.slice(new URL(base).pathname.length))||'index.html';if(rel.endsWith('/'))rel+='index.html';
  // The unrelated Studio preview requires editor i18n globals absent on this landing page.
  if(rel==='studio-preview.js')return route.fulfill({contentType:'text/javascript',body:''});
  try{await route.fulfill({contentType:types[path.extname(rel)]||'application/octet-stream',body:await fs.readFile(path.join('web',rel))});}
  catch{await route.fulfill({status:404,body:'Not found'});}
 });
 await page.route('https://raw.githubusercontent.com/**',async route=>route.fulfill({body:await fs.readFile('build/smk37/stock-pro.fwsc')}));
 await page.route(helper+'/**',async route=>{
  const req=route.request(),url=new URL(req.url());
  if(url.pathname.endsWith('/capabilities')){
   probes++;if(mode==='absent')return route.abort('connectionrefused');
   return route.fulfill({json:{available:true,models:['smk-pro'],protocol:1,token,app:mode==='wrong'?'other-app':'felucca-smk',firmware:'SMK-37_927'}});
  }
  if(req.method()==='POST'){
   assert.equal(req.headers()['x-felucca-token'],token);
   const body=req.postDataJSON();assert.deepEqual(body,{action:body.action,model:'smk-pro',app:'felucca-smk'});
   actions.push(body.action);return route.fulfill({json:{id:'test-job'}});
  }
  return route.fulfill({json:{state:mode==='error'?'error':'done',error:'Simulated USB interruption',phase:'reboot',logs:['Simulated restart verification'],identity:actions.at(-1)==='restore'?'SMK-37 Pro_016':'SMK-37_927'}});
 });
 await page.goto(base);await page.waitForFunction(()=>document.querySelector('#status').textContent.includes('ready.'));
 assert.equal(probes,0,'No helper access during page loading');
 await page.locator('#ack').check();
 for(const failure of ['absent','wrong']){
  mode=failure;await page.locator('#install-button').click();
  await page.waitForFunction(()=>!document.querySelector('#install-button').disabled);
  assert.match(await page.locator('#status').textContent(),/browser MIDI updater was not started/);
  assert.equal(await page.evaluate(()=>midiCalls),0);
 }
 mode='error';await page.locator('#install-button').click();
 await page.waitForFunction(()=>document.querySelector('#status').textContent.includes('Simulated USB interruption'));
 assert.equal(await page.evaluate(()=>midiCalls),0,'Native failure must not fall back to Web MIDI');
 mode='ready';await page.locator('#install-button').click();
 await page.waitForFunction(()=>document.querySelector('.fw-progress').dataset.state==='done');
 assert.equal(await page.evaluate(()=>midiCalls),0);
 // A retained browser MIDI document is removed on both failure and completion.
 await page.evaluate(async()=>{
  const s=await import('./midi-session.js?v=session1');
  try{await s.requestMidiSession();}catch{}
 });
 assert.equal(await page.locator('iframe[data-midi-session]').count(),0);
 await page.evaluate(async()=>{
  window.emptyMidi=true;window.closedPorts=0;
  const s=await import('./midi-session.js?v=session1');
  await s.requestMidiSession();await s.releaseMidiSession();
 });
 assert.equal(await page.locator('iframe[data-midi-session]').count(),0);
 assert.equal(await page.evaluate(()=>closedPorts),2);
 await page.goto(base+'restore-stock/');
 await page.locator('#prepare').click();await page.waitForFunction(()=>document.querySelector('#status').textContent.includes('verified.'));
 await page.locator('#ack').check();await page.locator('#restore').click();
 await page.waitForFunction(()=>document.querySelector('#status').textContent.includes('restored and USB identity confirmed'));
 assert.equal(await page.evaluate(()=>midiCalls),0);
 assert.deepEqual(actions,['install','install','restore']);assert.deepEqual(errors,[]);
 console.log('PASS Felucca hosted install and restore: native-only on Windows Pro, missing/wrong/failed helper never falls back; MIDI documents released.');
}finally{await browser.close();}
