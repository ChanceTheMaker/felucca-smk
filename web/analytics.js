// SPDX-License-Identifier: GPL-3.0-only
// Optional website analytics. Never awaited by audio or firmware installation.
(() => {
  'use strict';
  const id = 'G-JVF09MZEGD', key = 'felucca.web.analyticsConsent';
  const production = location.hostname === 'chancethemaker.github.io' &&
    ['/Felucca/', '/felucca-smk/'].some(path => location.pathname.startsWith(path));
  const allowed = new Set(['install_attempt', 'install_write_started', 'install_success', 'install_failed', 'install_resume_complete', 'browser_audio_started', 'download_click', 'theme_changed']);
  const themes = new Set(['stage','matrix','dx','modeld','chocolate','vapor','midnight','space','bauhaus','ocean','arcade','hicon']);
  const theme = () => themes.has(document.documentElement.dataset?.skin) ? document.documentElement.dataset.skin : 'stage';
  let consent = '', loaded = false;
  try { consent = localStorage.getItem(key) || ''; } catch (_) {}
  if (!['granted', 'denied', 'basic'].includes(consent)) consent = '';
  window.dataLayer = window.dataLayer || [];
  function tag() { window.dataLayer.push(arguments); }
  const denied = {analytics_storage: 'denied', ad_storage: 'denied', ad_user_data: 'denied', ad_personalization: 'denied'};
  tag('consent', 'default', {...denied, analytics_storage: production && (consent === '' || consent === 'granted') ? 'granted' : 'denied'});
  function enable() {
    if (!production || consent === 'denied') return;
    window['ga-disable-' + id] = false;
    tag('consent', 'update', {...denied, analytics_storage: consent === 'basic' ? 'denied' : 'granted'});
    if (loaded) return;
    loaded = true;
    tag('js', new Date());
    tag('config', id, {
      theme: theme(), allow_google_signals: false, allow_ad_personalization_signals: false,
      page_location: location.origin + location.pathname,
      page_referrer: (() => { try { return new URL(document.referrer).origin; } catch (_) { return ''; } })()
    });
    const script = document.createElement('script');
    script.async = true;
    script.src = 'https://www.googletagmanager.com/gtag/js?id=' + id;
    document.head.append(script);
  }
  window.FeluccaAnalytics = Object.freeze({
    track(name, props = {}) {
      try {
        if (!production || consent === 'denied' || !allowed.has(name)) return;
        const safe = {theme:theme()};
        for (const field of ['firmware_version', 'stage', 'error_code', 'file_type']) {
          if (typeof props[field] === 'string' && /^[a-zA-Z0-9_.-]{1,64}$/.test(props[field])) safe[field] = props[field];
        }
        tag('event', name, safe);
      } catch (_) { /* Tracking must never interrupt the product. */ }
    }
  });
  const panel = document.createElement('dialog');
  panel.id = 'analytics-preferences';
  panel.className = 'analytics-choice';
  panel.setAttribute('aria-labelledby', 'analytics-title');
  panel.innerHTML = '<div class="analytics-heading"><h2 id="analytics-title" data-i18n="ui.cookieSettings"></h2><button type="button" data-close data-i18n-aria-label="ui.closeCookieSettings" autofocus>&times;</button></div><p data-i18n="ui.cookieDetails"></p><a href="https://policies.google.com/privacy" target="_blank" rel="noopener" data-i18n="ui.googlePrivacy"></a><div class="analytics-actions"><button type="button" data-choice="granted" data-i18n="ui.keepAnalytics"></button><button type="button" data-choice="denied" data-i18n="ui.essentialOnly"></button></div>';
  const banner = document.createElement('section');
  banner.className = 'analytics-banner';
  banner.setAttribute('aria-labelledby', 'cookie-banner-title');
  banner.innerHTML = '<div><h2 id="cookie-banner-title" data-i18n="ui.cookieChoices"></h2><p data-i18n="ui.cookieSummary"></p></div><div class="analytics-actions"><button type="button" data-choice="granted" data-i18n="ui.keepAnalytics"></button><button type="button" data-choice="denied" data-i18n="ui.essentialOnly"></button><button type="button" data-settings aria-haspopup="dialog" aria-controls="analytics-preferences" data-i18n="ui.cookieSettings"></button></div>';
  banner.hidden = ['granted', 'denied'].includes(consent);
  function reflectChoice() {
    banner.hidden = ['granted', 'denied'].includes(consent);
    for (const container of [panel, banner]) {
      for (const button of container.querySelectorAll('[data-choice]')) button.setAttribute('aria-pressed', String(button.dataset.choice === (consent || 'granted')));
    }
  }
  function choose(value, persist = true) {
    consent = value;
    if (persist) { try { localStorage.setItem(key, consent); } catch (_) {} }
    if (consent !== 'denied') enable();
    else { window['ga-disable-' + id] = true; tag('consent', 'update', denied); }
    if (panel.open) panel.close();
    reflectChoice();
  }
  panel.addEventListener('click', event => {
    const choice = event.target.closest('[data-choice]')?.dataset.choice;
    if (choice) choose(choice);
    if (event.target.closest('[data-close]')) panel.close();
    if (event.target === panel) {
      const rect = panel.getBoundingClientRect();
      if (event.clientX < rect.left || event.clientX > rect.right || event.clientY < rect.top || event.clientY > rect.bottom) panel.close();
    }
  });
  banner.addEventListener('click', event => {
    const choice = event.target.closest('[data-choice]')?.dataset.choice;
    if (choice) choose(choice);
    if (event.target.closest('[data-settings]')) { reflectChoice(); panel.showModal(); }
  });
  window.addEventListener('storage', event => {
    if (event.key === key || event.key === null) choose(['granted', 'denied', 'basic'].includes(event.newValue) ? event.newValue : '', false);
  });
  function mountPreferences() {
    reflectChoice();
    document.body.append(panel, banner);
    window.FeluccaI18n?.apply();
    const host = document.querySelector('.settings-menu [data-appearance]');
    if (!host) return;
    const open = document.createElement('button');
    open.type = 'button';
    open.className = 'analytics-menu-button';
    open.dataset.i18n = 'ui.cookieSettings';
    open.setAttribute('aria-haspopup', 'dialog');
    open.setAttribute('aria-controls', panel.id);
    open.addEventListener('click', () => {
      host.closest('.settings-menu').open = false;
      reflectChoice();
      panel.showModal();
    });
    panel.addEventListener('close', () => host.closest('.settings-menu').querySelector('summary').focus());
    host.append(open);
    window.FeluccaI18n?.apply();
  }
  if (document.readyState === 'complete') mountPreferences();
  else document.addEventListener('DOMContentLoaded', mountPreferences, {once:true});
  document.addEventListener('click', event => {
    const link = event.target.closest('a[href]');
    if (!link) return;
    const path = new URL(link.href, location.href).pathname;
    const match = path.match(/\.(zip|fwsc)$/i);
    if (match) window.FeluccaAnalytics.track('download_click', {file_type: match[1].toLowerCase()});
  });
  document.addEventListener('felucca:theme-change', () => window.FeluccaAnalytics.track('theme_changed'));
  try { enable(); } catch (_) {}
})();
