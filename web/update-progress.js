// SPDX-License-Identifier: GPL-3.0-only
// Browser-only presentation events. No MIDI access or device commands.
export const PROGRESS_EVENT='firmware-transfer-progress';
export function reportProgress(detail){
  try{if(typeof document!=='undefined')document.dispatchEvent(new CustomEvent(PROGRESS_EVENT,{detail}));}catch{}
}
// Count unique requested bytes, not requests: validation passes and retries
// often ask for the same data again. Never imply these are flash bytes written.
export function createTransferCounter(total){
  let ranges=[];
  return (start,length)=>{
    if(!Number.isSafeInteger(start)||!Number.isSafeInteger(length)||start<0||length<0||start+length>total)return null;
    let lo=start,hi=start+length,next=[],added=false;
    for(const [a,b] of ranges){
      if(b<lo)next.push([a,b]);
      else if(a>hi){if(!added){next.push([lo,hi]);added=true;}next.push([a,b]);}
      else{lo=Math.min(lo,a);hi=Math.max(hi,b);}
    }
    if(!added)next.push([lo,hi]);ranges=next;
    return {bytes:ranges.reduce((n,[a,b])=>n+b-a,0),total};
  };
}
export function progressView({phase,bytes,total,label}={}){
  if(phase==='done')return {label:label||'Complete — device restart confirmed',value:100,detail:'100%',state:'done'};
  if(phase==='transfer'&&Number.isFinite(bytes)&&total>0){
    const percent=Math.min(99,Math.floor(bytes*100/total));
    return {label:'Transferring firmware data',value:percent,detail:`${percent}% · ${Math.floor(bytes/1024)} / ${Math.ceil(total/1024)} KiB served`,state:'active'};
  }
  const labels={prepare:'Preparing and checking firmware',ready:'Firmware ready to install',connect:'Connecting to device / preparing loader',loader:'Preparing update loader',reboot:'Waiting for restart and checking firmware',check:'Transfer finished — check the device',error:'Update stopped'};
  return {label:label||labels[phase]||labels.connect,value:null,detail:phase==='ready'?'Ready':phase==='error'?'Not completed':phase==='check'?'Boot not confirmed':'Please wait…',state:phase==='error'?'error':phase==='ready'||phase==='check'?'idle':'active'};
}
export function createProgress(anchor){
  if(!document.getElementById('firmware-progress-style')){
    const style=document.createElement('style');style.id='firmware-progress-style';
    style.textContent=`.fw-progress{margin:16px 0;padding:14px 16px;border:1px solid #7f8b9955;border-radius:8px;background:#00000012;color:inherit;text-align:left;max-width:100%;min-width:0}.fw-progress[hidden]{display:none}.fw-progress-head{display:flex;align-items:baseline;justify-content:space-between;gap:8px 18px;flex-wrap:wrap;font:600 13px/1.5 system-ui,sans-serif}.fw-progress-detail{font:12px/1.5 ui-monospace,monospace;font-variant-numeric:tabular-nums}.fw-progress progress{display:block;appearance:none;width:100%;height:14px;margin:10px 0 0;border:1px solid #7f8b9970;border-radius:5px;overflow:hidden;background:#7f8b992b;accent-color:#df8d2e}.fw-progress progress::-webkit-progress-bar{background:#7f8b992b}.fw-progress progress::-webkit-progress-value{background:linear-gradient(90deg,#bb6823,#ffbe57);border-radius:3px}.fw-progress progress::-moz-progress-bar{background:#e69a37}.fw-progress[data-state=done] progress::-webkit-progress-value{background:#399968}.fw-progress[data-state=error] progress::-webkit-progress-value{background:#cc5146}.fw-progress[data-state=active] progress:not([value]){background:repeating-linear-gradient(110deg,#7f8b992b 0 16px,#db933d88 16px 32px);background-size:70px 100%;animation:fw-progress-wait 1.2s linear infinite}.fw-progress[data-state=error]{border-color:#c96860}.fw-progress-note{font:11px/1.6 system-ui,sans-serif;margin:8px 0 0;opacity:.8}.fw-progress[data-state=done] .fw-progress-note{display:none}@keyframes fw-progress-wait{to{background-position:70px 0}}@media(prefers-reduced-motion:reduce){.fw-progress progress{animation:none!important}}`;
    document.head.append(style);
  }
  const box=document.createElement('div');box.setAttribute('role','region');box.className='fw-progress';box.hidden=true;box.setAttribute('aria-label','Firmware update progress');
  const head=document.createElement('div');head.className='fw-progress-head';
  const label=document.createElement('span');label.setAttribute('role','status');label.setAttribute('aria-live','polite');
  const detail=document.createElement('span');detail.className='fw-progress-detail';
  const bar=document.createElement('progress');bar.max=100;bar.setAttribute('aria-label','Firmware transfer progress');
  const note=document.createElement('p');note.className='fw-progress-note';note.textContent='Keep power and USB connected. Data transfer is followed by device verification and restart.';
  head.append(label,detail);box.append(head,bar,note);anchor.insertAdjacentElement('afterend',box);
  let active=false,last=0;
  const show=event=>{
    const view=progressView(event);box.hidden=false;box.dataset.state=view.state;
    if(label.textContent!==view.label)label.textContent=view.label;
    detail.textContent=view.detail;note.hidden=['prepare','ready','done'].includes(event.phase);
    if(view.value===null)bar.removeAttribute('value');else{bar.value=view.value;last=view.value;}
    bar.setAttribute('aria-valuetext',view.label+'; '+view.detail);
  };
  const receive=event=>{if(active)show(event.detail);};document.addEventListener(PROGRESS_EVENT,receive);
  return {
    start(phase='connect'){active=true;last=0;show({phase});},
    ready(){active=false;show({phase:'ready'});bar.value=0;},
    stage(phase,label){show({phase,label});},
    done(label){active=false;show({phase:'done',label});},
    unconfirmed(){active=false;show({phase:'check'});bar.value=last;},
    fail(){active=false;show({phase:'error'});bar.value=Math.min(last,99);},
    reset(){active=false;last=0;box.hidden=true;bar.value=0;},
    destroy(){document.removeEventListener(PROGRESS_EVENT,receive);box.remove();}
  };
}
