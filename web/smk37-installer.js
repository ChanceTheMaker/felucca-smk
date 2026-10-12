// SPDX-License-Identifier: GPL-3.0-only
import {parsePackage,installSmk,installStockSmk} from './smk37-update.js?v=smk-doom-r1';
import {buildSmkPublicPackage} from './smk37-package.js?v=0.9.22-r4';
import {STOCK_SOURCES} from './smk37-stock.js?v=0.9.22-r4';
const status=document.getElementById('status'),button=document.getElementById('install-button'),ack=document.getElementById('ack'),output=document.getElementById('log');
const file=document.getElementById('stock-file'),path=document.getElementById('install-path'),model=document.getElementById('smk-model');
const dialog=document.getElementById('loader-confirm');
async function confirmGeneric({identity,port,model}){
  document.getElementById('loader-identity').textContent=identity+' on '+port;
  document.getElementById('loader-model').textContent=model;
  log('Confirm the connected Elite in the dialog to continue this installation.');
  dialog.returnValue='cancel';
  return new Promise(resolve=>{
    dialog.addEventListener('close',()=>resolve(dialog.returnValue==='continue'),{once:true});
    dialog.showModal();
  });
}
const loopback=['localhost','127.0.0.1','[::1]'].includes(location.hostname);
const privateRequested=loopback&&new URLSearchParams(location.search).has('private');
const publicPreview=loopback&&new URLSearchParams(location.search).has('public');
let release,pkg,busy=false,selection=0,usingPrivate=false;
const sync=()=>{button.disabled=busy||!pkg||!ack.checked||!navigator.requestMIDIAccess;file.disabled=busy;path.disabled=busy;model.disabled=busy;ack.disabled=busy;};
const log=message=>{status.textContent=message;output.textContent+=message+'\n';};
ack.addEventListener('change',sync);
window.addEventListener('beforeunload',event=>{if(busy){event.preventDefault();event.returnValue='';}});
async function fetchBytes(url){const r=await fetch(new URL(url,import.meta.url),{cache:'no-store',signal:AbortSignal.timeout(20000)});if(!r.ok)throw Error('Firmware download is unavailable. Reload this page or use the optional official-file selector.');return new Uint8Array(await r.arrayBuffer());}
async function prepare(stockOverride){
  const request=++selection;pkg=null;ack.checked=false;sync();
  if(!release)return;
  try{
    log(`Preparing ${release.version} for ${model.value} automatically...`);
    const [stock,app]=await Promise.all([stockOverride?Promise.resolve(stockOverride):fetchBytes(STOCK_SOURCES[model.value]),fetchBytes('./firmware/'+release.app)]);
    const prepared=await buildSmkPublicPackage(stock,app,release);
    await parsePackage(prepared.bytes,prepared.sha);
    if(request!==selection)return;
    pkg=prepared;model.value=prepared.stockModel;
    log(`${prepared.stockModel}: ${prepared.version} ready. Confirm the beta notice, then Install.`);
  }catch(e){if(request===selection){log(e.message);document.getElementById('manual-file').open=true;}}
  finally{if(request===selection)sync();}
}
model.addEventListener('change',()=>{file.value='';prepare();});
file.addEventListener('change',async()=>{
  if(!file.files?.length)return;
  const selected=file.files[0];
  if(selected.size>32*1024*1024){pkg=null;sync();log('Select an official SMK .fwsc file, not a ZIP archive or backup.');return;}
  await prepare(new Uint8Array(await selected.arrayBuffer()));
});
try{
  if(loopback&&(!publicPreview||privateRequested)){
    try{
      const r=await fetch(new URL('./smk37-candidate.json',import.meta.url),{cache:'no-store',signal:AbortSignal.timeout(3000)});
      if(r.ok){const candidate=await r.json();if(candidate.private===true&&(privateRequested||candidate.migration===true)){release=candidate;usingPrivate=true;}}
    }catch(e){if(privateRequested)throw e;}
    if(privateRequested&&!usingPrivate)throw Error('No private candidate is staged.');
  }
  if(usingPrivate){
    pkg=await parsePackage(await fetchBytes('./firmware/smk37-candidate.smkupdate'),release.sha256);
    if(pkg.product!==release.product)throw Error('Local migration identity mismatch.');
    document.getElementById('public-package-controls').hidden=true;
    log(release.migration?'Felucca SMK 0.9.22 Beta is ready for your Test21 device. Confirm the beta notice, then Install.':`${release.version}: local device-specific preview ready.`);
    document.getElementById('release').textContent=release.migration?'Felucca SMK 0.9.22 Beta - local migration':release.version;
  }else{
    release=JSON.parse(new TextDecoder().decode(await fetchBytes('./firmware/public-release.json')));
    if(release.public!==true||release.product!=='SMK-37_923'||release.app!=='felucca-smk-0.9.23-app.bin'||!/^[a-f0-9]{64}$/.test(release.sha256))throw Error('Invalid public release metadata.');
    document.getElementById('release').textContent=release.version;
    prepare();
  }
}catch(e){release=null;pkg=null;log(e.message);document.getElementById('release').textContent='Firmware download unavailable';}
if(!navigator.requestMIDIAccess)log('Web MIDI is unavailable. Open this page in Chrome or Edge.');
sync();
button.addEventListener('click',async()=>{
  if(button.disabled)return;busy=true;sync();
  try{
    const midi=await navigator.requestMIDIAccess({sysex:true});
    if(usingPrivate||path.value==='update')await installSmk(midi,pkg,log);
    else await installStockSmk(midi,pkg,log,{confirmGeneric});
  }catch(e){log(e.message);}finally{busy=false;sync();}
});
