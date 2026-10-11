// SPDX-License-Identifier: GPL-3.0-only
// Build locally from the owner's official package. Vendor binaries are never
// fetched by this module or uploaded; the public download is our GPL app only.
import {Package,crc16,sha256hex} from './fm1pkg.js';
import {crc32,HEADER,LO,HI} from './smk37-update.js';
const profiles = Object.freeze({
  '2d98ce72530e71d617384963423820536f094501299e2cfec0a8d3c3fb6bcfb0': {model:'SMK-37 Pro',blocks:36,appBytes:619704},
  'b2ee38abfef3ae922e4eecdb8a551e14345300ae6f270265feacd33b6ef99cb6': {model:'SMK-37 Elite',blocks:20,appBytes:621320}
});
const u16=(b,o)=>b[o]|b[o+1]<<8;
const u32=(b,o)=>(b[o]|b[o+1]<<8|b[o+2]<<16|b[o+3]<<24)>>>0;
function enc(b){let key=65535;for(let i=0;i<b.length;i++){b[i]^=key&255;key=((key*2)^((key&32768)?0x1021:0))&65535;}return b;}
export function smkLogicalImage(bytes){
  if(bytes.length<960||bytes.length>32*1024*1024)throw Error('Invalid SMK firmware file size.');
  const probe=new Uint8Array(64);for(let i=0;i<64;i++)probe[i]=bytes[Math.floor(i/47)*48+i%47];enc(probe);
  if(crc16(probe.subarray(2))!==u16(probe,0))throw Error('SMK firmware header checksum failed.');
  const blocks=bytes.length-u32(probe,4);
  if(![20,36].includes(blocks))throw Error('Unsupported SMK firmware wrapper.');
  const raw=new Uint8Array(bytes.length-blocks);
  for(let i=0;i<blocks;i++)raw.set(bytes.subarray(i*48,i*48+47),i*47);
  raw.set(bytes.subarray(blocks*48),blocks*47);
  return {raw,blocks};
}
class SMKPackage extends Package {
  parseUfw(){
    this.container=this.raw.slice();
    const {raw,blocks}=smkLogicalImage(this.raw);this.blocks=blocks;this.skew=blocks;
    const h=raw.slice();enc(h.subarray(0,64));this.ufw=h;this.nent=u16(h,8);
    if(this.nent<1||this.nent>128||64+this.nent*80>h.length||crc16(h.subarray(64,64+this.nent*80))!==u16(h,2))throw Error('SMK package directory checksum failed.');
    this.entries=[];const ranges=[];
    for(let off=64;off<64+this.nent*80;off+=80){
      const e=enc(h.slice(off,off+80));const name=new TextDecoder().decode(e.subarray(64,80)).replace(/\0.*$/s,'');
      const item={off,type:u16(e,0),dcrc:u16(e,4),dataOff:u32(e,8),size:u32(e,12),name,plain:e};
      if(item.dataOff<64+this.nent*80||item.dataOff>raw.length||item.size>raw.length-item.dataOff||u32(e,16)<item.size)throw Error('Invalid SMK payload bounds.');
      if(ranges.some(([lo,hi])=>item.dataOff<hi&&item.dataOff+item.size>lo))throw Error('Overlapping SMK payloads.');
      ranges.push([item.dataOff,item.dataOff+item.size]);
      if([0,2,100,255].includes(item.type)&&crc16(raw.subarray(item.dataOff,item.dataOff+item.size))!==item.dcrc)throw Error('SMK payload checksum failed.');
      this.entries.push(item);
    }
    const matches=this.entries.filter(e=>e.type===0&&e.name==='flash.bin');
    if(matches.length!==1)throw Error('Expected one SMK flash payload.');
    // Pro's marker bytes extend into flash.bin: work in logical coordinates.
    this.raw=raw;
    this.flashEntry=matches[0];this.flashFileOff=this.flashEntry.dataOff;this.flashSize=this.flashEntry.size;
  }
  setProduct(name){
    if(name.length>this.blocks)throw Error('SMK product identity is too long.');
    for(let i=0;i<this.blocks;i++){const value=i<name.length?(name.charCodeAt(i)+i+1)&255:125;if(i<name.length&&value===125)throw Error('Invalid product identity.');this.container[i*48+47]=value;}
  }
  hdrFileOff(i){return i;}
  serialize(){
    const logical=super.serialize(),out=this.container.slice();
    for(let i=0;i<this.blocks;i++)out.set(logical.subarray(i*47,i*47+47),i*48);
    out.set(logical.subarray(this.blocks*47),this.blocks*48);
    return out;
  }
}
export async function buildSmkPublicPackage(stock,app,release){
  const profile=profiles[await sha256hex(stock)];
  if(!profile)throw Error('Choose the unmodified official SMK-37 Pro 016 or Elite 016 .fwsc file.');
  if(!release||release.public!==true||!/^SMK-37_9\d\d$/.test(release.product)||typeof release.version!=='string'||!/^Felucca SMK [0-9.]+ Beta$/.test(release.version)||release.version.length>31||await sha256hex(app)!==release.sha256||app.length!==release.bytes||app.length%4||app.length<4||u32(app,0)!==0x00808104)throw Error('Public SMK application verification failed.');
  const pkg=new SMKPackage(stock);
  if(pkg.blocks!==profile.blocks||pkg.flashSize!==HI||pkg.appBase!==LO||pkg.appOff!==0x4120||pkg.app.size!==profile.appBytes||pkg.chipKey!==0x980f||crc32(pkg.flash.subarray(0,LO))!==0x3a04ad23)throw Error('Unsupported SMK flash layout.');
  const before=pkg.flash.slice();pkg.setApp(app);
  // Only app contents and the directory/block checksum fields may change.
  const allowed=[[LO,LO+4],[pkg.app.hdrOff,pkg.app.hdrOff+4],[pkg.appOff,pkg.appOff+pkg.app.size]];
  for(let i=0;i<before.length;i++)if(before[i]!==pkg.flash[i]&&!allowed.some(([lo,hi])=>i>=lo&&i<hi))throw Error('Protected firmware bytes changed while packaging.');
  const factoryProduct=profile.model+'_'+release.product.split('_')[1];pkg.setProduct(factoryProduct);
  const factory=pkg.serialize(),image=smkLogicalImage(factory).raw;
  // Parse our serialized package again, including all container CRCs.
  const checked=new SMKPackage(factory);
  if(checked.flash.some((b,i)=>b!==pkg.flash[i]))throw Error('SMK package round-trip mismatch.');
  const bytes=new Uint8Array(HEADER+HI-LO),h=new DataView(bytes.buffer);
  bytes.set(new TextEncoder().encode('SMK37-SALT-USB1'));
  [2,LO,HI,HI-LO,crc32(before.subarray(0,LO)),0,crc32(pkg.flash.subarray(LO,HI))].forEach((v,i)=>h.setUint32(16+i*4,v,true));
  bytes.set(new TextEncoder().encode(release.product),48);bytes.set(new TextEncoder().encode(release.version),80);
  bytes.set(pkg.flash.subarray(LO,HI),HEADER);
  for(let i=0;i<(HI-LO)/4096;i++)h.setUint32(128+i*4,crc32(bytes.subarray(HEADER+i*4096,HEADER+(i+1)*4096)),true);
  h.setUint32(44,crc32(bytes.subarray(0,HEADER)),true);
  return {bytes,sha:await sha256hex(bytes),product:release.product,version:release.version,format:2,factory,image,factoryProduct,stockModel:profile.model};
}
