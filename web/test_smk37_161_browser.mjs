// SPDX-License-Identifier: GPL-3.0-only
// Real SMK WASM firmware behind a simulated MIDI cable. No device access.
import assert from 'node:assert/strict';
import {mkdirSync} from 'node:fs';
const {chromium}=await import(process.env.PLAYWRIGHT_MODULE || 'playwright');
const browser=await chromium.launch({channel:'chrome',headless:true});
const page=await browser.newPage({viewport:{width:1440,height:1050}}),errors=[];
const pending=new Set();page.on('request',r=>pending.add(r.url()));page.on('requestfinished',r=>pending.delete(r.url()));page.on('requestfailed',r=>pending.delete(r.url()));
page.on('pageerror',e=>errors.push(e.message));
await page.route('**/*',r=>/^https?:\/\/(?:127\.0\.0\.1|localhost)(?::|\/)/.test(r.request().url())?r.continue():r.abort());
await page.addInitScript(()=>{
 navigator.requestMIDIAccess=async()=>{
  const {instance:{exports:e}}=await WebAssembly.instantiate(await (await fetch('/audio/smk-engine.wasm')).arrayBuffer());e.synth_init();
  const input={id:'in',name:'Felucca SMK-37',type:'input',state:'connected',open:async()=>{},close:async()=>{}};
  const output={id:'out',name:'Felucca SMK-37',type:'output',state:'connected',open:async()=>{},close:async()=>{},send(bytes){
   if(bytes[0]!==240)return;const b=Uint8Array.from(bytes).slice(1,-1),mem=new Uint8Array(e.memory.buffer);mem.set(b,e.fw_request());const n=e.fw_command(b.length);
   if(n){const data=mem.slice(e.fw_reply(),e.fw_reply()+n);queueMicrotask(()=>input.onmidimessage?.({data}));}
  }};
  window.smk161=e;return {inputs:new Map([['in',input]]),outputs:new Map([['out',output]])};
 };
});
try{
 await page.goto((process.env.SITE_URL||'http://127.0.0.1:8897')+'/editor.html?view=soundboard',{waitUntil:'commit'});
 const consent=page.locator('.analytics-banner [data-choice=denied]');if(await consent.isVisible())await consent.click();
 await page.keyboard.press('Escape');await page.getByRole('button',{name:'Connect',exact:true}).click();
 await page.waitForFunction(()=>document.querySelector('#status').textContent==='Connected');
 const essential=page.getByRole('button',{name:'Essential only',exact:true});if(await essential.isVisible())await essential.click();
 await page.getByRole('tab',{name:'Sequencer',exact:true}).click();
 const condition=page.getByLabel('Condition 1',{exact:true});await condition.selectOption('10');
 await page.waitForTimeout(200);assert.equal(await condition.inputValue(),'10');
 await page.getByRole('tab',{name:'Settings',exact:true}).click();
 await page.getByLabel('T1 CHANNEL',{exact:true}).selectOption({value:'9'});
 await page.waitForTimeout(200);assert.equal(await page.getByLabel('T1 CHANNEL',{exact:true}).inputValue(),'9');
 const bg=page.getByLabel('Background',{exact:true});await bg.fill('#102040');
 await page.getByRole('button',{name:'Save palette',exact:true}).click();
 await page.waitForFunction(()=>Array.from(document.querySelectorAll('option')).some(x=>x.textContent==='CUSTOM'));
 assert.equal(await page.getByLabel('Background',{exact:true}).inputValue(),'#102042');
 mkdirSync('build/screenshots',{recursive:true});await page.screenshot({path:'build/screenshots/smk161-settings.png'});
 await page.getByRole('button',{name:'Remove',exact:true}).click();
 await page.waitForFunction(()=>!Array.from(document.querySelectorAll('option')).some(x=>x.textContent==='CUSTOM'));
 await page.getByRole('tab',{name:'Sequencer',exact:true}).click();assert.equal(await condition.inputValue(),'10');
 await page.setViewportSize({width:390,height:844});await condition.scrollIntoViewIfNeeded();await page.screenshot({path:'build/screenshots/smk161-mobile.png'});
 assert.deepEqual(errors,[]);console.log('PASS: actual SMK firmware in Chrome: conditional steps, track MIDI channels, custom palette save/remove, mobile rendering, no script errors');
}catch(error){console.error("Browser errors:",errors,"Pending:",[...pending]);throw error;}finally{await browser.close();}
