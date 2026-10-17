import {hasNativeSmk,runNativeSmk,nativeModel} from '../native-smk.js?v=native1';
import {requestMidiSession,releaseMidiSession} from '../midi-session.js?v=session1';
import {createProgress} from '../update-progress.js?v=progress1';
// SPDX-License-Identifier: GPL-3.0-only
import {prepareStock,restoreStock} from './core.js?v=progress1';
import {MODELS,SITE_NAME,BACK_URL} from './site.js';
const base='https://raw.githubusercontent.com/jonathaslacerda/smk-37-pro-docs/ab85d7340163ba9e171dc9396e31c034829bbc02/firmware/';
const sources={'SMK-37 Pro':base+'smk37pro/SMK-37_Pro_016.fwsc','SMK-37 Elite':base+'smk37elite/SMK-37_Elite_016.fwsc'};
const $=id=>document.getElementById(id);let busy=false,pkg=null;
const progress=createProgress($('status'));
$('back').textContent='Back to '+SITE_NAME;$('back').href=BACK_URL;
for(const model of MODELS){const o=document.createElement('option');o.value=o.textContent=model;$('model').append(o);}
const log=text=>{$('log').textContent+=text+'\n';$('log').scrollTop=$('log').scrollHeight;};
function sync(){for(const id of ['model','file','prepare'])$(id).disabled=busy;$('restore').disabled=busy||!pkg||!$('ack').checked||!navigator.requestMIDIAccess;$('ack').disabled=busy;}
function reset(){progress.reset();pkg=null;$('ack').checked=false;$('status').textContent='Select your exact physical model, then prepare the official firmware.';sync();}
$('model').onchange=()=>{$('file').value='';reset();};$('file').onchange=reset;$('ack').onchange=sync;
$('prepare').onclick=async()=>{
  if(busy)return;busy=true;pkg=null;sync();progress.start('prepare');
  try{
    const model=$('model').value,file=$('file').files[0];let bytes;
    if(file)bytes=new Uint8Array(await file.arrayBuffer());
    else{
      if(!sources[model])throw Error('Select the unmodified official FM-1 V15 file, FM-1.fwsc, from M-VAVE. It stays in this browser.');
      $('status').textContent='Downloading and checking official '+model+' 016...';
      const r=await fetch(sources[model],{cache:'no-store',signal:AbortSignal.timeout(45000)});
      if(!r.ok)throw Error('Official download unavailable. Select the matching official .fwsc file below.');
      bytes=new Uint8Array(await r.arrayBuffer());
    }
    pkg=await prepareStock(bytes,model);progress.ready();
    $('status').textContent=pkg.version+' verified. Confirm the notice to restore.';
    log('Verified official file for '+model+'. Prepared package SHA-256: '+pkg.sha);
  }catch(e){progress.fail();pkg=null;$('status').textContent=e.message;log(e.message);}finally{busy=false;sync();}
};
$('restore').onclick=async()=>{
  if($('restore').disabled)return;busy=true;sync();progress.start();
  try{
    let result;
    if(await hasNativeSmk(nativeModel(pkg.model))){
      if(!confirm('Restore '+pkg.version+'? Confirm the connected instrument is physically an SMK-37 Pro. This replaces its current firmware. Keep USB and power connected.'))throw Error('Restore cancelled. No update was started.');
      result={identity:await runNativeSmk('restore',log),alreadyStock:false};
    }else{
      const access=await requestMidiSession();
      result=await restoreStock(access,pkg,log,{confirmModel:({model,identity,version})=>{
        return confirm('Restore '+version+'?\n\nConfirm the ONLY connected instrument is physically an '+model+'. It currently reports '+identity+'.\n\nThis replaces the current firmware. Custom projects may be incompatible or overwritten by the original firmware. Keep USB and power connected.');
      }});
    }
    progress.done(result.alreadyStock?'Original firmware already installed and confirmed':undefined);
    $('status').textContent=result.alreadyStock?'Original firmware is already installed.':'Original firmware restored and USB identity confirmed.';pkg=null;
  }catch(e){progress.fail();$('status').textContent=e.message;log('Stopped: '+e.message);}finally{await releaseMidiSession();busy=false;sync();}
};
$('save-log').onclick=()=>{const url=URL.createObjectURL(new Blob([$('log').textContent],{type:'text/plain'}));const a=document.createElement('a');a.href=url;a.download='stock-restore-log.txt';a.click();setTimeout(()=>URL.revokeObjectURL(url),10000);};
window.addEventListener('beforeunload',e=>{if(busy){e.preventDefault();e.returnValue='';}});
if(!navigator.requestMIDIAccess)$('browser-note').textContent='Use desktop Chrome or Edge for USB installation.';
reset();
