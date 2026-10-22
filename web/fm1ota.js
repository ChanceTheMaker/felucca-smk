import {parseSmkStatus, smkStatusError} from './smk-diagnostics.js';
import {createTransferCounter,reportProgress} from './update-progress.js?v=progress1';
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
//
// The FM-1 USB-MIDI update protocol over WebMIDI (after tools/fm1_install.py).
// Step 1: the running firmware (official
// or Felucca) reads parts of the package and stages the update loader. Step 2:
// the loader reads the whole image and writes it. Both steps are "the device
// asks, we answer": cmd 0x30 read requests on the logical image.

const HS_QUERY = [0xF0, 0x00, 0x32, 0x45, 0x00, 0x00, 0x00, 0x40, 0x7F, 0xF7];
const UPGRADE = [0xF0, 0x22, 0x24, 0x35, 0x7F, 0xF7];
const MAXDATA = 512;
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const IS_OTA = (id) => /^ota-/i.test(id.model);

// errors carry a code the pages translate: notfound, model, stopped, noloader, lost, noreturn,
// mismatch (detail: the identity the device reports), badreq
function fail(code, msg, detail = "") { const e = new Error(msg); e.code = code; e.detail = detail; return e; }

export function pack7(data) {
  const out = []; let acc = 0, nb = 0;
  for (const b of data) {
    acc |= b << nb; nb += 8;
    while (nb >= 7) { out.push(acc & 0x7F); acc >>>= 7; nb -= 7; }
  }
  if (nb) out.push(acc & 0x7F);
  return out;
}

export function unpack7(s) {
  const out = []; let acc = 0, nb = 0;
  for (const b of s) {
    acc |= (b & 0x7F) << nb; nb += 7;
    while (nb >= 8) { out.push(acc & 0xFF); acc >>>= 8; nb -= 8; }
  }
  return out;
}

export function response(addr, data, fl = 0) {
  const len = data.length;
  const body = [0x00, 0x59, 0x30, (len + 8) & 0xFF, ((len + 8) >> 8) & 0xFF, 0, fl,
    addr & 0xFF, (addr >>> 8) & 0xFF, (addr >>> 16) & 0xFF, (addr >>> 24) & 0xFF,
    len & 0xFF, (len >> 8) & 0xFF, 0, ...data];
  let s = 0;
  for (let i = 6; i < body.length; i++) s += body[i];
  body.push(~s & 0xFF);
  return [0xF0, ...pack7(body), 0xF7];
}

export function parseRequest(pkt) {
  if (pkt[0] !== 0xF0 || pkt[pkt.length - 1] !== 0xF7) return null;
  const u = unpack7(pkt.slice(1, -1));
  if (u.length !== 15 || u[0] !== 0 || u[1] !== 0x59 || u[2] !== 0x30) return null;
  let s = 0;
  for (let i = 6; i < 14; i++) s += u[i];
  if ((~s & 0xFF) !== u[14]) return null;
  return { fl: u[6], addr: (u[7] | (u[8] << 8) | (u[9] << 16) | (u[10] << 24)) >>> 0, len: u[11] | (u[12] << 8) | (u[13] << 16) };
}

function parseIdentity(pkt) {
  const d = unpack7(pkt.slice(1, -1));
  if (d.length !== 34 || d[0] !== 0 || d[1] !== 0x59 || d[2] !== 0x11) return null;
  const txt = String.fromCharCode(...d.slice(6, 33)).replace(/\0+$/, "");
  const m = /^([^_]+)_(\d+)$/.exec(txt);
  return m ? { text: txt, model: m[1], version: parseInt(m[2], 10) } : null;
}

// one MIDI in/out pair with a SysEx queue
class Link {
  constructor(input, output) {
    this.input = input; this.output = output; this.q = []; this.waiter = null;
    input.onmidimessage = (ev) => {
      if (ev.data[0] !== 0xF0) return;
      this.q.push(Array.from(ev.data));
      if (this.waiter) { const w = this.waiter; this.waiter = null; w(); }
    };
  }
  get lost() { return this.input.state === "disconnected" || this.output.state === "disconnected"; }
  // false when the port is gone (unplugged): WebMIDI throws on a disconnected output
  send(bytes) {
    try { this.output.send(bytes); return true; } catch (_) { return false; }
  }
  drain() { this.q.length = 0; }
  async read(ms) {
    if (!this.q.length) {
      let tm;
      await Promise.race([new Promise((r) => { this.waiter = r; }), new Promise((r) => { tm = setTimeout(r, ms); })]);
      clearTimeout(tm);
      this.waiter = null;
    }
    return this.q.shift() || null;
  }
  close() { this.input.onmidimessage = null; }
}

async function handshake(link, tries = 3) {
  link.drain();
  for (let t = 0; t < tries; t++) {
    if (!link.send(HS_QUERY)) return null;
    const end = Date.now() + 1000;
    while (Date.now() < end) {
      const p = await link.read(end - Date.now());
      if (!p) break;
      const id = parseIdentity(p);
      if (id) return id;
    }
  }
  return null;
}

