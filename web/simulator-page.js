// SPDX-License-Identifier: GPL-3.0-only
import {SMKBrowserSynth} from './audio/smk-browser.js?startup=2';
import {createSMKSimulator} from './smk-simulator.js';
const $=id=>document.getElementById(id),synth=new SMKBrowserSynth();
let panel=null,running=false,busy=false,starting=null;
const title=()=>{document.title='Felucca SMK — Simulator';};
document.addEventListener('DOMContentLoaded',title);window.FeluccaI18n.onChange(title);title();
function controls(){for(const id of ['session-play','session-panic','session-export','session-import'])$(id).disabled=!running||busy;$('sim-audio').disabled=busy;$('sim-audio').textContent=running?'Pause audio':'Enable audio';}
function startAudio(){
  if(starting)return starting;
  busy=true;controls();
  starting=(async()=>{
    try{ $('audio-status').textContent='Starting simulator…';await synth.start({waitForAudio:false});synth.volume(+$('audio-volume').value/100);if(!panel)panel=createSMKSimulator($('smk-simulator'),synth);panel.show(true);$('sim-intro').hidden=true;synth.showScope($('scope-canvas'));running=synth.context.state==='running';$('audio-status').textContent=running?'Native SMK engine · Browser audio':'Play a key or pad to enable audio.';}
    catch(e){$('audio-status').textContent=e.message;}
    finally{starting=null;busy=false;controls();}
  })();
  return starting;
}
function unlockAudio(){
  if(!synth.node||synth.context.state==='running')return;
  // Called in the original input gesture; the note message then queues on the
  // prepared worklet and plays as soon as the context starts.
  synth.context.resume().then(()=>{running=true;controls();$('audio-status').textContent='Native SMK engine · Browser audio';}).catch(e=>{$('audio-status').textContent=e.message;});
}
document.addEventListener('pointerdown',e=>{if(e.target.closest('.sim-key,.sim-pad'))unlockAudio();},true);
document.addEventListener('keydown',e=>{
  if(e.ctrlKey||e.metaKey||e.altKey||e.target.closest('input,select,textarea,[contenteditable=true],.sim-tour,.sim-finish-picker'))return;
  if(['KeyA','KeyW','KeyS','KeyE','KeyD','KeyF','KeyT','KeyG','KeyY','KeyH','KeyU','KeyJ','KeyK','KeyO','KeyL','KeyP','Semicolon'].includes(e.code)||(['Space','Enter'].includes(e.code)&&e.target.closest('.sim-key,.sim-pad')))unlockAudio();
},true);
$('sim-audio').onclick=async()=>{
  if(starting){try{await synth.context?.resume();}catch(e){$('audio-status').textContent=e.message;}return;}
  if(!running){await startAudio();return;}
  busy=true;controls();
  try{panel.release();await synth.stop();running=false;$('audio-status').textContent='Audio paused';}
  catch(e){$('audio-status').textContent=e.message;}
  finally{busy=false;controls();}
};
$('audio-volume').oninput=()=>synth.volume(+$('audio-volume').value/100);
$('session-panic').onclick=()=>panel?.release();
$('session-play').onclick=()=>{synth.panel(0,16,1);setTimeout(()=>synth.panel(0,16,0),60);};
async function sessionTask(task){busy=true;controls();try{await task();}catch(e){$('audio-status').textContent=e.message;}finally{busy=false;controls();}}
$('session-export').onclick=()=>sessionTask(async()=>{
  const data=await synth.exportSession(),url=URL.createObjectURL(new Blob([JSON.stringify(data)],{type:'application/json'}));
  const link=document.createElement('a');link.href=url;link.download='felucca-smk-browser-session.json';link.click();setTimeout(()=>URL.revokeObjectURL(url),1000);
});
$('session-import').onclick=()=>$('session-file').click();
$('session-file').onchange=()=>sessionTask(async()=>{const file=$('session-file').files[0];$('session-file').value='';if(!file)return;if(file.size>2000000)throw Error('Session is too large');panel.release();await synth.importSession(JSON.parse(await file.text()));$('audio-status').textContent='Browser session restored';});
setInterval(()=>{if(synth.storageMessage)$('session-status').textContent=synth.storageMessage;},1000);
// Save before moving to Studio so both pages reopen the same native session.
document.addEventListener('click',async e=>{
  const link=e.target.closest('a[href]');if(!link||!synth.node||e.defaultPrevented||e.button!==0||e.ctrlKey||e.metaKey||e.shiftKey||e.altKey||link.target||link.download)return;
  const url=new URL(link.href);if(url.origin!==location.origin||url.pathname===location.pathname)return;
  e.preventDefault();if(busy)return;busy=true;controls();
  try{panel.release();await synth.save();await synth.stop();location.assign(url.href);}catch(error){$('audio-status').textContent='Could not save before leaving: '+error.message;busy=false;controls();}
});
window.addEventListener('pagehide',()=>{panel?.release();});
startAudio();
