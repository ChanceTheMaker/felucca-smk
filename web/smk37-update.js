// SPDX-License-Identifier: GPL-3.0-only
import {Updater} from './fm1ota.js';
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
const SMK_PORT=/smk|felucca|ota|composite|sinco|usb-midi/i;
const START=[0xf0,0x22,0x24,0x35,0x7f,0xf7];
export async function installSmk(access,pkg,log=()=>{}){
  const up=new Updater(access,log,SMK_PORT);let link;
  try {
    // Exact model filter; the shared transport never sends START to FM-1.
    let dev=await up.find(id=>id.model==='ota-SMK-37'||id.model==='SMK-37');
    if(!dev)throw Error('Updater-enabled SMK not found. Test10 and stock firmware need the one-time adapter installation first. Close Studio and other MIDI apps.');
    link=dev.link;
    if(pkg.format===2 && (!Number.isInteger(dev.id.version)||dev.id.version<922))throw Error('This older private updater cannot accept the portable package. It needs a separate migration; no update was started.');
    if(dev.id.model==='SMK-37'){
      log('Entering SMK update mode. No flash has been written.');
      if(!link.send(START))throw Error('SMK disconnected before update mode.');
      link.close();link=null;
      dev=await up.waitFor(id=>id.model==='ota-SMK-37',30000);
      if(!dev)throw Error('SMK RAM loader did not appear. No browser programming started.');
      link=dev.link;
    }
    log('Checking package and device; then programming application only. Keep power and USB connected.');
    if(!link.send(START))throw Error('Loader disconnected.');
    const r=await up.serve(link,pkg.bytes,0xf0000000,20000,(n)=>{if(n%64===0)log(`Transfer request ${n}`);});
    if(!r.finished)throw Error('Update stopped. Keep SMK powered on; retry while its loader is connected. If power was lost during programming, use the recovery adapter.');
    link.close();link=null;log('Application readback verified. Waiting for restart...');
    const back=await up.waitFor(id=>id.model==='SMK-37',40000);
    if(!back)throw Error('Programming verified, but normal USB identity did not return. Inspect the device before taking further action.');
    link=back.link;
    if(back.id.text!==pkg.product)throw Error(`Unexpected returned version: ${back.id.text}`);
    log(`Installed ${pkg.version}. Device returned as ${back.id.text}.`);
  } finally {link?.close();}
}

// Stock first-install uses only the official 016 loader from the owner's
// hash-checked file. This is separate from the bounded Felucca RAM loader.
export async function installStockSmk(access,pkg,log=()=>{}){
  if(!pkg?.image||!['SMK-37 Pro','SMK-37 Elite'].includes(pkg.stockModel)||pkg.format!==2)throw Error('A verified SMK stock-derived package is required.');
  const up=new Updater(access,log,SMK_PORT);let link;
  try{
    const dev=await up.find(id=>id.model===pkg.stockModel&&id.version===16);
    if(!dev)throw Error(`Connect ${pkg.stockModel} running official 016 firmware. Close Studio and other MIDI apps.`);
    link=dev.link;
    log(`Found ${dev.id.text}. Loading its official update loader. Keep USB and power connected.`);
    if(!link.send(START))throw Error('SMK disconnected before update mode.');
    const first=await up.serve(link,pkg.image,0xe0000000,15000,n=>{if(n%64===0)log(`Preparing loader: ${n}`);});
    if(!first.finished)throw Error('The stock firmware did not complete loader preparation. Stop and inspect the device before retrying.');
    link.close();link=null;
    const ota=await up.waitFor(id=>id.model==='ota-'+pkg.stockModel,30000);
    if(!ota)throw Error('The official SMK loader did not appear. Stop and inspect the device.');
    link=ota.link;
    log('Official loader connected. Installing Felucca SMK beta; keep power and USB connected.');
    if(!link.send(START))throw Error('SMK loader disconnected.');
    const result=await up.serve(link,pkg.image,0xf0000000,180000,n=>{if(n%64===0)log(`Installing: ${n} transfer requests`);});
    if(!result.finished)throw Error('Installation did not finish. Keep the SMK powered on and save this log before attempting recovery.');
    link.close();link=null;
    const back=await up.waitFor(id=>id.model==='SMK-37',40000);
    if(!back)throw Error('The loader completed, but Felucca SMK did not return. Inspect the device.');
    link=back.link;
    if(back.id.text!==pkg.product)throw Error(`Expected ${pkg.product}; received ${back.id.text}.`);
    log(`Installed ${pkg.version}. Device returned as ${back.id.text}.`);
  }finally{link?.close();}
}