export class Updater {
  constructor(access, log, portPattern = /fm-1|felucca|sloop|melodee|doom|gamedude|nesynth|ota|composite|sinco|usb-midi/i) {
    this.access = access;
    this.log = log || (() => {});
    this.portPattern = portPattern;
    this.reported = new Set();
    this.lastPorts = "";
  }

  diagnostic(message) {
    if (!this.reported.has(message)) { this.reported.add(message); this.log(message); }
  }

  // every in/out pair with the same port name that answers the handshake
  async find(filter) {
    const outs = [...this.access.outputs.values()];
    const ports = JSON.stringify([...this.access.inputs.values(), ...outs].map(p => ({
      type:p.type, name:p.name ?? "", manufacturer:p.manufacturer ?? "", state:p.state
    })));
    if (ports !== this.lastPorts) { this.lastPorts = ports; this.log(`MIDI ports: ${ports}`); }
    for (const input of this.access.inputs.values()) {
      if (input.state === "disconnected") continue;
      if (!this.portPattern.test(input.name || "")) {
        this.diagnostic(`Input skipped by updater name filter: ${JSON.stringify(input.name)}`); continue;
      }
      const output = outs.find((o) => o.name === input.name && o.state !== "disconnected");
      if (!output) { this.diagnostic(`No same-name output for input: ${JSON.stringify(input.name)}`); continue; }
      try { await input.open(); await output.open(); }
      catch (e) { this.diagnostic(`Could not open MIDI pair ${JSON.stringify(input.name)}: ${e.message}`); continue; }
      const link = new Link(input, output);
      const id = await handshake(link, 2);
      const accepted = id && (!filter || filter(id));
      this.diagnostic(id ? `Identity on ${JSON.stringify(input.name)}: ${JSON.stringify(id.text)} (${accepted ? "accepted" : "not the expected identity"})`
        : `No recognized identity reply on ${JSON.stringify(input.name)}`);
      if (accepted) return { link, id, name: input.name };
      link.close();
    }
    return null;
  }

  // Probe one output at a time, listening on every input. macOS can label
  // the loader's input and output differently. Discovery sends identity only.
  async findAny(filter, {probeMs=700, openMs=1500, deadline=Infinity}={}) {
    const inputs=[...this.access.inputs.values()].filter(p=>p.state!=='disconnected');
    const outputs=[...this.access.outputs.values()].filter(p=>p.state!=='disconnected');
    const opened=new Set(), listeners=[], matches=[];
    let querying=null, selected=null;
    const bounded=(operation,ms)=>new Promise((resolve,reject)=>{
      const timer=setTimeout(()=>reject(Error('MIDI port timed out')),ms);
      Promise.resolve(operation).then(v=>{clearTimeout(timer);resolve(v);},e=>{clearTimeout(timer);reject(e);});
    });
    const closePort=async p=>{try{await bounded(p.close(),openMs);}catch(e){this.log('Could not release MIDI port '+p.name+': '+e.message);}};
    const open=async p=>{
      let expired=false;
      try{
        if(p.connection!=='open')opened.add(p);
        const pending=p.open();
        Promise.resolve(pending).then(()=>{if(expired&&opened.has(p))return closePort(p);},()=>{});
        await bounded(pending,openMs);return true;
      }catch(e){expired=true;this.log('Cannot open MIDI port '+p.name+': '+e.message);return false;}
    };
    try {
      for(const input of inputs){
        if(!await open(input))continue;
        const receive=ev=>{
          if(!querying)return;
          const id=parseIdentity(Array.from(ev.data));
          if(!id)return;
          this.log('Identity '+id.text+' on input "'+input.name+'", queried output "'+querying.name+'".');
          if(filter(id)&&!matches.some(m=>m.input===input&&m.output===querying&&m.id.text===id.text))matches.push({input,output:querying,id});
        };
        input.addEventListener('midimessage',receive);listeners.push([input,receive]);
      }
      for(const output of outputs){
        if(Date.now()>=deadline)break;
        if(!await open(output))continue;
        querying=output;
        try{output.send(HS_QUERY);await sleep(Math.max(0,Math.min(probeMs,deadline-Date.now())));}catch(e){this.log('Identity query failed: '+e.message);}
        querying=null;
      }
      if(new Set(matches.map(m=>m.input)).size>1||new Set(matches.map(m=>m.id.text)).size>1)
        throw Error('More than one matching MIDI device replied. Connect only the target SMK and retry. No programming started.');
      selected=matches.find(m=>m.input.name===m.output.name)||matches[0];
    } finally {
      for(const [input,receive] of listeners)input.removeEventListener('midimessage',receive);
      await Promise.allSettled([...opened].filter(p=>p!==selected?.input&&p!==selected?.output).map(closePort));
    }
    if(!selected)return null;
    const {input,output,id}=selected,link=new Link(input,output),close=link.close.bind(link);
    link.close=()=>{close();return Promise.allSettled([input,output].filter(p=>opened.has(p)).map(closePort));};
    return {id,link,name:output.name,inputName:input.name};
  }

