// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {createPanelGlide} from './panel-glide.js';
const events=[],active=new Map();
function button(kind,id){const handlers=new Map();return {dataset:kind===3?{pad:String(id)}:{key:String(id)},classList:{contains:c=>c===(kind===3?'sim-pad':'sim-key')},closest(){return this;},addEventListener(n,f){handlers.set(n,f);},focus(){},setPointerCapture(){},fire(n,x,y=0,pointer=1,type='mouse',buttons=1){handlers.get(n)?.({pointerId:pointer,pointerType:type,button:0,buttons,clientX:x,clientY:y,preventDefault(){}});}};}
const pads=Array.from({length:16},(_,i)=>button(3,i)),key=button(2,0),outsider=button(3,99);
const host={ownerDocument:{elementFromPoint(x,y){if(y===99)return outsider;if(y===2)return key;if(y<0||y>1||x<0||x>=160)return null;const col=Math.floor(x/20);return x%20<16?pads[y*8+col]:null;}},contains:b=>pads.includes(b)||b===key};
const glide=createPanelGlide(host,(owner,kind,id,value)=>{active.set(owner,{kind,id});events.push(['on',owner,kind,id,value]);},owner=>{if(active.has(owner)){events.push(['off',owner]);active.delete(owner);}});
for(let i=0;i<16;i++)glide.bind(pads[i],3,i,100);glide.bind(key,2,0,100);
pads[0].fire('pointerdown',1);pads[0].fire('pointermove',151);pads[0].fire('pointerup',151);
assert.deepEqual(events.filter(e=>e[0]==='on').map(e=>e[3]),[0,1,2,3,4,5,6,7]);assert.equal(active.size,0);
events.length=0;
pads[0].fire('pointerdown',1,0,11,'touch');pads[8].fire('pointerdown',1,1,12,'touch');pads[0].fire('pointermove',31,0,11,'touch');pads[8].fire('pointermove',31,1,12,'touch');assert.equal(active.get('p11').id,1);assert.equal(active.get('p12').id,9);
const count=events.length;pads[0].fire('pointermove',32,0,11,'touch');assert.equal(events.length,count,'stationary pad must not retrigger');
pads[0].fire('pointercancel',32,0,11,'touch');assert.equal(active.size,1);pads[8].fire('lostpointercapture',31,1,12,'touch');assert.equal(active.size,0);
events.length=0;pads[0].fire('pointerdown',1);pads[0].fire('pointermove',18);assert.equal(active.size,0);pads[0].fire('pointermove',1);assert.equal(events.filter(e=>e[0]==='on').length,2,'re-entering a pad retriggers');pads[0].fire('pointermove',1,99);assert.equal(active.size,0,'other simulator hosts ignored');
pads[0].fire('pointerup',1,99);key.fire('pointerdown',1,2);assert.equal(active.get('p1').kind,2);key.fire('pointermove',1,2,1,'mouse',0);assert.equal(active.size,0);
pads[0].fire('pointerdown',1);glide.clear();pads[0].fire('pointermove',31);assert.equal(active.size,0,'hidden/blurred simulator cannot keep playing');
console.log('PASS: mouse sweep, touch pointers, gaps/re-entry, no repeated hits, cancellation, host isolation and piano release');
