import assert from 'node:assert/strict';
import {createKeyboardGlide} from './keyboard-glide.js';
const events=[],held=new Map();
const glide=createKeyboardGlide((x,y)=>y<0||x<0||x>=100?null:Math.floor(x/10),
  (id,key)=>{events.push([id,key]);if(key===null)held.delete(id);else held.set(id,key);},id=>held.delete(id));
glide.start(1,1,10);glide.move(1,95,10);
assert.deepEqual(events.map(e=>e[1]),[0,1,2,3,4,5,6,7,8,9],'fast swipes must visit crossed keys');
glide.move(1,95,-10);assert.equal(held.size,0,'leaving the keybed releases the note');
glide.move(1,95,10);assert.equal(held.get(1),9,'captured pointer can re-enter');
glide.start(2,25,10);glide.move(1,75,10);assert.equal(held.get(2),2,'second finger stays independent');
glide.end(1);assert.equal(held.size,1);glide.end(1);assert.equal(held.size,1,'duplicate cancellation is harmless');
glide.clear();assert.equal(held.size,0);const n=events.length;glide.move(2,55,10);assert.equal(events.length,n,'blur/reset cancels pending swipes');
console.log('PASS: crossed keys, leaving/re-entering, independent touches, release/cancel and reset');
