// SPDX-License-Identifier: GPL-3.0-only
// No synth or local page/bank/channel state: the physical SMK owns the panel.
export function decodeRemoteState(a) {
  if(a.length!==46 || a[0]!==83 || a[1]!==77 || a[2]!==1 || a[3]!==0)throw Error('Invalid SMK panel state');
  let i=5;const uint=n=>{let v=0;for(let k=0;k<n;k++)v+=a[i++]*2**(k*7);return v;},value=()=>uint(2)-8192;
  const s={buttons:uint(3),keysLo:uint(5),keysHi:uint(1),heldPads:uint(3),padLights:uint(3),padColor:uint(1),ledLevel:uint(2),
    page:uint(1),track:uint(1),playing:uint(1),recording:uint(1),bank:uint(1),faderBank:uint(1),octave:value(),padBank:uint(1),master:value()};
  s.faders=Array.from({length:4},value);s.pitch=value();s.mod=uint(1);s.sustain=uint(1);return s;
}
export function decodeRemoteDirty(a) {
  if(a.length!==14||a[0]!==83||a[1]!==77||a[2]!==1||a[3]!==3||a[13]>15)throw Error('Invalid SMK changed areas');
  return Array.from({length:60},(_,i)=>i).filter(i=>a[5+Math.floor(i/7)]&(1<<(i%7)));
}
export function decodeRemoteTile(a,{x=0,width=240}={}) {
  if(!Number.isInteger(x)||!Number.isInteger(width)||x<0||width<2||x+width>240||(x&1)||(width&1))throw Error('Invalid SMK rectangle');
  const compact=a[3]===4;
  if(a[0]!==83||a[1]!==77||a[2]!==1||(!compact&&a[3]!==2)||a[5]>=60||a.length<(compact?55:54)||a.length>604)throw Error('Invalid SMK screen tile');
  let i=6;const encoding=compact?a[i++]:0;
  if(encoding>1)throw Error('Invalid SMK screen encoding');
  const palette=Array.from({length:16},()=>a[i++]|(a[i++]<<7)|(a[i++]<<14));
  const total=width*4,packedBytes=width*2,pixels=new Uint8Array(total);
  if(encoding===1){
    let j=0;
    while(i<a.length){const length=a[i++],color=a[i++];if(!length||length>127||color===undefined||color>15||j+length>total)throw Error('Invalid SMK screen run');pixels.fill(color,j,j+length);j+=length;}
    if(j!==total)throw Error('Incomplete SMK screen tile');
  }else{
    if(a.length-i!==packedBytes+Math.ceil(packedBytes/7))throw Error('Incomplete SMK screen tile');
    for(let j=0;j<packedBytes;){const mask=a[i++];for(let k=0;k<7&&j<packedBytes;k++,j++){const v=a[i++]|(((mask>>k)&1)<<7);pixels[j*2]=v&15;pixels[j*2+1]=v>>4;}}
  }
  const rgba=new Uint8ClampedArray(total*4);
  for(let j=0;j<total;j++){const v=palette[pixels[j]],p=j*4;rgba[p]=(v>>11)*255/31;rgba[p+1]=((v>>5)&63)*255/63;rgba[p+2]=(v&31)*255/31;rgba[p+3]=255;}
  return {row:a[5]*4,rgba,...(x||width!==240?{x,width}:{})};
}
export function decodeRemoteBatch(a) {
  const rect=a[3]===6;
  if(a.length<104||a.length>1530||a[0]!==83||a[1]!==77||a[2]!==1||(!rect&&a[3]!==5)||a[103]>(rect?60:16))throw Error('Invalid SMK screen batch');
  const state=decodeRemoteState([83,77,1,0,a[4],...a.slice(5,46)]);
  const pending=decodeRemoteDirty([83,77,1,3,a[4],...a.slice(94,103)]),tiles=[],seen=new Set();
  let i=104;
  for(let n=0;n<a[103];n++){
    const index=a[i++],x=rect?a[i++]*2:0,width=rect?a[i++]*2:240,encoding=a[i++],length=a[i++]|(a[i++]<<7);
    if(index>=60||seen.has(index)||!length||length>549||i+length>a.length)throw Error('Invalid SMK batch strip');
    seen.add(index);
    tiles.push(decodeRemoteTile([83,77,1,4,a[4],index,encoding,...a.slice(46,94),...a.slice(i,i+length)],{x,width}));i+=length;
  }
  if(i!==a.length||pending.some(index=>seen.has(index)))throw Error('Invalid SMK batch remainder');
  return {state,pending,tiles};
}
export function createSMKRemotePanel(api) {
  const profile=typeof location!=='undefined'&&new URLSearchParams(location.search).has('paneldebug');
  let profileAt=performance.now();const timings={};
  const listeners=new Set();let shown=false,timer,connection=null,seq=0,tile=0,nextState=0,running=false,failed=false,nextDirty=0;
  const dirty=new Set(),missing=new Set();
  const emit=e=>{for(const f of listeners)f(e);};
  const available=()=>{const c=api.connection();return c?.supported?c:null;};
  async function request(op,body=[],options={}){
    const c=available();if(!c)throw Error('SMK panel firmware is required');
    const args=[83,77,1,op,seq++&127,...body];
    const began=performance.now();
    const a=await api.request(args,{timeout:500,retries:0,quiet:true,...options});
    if(profile){
      const t=timings[op]||(timings[op]={count:0,ms:0,bytes:0,max:0});const elapsed=performance.now()-began;
      t.count++;t.ms+=elapsed;t.bytes+=a?.length||0;t.max=Math.max(t.max,elapsed);
      if(performance.now()-profileAt>2000){console.debug('SMK panel timing '+JSON.stringify(timings));for(const key of Object.keys(timings))delete timings[key];profileAt=performance.now();}
    }
    if(api.connection()?.id!==c.id)throw Error('Device changed');
    if(!a||!args.slice(0,5).every((v,i)=>a[i]===v))throw Error('Mismatched SMK panel reply');
    return a;
  }
  const events=[];let pumping=false,lastInput=-Infinity;
  async function pump(){
    if(pumping)return;pumping=true;
    try{
      while(events.length&&!failed){
        const {kind,id,value,owner}=events.shift();
        if(api.connection()?.id!==owner)continue;
        const v=value+8192;
        // Only this serial pump submits inputs, so priority cannot reverse
        // press/release order. Skip queued background work, never an active reply.
        const a=await request(1,[kind,id,v&127,(v>>7)&127],{front:true});
        if(a.length!==6||a[5])throw Error(a[5]===2?'Device input queue is busy':'Device rejected panel input');
        lastInput=performance.now();nextState=0;nextDirty=0;
      }
    }catch(error){fail(error);}finally{pumping=false;}
  }
  function panel(kind,id,value){
    const c=available();if(!c||failed)return;
    lastInput=performance.now();
    const event={kind,id,value:Math.round(value),owner:c.id},last=events.at(-1);
    // Coalesce continuous values only; press/release edges must remain FIFO.
    if((kind===4||kind===5)&&last?.kind===kind&&last.id===id)events[events.length-1]=event;
    else if(events.length<128)events.push(event);
    else {fail(Error('Too many pending panel inputs'));return;}
    void pump();
  }
  function fail(error){failed=true;events.length=0;emit({remote:{ready:false,message:error.message+' — reconnect to retry'}});}
  function release(){if(available()){events.length=0;panel(7,0,0);}}
  async function poll(){
    if(!shown||running)return;
    running=true;let transferred=false;
    try{
      const c=api.connection();
      if(c?.id!==connection){connection=c?.id;failed=false;tile=0;nextState=0;nextDirty=0;dirty.clear();missing.clear();for(let i=0;i<60;i++){dirty.add(i);missing.add(i);}emit({clear:true});}
      if(!c)emit({remote:{ready:false,message:'Connect your SMK37 above.'}});
      else if(!c.supported)emit({remote:{ready:false,message:'Device-mirror firmware required. Use Device Studio with this firmware.'}});
      else if(!failed&&!c.busy&&!document.hidden&&!pumping&&!events.length){
        // Drain a short batch at USB response speed instead of sleeping after
        // each of 60 strips. Still one background request in flight, with input
        // checks between replies and a browser event-loop yield after each batch.
        const started=performance.now();
        for(let count=0;count<8&&performance.now()-started<12;count++){
          if(!shown||failed||document.hidden||pumping||events.length||api.connection()?.id!==c.id||api.connection()?.busy)break;
          if(c.batch){
            if(dirty.size||(c.rect&&missing.size)||performance.now()>=nextDirty){
              const mask=Array(9).fill(0);for(const index of c.rect&&missing.size?missing:dirty)mask[Math.floor(index/7)]|=1<<(index%7);
              const result=decodeRemoteBatch(await request(c.rect?6:5,c.rect?[...mask,missing.size?1:0]:mask));
              for(const tile of result.tiles)if(!tile.x&&(!tile.width||tile.width===240))missing.delete(tile.row/4);
              dirty.clear();for(const index of result.pending)dirty.add(index);
              emit({remote:{ready:true,...result.state}});for(const tile of result.tiles)emit({tile});
              nextDirty=performance.now()+25;
            }else break;
          }else if(performance.now()>=nextState){
            const state=decodeRemoteState(await request(0));
            nextState=performance.now()+(performance.now()-lastInput<100?16:40);
            emit({remote:{ready:true,...state}});
          }else if(c.delta){
            if(dirty.size){
              const index=dirty.values().next().value;
              const result=decodeRemoteTile(await request(4,[index]));
              dirty.delete(index);emit({tile:result});
            }else if(performance.now()>=nextDirty){
              for(const index of decodeRemoteDirty(await request(3)))dirty.add(index);
              nextDirty=performance.now()+25;
            }else break;
          }else{
            const result=decodeRemoteTile(await request(2,[tile]));
            tile=(tile+1)%60;emit({tile:result});
          }
          transferred=true;
        }
      }
    }catch(error){if(shown)fail(error);}
    finally{running=false;if(shown)timer=setTimeout(poll,transferred?0:8);}
  }
  return {listeners,panel,release,refresh(){if(shown){nextState=0;void poll();}},
    visible(v){if(shown===v)return;shown=v;clearTimeout(timer);if(v)void poll();else release();},
  };
}
