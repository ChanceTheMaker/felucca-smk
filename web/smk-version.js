// SPDX-License-Identifier: GPL-3.0-only
// Product version; private updater package identities remain separate.
export const SMK_VERSION='0.9.22';
function showVersion(){
  const tagline=document.querySelector('.masthead .tagline');
  if(tagline&&!tagline.querySelector('.smk-version')){
    const badge=document.createElement('span');badge.className='smk-version';badge.textContent=SMK_VERSION;badge.setAttribute('aria-label','Version '+SMK_VERSION);tagline.append(' · ',badge);
  }
  for(const node of document.querySelectorAll('[data-smk-version]'))node.textContent=SMK_VERSION;
}
if(document.readyState!=='complete')document.addEventListener('DOMContentLoaded',showVersion);else showVersion();
window.FeluccaI18n?.onChange(showVersion);
