// SPDX-License-Identifier: GPL-3.0-only
// Own Web MIDI in a disposable document. Closing ports alone retains the
// document's MIDI session and Windows device-change subscription.
let current=null;
export async function releaseMidiSession(){
 const session=current;if(!session)return;current=null;
 const ports=[...(session.access?.inputs?.values()||[]),...(session.access?.outputs?.values()||[])];
 for(const port of ports){port.onmidimessage=null;port.onstatechange=null;try{void Promise.resolve(port.close()).catch(()=>{});}catch{}}
 if(session.access)session.access.onstatechange=null;
 session.frame.remove(); // Ends the owning ExecutionContext even if callers retain access.
 session.access=null;
}
export async function requestMidiSession(){
 await releaseMidiSession();
 const frame=document.createElement('iframe');frame.hidden=true;frame.title='USB MIDI session';frame.allow='midi';
 frame.dataset.midiSession='true';frame.src='about:blank';
 const loaded=new Promise(resolve=>frame.addEventListener('load',resolve,{once:true}));
 document.body.append(frame);
 const session={frame,access:null};current=session;let timer;
 try{
  const deadline=new Promise((_,reject)=>{timer=setTimeout(()=>reject(Error('MIDI permission is still pending. Allow the browser prompt, then retry.')),20000);});
  await Promise.race([loaded,deadline]);if(current!==session)throw Error('USB MIDI session was closed.');
  const nav=frame.contentWindow.navigator;
  if(!nav.requestMIDIAccess)throw Error('Use Chrome or Edge for USB MIDI.');
  session.access=await Promise.race([nav.requestMIDIAccess({sysex:true}),deadline]);
  if(current!==session)throw Error('USB MIDI session was closed. Reconnect to continue.');
  return session.access;
 }catch(error){if(current===session)await releaseMidiSession();throw error;}
 finally{clearTimeout(timer);}
}
if(typeof window!=='undefined')window.addEventListener('pagehide',()=>{void releaseMidiSession();});
