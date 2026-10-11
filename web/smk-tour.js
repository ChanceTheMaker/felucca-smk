// SPDX-License-Identifier: GPL-3.0-only
export function createSMKTour(steps, replay) {
  const key='felucca.smk.simTutorial.v1';
  const bubble=document.createElement('div');bubble.className='sim-tour';bubble.hidden=true;
  bubble.setAttribute('role','dialog');bubble.setAttribute('aria-label','SMK simulator tutorial');
  bubble.innerHTML='<div class="sim-tour-heading"><span class="sim-tour-count"></span><button type="button" class="sim-tour-close" aria-label="Close tutorial" title="Close tutorial">×</button></div><h3></h3><p></p><div class="sim-tour-actions"><button type="button" data-back>Back</button><button type="button" data-next>Next</button></div>';
  document.body.append(bubble);
  const next=bubble.querySelector('[data-next]'),back=bubble.querySelector('[data-back]');
  let index=0,target=null,seen=false,frame=0;
  try{seen=localStorage.getItem(key)==='done';}catch{}
  function remember(){seen=true;try{localStorage.setItem(key,'done');}catch{}}
  function position(){
    if(bubble.hidden||!target)return;
    const r=target.getBoundingClientRect(),b=bubble.getBoundingClientRect(),pad=12;
    const width=innerWidth,height=innerHeight;
    const left=Math.max(pad,Math.min(width-b.width-pad,r.left+r.width/2-b.width/2));
    const below=r.bottom+b.height+18<height || r.top<b.height+18;
    const top=Math.max(pad,Math.min(height-b.height-pad,below?r.bottom+14:r.top-b.height-14));
    bubble.style.left=left+'px';bubble.style.top=top+'px';bubble.dataset.side=below?'below':'above';
    bubble.style.setProperty('--arrow-x',Math.max(20,Math.min(b.width-20,r.left+r.width/2-left))+'px');
  }
  function place(){cancelAnimationFrame(frame);frame=requestAnimationFrame(position);}
  function close(focus=true){if(bubble.hidden)return;remember();target?.classList.remove('sim-tour-target');bubble.hidden=true;target=null;if(focus)replay.focus({preventScroll:true});}
  function show(i){
    target?.classList.remove('sim-tour-target');index=i;
    const step=steps[i];target=step.target;
    bubble.querySelector('h3').textContent=step.title;bubble.querySelector('p').textContent=step.text;
    bubble.querySelector('.sim-tour-count').textContent=`${i+1} / ${steps.length}`;
    back.disabled=i===0;next.textContent=i===steps.length-1?'Done':'Next';
    bubble.hidden=false;target.classList.add('sim-tour-target');
    target.scrollIntoView({block:'center',inline:'nearest',behavior:'instant'});position();next.focus({preventScroll:true});
  }
  function start(){show(0);}
  back.onclick=()=>show(index-1);next.onclick=()=>index===steps.length-1?close():show(index+1);
  bubble.querySelector('.sim-tour-close').onclick=()=>close();
  document.addEventListener('keydown',e=>{if(!bubble.hidden&&e.key==='Escape'){e.preventDefault();close();}});
  window.addEventListener('resize',place);document.addEventListener('scroll',place,true);
  return {start,first(){if(!seen)start();},close};
}
