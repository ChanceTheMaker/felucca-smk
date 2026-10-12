// SPDX-License-Identifier: GPL-3.0-only
// One captured gesture may cross any number of keys or drum pads.
import {createKeyboardGlide} from './keyboard-glide.js';
export function createPanelGlide(host, press, release) {
  const doc=host.ownerDocument;
  const glide=createKeyboardGlide((x,y)=>{
    const target=doc.elementFromPoint(x,y)?.closest('.sim-key,.sim-pad');
    return target&&host.contains(target)?target:null;
  },(pointer,target)=>{
    release('p'+pointer);
    if(target)press('p'+pointer,target.classList.contains('sim-pad')?3:2,
      +(target.dataset.pad??target.dataset.key),100,target);
  },pointer=>release('p'+pointer));
  return {
    bind(button,kind,id,velocity=1){
      const playable=kind===2||kind===3;
      button.addEventListener('pointerdown',e=>{
        if(e.pointerType==='mouse'&&e.button!==0)return;
        e.preventDefault();button.focus({preventScroll:true});button.setPointerCapture(e.pointerId);
        if(playable)glide.start(e.pointerId,e.clientX,e.clientY);
        else press('p'+e.pointerId,kind,id,velocity,button);
      });
      if(playable)button.addEventListener('pointermove',e=>{
        if(e.pointerType==='mouse'&&!(e.buttons&1)){glide.end(e.pointerId);return;}
        glide.move(e.pointerId,e.clientX,e.clientY);
      });
      for(const name of ['pointerup','pointercancel','lostpointercapture'])
        button.addEventListener(name,e=>{if(playable)glide.end(e.pointerId);else release('p'+e.pointerId);});
    },
    clear(){glide.clear();}
  };
}
