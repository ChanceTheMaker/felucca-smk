// SPDX-License-Identifier: GPL-3.0-only
import {Updater} from './fm1ota.js?v=smk-flow-r1';
export const HEADER=1024, LO=0x4000, HI=0x9d000;
export function crc32(data){let c=0xffffffff;for(const b of data){c^=b;for(let i=0;i<8;i++)c=(c>>>1)^((0-(c&1))&0xedb88320);}return (c^0xffffffff)>>>0;}
export async function parsePackage(bytes,expectedSha){
  if(bytes.length!==HEADER+HI-LO)throw Error('Wrong SMK package size. FM-1 packages and full flash dumps are not accepted.');
  const h=bytes.slice(0,HEADER),v=new DataView(h.buffer),str=(a,b)=>new TextDecoder().decode(h.slice(a,b)).replace(/\0+$/,'');
  const format=v.getUint32(16,true);
  if(str(0,16)!=='SMK37-SALT-USB1'||![1,2].includes(format)||v.getUint32(20,true)!==LO||v.getUint32(24,true)!==HI||v.getUint32(28,true)!==HI-LO)throw Error('Not a supported SMK-37 update package.');
  if(format===2&&(v.getUint32(32,true)!==0x3a04ad23||v.getUint32(36,true)!==0))throw Error('Unsupported portable SMK profile.');
  const saved=v.getUint32(44,true);v.setUint32(44,0,true);
  if(crc32(h)!==saved)throw Error('Package header checksum failed.');
  if(crc32(bytes.subarray(HEADER))!==v.getUint32(40,true))throw Error('Package data checksum failed.');
  for(let s=0;s<(HI-LO)/4096;s++)if(crc32(bytes.subarray(HEADER+s*4096,HEADER+(s+1)*4096))!==v.getUint32(128+s*4,true))throw Error('Package sector checksum failed.');
  const product=str(48,80),version=str(80,112);
  if(!/^SMK-37_\d{3}$/.test(product)||!version)throw Error('Invalid SMK identity.');
  const sha=[...new Uint8Array(await crypto.subtle.digest('SHA-256',bytes))].map(b=>b.toString(16).padStart(2,'0')).join('');
  if(!expectedSha||sha!==expectedSha)throw Error('This package is not the reviewed candidate for this local installer.');
  return {bytes,product,version,sha,format};
}
const SMK_PORT=/smk|felucca|sloop|melodee|ota|composite|sinco|usb-midi/i;
const START=[0xf0,0x22,0x24,0x35,0x7f,0xf7];
export async function installSmk(access,pkg,log=()=>{},{device}={}){
  const up=new Updater(access,log,SMK_PORT);let link;
  try {
    // Exact model filter; the shared transport never sends START to FM-1.
    let dev=device||await up.findAny(id=>id.model==='ota-SMK-37'||id.model==='SMK-37');
    if(!dev)throw Error('No updater-enabled SMK replied. Close other MIDI apps, or choose Automatic install / resume for official 016 firmware.');
    link=dev.link;
    if(pkg.format===2 && (!Number.isInteger(dev.id.version)||dev.id.version<922))throw Error('This older private updater cannot accept the portable package. It needs a separate migration; no update was started.');
    if(dev.id.model==='SMK-37'){
      log('Entering SMK update mode. No flash has been written.');
      if(!link.send(START))throw Error('SMK disconnected before update mode.');
      await link.close();link=null;
      dev=await up.waitForAny(id=>id.model==='ota-SMK-37',30000);
      if(!dev)throw Error('SMK RAM loader did not appear. No browser programming started.');
      link=dev.link;
    }
    log('Checking package and device; then programming application only. Keep power and USB connected.');
    if(!link.send(START))throw Error('Loader disconnected.');
    const r=await up.serve(link,pkg.bytes,0xf0000000,20000,(n)=>{if(n%64===0)log(`Transfer request ${n}`);});
    if(!r.finished)throw Error('Update stopped. Keep SMK powered on; retry while its loader is connected. If power was lost during programming, use the recovery adapter.');
    await link.close();link=null;log('Application readback verified. Waiting for restart...');
    const back=await up.waitForAny(id=>id.model==='SMK-37',40000);
    if(!back)throw Error('Programming verified, but normal USB identity did not return. Inspect the device before taking further action.');
    link=back.link;
    if(back.id.text!==pkg.product)throw Error(`Unexpected returned version: ${back.id.text}`);
    log(`Installed ${pkg.version}. Device returned as ${back.id.text}.`);
  } finally {await link?.close();}
}

