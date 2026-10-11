// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import vm from 'node:vm';
const source = readFileSync(new URL('./analytics.js', import.meta.url), 'utf8');
function setup(host = 'chancethemaker.github.io', saved = null, blocked = false, path = '/Felucca/webapp/installer/') {
  const scripts = [], handlers = {}, listeners = {}, storage = new Map(), windowListeners = {}, bannerHandlers = {}, mounted = [];
  if (saved) storage.set('felucca.web.analyticsConsent', saved);
  const panel = {setAttribute() {}, close() {}, querySelectorAll: () => [], addEventListener: (k, fn) => {handlers[k] = fn;}};
  const banner = {setAttribute() {}, querySelectorAll: () => [], addEventListener: (k, fn) => {bannerHandlers[k] = fn;}};
  const context = {
    navigator: {language: 'en'},
    location: {hostname: host, pathname: path, origin: 'https://' + host, href: 'https://' + host + path + '?secret=private'},
    localStorage: {getItem: k => {if (blocked) throw Error('blocked'); return storage.get(k);}, setItem: (k, v) => {if (blocked) throw Error('blocked'); storage.set(k, v);}},
    document: {readyState:'loading', referrer: 'https://example.org/private?q=secret', documentElement: {lang: 'en', dataset:{skin:'space'}},
      createElement: name => name === 'dialog' ? panel : name === 'section' ? banner : {}, head: {append: s => scripts.push(s)},
      body: {append: (...elements) => mounted.push(...elements)},
      querySelector: () => null, addEventListener: (k, fn) => {listeners[k] = fn;}},
    URL, Date, Set, addEventListener: (k, fn) => {windowListeners[k] = fn;}
  };
  context.window = context;
  vm.runInNewContext(source, context);
  return {context, scripts, listeners, banner, storage, mounted, windowListeners,
    choose: choice => handlers.click({target: {closest: selector => selector === '[data-choice]' ? {dataset: {choice}} : null}}),
    bannerChoose: choice => bannerHandlers.click({target: {closest: selector => selector === '[data-choice]' ? {dataset: {choice}} : null}})};
}
const fresh = setup();
assert.equal(fresh.scripts.length, 1, 'Analytics starts automatically');
assert.equal(fresh.context.dataLayer.find(x => x[0] === 'consent' && x[1] === 'default')[2].analytics_storage, 'granted');
assert.equal(fresh.context.dataLayer.find(x => x[0] === 'consent' && x[1] === 'update')[2].analytics_storage, 'granted');
assert.equal(fresh.banner.hidden, false, 'New visitors see the bottom notice');
fresh.listeners.DOMContentLoaded();
assert.ok(fresh.mounted.includes(fresh.banner), 'Banner mounts even without a settings menu');
fresh.bannerChoose('denied');
assert.equal(fresh.banner.hidden, true);
assert.equal(fresh.storage.get('felucca.web.analyticsConsent'), 'denied');
fresh.choose('granted');
fresh.context.FeluccaAnalytics.track('install_attempt');
assert.equal(fresh.context.dataLayer.filter(x => x[0] === 'event').length, 1);
fresh.choose('granted');
assert.equal(fresh.scripts.length, 1);
assert.equal(Object.prototype.toString.call(fresh.context.dataLayer[0]), '[object Arguments]', 'Google tag commands use the documented arguments-object queue');
const config = fresh.context.dataLayer.find(x => x[0] === 'config')[2];
assert.equal(config.theme, 'space', 'Page views include the current theme');
fresh.context.document.documentElement.dataset.skin = 'matrix';
fresh.listeners['felucca:theme-change']();
assert.equal(fresh.context.dataLayer.at(-1)[1], 'theme_changed');
assert.equal(fresh.context.dataLayer.at(-1)[2].theme, 'matrix');
assert.ok(!config.page_location.includes('?'));
assert.equal(config.page_referrer, 'https://example.org');
fresh.context.FeluccaAnalytics.track('install_failed', {stage: 'write', error_code: 'lost', midi: 'private'});
assert.equal(fresh.context.dataLayer.at(-1)[2].midi, undefined);
fresh.context.FeluccaAnalytics.track('unknown_event');
assert.equal(fresh.context.dataLayer.at(-1)[1], 'install_failed');
fresh.choose('denied');
const count = fresh.context.dataLayer.length;
fresh.context.FeluccaAnalytics.track('install_attempt');
fresh.listeners['felucca:theme-change']();
assert.equal(fresh.context.dataLayer.length, count);
assert.equal(fresh.context['ga-disable-G-JVF09MZEGD'], true);
fresh.choose('granted');
assert.equal(fresh.scripts.length, 1, 'No duplicate tag');
fresh.choose('basic');
assert.equal(fresh.context.dataLayer.at(-1)[2].analytics_storage, 'denied');
assert.equal(fresh.context['ga-disable-G-JVF09MZEGD'], false);
fresh.context.FeluccaAnalytics.track('download_click', {file_type:'zip'});
assert.equal(fresh.context.dataLayer.at(-1)[1], 'download_click');
assert.equal(setup('localhost', 'granted').scripts.length, 0);
assert.equal(setup('hugelton.github.io', 'granted').scripts.length, 0);
assert.equal(setup('chancethemaker.github.io', 'denied').scripts.length, 0);
assert.equal(setup('chancethemaker.github.io', 'granted').scripts.length, 1);
assert.equal(setup('chancethemaker.github.io', 'denied').banner.hidden, true);
assert.equal(setup('chancethemaker.github.io', 'granted').banner.hidden, true);
const basic = setup('chancethemaker.github.io', 'basic');
assert.equal(basic.context.dataLayer.find(x => x[0] === 'consent' && x[1] === 'update')[2].analytics_storage, 'denied', 'Previous cookieless choice is respected');
assert.equal(basic.banner.hidden, false);
fresh.windowListeners.storage({key:'felucca.web.analyticsConsent', newValue:'denied'});
assert.equal(fresh.context['ga-disable-G-JVF09MZEGD'], true, 'Opt-out syncs across tabs');
fresh.windowListeners.storage({key:null, newValue:null});
assert.equal(fresh.banner.hidden, false, 'Clearing preferences restores notice');
assert.equal(fresh.context['ga-disable-G-JVF09MZEGD'], false);
assert.equal(setup('localhost').banner.hidden, false, 'Local previews show the notice');
assert.equal(setup('localhost').scripts.length, 0, 'Local previews do not send analytics');
for (const path of ['/felucca-smk/', '/felucca-smk/editor.html', '/felucca-smk/simulator.html']) {
  assert.equal(setup('chancethemaker.github.io', null, false, path).scripts.length, 1, 'SMK production path loads Analytics');
  assert.equal(setup('chancethemaker.github.io', 'denied', false, path).scripts.length, 0, 'SMK opt-out is respected');
}
assert.equal(setup('chancethemaker.github.io', null, false, '/felucca-smk-other/').scripts.length, 0);
assert.doesNotThrow(() => setup('chancethemaker.github.io', null, true).choose('granted'));
console.log('Analytics consent, host isolation, event filtering, and storage-failure checks passed.');
