// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import vm from 'node:vm';
const html=readFileSync('web/editor.html','utf8');
const proto=html.slice(html.indexOf('/*PROTO-BEGIN*/'),html.indexOf('/*PROTO-END*/'));
const {parse,CMD,req,replyMatches}=vm.runInNewContext(proto+';({parse,CMD,req,replyMatches})',{console,setTimeout,clearTimeout});
const frames=readFileSync('build/smk37-tests/test19-wire.jsonl','utf8').trim().split('\n').map(JSON.parse);
const rows=[];
for(const f of frames){
  const result=parse[f[4]](f.slice(5,-1));
  if(f[4]===CMD.MENU_DESC){rows.push(result);if(result.kind===0)assert.equal(result.choices.length,result.max-result.min+1);else{assert.equal(result.id,22);assert.equal(result.min,-50);assert.equal(result.max,50);}assert(result.tabName);assert(replyMatches(f[4],[result.index],f.slice(5,-1)));}
  else {assert.equal(result.rc,0);assert.equal(result.id,18);assert.equal(result.value,1);}
}
assert.equal(rows.length,26);assert.equal(new Set(rows.map(r=>r.id)).size,26);
assert.deepEqual(Array.from(rows.find(r=>r.id===9).choices),['FLAT','LOWCUT','BASS+']);
assert.equal(rows.find(r=>r.id===21).name,'MIDI IN');
for(let id=26;id<=29;id++){const row=rows.find(r=>r.id===id);assert.equal(row.choices.length,18);assert.equal(row.choices[16],'OMNI');assert.equal(row.choices[17],'OFF');}
assert.equal(rows.find(r=>r.id===4).choices.length,9);
assert.deepEqual(Array.from(rows.find(r=>r.id===23).choices),['SCOPE','TRACKS']);
assert.deepEqual(Array.from(rows.find(r=>r.id===3).choices),['ON','OFF']);
assert.equal(rows.find(r=>r.id===18).value,0);
assert.deepEqual(Array.from(req.menuSet(18,1)[1]),[18,1,64]);
assert.throws(()=>parse[CMD.MENU_DESC]([0,1,0]));
console.log('Test19 MENU: firmware descriptors/settings decoded by shipping Studio, stable IDs, choices and short reply rejection passed');