// Stock installation and an already-running official loader use the same flow.
// A generic identity never proves the model; require an explicit decision.
async function acceptLoader(dev,model,confirmGeneric,log){
  if(dev.id.model==='ota-'+model)return dev;
  try{
    if(dev.id.model!=='ota-FM-1'||model!=='SMK-37 Elite')throw Error('Loader identity does not match the selected SMK model. No programming started.');
    log('The loader reports '+dev.id.text+'. Confirm the connected keyboard before continuing.');
    if(!confirmGeneric||!await confirmGeneric({identity:dev.id.text,port:dev.name,model}))
      throw Error('Installation paused. No application programming started. Keep the Elite powered and use Install / resume when ready.');
    log('User confirmed the generic loader belongs to the connected '+model+'.');
    return dev;
  }catch(e){await dev.link.close();throw e;}
}
export async function installStockSmk(access,pkg,log=()=>{},{confirmGeneric}={}){
  if(!pkg?.image||!['SMK-37 Pro','SMK-37 Elite'].includes(pkg.stockModel)||pkg.format!==2)throw Error('A verified SMK stock-derived package is required.');
  const up=new Updater(access,log,SMK_PORT);let link;
  const loader=id=>id.model==='ota-'+pkg.stockModel||id.model==='ota-FM-1';
  const portable=id=>id.model==='SMK-37'||id.model==='ota-SMK-37';
  try{
    log('Looking for '+pkg.stockModel+' or its active loader...');
    let dev=await up.findAny(id=>portable(id)||loader(id)||(id.model===pkg.stockModel&&id.version===16));
    if(!dev)throw Error('No matching SMK replied. Connect only '+pkg.stockModel+' running official 016 firmware or its active loader. Close other MIDI apps, then use Install / resume.');
    if(portable(dev.id)){
      log('Found '+dev.id.text+' on '+dev.name+'. Using its existing SMK USB updater.');
      return await installSmk(access,pkg,log,{device:dev});
    }
    let ota;
    if(loader(dev.id)){
      ota=await acceptLoader(dev,pkg.stockModel,confirmGeneric,log);
      log('Official loader already active. Resuming without repeating loader preparation.');
    }else{
      link=dev.link;
      log('Found '+dev.id.text+'. Preparing its official loader. Keep USB and power connected.');
      if(!link.send(START))throw Error('SMK disconnected before update mode.');
      const first=await up.serve(link,pkg.image,0xe0000000,15000,n=>{if(n%64===0)log('Preparing loader: '+n);});
      if(!first.finished)throw Error('Loader preparation did not complete. Save the log and inspect the device before retrying.');
      await link.close();link=null;
      log('Loader prepared. Reconnecting automatically; the screen may stay black...');
      ota=await up.waitForAny(loader,30000);
      if(!ota)throw Error('The loader has not replied yet. Keep the SMK powered, close other MIDI apps, then click Install / resume on this page. Do not repeat loader preparation manually.');
      ota=await acceptLoader(ota,pkg.stockModel,confirmGeneric,log);
    }
    link=ota.link;
    log('Official loader connected. Installing '+pkg.version+'; keep power and USB connected.');
    if(!link.send(START))throw Error('SMK loader disconnected before programming.');
    const result=await up.serve(link,pkg.image,0xf0000000,180000,n=>{if(n%64===0)log('Installing: '+n+' transfer requests');});
    if(!result.finished)throw Error('Installation did not finish. Keep the SMK powered and save the log. No automatic write retry was attempted.');
    await link.close();link=null;
    log('Loader transfer complete. Waiting for Sloop to restart...');
    const back=await up.waitForAny(id=>id.model==='SMK-37',40000);
    if(!back)throw Error('The loader completed, but Felucca SMK did not return. Save the log and inspect the device.');
    link=back.link;
    if(back.id.text!==pkg.product)throw Error('Expected '+pkg.product+'; received '+back.id.text+'.');
    log('Installed '+pkg.version+'. Device returned as '+back.id.text+'.');
  }finally{await link?.close();}
}
