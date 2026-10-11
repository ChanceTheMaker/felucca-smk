// SPDX-License-Identifier: GPL-3.0-only
// Read-only identity probe. Deliberately has no START, update, or flash commands.
import {unpack7} from './fm1ota.js';
const QUERY = Object.freeze([0xf0,0x00,0x32,0x45,0x00,0x00,0x00,0x40,0x7f,0xf7]);
export function midiInventory(access) {
  return [...access.inputs.values(), ...access.outputs.values()].map(p => ({
    type:p.type, name:p.name ?? '', manufacturer:p.manufacturer ?? '',
    state:p.state, connection:p.connection
  }));
}
export function identityEvidence(packet) {
  if (packet[0] !== 0xf0 || packet[packet.length-1] !== 0xf7) return null;
  const data=unpack7(Array.from(packet).slice(1,-1));
  if (data.length < 6 || data[0] !== 0 || data[1] !== 0x59 || data[2] !== 0x11) return null;
  const text=String.fromCharCode(...data.slice(6,33)).replace(/\0+$/, '');
  const match=/^([^_]+)_(\d+)$/.exec(text);
  return {text, model:match?.[1] ?? null, version:match ? Number(match[2]) : null,
    decodedBytes:data.length, packet:Array.from(packet).map(b=>b.toString(16).padStart(2,'0')).join(' ')};
}
export async function probeMidiIdentity(access, outputId, {log=()=>{}, timeoutMs=1200}={}) {
  const output=access.outputs.get(outputId);
  if (!output || output.state !== 'connected') throw Error('Select a connected MIDI output.');
  const inputs=[...access.inputs.values()].filter(p=>p.state === 'connected');
  if (!inputs.length) throw Error('No connected MIDI inputs.');
  const opened=[], listeners=[], replies=[];
  let ready=0;
  try {
    for (const input of inputs) {
      const wasOpen=input.connection === 'open';
      try { await input.open(); } catch(e) { log(`Input could not open: ${JSON.stringify(input.name)}: ${e.message}`); continue; }
      if (!wasOpen) opened.push(input);
      const receive=event=>{
        const evidence=identityEvidence(event.data);
        if (!evidence) return; // Never collect notes, controllers, or unrelated SysEx.
        const reply={input:input.name ?? '', output:output.name ?? '', ...evidence};
        replies.push(reply); log(`Identity reply: ${JSON.stringify(reply)}`);
      };
      input.addEventListener('midimessage',receive);
      listeners.push([input,receive]); ready++;
    }
    if (!ready) throw Error('No MIDI inputs could be opened. Close Studio, the installer and other MIDI apps.');
    const wasOpen=output.connection === 'open';
    await output.open(); if (!wasOpen) opened.push(output);
    log(`Sending identity query only to ${JSON.stringify(output.name)}; listening on ${ready} input(s).`);
    output.send([...QUERY]);
    await new Promise(resolve=>setTimeout(resolve,timeoutMs));
    if (!replies.length) log('No identity reply received. This does not establish the device model or loader state.');
    return replies;
  } finally {
    for (const [input,listener] of listeners) input.removeEventListener('midimessage',listener);
    await Promise.allSettled(opened.map(port=>port.close()));
  }
}
