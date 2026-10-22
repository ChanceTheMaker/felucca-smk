// Discovery hints must accept peer names; transport still validates device identity.
import assert from 'node:assert/strict';
import {Updater} from '../web/fm1ota.js';
const up=new Updater({inputs:new Map(),outputs:new Map()},()=>{});
for(const app of ['Felucca','Sloop','Melodee','Doom','GameDude','GameDude Color','NESynth']){
 for(const name of [app,app+' Update',app+' FM-1',app+' FM-1 Update',app+' FM1'])
  assert(up.portPattern.test(name),'Missing peer name: '+name);
}
assert(!up.portPattern.test('Unrelated keyboard'),'Unrelated MIDI must not be probed');
console.log('PASS: legacy and current peer names are discovered; unrelated MIDI is excluded');
