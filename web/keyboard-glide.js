// SPDX-License-Identifier: GPL-3.0-only
// Follow each captured pointer independently, including keys crossed between events.
export function createKeyboardGlide(hit, change, release) {
  const pointers=new Map();
  function visit(id,p,x,y){const key=hit(x,y);if(key!==p.key){p.key=key;change(id,key);}}
  function end(id){if(pointers.delete(id))release(id);}
  return {
    start(id,x,y){end(id);const p={x,y,key:null};pointers.set(id,p);visit(id,p,x,y);},
    move(id,x,y){const p=pointers.get(id);if(!p)return;
      const dx=x-p.x,dy=y-p.y,steps=Math.max(1,Math.ceil(Math.hypot(dx,dy)/4));
      for(let i=1;i<=steps;i++)visit(id,p,p.x+dx*i/steps,p.y+dy*i/steps);
      p.x=x;p.y=y;
    },
    end,
    clear(){for(const id of [...pointers.keys()])end(id);}
  };
}
