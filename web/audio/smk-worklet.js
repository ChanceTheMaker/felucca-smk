// SPDX-License-Identifier: GPL-3.0-only
class SMKProcessor extends AudioWorkletProcessor {
  constructor(options) {
    super();
    this.e = new WebAssembly.Instance(options.processorOptions.module).exports;
    this.e.synth_init(); this.memory = new Uint8Array(this.e.memory.buffer);
    if (options.processorOptions.snapshot?.byteLength === this.e.fw_snapshot_size()) {
      this.memory.set(new Uint8Array(options.processorOptions.snapshot), this.e.fw_snapshot());
      if (this.e.fw_restore()) this.port.postMessage({type:'restore-error'});
    }
    // Settle the native voice setup before a gesture can deliver the first note.
    this.e.synth_render();
    this.samples = new Int32Array(this.e.memory.buffer);this.index=32;this.phase=1;
    this.a=[0,0];this.b=[0,0];this.frames=0;this.visible=false;
    this.port.onmessage = ({data:d}) => {
      try {
        if(d.type==='command') {
          if(d.bytes.length>2048)throw Error('Command too large');
          this.memory.set(d.bytes,this.e.fw_request());
          const n=this.e.fw_command(d.bytes.length);
          this.port.postMessage({id:d.id,bytes:this.memory.slice(this.e.fw_reply(),this.e.fw_reply()+n)});
        } else if(d.type==='midi') this.e.synth_midi(d.bytes[0],d.bytes[1]||0,d.bytes[2]||0);
        else if(d.type==='panel')this.e.fw_event(d.kind,d.control,d.value);
        else if(d.type==='release')this.e.fw_release();
        else if(d.type==='visible'){this.visible=d.value;if(this.visible)this.screen();}
        else if(d.type==='restore') {
          if(d.buffer?.byteLength!==this.e.fw_snapshot_size())throw Error('Invalid session size');
          const ptr=this.e.fw_snapshot(),backup=this.memory.slice(ptr,ptr+this.e.fw_snapshot_size());
          this.memory.set(new Uint8Array(d.buffer),ptr);
          if(this.e.fw_restore()){this.memory.set(backup,ptr);this.e.fw_restore();throw Error('Invalid session data');}
          this.port.postMessage({id:d.id,restored:true});
        }
        else if(d.type==='snapshot') {
          const p=this.e.fw_snapshot(),buffer=this.memory.slice(p,p+this.e.fw_snapshot_size()).buffer;
          this.port.postMessage({id:d.id,buffer},[buffer]);
        }
      } catch(e) {this.port.postMessage({id:d.id,error:e.message});}
    };
  }
  screen(){
    const p=this.e.fw_screen(),pixels=this.memory.slice(p,p+240*240*2).buffer;
    const status=Array.from(new Uint32Array(this.e.memory.buffer,this.e.fw_status(),16));
    this.port.postMessage({type:'screen',pixels,status},[pixels]);
  }
  next() {
    if(this.index===32){this.offset=this.e.synth_render()/4;this.index=0;}
    const p=this.offset+2*this.index++;
    this.b[0]=Math.max(-1,Math.min(1,this.samples[p]/32768));this.b[1]=Math.max(-1,Math.min(1,this.samples[p+1]/32768));
  }
  process(_,outputs) {
    const channels=outputs[0];if(!channels.length)return true;
    for(let i=0;i<channels[0].length;i++){
      while(this.phase>=1){this.a[0]=this.b[0];this.a[1]=this.b[1];this.next();this.phase--;}
      for(let c=0;c<channels.length;c++)channels[c][i]=this.a[c%2]+(this.b[c%2]-this.a[c%2])*this.phase;
      this.phase+=44100/sampleRate;
    }
    this.frames+=channels[0].length;
    if(this.frames>=sampleRate/12){
      this.frames=0;
      for(let i=0;i<16;i++){const n=this.e.fw_changed();if(!n)break;this.port.postMessage({type:'push',bytes:this.memory.slice(this.e.fw_reply(),this.e.fw_reply()+n)});}
      if(this.visible)this.screen();
    }
    return true;
  }
}
registerProcessor('smk-native',SMKProcessor);
