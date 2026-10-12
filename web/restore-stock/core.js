// SPDX-License-Identifier: GPL-3.0-only
import {Updater} from './transport.js?v=progress1';
import {validateStockPackage,sha256hex} from './fm1pkg.js';
import {buildSmkStock} from './package-smk.js';
const prepared=new WeakSet();
const START=[0xf0,0x22,0x24,0x35,0x7f,0xf7];
export const MODELS=['FM-1','SMK-37 Pro','SMK-37 Elite'];
export async function prepareStock(bytes,model){
  if(!MODELS.includes(model))throw Error('Choose an exact supported device model.');
  let pkg;
  if(model==='FM-1'){
    const checked=await validateStockPackage(bytes);
    pkg={family:'fm1',model,product:checked.product,version:'FM-1 official V15',image:checked.image,sha:await sha256hex(checked.image)};
  }else pkg=await buildSmkStock(bytes,model);
  Object.freeze(pkg);prepared.add(pkg);return pkg;
}
const smk=id=>['SMK-37','ota-SMK-37','Doom-SMK-37','ota-Doom-SMK-37'].includes(id.model);
const fm1=id=>['FM-1','ota-FM-1','Doom-FM-1','ota-Doom-FM-1'].includes(id.model);
function compatibleSmk(id){return smk(id)&&Number.isInteger(id.version)&&id.version>=(id.model.includes('Doom')?7:922);}
export async function restoreStock(access,pkg,log=()=>{},{confirmModel}={}){
  if(!prepared.has(pkg))throw Error('Prepare a checksum-verified official firmware file first.');
  // Snapshot the verified bytes before any await/device operation.
  const payload=(pkg.bytes||pkg.image).slice();
  if(await sha256hex(payload)!==pkg.sha)throw Error('Prepared firmware changed; prepare it again. Nothing was written.');
  const up=new Updater(access,log,/smk|fm-1|felucca|sloop|melodee|doom|ota|composite|sinco|usb-midi/i);
  let link;
  try{
    const accepts=id=>pkg.family==='smk'?(smk(id)||id.model===pkg.model):fm1(id);
    let dev=await up.findAny(accepts);
    if(!dev)throw Error('No matching updater replied. Connect only the selected device by normal USB and close other MIDI apps.');
    link=dev.link;
    if(dev.id.text===pkg.product){log('The selected official firmware is already running. No update needed.');return {alreadyStock:true,identity:dev.id.text};}
    if(pkg.family==='smk'&&!compatibleSmk(dev.id))throw Error('This SMK updater cannot receive the stock package. No update was started.');
    if(pkg.family==='fm1'&&dev.id.model.includes('Doom')&&(!Number.isInteger(dev.id.version)||dev.id.version<7))throw Error('This Doom FM-1 build has no compatible USB updater.');
    // Custom SMK identities do not distinguish Pro from Elite. The physical
    // model is an explicit user decision, never guessed from a port name.
    if(!confirmModel||!await confirmModel({model:pkg.model,identity:dev.id.text,version:pkg.version}))throw Error('Restore cancelled. No update was started.');
    if(!dev.id.model.startsWith('ota-')){
      const target=pkg.family==='smk'?'ota-'+dev.id.model:dev.id.model==='Doom-FM-1'?'ota-Doom-FM-1':'ota-FM-1';
      log('Entering update mode. Keep USB and power connected.');
      if(!link.send(START))throw Error('Device disconnected before update mode.');
      if(pkg.family==='fm1'&&dev.id.model==='FM-1'){
        const first=await up.serve(link,payload,0xe0000000,15000,()=>{});
        if(!first.finished)throw Error('FM-1 loader preparation stopped. No automatic retry was attempted.');
      }
      await link.close();link=null;
      dev=await up.waitForAny(id=>id.model===target,30000);
      if(!dev)throw Error('The expected RAM loader did not reply. Stop and inspect the device; no browser programming started.');
      link=dev.link;
    }
    if(pkg.family==='smk'&&!compatibleSmk(dev.id))throw Error('The RAM loader is not compatible. No programming started.');
    log('Restoring '+pkg.version+'. Do not disconnect or switch off.');
    if(!link.send(START))throw Error('Loader disconnected before programming.');
    const result=await up.serve(link,payload,0xf0000000,180000,n=>{if(n%64===0)log('Transfer request '+n);});
    if(!result.finished)throw Error('Restore interrupted. Keep power connected and save the log. No automatic retry was attempted.');
    await link.close();link=null;
    log('Transfer complete. Waiting for the official firmware identity...');
    const back=await up.waitForAny(accepts,40000);
    if(!back)throw Error('Transfer finished, but stock boot was not confirmed. Save the log and inspect the screen before another attempt.');
    link=back.link;
    if(back.id.text!==pkg.product)throw Error('Expected '+pkg.product+' but received '+back.id.text+'. Stock boot was not confirmed.');
    log('Restored '+pkg.version+'. Device confirmed '+back.id.text+'.');
    return {alreadyStock:false,identity:back.id.text};
  }finally{await link?.close();}
}
