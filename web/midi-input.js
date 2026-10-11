// SPDX-License-Identifier: GPL-3.0-only
// Performance input for browser audio only. Never opens a MIDI output or requests SysEx.
export class MIDIInput {
  constructor({send,enabled,changed=()=>{},requestAccess=()=>navigator.requestMIDIAccess({sysex:false})}) {
    Object.assign(this,{send,enabled,changed,requestAccess});
    this.notes=new Set();this.channels=new Set();
    this.receive=e=>this.message(e.data);
    this.refresh=()=>{if(this.port?.state==='disconnected')this.select('');this.changed([...this.access.inputs.values()].filter(p=>p.state==='connected'));};
  }
  async connect(){if(!this.access){this.access=await this.requestAccess();this.access.addEventListener('statechange',this.refresh);}this.refresh();}
  select(id){this.release();this.port?.removeEventListener('midimessage',this.receive);this.port=this.access?.inputs.get(id)||null;if(this.port?.state==='disconnected')this.port=null;this.port?.addEventListener('midimessage',this.receive);}
  message(data){
    if(!this.enabled()||!data?.length)return;
    const [status,a,b=0]=data,kind=status&240,ch=status&15;
    if(!Number.isInteger(status)||status<128||status>=240)return;
    const n=kind===192||kind===208?2:3;
    if(data.length!==n||!Array.from(data).slice(1).every(v=>Number.isInteger(v)&&v>=0&&v<128))return;
    this.channels.add(ch);const key=ch*128+a;
    if(kind===144&&b)this.notes.add(key);
    else if(kind===128||kind===144){if(!this.notes.delete(key))return;}
    // Sustain, expression and bend go to the native firmware engine unchanged.
    this.send(Array.from(data));
  }
  release(){
    for(const key of this.notes)this.send([128|(key>>7),key&127,0]);
    for(const ch of this.channels){this.send([176|ch,64,0]);this.send([224|ch,0,64]);}
    this.notes.clear();this.channels.clear();
  }
  dispose(){this.select('');this.access?.removeEventListener('statechange',this.refresh);}
}

export function mountMIDIInput({container,getSynth,active,enableAudio}) {
  const row=document.createElement('div');row.className='bar browser-midi-input';
  row.innerHTML='<button type="button">Enable USB MIDI input</button><label>MIDI input <select aria-label="Browser MIDI input"><option value="">None</option></select></label><span role="status"></span>';
  container.append(row);
  const button=row.querySelector('button'),select=row.querySelector('select'),status=row.querySelector('[role=status]');
  const controller=new MIDIInput({send:bytes=>getSynth()?.midi(bytes),enabled:()=>active()&&getSynth()?.context?.state==='running',changed:ports=>{
    const selected=controller.port?.id||'';select.replaceChildren(new Option('None',''),...ports.map(p=>new Option(p.name||'MIDI input',p.id)));select.value=selected;
    status.textContent=selected?'MIDI input ready':ports.length?'Choose your controller':'No MIDI inputs connected';
  }});
  button.onclick=async()=>{button.disabled=true;try{await enableAudio();await controller.connect();button.textContent='Refresh MIDI inputs';}catch(e){status.textContent=e.message;}finally{button.disabled=false;}};
  select.onchange=()=>{controller.select(select.value);status.textContent=select.value?'MIDI input ready':'MIDI input off';};
  if(!navigator.requestMIDIAccess){button.disabled=true;status.textContent='USB MIDI input requires a browser with Web MIDI, such as Chrome or Edge.';}
  let wasActive=false;
  const timer=setInterval(()=>{const now=active()&&getSynth()?.context?.state==='running';if(wasActive&&!now)controller.release();wasActive=now;},250);
  window.addEventListener('pagehide',()=>{clearInterval(timer);controller.dispose();},{once:true});
  return controller;
}
