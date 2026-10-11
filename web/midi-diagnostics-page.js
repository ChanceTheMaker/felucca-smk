// SPDX-License-Identifier: GPL-3.0-only
import {midiInventory,probeMidiIdentity} from './midi-diagnostics.js';
const $=id=>document.getElementById(id);
let access, busy=false;
const lines=['Felucca SMK MIDI diagnostics v1',`Browser: ${navigator.userAgent}`];
function log(text){lines.push(`${new Date().toISOString()} ${text}`);$('log').textContent=lines.join('\n');$('save').disabled=false;}
function ports(){
  const selected=$('output').value;
  $('output').replaceChildren(...[...access.outputs.values()].filter(p=>p.state==='connected').map(p=>new Option(p.name||'(unnamed output)',p.id)));
  if(access.outputs.has(selected)&&access.outputs.get(selected).state==='connected')$('output').value=selected;
  $('output').disabled=busy||!$('output').options.length;
  $('probe').disabled=busy||!$('output').options.length;
  log(`MIDI ports: ${JSON.stringify(midiInventory(access))}`);
}
$('enable').onclick=async()=>{
  $('enable').disabled=true;
  try{
    if(!navigator.requestMIDIAccess)throw Error('Web MIDI is unavailable. Use Chrome or Edge over HTTPS or localhost.');
    access=await navigator.requestMIDIAccess({sysex:true});
    access.addEventListener('statechange',ports);ports();
    $('status').textContent='Select the SMK or loader output, then Check identity.';
  }catch(e){log(e.message);$('status').textContent=e.message;$('enable').disabled=false;}
};
$('probe').onclick=async()=>{
  busy=true;$('probe').disabled=$('output').disabled=true;
  try{await probeMidiIdentity(access,$('output').value,{log});$('status').textContent='Check complete. Save the log to share the results.';}
  catch(e){log(e.message);$('status').textContent=e.message;}
  finally{busy=false;ports();}
};
$('save').onclick=()=>{
  const url=URL.createObjectURL(new Blob([lines.join('\n')+'\n'],{type:'text/plain'}));
  const a=document.createElement('a');a.href=url;a.download='felucca-smk-midi-diagnostics.txt';a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);
};
