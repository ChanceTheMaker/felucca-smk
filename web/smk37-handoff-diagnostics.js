// SPDX-License-Identifier: GPL-3.0-only
// Stage 1 only. No application image, stage-2 START, or updater.write call.
import {parseRequest,response} from './fm1ota.js?v=elite-handoff2';
import {QUERY,identityEvidence,midiInventory,probeMidiIdentity} from './midi-diagnostics.js?v=elite-handoff2';
import {isStockHandoffPackage} from './smk37-package.js?v=elite-handoff2';
const START=Object.freeze([0xf0,0x22,0x24,0x35,0x7f,0xf7]);
const sleep=ms=>new Promise(resolve=>setTimeout(resolve,ms));
function check(signal){if(signal?.aborted)throw Error('Diagnostic stopped. No flash-programming stage was started.');}

// Exported for fake-port tests. The caller must provide the verified header/OTA ranges.
export async function serveLoaderOnly(input,output,pkg,{log=()=>{},signal,idleMs=15000,stageMs=60000}={}){
  const queue=[];let overflow=false,served=0;
  const receive=event=>{const request=parseRequest(Array.from(event.data));if(request){if(queue.length>=256)overflow=true;else queue.push(request);}};
  input.addEventListener('midimessage',receive);
  const begin=Date.now();let last=begin;
  try{
    check(signal);log('Stage 1: sending START to the verified stock device only.');
    output.send([...START]);
    for(;;){
      check(signal);
      if(overflow)throw Error('Too many pending loader requests; diagnostic stopped.');
      if(Date.now()-begin>=stageMs)throw Error('Loader preparation exceeded the diagnostic time limit.');
      const r=queue.shift();
      if(!r){
        if(input.state==='disconnected'||output.state==='disconnected')throw Error('Stock MIDI port disconnected before the loader handoff completed.');
        if(Date.now()-last>=idleMs)throw Error('Stock device stopped requesting its RAM loader.');
        await sleep(20);continue;
      }
      last=Date.now();
      log('Stage-1 request: '+JSON.stringify({address:'0x'+r.addr.toString(16),bytes:r.len,flags:r.fl}));
      if(r.addr===0xe0000000){
        if(!served)throw Error('Unexpected handoff before any loader data was requested.');
        output.send(response(r.addr,[...new TextEncoder().encode('success'),0]));
        log('Stage 1 complete (0xe0000000). Flash-writing stage is disabled.');
        return {served};
      }
      if(r.addr===0xf0000000)throw Error('Blocked unexpected flash-stage completion request.');
      if(r.len<1||r.len>512||!pkg.ranges.some(([lo,hi])=>r.addr>=lo&&r.addr+r.len<=hi))
        throw Error('Blocked request outside official header/ota.bin: 0x'+r.addr.toString(16)+'+'+r.len+'. No application bytes were sent.');
      if(++served>4096)throw Error('Loader request limit exceeded.');
      await sleep(10);check(signal);
      output.send(response(r.addr,Array.from(pkg.image.subarray(r.addr,r.addr+r.len)),r.fl));
    }
  }finally{input.removeEventListener('midimessage',receive);}
}

