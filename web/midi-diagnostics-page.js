// SPDX-License-Identifier: GPL-3.0-only
import {midiInventory,probeMidiIdentity} from './midi-diagnostics.js?v=elite-handoff2';
import {buildStockHandoffPackage} from './smk37-package.js?v=elite-handoff2';
import {diagnoseStockHandoff} from './smk37-handoff-diagnostics.js?v=elite-handoff2';
import {STOCK_SOURCES} from './smk37-stock.js';
const $=id=>document.getElementById(id);
let access,busy=false,controller;
const lines=['Felucca SMK MIDI diagnostics v2 — loader handoff capture','Browser: '+navigator.userAgent];
function log(text){lines.push(new Date().toISOString()+' '+text);$('log').textContent=lines.join('\n');$('save').disabled=false;}
function controls(){
  const available=!!access&&!!$('output').options.length;
  $('output').disabled=busy||!available;$('probe').disabled=busy||!available;
  for(const id of ['model','stock-file','ack'])$(id).disabled=busy;
  $('handoff-start').disabled=busy||!available||!$('ack').checked;
  $('stop').disabled=!controller||controller.signal.aborted;
}
function ports(){
  const selected=$('output').value;
  $('output').replaceChildren(...[...access.outputs.values()].filter(p=>p.state==='connected').map(p=>new Option(p.name||'(unnamed output)',p.id)));
  if(access.outputs.get(selected)?.state==='connected')$('output').value=selected;
  else if(!busy){const preferred=[...access.outputs.values()].find(p=>p.state==='connected'&&/smk.*private/i.test(p.name));if(preferred)$('output').value=preferred.id;}
  controls();log('MIDI ports: '+JSON.stringify(midiInventory(access)));
}
$('ack').onchange=controls;
$('enable').onclick=async()=>{
  $('enable').disabled=true;
  try{
    if(!navigator.requestMIDIAccess)throw Error('Web MIDI is unavailable. Use Chrome or Edge over HTTPS or localhost.');
    access=await navigator.requestMIDIAccess({sysex:true});
    access.addEventListener('statechange',ports);ports();
    $('status').textContent='Select the SMK or loader output, then Check identity. Loader testing is a separate action below.';
  }catch(e){log(e.message);$('status').textContent=e.message;$('enable').disabled=false;}
};
$('probe').onclick=async()=>{
  busy=true;controls();
  try{await probeMidiIdentity(access,$('output').value,{log});$('status').textContent='Identity check complete. Save the log to share the results.';}
  catch(e){log(e.message);$('status').textContent=e.message;}
  finally{busy=false;ports();}
};
$('handoff-start').onclick=async()=>{
  if(busy||!access||!$('ack').checked)return;
  const outputId=$('output').value,model=$('model').value,file=$('stock-file').files[0];
  busy=true;controller=new AbortController();controls();
  let timeout;
  try{
    $('status').textContent='Preparing and verifying the official RAM loader. No device change yet.';
    let stock;
    if(file){if(file.size>8*1024*1024)throw Error('Official firmware file is too large.');stock=new Uint8Array(await file.arrayBuffer());}
    else{
      log('Fetching official 016 file: '+STOCK_SOURCES[model]);
      timeout=setTimeout(()=>controller.abort(),45000);
      const result=await fetch(STOCK_SOURCES[model],{signal:controller.signal,cache:'no-store'});
      if(!result.ok)throw Error('Official file download failed ('+result.status+'). Select your official .fwsc file above and retry.');
      stock=new Uint8Array(await result.arrayBuffer());clearTimeout(timeout);
    }
    if(controller.signal.aborted)throw Error('Diagnostic stopped before loader staging.');
    const pkg=await buildStockHandoffPackage(stock);
    if(pkg.stockModel!==model)throw Error('The file is for '+pkg.stockModel+'. Select the matching stock model. No loader was staged.');
    $('status').textContent='Testing the official loader handoff. Keep USB and power connected; no flash-writing stage will start.';
    await diagnoseStockHandoff(access,outputId,pkg,{log,signal:controller.signal});
    $('status').textContent='Capture complete. Save the diagnostic log, then power-cycle the SMK to return to stock firmware.';
  }catch(e){
    const message=controller.signal.aborted?'Diagnostic stopped. No flash-writing stage was started.':e.message;
    log(message);$('status').textContent=message+' Save the log. If the loader started or the screen is black, power-cycle the SMK after this test stops.';
  }finally{clearTimeout(timeout);controller=null;busy=false;$('ack').checked=false;ports();}
};
$('stop').onclick=()=>{controller?.abort();controls();$('status').textContent='Stopping diagnostic and releasing MIDI ports…';};
$('save').onclick=()=>{
  const url=URL.createObjectURL(new Blob([lines.join('\n')+'\n'],{type:'text/plain'}));
  const a=document.createElement('a');a.href=url;a.download='felucca-smk-midi-diagnostics.txt';a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);
};
