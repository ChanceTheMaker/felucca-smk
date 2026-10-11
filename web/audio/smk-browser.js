// SPDX-License-Identifier: GPL-3.0-only
import {BrowserSynth} from './browser.js';
const DB='felucca-smk-browser-v1', KEY='session';
async function dbAccess(write,value,revision){
  return new Promise((resolve,reject)=>{
    const r=indexedDB.open(DB,1);r.onupgradeneeded=()=>r.result.createObjectStore('state');r.onerror=()=>reject(r.error);
    r.onsuccess=()=>{const db=r.result,tx=db.transaction('state',write?'readwrite':'readonly'),s=tx.objectStore('state');
      const key=KEY+':'+revision;
      const q=write?s.put(value,key):s.get(key);let result;q.onsuccess=()=>result=q.result;
      tx.oncomplete=()=>{db.close();resolve(result);};tx.onerror=()=>{db.close();reject(tx.error);};};
  });
}
export class SMKBrowserSynth extends BrowserSynth {
  constructor(){super();this.requests=new Map();this.serial=0;this.listeners=new Set();this.saveTimer=null;}
  async start({waitForAudio=true}={}){
    if(this.stopping)await this.stopping;
    if(this.context){const resumed=this.context.resume();if(waitForAudio)await resumed;else resumed.catch(()=>{});return;}
    const context=new AudioContext({sampleRate:44100,latencyHint:'interactive'});this.context=context;
    try {
      const resumed=context.resume();if(waitForAudio)await resumed;else resumed.catch(()=>{});
      const manifest=await fetch(new URL('./smk-build.json',import.meta.url),{cache:'no-store'}).then(r=>{if(!r.ok)throw Error('SMK runtime unavailable');return r.json();});
      this.revision=manifest.sha256;
      const response=await fetch(new URL('./smk-engine.wasm?v='+this.revision,import.meta.url));if(!response.ok)throw Error('SMK runtime unavailable');
      const module=await WebAssembly.compile(await response.arrayBuffer());
      let snapshot;
      try{const saved=await dbAccess(false,undefined,this.revision);if(saved?.schema===1&&saved.revision===this.revision)snapshot=saved.buffer;
        this.storageMessage='Session saved automatically in this browser. Export a backup to keep it.';
      }catch{this.storageMessage='Browser storage unavailable. Export your session to keep it.';}
      await context.audioWorklet.addModule(new URL('./smk-worklet.js?v='+this.revision+'&startup=2',import.meta.url));
      this.node=new AudioWorkletNode(context,'smk-native',{numberOfInputs:0,numberOfOutputs:1,outputChannelCount:[2],processorOptions:{module,snapshot}});
      this.node.port.onmessage=({data:d})=>{
        if(d.id){const r=this.requests.get(d.id);if(r){clearTimeout(r.timer);this.requests.delete(d.id);d.error?r.reject(Error(d.error)):r.resolve(d);}}
        else if(d.type==='push')this.input?.onmidimessage?.({data:d.bytes});
        else if(d.type==='screen')for(const listener of this.listeners)listener(d);
        else if(d.type==='restore-error')this.storageMessage='Session could not be restored. Exported backups remain separate.';
      };
      this.gain=context.createGain();this.gain.gain.value=.35;this.node.connect(this.gain).connect(context.destination);
      this.analyser=context.createAnalyser();this.analyser.fftSize=2048;this.gain.connect(this.analyser);
      this.node.onprocessorerror=()=>{this.gain.gain.value=0;this.storageMessage='Browser audio stopped. Reload to restart.';};
      this.input={id:'smk-browser-in',name:'Felucca SMK Browser',state:'connected',type:'input',open:async()=>{}};
      const output={id:'smk-browser-out',name:'Felucca SMK Browser',state:'connected',type:'output',send:bytes=>{
        if(bytes[0]===0xf0){const cmd=bytes[4];this.request('command',{bytes:Array.from(bytes).slice(1,-1)}).then(d=>{if(d.bytes?.length)this.input.onmidimessage?.({data:d.bytes});}).catch(e=>console.error(e));
          if([3,7,8,9,12,13,14,18,19,20,21,27,28,30,31,33,35,36,38,64,69,73,79].includes(cmd))this.scheduleSave();
        }else this.midi(bytes);
      }};
      this.access={inputs:new Map([[this.input.id,this.input]]),outputs:new Map([[output.id,output]]),onstatechange:null};
    }catch(e){await context.close();this.context=null;throw e;}
  }
  request(type,data={}){return new Promise((resolve,reject)=>{const id=++this.serial,timer=setTimeout(()=>{this.requests.delete(id);reject(Error('SMK browser request timed out'));},4000);this.requests.set(id,{resolve,reject,timer});this.node.port.postMessage({type,id,...data});});}
  panel(kind,control,value){this.node?.port.postMessage({type:'panel',kind,control,value});this.scheduleSave();}
  visible(value){this.node?.port.postMessage({type:'visible',value});}
  release(){this.node?.port.postMessage({type:'release'});}
  midi(bytes){super.midi(bytes);this.scheduleSave();}
  sync(){} // Native editor commands already update the one authoritative engine.
  scheduleSave(){clearTimeout(this.saveTimer);this.saveTimer=setTimeout(()=>this.save().catch(()=>{}),1800);}
  async save(){if(!this.node)return;const {buffer}=await this.request('snapshot');
    try{await dbAccess(true,{schema:1,revision:this.revision,buffer},this.revision);this.storageMessage='Session saved in this browser.';}catch(e){this.storageMessage='Could not save in browser storage. Export your session.';throw e;}}
  async exportSession(){const {buffer}=await this.request('snapshot');return {format:'felucca-smk-browser',schema:1,revision:this.revision,data:btoa(Array.from(new Uint8Array(buffer),b=>String.fromCharCode(b)).join(''))};}
  async importSession(data){
    if(data?.format!=='felucca-smk-browser'||data.schema!==1||typeof data.data!=='string')throw Error('Not a Felucca SMK browser session');
    if(data.revision!==this.revision)throw Error('This session belongs to a different browser runtime version');
    if(data.data.length>1600000)throw Error('Session is too large');
    const buffer=Uint8Array.from(atob(data.data),c=>c.charCodeAt(0)).buffer;
    await this.request('restore',{buffer});await this.save();
  }
  async stop(){
    if(!this.context)return;
    if(this.stopping)return this.stopping;
    this.stopping=(async()=>{clearTimeout(this.saveTimer);this.release();try{await this.save();}catch{}await this.context.suspend();})();
    try{await this.stopping;}finally{this.stopping=null;}
  }
}
