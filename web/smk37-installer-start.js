// SPDX-License-Identifier: GPL-3.0-only
// Catch module download/link errors before installer initialization can run.
import('./smk37-installer.js?v=0.9.22-r4').catch(error=>{
  const message='Installer could not load. Refresh this page and try again. '+error.message;
  document.getElementById('status').textContent=message;
  document.getElementById('release').textContent='Installer loading failed';
  document.getElementById('install-button').disabled=true;
  document.getElementById('log').textContent+=message+'\n';
});
