// SPDX-License-Identifier: GPL-3.0-only
// Browser panel for the native Felucca SMK input and LCD adapter.
import {createSMKTour} from './smk-tour.js';
import {createPanelGlide} from './panel-glide.js?v=1';
export function createSMKSimulator(host, synth, {connected=false}={}) {
  const el=(tag,cls,text)=>{const n=document.createElement(tag);if(cls)n.className=cls;if(text)n.textContent=text;return n;};
  const toolbar=el('div','sim-toolbar'),title=el('strong','','SMK-37 · Felucca SMK');
  const finishes=[['red','Pro Red','#b92832'],['white','Pro White','#e8e7e0'],['elite','Elite','linear-gradient(135deg,#687481 55%,#e76522 55%)'],['black','Pro Black','#16191b'],['pink','Pro Pink','#e5a2b4'],['charcoal','Pro Charcoal','#52575b']];
  const picker=el('div','sim-finish-picker'),finish=el('button','sim-icon-button sim-finish-toggle'),menu=el('div','sim-finish-menu');
  finish.type='button';finish.setAttribute('aria-haspopup','listbox');finish.setAttribute('aria-expanded','false');
  const currentSwatch=el('span','sim-swatch');currentSwatch.setAttribute('aria-hidden','true');finish.append(currentSwatch,el('span','sim-caret','⌄'));
  menu.id='smk-finish-options';menu.hidden=true;menu.setAttribute('role','listbox');menu.setAttribute('aria-label','Device finishes');finish.setAttribute('aria-controls',menu.id);picker.append(finish,menu);
  const expanded=el('button','sim-icon-button');expanded.type='button';
  function sizeIcon(on){expanded.innerHTML='<svg viewBox="0 0 24 24" aria-hidden="true" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round"><path d="'+(on?'M4 9h5V4M20 9h-5V4M4 15h5v5M20 15h-5v5':'M9 4H4v5M15 4h5v5M4 15v5h5M20 15v5h-5')+'"/></svg>';expanded.title=on?'Compact simulator':'Expand simulator';expanded.setAttribute('aria-label',expanded.title);expanded.setAttribute('aria-pressed',String(on));}
  sizeIcon(false);
  if(connected){expanded.title='Expand panel';expanded.setAttribute('aria-label',expanded.title);}
  expanded.onclick=()=>{const on=!host.classList.contains('sim-expanded');host.classList.toggle('sim-expanded',on);sizeIcon(on);if(connected){expanded.title=on?'Compact panel':'Expand panel';expanded.setAttribute('aria-label',expanded.title);}};
  const fullscreen=el('button','sim-icon-button sim-fullscreen-button');fullscreen.type='button';
  const fullscreenStatus=el('span','sim-fullscreen-status');fullscreenStatus.setAttribute('role','status');fullscreenStatus.hidden=true;
  function fullscreenIcon(){const on=document.fullscreenElement===host;fullscreen.title=on?'Exit fullscreen':'Enter fullscreen';fullscreen.setAttribute('aria-label',fullscreen.title);fullscreen.setAttribute('aria-pressed',String(on));fullscreen.innerHTML='<svg viewBox="0 0 24 24" aria-hidden="true" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="4" width="18" height="16" rx="2"/><path d="'+(on?'M8 8l8 8m0-8-8 8':'M7 12V8h4m2 8h4v-4M7 8l4 4m2 0 4 4')+'"/></svg>';}
  fullscreenIcon();fullscreen.hidden=!document.fullscreenEnabled;
  fullscreen.onclick=async()=>{fullscreenStatus.hidden=true;releaseAll();tour.close(false);closeFinishes();try{if(document.fullscreenElement===host)await document.exitFullscreen();else await host.requestFullscreen();}catch{fullscreenStatus.textContent='Fullscreen is unavailable in this browser window.';fullscreenStatus.hidden=false;}};
  document.addEventListener('fullscreenchange',()=>{fullscreenIcon();if(shown)releaseAll();});
  const help=el('button','sim-icon-button sim-help-button','?');help.type='button';help.title='How to play';help.setAttribute('aria-label','Start simulator tutorial');
  if(connected) {title.textContent='SMK37 · Connected device';menu.id=host.id+'-finish-options';finish.setAttribute('aria-controls',menu.id);help.setAttribute('aria-label','Connected SMK tutorial');}
  const screenOnly=el('button','sim-icon-button sim-screen-button');screenOnly.type='button';
  function screenIcon(on){screenOnly.title=on?'Full panel':'Screen only';screenOnly.setAttribute('aria-label',screenOnly.title);screenOnly.setAttribute('aria-pressed',String(on));screenOnly.innerHTML='<svg viewBox="0 0 24 24" aria-hidden="true" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="3" width="18" height="14" rx="2"/><path d="M9 21h6m-3-4v4"/>'+(on?'<path d="M7 7h4v6H7zm8 0h2m-2 3h2m-2 3h2"/>':'')+'</svg>';}
  screenIcon(false);
  screenOnly.onclick=()=>{releaseAll();tour.close(false);closeFinishes();const on=host.classList.toggle('sim-screen-only');screenIcon(on);expanded.hidden=on;};
  toolbar.append(title,picker,screenOnly,expanded,fullscreen,help,fullscreenStatus);
  const liveControls=el('div','sim-live-controls'),liveStatus=el('span','sim-live-status');
  if(connected){
    const stop=el('button','','Release browser notes');stop.type='button';stop.onclick=()=>releaseAll();
    liveStatus.setAttribute('role','status');liveControls.append(stop,liveStatus);
  }
  const shell=el('div','smk-simulator-shell');shell.dataset.finish='red';
  function closeFinishes(focus=false){menu.hidden=true;finish.setAttribute('aria-expanded','false');if(focus)finish.focus();}
  function chooseFinish(id,save=true){const f=finishes.find(f=>f[0]===id)||finishes[0];shell.dataset.finish=f[0];currentSwatch.style.background=f[2];finish.title='Device finish: '+f[1];finish.setAttribute('aria-label',finish.title);for(const b of menu.children)b.setAttribute('aria-selected',String(b.dataset.finish===f[0]));if(save)try{localStorage.setItem('felucca.smk.simFinish',f[0]);}catch{}}
  for(const [id,name,color] of finishes){const b=el('button','sim-finish-option');b.type='button';b.dataset.finish=id;b.setAttribute('role','option');const swatch=el('span','sim-swatch');swatch.style.background=color;swatch.setAttribute('aria-hidden','true');b.append(swatch,el('span','',name));b.onclick=()=>{chooseFinish(id);closeFinishes(true);};menu.append(b);}
  finish.onclick=()=>{const open=menu.hidden;menu.hidden=!open;finish.setAttribute('aria-expanded',String(open));if(open)menu.querySelector('[aria-selected=true]').focus();};
  picker.onkeydown=e=>{if(e.key==='Escape'){e.preventDefault();closeFinishes(true);}else if(!menu.hidden&&['ArrowDown','ArrowUp','Home','End'].includes(e.key)){e.preventDefault();const list=[...menu.children],i=list.indexOf(document.activeElement),n=e.key==='Home'?0:e.key==='End'?list.length-1:(i+(e.key==='ArrowDown'?1:-1)+list.length)%list.length;list[n].focus();}};
  document.addEventListener('pointerdown',e=>{if(!picker.contains(e.target))closeFinishes();});
  picker.addEventListener('focusout',e=>{if(!picker.contains(e.relatedTarget))closeFinishes();});
  let savedFinish='red';try{savedFinish=localStorage.getItem('felucca.smk.simFinish')||'red';}catch{}chooseFinish(savedFinish,false);
  const top=el('div','sim-top'),left=el('div','sim-left'),center=el('div','sim-center'),right=el('div','sim-right');
  const lcd=el('canvas','sim-lcd');lcd.width=lcd.height=240;lcd.setAttribute('role','img');lcd.setAttribute('aria-label','Live Felucca SMK firmware display');
  if(connected)lcd.setAttribute('aria-label','Live screen from your SMK37');
  const bezel=el('div','sim-bezel');bezel.append(lcd);left.append(bezel);
  const owners=new Map(),counts=new Map(),buttons=[],pads=[],keys=[],faders=[];let shown=false;
  const send=(kind,id,value)=>synth.panel(kind,id,value);
  function release(owner){const v=owners.get(owner);if(!v)return;owners.delete(owner);const n=counts.get(v.key)-1;if(n)counts.set(v.key,n);else{counts.delete(v.key);send(v.kind,v.id,0);}if(!connected)v.button?.setAttribute('aria-pressed',String(n>0));}
  function press(owner,kind,id,value,button){if(owners.has(owner))return;const key=kind+':'+id,n=counts.get(key)||0;counts.set(key,n+1);owners.set(owner,{key,kind,id,button});if(!n)send(kind,id,value);if(!connected)button?.setAttribute('aria-pressed','true');}
  const glide=createPanelGlide(host,press,release);
  function wire(button,kind,id,velocity=1){
    button.type='button';button.setAttribute('aria-pressed','false');
    glide.bind(button,kind,id,velocity);
    button.onkeydown=e=>{if(['Space','Enter'].includes(e.code)){e.preventDefault();if(!e.repeat)press('k'+e.code,kind,id,velocity,button);}};
    button.onkeyup=e=>{if(['Space','Enter'].includes(e.code)){e.preventDefault();release('k'+e.code);}};
    button.onblur=()=>{release('kSpace');release('kEnter');};
    button.onclick=e=>{if(!e.detail){const owner='a'+kind+':'+id;press(owner,kind,id,velocity,button);setTimeout(()=>release(owner),60);}};
  }
  const knobs=el('div','sim-knobs');
  const functions=['Volume','Tempo','Presets','Track','K1','K2','K3','K4'];
  functions.forEach((name,id)=>{
    const box=el('div','sim-encoder'),label=el('span','',`K${id+1}`),dial=el('button','sim-dial'),hint=el('small','',name);dial.type='button';dial.setAttribute('aria-label',`Knob ${id+1} (${name})`);
    // These are endless encoders: rotation shows the gesture, not a parameter
    // value. The connected instrument still supplies every value on the LCD.
    let angle=0,drag;const turn=n=>{if(dial.disabled)return;n=Math.max(-32,Math.min(32,n));send(1,id,n);angle+=n*8;dial.style.setProperty('--angle',angle+'deg');};
    dial.onpointerdown=e=>{if(e.button)return;e.preventDefault();dial.focus();dial.setPointerCapture(e.pointerId);drag={id:e.pointerId,y:e.clientY};};
    dial.onpointermove=e=>{if(drag?.id!==e.pointerId)return;const step=e.shiftKey?24:5,n=Math.trunc((drag.y-e.clientY)/step);if(n){turn(n);drag.y-=n*step;}};
    for(const n of ['pointerup','pointercancel','lostpointercapture'])dial.addEventListener(n,()=>drag=null);
    dial.onkeydown=e=>{const n={ArrowUp:1,ArrowRight:1,ArrowDown:-1,ArrowLeft:-1}[e.key];if(n){e.preventDefault();turn(n*(e.shiftKey?10:1));}};
    dial.addEventListener('wheel',e=>{if(document.activeElement!==dial)return;e.preventDefault();turn(e.deltaY<0?1:-1);},{passive:false});box.append(label,dial,hint);knobs.append(box);
  });
  const padGrid=el('div','sim-pads');for(let i=0;i<16;i++){const b=el('button','sim-pad',String(i+1));b.setAttribute('aria-label',`Pad ${i+1}`);b.dataset.pad=String(i);wire(b,3,i,100);pads.push(b);padGrid.append(b);}
  center.append(knobs,padGrid);
  const faderRow=el('div','sim-faders');for(let i=0;i<4;i++){const label=el('label','sim-fader',`F${i+1}`),f=el('input');f.type='range';f.min=0;f.max=1023;f.value=0;f.setAttribute('aria-label',`Fader ${i+1}`);f.oninput=()=>send(4,i,+f.value);label.append(f);faders.push(f);faderRow.append(label);}right.append(faderRow);
  const bankRow=el('div','sim-banks'),panel=el('div','sim-buttons');
  ['KNOB BANK','FADER BANK','◀','▶','OCT−','OCT+','ARP','NOTE REPEAT','SCALE','CHORD','GLO','BT / EDIT','PATCH / HOME','PARA / SAVE','FX','SEQ','PLAY / STOP','REC'].forEach((name,id)=>{
    const short={7:'REPEAT',11:'BT',12:'PATCH',13:'PARA',16:'PLAY'};
    const b=el('button','sim-button',short[id]||name);b.setAttribute('aria-label',name);b.title=name;wire(b,0,id);buttons.push(b);(id<6?bankRow:panel).append(b);
  });right.append(panel);top.append(left,center,right);
  const bottom=el('div','sim-bottom'),wheels=el('div','sim-wheels'),resetWheels=[],wheelViews=[];
  for(const [id,name,min,max,value] of [[0,'Pitch',-8192,8191,0],[1,'Mod',0,127,0]]){
    const label=el('label','sim-wheel',name),housing=el('span','sim-wheel-housing'),tread=el('span','sim-wheel-tread'),input=el('input');
    tread.setAttribute('aria-hidden','true');input.type='range';input.min=min;input.max=max;input.value=value;input.setAttribute('aria-label',name+' wheel');
    const paint=()=>{const position=(+input.value-min)/(max-min);housing.style.setProperty('--wheel-turn',(position*48)+'px');housing.style.setProperty('--wheel-position',(12+(1-position)*76)+'%');};
    input.oninput=()=>{paint();send(5,id,+input.value);};
    if(!id){const reset=()=>{input.value=0;paint();send(5,id,0);};input.addEventListener('pointerup',reset);input.addEventListener('pointercancel',reset);input.addEventListener('keyup',reset);input.addEventListener('blur',reset);}
    resetWheels.push(()=>{input.value=0;paint();});
    wheelViews.push({input,paint});
    paint();housing.append(tread,input);label.append(housing);wheels.append(label);
  }
  const pedal=el('button','sim-button','SUSTAIN');wire(pedal,6,0);wheels.append(pedal,bankRow);
  const piano=el('div','sim-piano');piano.setAttribute('aria-label','37-key SMK keyboard');
  const notes=['C','C♯','D','D♯','E','F','F♯','G','G♯','A','A♯','B'];let white=0;
  for(let i=0;i<37;i++){const n=i+48,black=[1,3,6,8,10].includes(n%12),b=el('button','sim-key '+(black?'black':'white'),notes[n%12]+(Math.floor(n/12)-1));b.setAttribute('aria-label',b.textContent);b.dataset.key=String(i);wire(b,2,i,100);b.style.setProperty('--key-x',String(black?white-.31:white++));keys.push(b);piano.append(b);}
  piano.style.setProperty('--white-count',white);top.prepend(wheels);bottom.append(piano);shell.append(top,bottom);host.replaceChildren(toolbar,shell);
  if(connected){
    host.classList.add('smk-connected-panel');toolbar.after(liveControls);
    host.append(el('p','sim-live-note','The SMK37 controls the screen, pages, banks and lights. Browser controls operate the same panel on your instrument.'));
    shell.querySelectorAll('button,input').forEach(c=>c.disabled=true);
  }
  const context=lcd.getContext('2d'),img=context.createImageData(240,240);
  const listener=({pixels,status:s,remote,tile,clear})=>{
    if(connected){
      if(!shown)return;
      if(clear){context.fillStyle='#080b10';context.fillRect(0,0,240,240);}
      if(tile)context.putImageData(new ImageData(tile.rgba,tile.width||240,4),tile.x||0,tile.row);
      if(!remote)return;
      const ready=!!remote.ready;
      liveStatus.textContent=ready?'USB connected · Device controls this view':remote.message;
      shell.querySelectorAll('button,input').forEach(c=>c.disabled=!ready);
      if(!ready){
        context.fillStyle='#080b10';context.fillRect(0,0,240,240);context.fillStyle='#e9eff6';context.font='13px sans-serif';context.textAlign='center';
        context.fillText('SMK37 device screen',120,100);context.font='11px sans-serif';context.fillText('See connection status above',120,126);
        [...buttons,...pads,...keys,pedal].forEach(b=>{b.classList.remove('lit');b.setAttribute('aria-pressed','false');});return;
      }
      lcd.setAttribute('aria-label',`Live SMK37 screen, track ${remote.track+1}, page ${remote.page}`);
      buttons.forEach((b,i)=>{
        const mask=i===16?3<<16:i===17?1<<18:1<<i;
        b.classList.toggle('lit',!!(remote.buttons&mask));
        b.style.setProperty('--device-led',i===16&&(remote.buttons&(1<<17))||i===17?'#ff4848':'#6bd993');
      });
      keys.forEach((b,i)=>{b.setAttribute('aria-pressed',String(!!(i<32?remote.keysLo&(1<<i):remote.keysHi&(1<<(i-32)))));const note=48+i+remote.octave*12;b.textContent=notes[((note%12)+12)%12]+(Math.floor(note/12)-1);b.setAttribute('aria-label',b.textContent);});
      const colors=['#ff4545','#5de478','#4d8dff'];
      pads.forEach((b,i)=>{b.setAttribute('aria-pressed',String(!!(remote.heldPads&(1<<i))));b.classList.toggle('lit',!!remote.ledLevel&&!!(remote.padLights&(1<<i)));b.style.setProperty('--device-led',colors[remote.padColor]||'#4d8dff');b.style.setProperty('--device-led-alpha',String(Math.min(remote.ledLevel,8)/8));});
      pedal.setAttribute('aria-pressed',String(!!remote.sustain));
      faders.forEach((f,i)=>{f.value=remote.faders[i];f.setAttribute('aria-label',remote.faderBank?['Attack','Decay','Sustain','Release'][i]:`Track ${i+1} level`);});
      wheelViews.forEach(({input,paint},i)=>{input.value=i?remote.mod:remote.pitch;paint();});
      const names=remote.bank?['Track','Engine','Presets','Page','K1','K2','K3','K4']:functions;
      [...knobs.children].forEach((box,i)=>{box.querySelector('small').textContent=names[i];box.querySelector('button').setAttribute('aria-label',`Knob ${i+1} (${names[i]})`);});
      return;
    }
    if(!shown)return;const p=new Uint8Array(pixels);for(let i=0,j=0;i<p.length;i+=2,j+=4){const v=p[i]*256+p[i+1];img.data[j]=(v>>11)*255/31;img.data[j+1]=((v>>5)&63)*255/63;img.data[j+2]=(v&31)*255/31;img.data[j+3]=255;}context.putImageData(img,0,0);
    buttons.forEach((b,i)=>b.classList.toggle('lit',!!(s[0]&(i===16?3<<16:i===17?1<<18:1<<i))));pads.forEach((b,i)=>b.classList.toggle('lit',!!(s[3]&(1<<i))));
    faders.forEach((f,i)=>{if(document.activeElement!==f)f.value=s[12+i];});
  };synth.listeners.add(listener);
  const typing=['KeyA','KeyW','KeyS','KeyE','KeyD','KeyF','KeyT','KeyG','KeyY','KeyH','KeyU','KeyJ','KeyK','KeyO','KeyL','KeyP','Semicolon'];
  document.addEventListener('keydown',e=>{if(!shown||e.ctrlKey||e.metaKey||e.altKey||e.target.closest('input,select,textarea,[contenteditable=true],.sim-tour,.sim-finish-picker'))return;const i=typing.indexOf(e.code);if(i>=0&&!e.repeat){e.preventDefault();press('q'+e.code,2,i,100,keys[i]);}});
  document.addEventListener('keyup',e=>release('q'+e.code));
  function releaseAll(){glide.clear();for(const owner of [...owners.keys()])release(owner);synth.release();}
  window.addEventListener('blur',()=>{if(shown)releaseAll();});document.addEventListener('visibilitychange',()=>{if(document.hidden&&shown)releaseAll();});
  const tour=createSMKTour(connected ? [
    {target:document.getElementById('audio-mode'),title:'Your SMK, two connected views',text:'Live Panel mirrors your physical SMK. Device Studio opens the detailed editor using the same USB connection. Simulator and Browser Studio make sound in the browser.'},
    {target:bezel,title:'Values from your instrument',text:'This view receives the screen drawn by your instrument. Pages, banks and settings follow the device. Screen colors use its UI palette; changes arrive over USB.'},
    {target:knobs,title:'Edit the connected sound',text:'Knobs and buttons operate the physical panel handlers, including master volume and transport. Changing a bank on either panel changes both.'},
    {target:piano,title:'Play over USB MIDI',text:'Play from either keyboard. Held keys and pad lights come from the SMK. Its octave, scale, pad bank, routing and LED settings apply to browser input too.'}
  ] : [
    {target:document.getElementById('audio-mode'),title:'One instrument, two browser views',text:'Simulator runs Felucca SMK in your browser. Browser Studio edits the same session. Live Panel and Device Studio connect to your physical keyboard.'},
    {target:bezel,title:'The actual firmware display',text:'This screen is drawn by the SMK firmware. Pages, parameter changes and menus behave like the instrument. USB/TRS ports and Bluetooth radio hardware are not simulated.'},
    {target:knobs,title:'Eight hands-on knobs',text:'Drag a knob up or down, or focus it and press the arrow keys. Bank 1 maps to Volume, Tempo, Presets, Track and K1–K4. KNOB BANK switches to the alternate assignments.'},
    {target:padGrid,title:'Sixteen playable pads',text:'Click or touch the pads to play. On step-editing pages, they enter steps. Choose the second pad bank in the firmware Settings menu.'},
    {target:faderRow,title:'Levels or an envelope',text:'The four faders control track levels. Press FADER BANK to switch them to attack, decay, sustain and release for the selected track.'},
    {target:piano,title:'Play the keyboard',text:'Click or touch the 37 keys, or type A W S E D F T G Y H U J K O L P ;. Use the pitch and mod wheels for expression. Hold SUSTAIN to keep notes ringing.'},
    {target:panel,title:'Tap a page. Hold for more.',text:'Tap PATCH for Home, PARA for Projects, or BT for editing. Hold GLO for navigation, PATCH for presets, PARA for undo, or BT for Settings. PLAY and REC control the sequencer.'},
    {target:document.getElementById('browser-session'),title:'Keep your music',text:'Your session saves in this browser, including virtual projects, presets and samples. Export session keeps a file backup for this runtime version. Use the swatch to change finish, the corner icon to expand or compact, and ? to replay this tour.'}
  ],help,connected?'felucca.smk.deviceTutorial.v1':'felucca.smk.simTutorial.v1');
  help.onclick=async()=>{releaseAll();if(host.classList.contains('sim-screen-only'))screenOnly.click();if(document.fullscreenElement===host)await document.exitFullscreen();tour.start();};
  return {show(value){const wasShown=shown;shown=value;host.hidden=!value;synth.visible(value);if(!value){releaseAll();tour.close(false);closeFinishes();}else if(!wasShown&&!connected)tour.first();},release:releaseAll};
}
