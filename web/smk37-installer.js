// SPDX-License-Identifier: GPL-3.0-only
import {parsePackage,installSmk,installStockSmk} from './smk37-update.js?v=0.9.22-r2';
import {buildSmkPublicPackage} from './smk37-package.js?v=0.9.22-r2';
const status=document.getElementById('status'),button=document.getElementById('install-button'),ack=document.getElementById('ack'),output=document.getElementById('log');
const file=document.getElementById('stock-file'),path=document.getElementById('install-path');
const privatePreview=['localhost','127.0.0.1','[::1]'].includes(location.hostname)&&new URLSearchParams(location.search).has('private');
let release,pkg,busy=false,selection=0;
const sync=()=>{button.disabled=busy||!pkg||!ack.checked||!navigator.requestMIDIAccess;file.disabled=busy;path.disabled=busy;ack.disabled=busy;};
const log=message=>{status.textContent=message;output.textContent+=message+'\n';};
ack.addEventListener('change',sync);
window.addEventListener('beforeunload',event=>{if(busy){event.preventDefault();event.returnValue='';}});
async function fetchBytes(url){const r=await fetch(new URL(url,import.meta.url),{cache:'no-store',signal:AbortSignal.timeout(15000)});if(!r.ok)throw Error('Firmware download is unavailable. Reload this page and try again.');return new Uint8Array(await r.arrayBuffer());}
try{
  if(privatePreview){
    release=JSON.parse(new TextDecoder().decode(await fetchBytes('./smk37-candidate.json')));
    if(release.private!==true)throw Error('Invalid private preview metadata.');
    pkg=await parsePackage(await fetchBytes('./firmware/smk37-candidate.smkupdate'),release.sha256);
    document.getElementById('public-package-controls').hidden=true;
    log(`${release.version}: local device-specific preview.`);
  }else{
    release=JSON.parse(new TextDecoder().decode(await fetchBytes('./firmware/public-release.json')));
    if(release.public!==true||release.product!=='SMK-37_922'||release.app!=='felucca-smk-0.9.22-app.bin'||!/^[a-f0-9]{64}$/.test(release.sha256))throw Error('Invalid public release metadata.');
    log(`${release.version} is available. Select your official Pro 016 or Elite 016 firmware file to prepare installation.`);
  }
  document.getElementById('release').textContent=release.version;
}catch(e){release=null;log(e.message);document.getElementById('release').textContent='Firmware download unavailable';}
file.addEventListener('change',async()=>{
  const request=++selection;pkg=null;ack.checked=false;sync();
  if(!release||!file.files?.length)return;
  try{
    const selected=file.files[0];if(selected.size>32*1024*1024)throw Error('Select an official SMK .fwsc file, not a ZIP archive or backup.');
    log('Checking the official file and preparing the beta locally...');
    const stock=new Uint8Array(await selected.arrayBuffer());
    const app=await fetchBytes('./firmware/'+release.app);
    const prepared=await buildSmkPublicPackage(stock,app,release);
    await parsePackage(prepared.bytes,prepared.sha);
    if(request!==selection)return;
    pkg=prepared;log(`${prepared.stockModel}: ${prepared.version} ready. Choose first installation or update, confirm the beta notice, then Install.`);
  }catch(e){if(request===selection)log(e.message);}finally{if(request===selection)sync();}
});
if(!navigator.requestMIDIAccess)log('Web MIDI is unavailable. Open this page in Chrome or Edge.');
sync();
button.addEventListener('click',async()=>{
  if(button.disabled)return;busy=true;sync();
  try{
    const midi=await navigator.requestMIDIAccess({sysex:true});
    if(privatePreview||path.value==='update')await installSmk(midi,pkg,log);
    else await installStockSmk(midi,pkg,log);
  }catch(e){log(e.message);}finally{busy=false;sync();}
});