  async waitForAny(filter, ms) {
    const end=Date.now()+ms;
    do {
      const dev=await this.findAny(filter,{deadline:end});
      if(dev)return dev;
      if(Date.now()<end)await sleep(Math.min(500,end-Date.now()));
    } while(Date.now()<end);
    return null;
  }

  async waitFor(filter, ms) {
    const end = Date.now() + ms;
    while (Date.now() < end) {
      const dev = await this.find(filter);
      if (dev) return dev;
      await sleep(1000);
    }
    return null;
  }

  // serve read requests until the device asks for one of the finish addresses; stops early
  // when the port disappears (lost: true) instead of waiting out idleMs
  async serve(link, image, finishAddr, idleMs, onProgress) {
    const count = createTransferCounter(image.length), writing = finishAddr === 0xF0000000;
    reportProgress({phase:writing?'transfer':'loader',bytes:0,total:image.length});
    let served = 0, last = Date.now();
    for (;;) {
      const pkt = await link.read(1000);
      if (!pkt) {
        if (link.lost) return { served, finished: false, lost: true };
        if (Date.now() - last > idleMs) return { served, finished: false };
        continue;
      }
      const status = parseSmkStatus(pkt);
      if (status) throw smkStatusError(status);
      const r = parseRequest(pkt);
      if (!r) continue;
      last = Date.now();
      if (r.addr === 0xE0000000 || r.addr === 0xF0000000) {
        link.send(response(r.addr, [...new TextEncoder().encode("success"), 0]));
        if (r.addr === finishAddr) {
          reportProgress({phase:writing?'reboot':'loader'});
          return { served, finished: true };
        }
        continue;
      }
      if (r.len > MAXDATA || r.addr + r.len > image.length) throw fail("badreq", `bad request ${r.addr.toString(16)}+${r.len}`);
      await sleep(10);
      if (!link.send(response(r.addr, Array.from(image.subarray(r.addr, r.addr + r.len)), r.fl))) {
        return { served, finished: false, lost: true };
      }
      served++;
      if (writing) reportProgress({phase:'transfer',...count(r.addr,r.len)});
      if (onProgress) onProgress(served, r.addr);
    }
  }

  // full install: running firmware -> loader -> new firmware
  async install(image, product, onStep) {
    const step = onStep || (() => {});
    const dev = await this.find((id) => !IS_OTA(id));
    if (!dev) throw fail("notfound", "FM-1 not found (USB cable, and no other app using it?)");
    const [model] = product.split("_");
    if (dev.id.model !== model) { dev.link.close(); throw fail("model", `the device is ${dev.id.text}, the package is for ${product}`); }
    step("start", dev.id.text);
    dev.link.send(UPGRADE);
    await sleep(2000);
    const s1 = await this.serve(dev.link, image, 0xE0000000, 8000, (n) => step("verify", n));
    dev.link.close();
    if (!s1.finished) throw fail(s1.lost ? "lost" : "stopped", `the device ${s1.lost ? "was disconnected" : "stopped"} after ${s1.served} requests: nothing was written`);
    step("loader");
    await sleep(3000);
    const ota = await this.waitFor((id) => id.model === "ota-" + model, 30000);
    if (!ota) throw fail("noloader", "the update loader did not appear. Replug the USB cable and press Install again: the device stays in update mode until it is finished.");
    const s2 = await this.write(ota, image, step);
    if (!s2.finished) throw fail(s2.lost ? "lost" : "stopped", `the loader ${s2.lost ? "was disconnected" : "stopped"} after ${s2.served} requests. Replug and press Install again to resume.`);
    step("reboot");
    await sleep(3000);
    const back = await this.waitFor((id) => !IS_OTA(id), 40000);
    if (!back) throw fail("noreturn", "the device did not come back: power-cycle it");
    back.link.close();
    if (back.id.text !== product) throw fail("mismatch", `installed, but the device reports ${back.id.text}`, back.id.text);
    step("done", back.id.text);
    return back.id.text;
  }

  // step 2 with the loader found by find()/waitFor()
  async write(ota, image, step) {
    ota.link.send(UPGRADE);
    await sleep(2000);
    const s2 = await this.serve(ota.link, image, 0xF0000000, 180000, (n) => step("write", n));
    ota.link.close();
    return s2;
  }

  // resume: the device is already in update mode (loader) -> true when the write finished
  async resume(image, onStep, product = null) {
    const model = product && product.split("_")[0];
    const ota = await this.find((id) => model ? id.model === "ota-" + model : IS_OTA(id));
    if (!ota) return false;
    const step = onStep || (() => {});
    const result = await this.write(ota, image, step);
    if (!result.finished) return false;
    if (product) {
      step("reboot");
      await sleep(3000);
      const back = await this.waitFor((id) => id.model === model, 40000);
      if (!back) throw fail("noreturn", "The FM-1 did not return after resuming the write.");
      back.link.close();
      if (back.id.text !== product) throw fail("mismatch", "The FM-1 reports a different firmware after resuming.", back.id.text);
      step("done", back.id.text);
    }
    return true;
  }
}
