// SPDX-License-Identifier: GPL-3.0-only
const reasons=['','Package header transfer failed','Package header or device profile rejected',
  'Protected boot or calibration validation failed','Package sector preflight failed','Whole-package checksum failed',
  'Package changed or transfer failed during programming','Application sector read failed','Flash sector erase failed',
  'Flash page programming failed','Flash readback failed','Flash sector verification failed','Application verification failed',
  'Final acknowledgement failed','Unsupported flash ID','Protected boot or calibration changed during verification'];
export function parseSmkStatus(packet){
  if(!packet||packet[0]!==0xf0||packet[packet.length-1]!==0xf7)return null;
  const raw=[];let acc=0,bits=0;
  for(const value of packet.slice(1,-1)){
    if(value>127)return null;acc|=value<<bits;bits+=7;
    if(bits>=8){raw.push(acc&255);acc>>>=8;bits-=8;}
  }
  const prefix=[0,0x59,0x7e,12,0,0,83,77,75,85,1];
  if(raw.length!==19||prefix.some((v,i)=>raw[i]!==v)||raw[13]!==0||raw[12]>1||raw[11]<1||raw[11]>15||
     (raw.slice(6).reduce((a,b)=>a+b,0)&255)!==255)return null;
  return {code:raw[11],writing:!!raw[12],flashId:(raw[14]|raw[15]<<8|raw[16]<<16|raw[17]<<24)>>>0};
}
export function smkStatusError(status){
  const action=status.writing?'Programming may have started. Keep power and USB connected; save this error. No automatic retry was made.':
    'Stopped before any application erase or program. Save this error; do not bypass the device checks.';
  return new Error(`SMK loader error ${status.code}: ${reasons[status.code]}. Flash ID ${status.flashId.toString(16).padStart(6,'0').toUpperCase()}. ${action}`);
}
