// SPDX-License-Identifier: GPL-3.0-only
// Real host-compiled firmware descriptors; simulated MIDI transport only.
import assert from 'node:assert/strict';
import {readFileSync,mkdirSync} from 'node:fs';
const {chromium}=await import(process.env.PLAYWRIGHT_MODULE || 'playwright');
const core=readFileSync('build/smk37-tests/editor-wire.jsonl','utf8').trim().split('\n').map(JSON.parse);
const extra=readFileSync('build/smk37-tests/completion-wire.jsonl','utf8').trim().split('\n').map(JSON.parse);
const frames=[...extra,...core.filter(f=>!extra.some(e=>e[4]===f[4]))];
const browser=await chromium.launch({channel:'chrome',headless:true});
const page=await browser.newPage({viewport:{width:1440,height:1050}}), errors=[];
page.setDefaultTimeout(15000);
page.on('pageerror',e=>{errors.push(e.message);console.error(e.message);});
await page.route('**/*',r=>/^https?:\/\/(?:127\.0\.0\.1|localhost)(?::|\/)/.test(r.request().url())?r.continue():r.abort());
await page.addInitScript(frames=>{
 const input={id:'smk-in',name:'Felucca SMK37',type:'input',state:'connected',open:async()=>{},close:async()=>{}}, sent=[];
 const output={id:'smk-out',name:'Felucca SMK37',type:'output',state:'connected',open:async()=>{},close:async()=>{},send(bytes){
  if(bytes[0]!==240)return;
  const cmd=bytes[4],a=Array.from(bytes).slice(5,-1);sent.push([cmd,...a]);
  let reply=frames.find(f=>f[4]===cmd && (cmd===5 ? f[5]===a[0]&&f[6]===a[1] : [10,28,29,31].includes(cmd) ? f[5]===a[0] : true));
  if(cmd===6){reply=frames.find(f=>f[4]===6).slice();reply[5]=a[0];}
  if(cmd===9)reply=[240,125,70,76,cmd,...a,0,247];
  if(cmd===22)reply=[240,125,70,76,cmd,3,247];
  if(cmd===25)reply=[240,125,70,76,cmd,247];
  if(cmd===64)reply=[240,125,70,76,cmd,a[0],0,0,0,0,0,1,247];
  if(cmd===79)reply=[240,125,70,76,cmd,3,247];
  if(reply)queueMicrotask(()=>input.onmidimessage?.({data:new Uint8Array(reply)}));
 }};
 window.smkSent=sent;
 navigator.requestMIDIAccess=async()=>({inputs:new Map([[input.id,input]]),outputs:new Map([[output.id,output]])});
},frames);
try{
 await page.goto((process.env.SITE_URL || 'http://127.0.0.1:8770')+'/editor.html?view=soundboard',{waitUntil:'domcontentloaded'});console.log('Page loaded');
 const consent=page.locator('.analytics-banner [data-choice=denied]');if(await consent.isVisible())await consent.click();
 await page.keyboard.press('Escape');await page.getByRole('button',{name:'Connect',exact:true}).click();
 await page.waitForFunction(()=>document.querySelector('#status').textContent==='Connected');console.log('Connected');
 assert.equal(await page.locator('#play-keys button[data-hardware="true"]').count(),37);
 assert.equal(await page.locator('#play-octave option:checked').textContent(),'C3');
 assert(await page.locator('[data-section="INSERT"]').count());
 await page.getByRole('tab',{name:'Settings',exact:true}).click();
 assert(await page.locator('#smk-learn').isVisible());
 assert.equal(await page.locator('#smk-learn-param option').count(),111);
 await page.locator('#smk-learn-assign').click();
 await page.waitForFunction(()=>document.querySelector('#smk-learn-result').textContent.includes('assigned'));
 await page.getByRole('tab',{name:'Projects',exact:true}).click();
 assert(await page.locator('#songchain').isVisible());
 assert.equal(await page.locator('#songrows select').count(),4);
 assert.equal(await page.locator('#songrows select').nth(2).inputValue(),'4');
 await page.getByRole('tab',{name:'Library',exact:true}).click();
 assert.match(await page.locator('#bankhead').textContent(),/3 \/ 8/);
 assert.match(await page.locator('#banklist').textContent(),/TEST/);
 await page.getByRole('tab',{name:'Sequencer',exact:true}).click();
 assert(await page.getByRole('columnheader',{name:'Nudge',exact:true}).isVisible());
 mkdirSync('build/screenshots',{recursive:true});await page.screenshot({path:'build/screenshots/smk37-upstream-sequencer.png'});
 assert.deepEqual(errors,[]);
 console.log('SMK browser: real 1.5.1 descriptors, INSERT cards, 37-key range, MIDI learn, step nudge, 8 user slots and four song lanes passed');
}catch(e){console.error('Status:',await page.locator('#status').textContent());console.error('Requests:',await page.evaluate(()=>window.smkSent?.slice(-8)));throw e;}finally{await browser.close();}
