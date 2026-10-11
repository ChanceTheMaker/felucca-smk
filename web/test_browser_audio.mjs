// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {mkdir} from 'node:fs/promises';
const {chromium} = await import(process.env.PLAYWRIGHT_MODULE || 'playwright');
const browser = await chromium.launch({channel:'chrome',headless:true});
const page = await browser.newPage({viewport:{width:1440,height:1100}});
const errors=[]; page.on('pageerror',e=>errors.push(e.message));
await page.addInitScript(() => {
  navigator.requestMIDIAccess = () => { throw Error('Browser mode must not request MIDI access'); };
  const Context = window.AudioContext;
  window.AudioContext = class extends Context {
    constructor(options) { super(options); window.testAudioContext=this; }
    createGain() {
      const gain=super.createGain();
      window.testAnalyser=this.createAnalyser(); gain.connect(window.testAnalyser);
      return gain;
    }
  };
});
try {
  await page.goto((process.env.SITE_URL || 'http://127.0.0.1:8768') + '/webapp/editor/#sound', {waitUntil:'domcontentloaded'});
  const consent = page.locator('.analytics-banner [data-choice=denied]');
  if (await consent.isVisible()) await consent.click();
  await page.keyboard.press('Escape');
  await page.locator('#mode-browser').click();
  await page.waitForFunction(()=>document.querySelector('#status').textContent==='Audio ready');
  assert.equal(await page.locator('#trackbar').isVisible(),true);
  assert.equal(await page.locator('#tabs').isVisible(),true);
  assert.equal(await page.locator('#groups > .studio-section').count(),6);
  assert.equal(await page.locator('#mode-fm1').isVisible(),true);
  assert.equal(await page.locator('#mode-browser').getAttribute('aria-pressed'),'true');
  assert.equal(await page.locator('#audio-scope').isVisible(),true);
  const idleTrace = await page.locator('#scope-canvas').evaluate(c=>c.toDataURL());
  const key=page.locator('#play-keys button[data-note="60"]');
  const box=await key.boundingBox();
  await page.mouse.move(box.x+box.width/2,box.y+box.height-10); await page.mouse.down();
  await page.waitForFunction(()=>{
    const a=new Float32Array(window.testAnalyser.fftSize); window.testAnalyser.getFloatTimeDomainData(a);
    return a.some(v=>Math.abs(v)>0.001);
  });
  await page.waitForFunction(idle=>document.querySelector('#scope-canvas').toDataURL()!==idle,idleTrace);
  await mkdir('build/screenshots',{recursive:true});
  await page.screenshot({path:'build/screenshots/browser-scope-playing.png'});
  await page.mouse.up();
  await page.locator('#engine').selectOption('4');
  await page.waitForFunction(()=>document.querySelector('#preset').options.length===4);
  assert.equal(await page.locator('#engine option').count(),13);
  assert.match(await page.locator('#play-help').textContent(),/Playing in your browser/);
  await page.getByRole('button',{name:'Stop audio',exact:true}).click();
  await page.waitForFunction(()=>window.testAudioContext.state==='suspended');
  await page.getByRole('button',{name:'Enable audio',exact:true}).click();
  await page.waitForFunction(()=>window.testAudioContext.state==='running');
  await page.waitForFunction(()=>document.querySelector('#status').textContent==='Audio ready');
  await page.locator('#mode-fm1').click();
  await page.waitForFunction(()=>window.testAudioContext.state==='suspended');
  assert.equal(await page.locator('#audio-scope').isVisible(),false);
  assert.equal(await page.locator('#tabs').isVisible(),true);
  await page.locator('#mode-browser').click();
  await page.waitForFunction(()=>document.querySelector('#status').textContent==='Audio ready');
  await page.locator('#mode-simulator').click();
  assert.equal(await page.locator('.sim-key').count(),37);
  assert.equal(await page.locator('.sim-pad').count(),16);
  assert.equal(await page.locator('#tabs').isVisible(),false);
  await page.locator('#mode-browser').click();
  await mkdir('build/screenshots',{recursive:true});
  await page.screenshot({path:'build/screenshots/browser-audio-stage.png'});
  assert.deepEqual(errors,[]);
  await page.setViewportSize({width:390,height:844});
  assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth+1),'Mobile mode switch and scope fit');
  console.log('Browser audio: mode toggle/autostart, live scope, keyboard audio, MIDI isolation, engine switch, stop/restart, mobile layout passed');
} catch(error) {
  console.error('Original browser regression failure:',error);
  await mkdir('build/screenshots',{recursive:true});
  await page.screenshot({path:'build/screenshots/browser-audio-failure.png',timeout:5000}).catch(()=>{});
  console.error(errors, await page.locator('#status').textContent(), await page.locator('#audio-status').textContent(), await page.locator('#mode-fm1').getAttribute('aria-pressed'));
  throw error;
} finally { await browser.close(); }
