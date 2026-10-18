// SPDX-License-Identifier: GPL-3.0-only
import {reportProgress} from './update-progress.js?v=progress1';
// The Windows SMK handoff must stay out of Chrome's MIDI process.
const loopback=()=>['127.0.0.1','localhost'].includes(location.hostname);
export const needsHostedSmkHelper=model=>model==='smk-pro'&&!loopback()&&/win/i.test(navigator.userAgentData?.platform||navigator.platform||'');
let apiBase='',token='';
export async function hasNativeSmk(model,fetcher=fetch){
 if(model!=='smk-pro')return false;
 const hosted=needsHostedSmkHelper(model),required=/win/i.test(navigator.userAgentData?.platform||navigator.platform||'');
 if(!loopback()&&!hosted)return false;
 apiBase=hosted?'http://127.0.0.1:8893':'';token='';
 try{
  const response=await fetcher(apiBase+'/api/smk/capabilities',{cache:'no-store',signal:AbortSignal.timeout(20000)});
  if(!response.ok){if(required)throw Error();return false;}
  const result=await response.json();
  if(required&&(result.app!=='felucca-smk'||result.firmware!=='SMK-37_928'||result.protocol!==1||typeof result.token!=='string'||result.token.length<32))throw Error();
  const available=result.available===true&&result.models?.includes(model);
  if(required&&!available)throw Error();
  token=result.token||'';return available;
 }catch{throw Error(hosted?'Start the Felucca 0.9.28 Windows USB helper (update older copies), then retry here. Allow local-network access if Chrome asks. The browser MIDI updater was not started.':'The local SMK USB service could not be reached. Keep the device powered and restart the local preview before retrying.');}
}
export async function runNativeSmk(action,log=()=>{},fetcher=fetch){
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
