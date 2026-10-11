/* SPDX-License-Identifier: GPL-3.0-only
 * Parameter guides: normalized visualizations, not measured response plots. */
(() => {
  let graphs=[],pending=0;
  const ns='http://www.w3.org/2000/svg';
  function svg(tag,attrs){const e=document.createElementNS(ns,tag);for(const [k,v] of Object.entries(attrs))e.setAttribute(k,v);return e;}
  function mount(root){
    graphs=[];
    for(const bank of root.querySelectorAll('.section-bank')){
      const fields=[...bank.querySelectorAll('.row')].map(row=>({input:row.querySelector('input,select'),out:row.querySelector('.v')})).filter(x=>x.input);
      const labels=fields.map(f=>f.input.getAttribute('aria-label'));
      let type=labels.slice(0,4).join(',')==='ATK,DEC,SUS,REL'?'env':labels.includes('RATE')&&labels.includes('WAVE')?'lfo':labels.includes('CUT')&&labels.includes('RES')?'filter':null;
      if(!type)continue;
      const figure=document.createElement('figure');figure.className='studio-graph';
      const caption=document.createElement('figcaption'),title=document.createElement('span'),readout=document.createElement('span');
      title.textContent={env:'Envelope guide',lfo:'Waveform guide',filter:'Filter guide'}[type];caption.append(title,readout);
      const plot=svg('svg',{viewBox:'0 0 280 80',role:'img','aria-label':title.textContent});
      const grid=svg('path',{d:'M10 15H270M10 40H270M10 65H270M10 10V70M75 10V70M140 10V70M205 10V70M270 10V70',stroke:'currentColor','stroke-opacity':'.15',fill:'none'});
      const line=svg('path',{stroke:'currentColor','stroke-width':1.8,fill:'none'});plot.append(grid,line);figure.append(caption,plot);bank.append(figure);
      graphs.push({type,fields,labels,readout,line,plot});
    }
    refresh();
  }
  function refresh(){if(pending)return;pending=requestAnimationFrame(()=>{pending=0;for(const g of graphs){
    const norm=f=>Math.max(0,Math.min(1,(+f.input.value- +f.input.min)/(+f.input.max- +f.input.min||1)));
    const field=label=>g.fields[g.labels.indexOf(label)];let d='';
    if(g.type==='env'){
      const [a,b,s,r]=g.fields.slice(0,4).map(norm),total=.5+a+b+r;
      const x1=10+220*(.08+a)/total,x2=x1+220*(.08+b)/total,x3=Math.min(260,x2+220*.25/total),y=65-50*s;
      d=`M10 65L${x1} 15L${x2} ${y}H${x3}L270 65`;
      g.readout.textContent=g.fields.slice(0,4).map(f=>f.out.textContent).join(' / ');
    }else if(g.type==='filter'){
      const cut=norm(field('CUT')),res=norm(field('RES')),x=25+cut*210;
      d=`M10 30H${Math.max(10,x-18)}Q${x} ${30-20*res} ${x+8} 35L270 68`;
      g.readout.textContent=field('CUT').out.textContent;
    }else{
      const name=field('WAVE').out.textContent.toUpperCase(),rate=field('RATE').out.textContent;
      for(let i=0;i<=160;i++){const phase=i/80%1;let y;
        if(name.includes('TRI'))y=1-4*Math.abs(phase-.5);
        else if(name.includes('SAW'))y=2*phase-1;
        else if(name.includes('SQR')||name.includes('SQU'))y=phase<.5?1:-1;
        else if(name.includes('S&H')||name.includes('RND'))y=[.35,-.6,.8,-.2][Math.floor(i/20)%4];
        else y=Math.sin(i/80*Math.PI*2);
        d+=(i?'L':'M')+(10+i*260/160)+' '+(40-y*25);
      }
      g.readout.textContent=name+' · '+rate;
    }
    g.line.setAttribute('d',d);g.plot.setAttribute('aria-label',g.type+' guide: '+g.readout.textContent);
  }});}
  window.SMKStudioGraphs={mount,refresh};
})();
