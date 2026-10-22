// SPDX-License-Identifier: GPL-3.0-only
import {reportProgress} from './update-progress.js?v=progress1';
// The Windows SMK handoff must stay out of Chrome's MIDI process.
const loopback=()=>['127.0.0.1','localhost'].includes(location.hostname);
export const needsHostedSmkHelper=model=>model==='smk-pro'&&!loopback()&&/win/i.test(navigator.userAgentData?.platform||navigator.platform||'');
let apiBase='',token='',verifiedAction='';
// Protocol 1 restore is the fixed official Pro 016 operation, independent of
// the helper's advertised custom-firmware release. Explicit incompatible targets fail.
const stopped=message=>new Error(message+' The browser MIDI updater was not started.');
export async function hasNativeSmk(model,fetcher=fetch,action='install',applicationSha256){
 verifiedAction='';token='';
 if(!['install','restore','connect'].includes(action))throw stopped('Unsupported USB operation.');
 if(model!=='smk-pro')return false;
 const hosted=needsHostedSmkHelper(model),required=/win/i.test(navigator.userAgentData?.platform||navigator.platform||'');
 if(!loopback()&&!hosted)return false;
 apiBase=hosted?'http://127.0.0.1:8893':'';
 let response,result;
 try{response=await fetcher(apiBase+'/api/smk/capabilities',{cache:'no-store',signal:AbortSignal.timeout(20000)});}
 catch{throw stopped('Cannot reach the Felucca USB helper. Start it and allow local-network access if Chrome asks.');}
 if(!response.ok){
  if(!required)return false;
  throw stopped(response.status===403?'The Felucca USB helper has not allowed this site. Start it with --allow-hosted.':'The service on port 8893 did not provide Felucca USB capabilities.');
 }
 try{result=await response.json();}catch{throw stopped('The local USB service returned an invalid capability response.');}
 if(result.app!=='felucca-smk'||result.protocol!==1||typeof result.token!=='string'||result.token.length<32)
  throw stopped('The local service is not a compatible Felucca USB helper.');
 if(result.available!==true||!result.models?.includes(model)){
  if(!required)return false;
  throw stopped('The Felucca USB helper is running, but its USB dependencies or official firmware file are missing. Run its setup command.');
 }
 if(action==='install'&&applicationSha256===undefined){
  const selectedReply=await fetcher(new URL('./firmware/public-release.json',import.meta.url),{cache:'no-store'});
  if(!selectedReply.ok)throw stopped('The selected firmware manifest could not be loaded.');
  const selected=await selectedReply.json();applicationSha256=selected.sha256;
 }
 if(action==='install'&&(!/^[a-f0-9]{64}$/.test(applicationSha256||'')||result.application_sha256!==applicationSha256))
  throw stopped('The Felucca USB helper does not have the firmware selected on this page. Update the helper for this installation. Original-firmware restore remains available with compatible helpers.');
 if(action==='restore'&&result.restore_product!==undefined&&result.restore_product!=='SMK-37 Pro_016')
  throw stopped('This Felucca USB helper does not declare a compatible original Pro 016 restore package.');
 token=result.token;verifiedAction=action;return true;
}
export async function runNativeSmk(action,log=()=>{},fetcher=fetch){
 if(action!==verifiedAction||!token)throw stopped('Verify the selected USB operation before starting it.');
 async function request(url,options={}){
  try{return await fetcher(apiBase+url,{...options,signal:AbortSignal.timeout(10000)});}
  catch{throw Error('The local USB connection was lost. Keep the SMK powered; its operation may still be running. Check the local preview before retrying.');}
 }
 const response=await request('/api/smk/jobs',{method:'POST',headers:{'Content-Type':'application/json',...(token?{'X-Felucca-Token':token}:{})},body:JSON.stringify({action,model:'smk-pro',app:'felucca-smk'})});
 const created=await response.json();if(!response.ok||!created.id)throw Error(created.error||'The local USB installer could not start.');
 let logged=0;
 for(;;){
  const reply=await request('/api/smk/jobs/'+encodeURIComponent(created.id),{cache:'no-store'});
  if(!reply.ok)throw Error('USB status could not be read. Keep the SMK powered; check the connection before retrying.');
  const state=await reply.json();
  for(const line of state.logs.slice(logged))log(line);logged=state.logs.length;
  reportProgress({phase:state.phase,...(state.phase==='transfer'?{bytes:state.bytes||0,total:state.total||1}:{})});
  if(state.state==='error')throw Error(state.error);
  if(state.state==='done')return state.identity;
  await new Promise(resolve=>setTimeout(resolve,300));
 }
}

export const nativeModel=model=>model==='SMK-37 Pro'?'smk-pro':model;