export function captureHandoff(access,{log=()=>{}}={}){
  const listeners=new Map(),opened=new Set(),pending=new Set(),seen=new Set();
  let active=true,lastInventory='',lastQuery=null;
  function inventory(){const text=JSON.stringify(midiInventory(access));if(text!==lastInventory){lastInventory=text;log('MIDI ports: '+text);}}
  async function open(port){
    if(!active||port.state!=='connected')return false;
    if(port.connection!=='open')opened.add(port);
    try{await port.open();return active;}catch(e){log('Port open failed: '+JSON.stringify({id:port.id,type:port.type,name:port.name,error:e.message}));return false;}
  }
  function refresh(){
    if(!active)return;inventory();
    for(const input of access.inputs.values()){
      if(input.state!=='connected'||listeners.has(input))continue;
      const receive=event=>{
        const evidence=identityEvidence(event.data);if(!evidence)return;
        // Raw identity replies only; never notes, playing, or unrelated SysEx.
        const item={inputId:input.id,input:input.name,...evidence,lastQuery};
        const key=input.id+'|'+evidence.packet+'|'+(lastQuery?.outputId||'');
        if(seen.has(key))return;seen.add(key);log('Handoff identity reply: '+JSON.stringify(item));
      };
      listeners.set(input,receive);input.addEventListener('midimessage',receive);
      const p=open(input);pending.add(p);p.finally(()=>pending.delete(p));
    }
  }
  access.addEventListener('statechange',refresh);refresh();
  return {
    async ready(){await Promise.allSettled([...pending]);},
    async query(output){
      refresh();await Promise.allSettled([...pending]);
      if(!await open(output))return;
      lastQuery={outputId:output.id,output:output.name,sentAt:new Date().toISOString()};
      log('Handoff identity query: '+JSON.stringify(lastQuery));
      try{output.send([...QUERY]);}catch(e){log('Identity send failed: '+e.message);}
    },
    async stop(){
      active=false;access.removeEventListener('statechange',refresh);
      for(const [input,listener] of listeners)input.removeEventListener('midimessage',listener);
      await Promise.allSettled([...pending]);
      await Promise.allSettled([...opened].map(p=>p.close()));
      log('Diagnostic released its MIDI ports. Closed connection does not mean USB disconnected.');
    }
  };
}

export async function diagnoseStockHandoff(access,outputId,pkg,{log=()=>{},signal,observeMs=30000,probeMs=1200,queryMs=600,...stageOptions}={}){
  if(!isStockHandoffPackage(pkg))throw Error('A hash-verified official loader-only package is required.');
  check(signal);
  const baselineOutputs=new Set(access.outputs.keys());
  const smkOutputs=new Set([...access.outputs.values()].filter(p=>/smk|ota/i.test(p.name||'')).map(p=>p.id));
  const replies=await probeMidiIdentity(access,outputId,{log,timeoutMs:probeMs});check(signal);
  const matches=replies.filter(r=>r.model===pkg.stockModel&&r.version===16);
  if(matches.length!==1)throw Error('Expected exactly one '+pkg.stockModel+'_016 identity. No loader was staged.');
  const input=access.inputs.get(matches[0].inputId),output=access.outputs.get(outputId);
  if(!input||!output||input.state!=='connected'||output.state!=='connected')throw Error('Stock MIDI ports changed before staging; no loader was staged.');
  const previouslyOpen=output.connection==='open';
  const capture=captureHandoff(access,{log});
  try{
    await capture.ready();
    if(input.connection!=='open')throw Error('Cannot open the stock MIDI input; no loader was staged.');
    await output.open();check(signal);
    log('Verified stock identity '+matches[0].text+'. Official file SHA-256: '+pkg.stockSha);
    log('Official ota.bin SHA-256: '+pkg.loaderSha);
    await serveLoaderOnly(input,output,pkg,{...stageOptions,log,signal});
    const end=Date.now()+observeMs;
    log('Observing the handoff for '+Math.round(observeMs/1000)+' seconds. Only identity queries follow; alternate identities are evidence, never permission to flash.');
    while(Date.now()<end){
      check(signal);let queried=false;
      for(const candidate of access.outputs.values()){
        if(Date.now()>=end)break;
        if(candidate.state!=='connected')continue;
        // Exclude pre-existing unrelated devices (e.g. IAC). New outputs are
        // queried regardless of name; replies are heard on every input.
        if(candidate.id!==outputId&&!smkOutputs.has(candidate.id)&&baselineOutputs.has(candidate.id))continue;
        check(signal);queried=true;await capture.query(candidate);await sleep(queryMs);
      }
      if(!queried)await sleep(Math.min(queryMs,200));
    }
    log('Handoff diagnostic finished. No stage-2 START or flash payload was sent. Save this log, then power-cycle the SMK to leave the loader.');
  }finally{
    await capture.stop();
    if(!previouslyOpen)await output.close().catch(()=>{});
  }
}
