// minasim SPA shell: hash router, top bar, run picker, data + report cache, lazy views.
import * as lib from './lib.js';
const { el, clear, api } = lib;

const TABS = [
  ['home', 'Red flags'], ['journey', 'Journey'], ['fight', 'Fights'], ['dex', 'Species'],
  ['tables', 'Moves & items'], ['duel', 'Duel lab'], ['compare', 'Compare'],
];
const TAB_OF = { species: 'dex' };
const VIEWS = new Set(['home', 'journey', 'fight', 'dex', 'species', 'tables', 'duel', 'compare']);

const S = { status: null, data: null, dataErr: null, runs: [], runsErr: null, runId: null, reports: new Map(), report: null, baseline: null, reportErr: null, job: null, menuOpen: false };
let cleanup = null, renderToken = 0, lastKey = '';

/* ---------------------------------------------------------------- routing */
export function parseHash(h = location.hash) {
  const m = /^#\/?([^/?]*)(?:\/([^?]*))?(?:\?(.*))?$/.exec(h || '') || [];
  const route = m[1] || 'home';
  const arg = m[2] ? decodeURIComponent(m[2]) : null;
  const params = {};
  (m[3] || '').split('&').filter(Boolean).forEach((kv) => { const [k, v = ''] = kv.split('='); params[decodeURIComponent(k)] = decodeURIComponent(v.replace(/\+/g, ' ')); });
  if (arg != null) params.id = arg;
  return { route, arg, params };
}
function navigate(hash, o = {}) {
  if (!hash.startsWith('#')) hash = '#' + hash;
  if (o.replace) history.replaceState(null, '', hash), route(); else if (location.hash === hash) route(); else location.hash = hash;
}

/* ---------------------------------------------------------------- loading */
async function loadReport(id) {
  if (!id) return null;
  if (!S.reports.has(id)) S.reports.set(id, api('/api/report/' + encodeURIComponent(id)).catch((e) => { S.reports.delete(id); throw e; }));
  return S.reports.get(id);
}
async function selectRun(id) {
  S.runId = id; S.report = null; S.baseline = null; S.reportErr = null;
  if (id) lib.store.set('run', id);
  try {
    S.report = await loadReport(id);
    lib.setFights(S.report && S.report.fights);
    const b = S.report && S.report.meta && S.report.meta.baseline_run;
    if (b && S.runs.some((r) => r.run_id === b)) { try { S.baseline = await loadReport(b); } catch (e) { S.baseline = null; } }
  } catch (e) { S.reportErr = e; }
}
async function boot() {
  const [st, data, runs] = await Promise.allSettled([api('/api/status'), api('/api/data'), api('/api/runs')]);
  S.status = st.value || null;
  S.data = data.value || null; S.dataErr = data.reason || null;
  S.runs = runs.value || []; S.runsErr = runs.reason || null;
  const want = lib.store.get('run', null);
  const id = (S.runs.find((r) => r.run_id === want) || S.runs[0] || {}).run_id || null;
  await selectRun(id);
  drawTop();
  route();
  if (lib.store.get('welcomed', null) !== '1') showWelcome();
}

/* ---------------------------------------------------------------- top bar */
function drawTop() {
  const top = document.getElementById('top'); clear(top);
  const cur = S.runs.find((r) => r.run_id === S.runId);
  const { route: r } = parseHash();
  const active = TAB_OF[r] || r;
  const demo = (S.status && S.status.demo) || (cur && cur.demo);
  const banner = document.getElementById('demo');
  banner.hidden = !demo;
  if (demo) { clear(banner); banner.append(el('b', { class: 'px' }, 'Demo'), 'Sample data from the fixtures. Start a real run to replace it.'); }

  top.append(
    el('a', { class: 'logo', href: '#/home', style: 'text-decoration:none' }, el('i'), 'Balance Lab'),
    el('nav', { class: 'tabs', 'aria-label': 'Sections' }, TABS.map(([k, label]) => el('a', { class: 'tab' + (k === active ? ' on' : ''), href: '#/' + k, 'aria-current': k === active ? 'page' : null }, label))),
  );

  // run picker
  const picker = el('div', { class: 'runwrap' });
  const label = cur ? [el('span', { class: 'px' }, 'Run'), el('b', null, lib.fmtDate(cur.created)), el('span', { class: 'mut' }, (cur.commit || '').slice(0, 8)), el('span', { 'aria-hidden': 'true' }, '▾')]
    : [el('span', { class: 'px' }, 'Run'), el('b', null, 'none yet')];
  const btn = el('button', { class: 'runpick', type: 'button', 'aria-haspopup': 'listbox', 'aria-expanded': String(S.menuOpen), disabled: !S.runs.length, onclick: () => { S.menuOpen = !S.menuOpen; drawTop(); if (S.menuOpen) setTimeout(() => picker.querySelector('[aria-current=true]')?.focus(), 0); } }, label);
  picker.append(btn);
  if (S.menuOpen) {
    picker.append(el('div', { class: 'runmenu', role: 'listbox', onkeydown: (e) => { if (e.key === 'Escape') { S.menuOpen = false; drawTop(); document.querySelector('.runpick')?.focus(); } } },
      S.runs.map((r2) => el('button', { type: 'button', role: 'option', 'aria-current': r2.run_id === S.runId ? 'true' : 'false', onclick: async () => {
        S.menuOpen = false; await selectRun(r2.run_id); drawTop(); route(true);
      } }, el('b', null, lib.fmtDate(r2.created)), el('span', { class: 'mut' }, (r2.commit || '').slice(0, 8)),
      el('span', { style: 'margin-left:auto;display:flex;gap:6px' }, lib.chip(r2.flags_red + ' red', 'red'), lib.chip(r2.flags_amber + ' amber', 'amb'))))));
  }
  top.append(picker);
  top.append(lib.btn('?', { kind: 'sec2', onclick: showWelcome, title: 'What is this? (help)', 'aria-label': 'Help' }));
  top.lastChild.classList.add('helpbtn');

  // start new run / progress
  if (S.job && S.job.state === 'running') {
    const j = S.job, frac = j.total ? j.done / j.total : 0;
    top.append(el('div', { class: 'progress', role: 'status' }, el('span', { class: 'px' }, j.phase || 'starting'), lib.bar(frac, 'var(--lcd)'), el('span', { class: 'num mut' }, j.total ? Math.round(frac * 100) + '%' : '...')));
  } else {
    top.append(lib.btn('Start new run', { kind: 'go', onclick: chooseRun, disabled: S.status && !S.status.engine, title: S.status && !S.status.engine ? 'The simulator is not built yet. Run make balance-lib.' : '' }));
  }
  if (S.job && S.job.state === 'error') top.append(el('div', { class: 'chip c-red', role: 'alert' }, 'Run failed: ' + (S.job.error || 'unknown')));
  if (S.job && S.job.msg) top.append(el('div', { class: 'chip c-amb', role: 'alert' }, S.job.msg));
}
function chooseRun() {
  if (document.getElementById('runchoice')) return;
  const { el } = lib;
  const close = () => document.getElementById('runchoice')?.remove();
  const go = (quick) => { close(); startRun(quick); };
  const box = el('div', { id: 'runchoice', class: 'welcome', role: 'dialog', 'aria-modal': 'true', 'aria-labelledby': 'runchoice-h', onclick: (e) => { if (e.target.id === 'runchoice') close(); }, onkeydown: (e) => { if (e.key === 'Escape') close(); } },
    lib.card({ title: el('h3', { id: 'runchoice-h' }, 'Start a new run') },
      el('div', { class: 'col g16', style: 'max-width:520px' },
        el('p', { style: 'margin:0' }, 'The simulator replays the whole game with the current game data and compares it to the run you have open now.'),
        el('div', { class: 'flex g12 wrap' },
          lib.btn('Quick check (about 1 minute)', { kind: 'go', onclick: () => go(true) }),
          lib.btn('Full run (about 5 minutes)', { kind: 'sec2', onclick: () => go(false) }),
          lib.btn('Cancel', { kind: 'sec2', onclick: close })),
        el('p', { class: 'mut', style: 'margin:0;font-size:13px' }, 'A quick check uses fewer fights, so the margins (± pts) are wider. Use it while you tweak, then a full run to confirm.'))));
  document.body.append(box);
  setTimeout(() => box.querySelector('.btn')?.focus(), 0);
}
async function startRun(quick) {
  S.job = { state: 'running', phase: 'starting', done: 0, total: 0 }; drawTop();
  const r = await lib.startRun({ quick: !!quick, baseline: S.runId || undefined });
  if (!r.ok) { S.job = { state: 'idle', msg: r.message + ' ' + r.hint }; drawTop(); setTimeout(() => { if (S.job && S.job.msg) { S.job = null; drawTop(); } }, 8000); return; }
  lib.pollProgress('/api/run/' + encodeURIComponent(r.run_id) + '/progress', async (p) => {
    S.job = Object.assign({}, p, { state: p.state || 'running' });
    if (p.state === 'done' || p.phase === 'done') {
      S.job = null; S.runs = await api('/api/runs').catch(() => S.runs); S.reports.clear();
      await selectRun(r.run_id); drawTop(); route(true); lib.toast('Run finished');
    } else drawTop();
  });
}

/* ---------------------------------------------------------------- welcome / help */
function closeWelcome() {
  document.getElementById('welcome')?.remove(); lib.store.set('welcomed', '1');
  document.removeEventListener('keydown', welcomeKey); document.querySelector('.helpbtn')?.focus();
}
function welcomeKey(e) { if (e.key === 'Escape') closeWelcome(); }
function showWelcome() {
  if (document.getElementById('welcome')) return;
  const pages = [
    ['Red flags', 'The to-do list: the things that are too strong or too weak, worst first.'],
    ['Journey', 'The whole game as one curve. Every dot is a fight; red dots are outside the wanted range.'],
    ['Fights', 'Every fight with its team, who wins, what it costs, and a replay.'],
    ['Species', 'All 151 Pokemon as tiles. Slide through the game to see who is too strong or too weak when.'],
    ['Moves & items', 'Which moves and items win fights, and which nobody should bother with.'],
    ['Duel lab', 'Pick two teams, fight them thousands of times, see who wins.'],
    ['Compare', 'Two runs side by side: what got better or worse after your changes.'],
  ];
  const demo = lib.gauge({ value: 0.31, ci: [0.27, 0.35], target: [0.55, 0.75], tone: 'red', aria: 'Example: 31 percent, wanted 55 to 75 percent', ticks: true });
  const { el } = lib;
  const box = el('div', { id: 'welcome', class: 'welcome', role: 'dialog', 'aria-modal': 'true', 'aria-labelledby': 'welcome-h', onclick: (e) => { if (e.target.id === 'welcome') closeWelcome(); } },
    lib.card({ title: el('h3', { id: 'welcome-h' }, 'Welcome to Balance Lab') },
      el('div', { class: 'col g16' },
        el('p', { style: 'margin:0' }, 'This tool plays the game thousands of times with the real battle engine and tells you where it is too hard, too easy, too strong or too weak. Nothing here changes the game.'),
        el('ul', null, pages.map(([t, d]) => el('li', null, el('b', null, t), d))),
        lib.lcdBox(el('span', { class: 'px', style: 'font-size:9px' }, 'How to read a gauge'), el('br'),
          'The green stripe is the range the game should hit. The marker is what was measured; the thin line around it is the margin of error ("± 4 pts" means run it again and it stays within 4 points). A red marker is outside the green stripe.'),
        el('div', { style: 'padding:4px 0' }, demo, el('div', { class: 'mut', style: 'font-size:13px;margin-top:6px' }, 'Example: 31% ± 4 pts, wanted 55 to 75%. Too hard.')),
        el('p', { class: 'mut', style: 'margin:0;font-size:13px' }, 'Average player = plays simply, a level behind. Skilled player = plans ahead and uses TP and Talk. Many fights are still planned or placeholder: look for the badges. You can reopen this with the ? button.'),
        el('div', { class: 'flex g8' }, lib.btn('Got it, show me', { kind: 'go', onclick: closeWelcome })))));
  document.body.append(box); document.addEventListener('keydown', welcomeKey);
  setTimeout(() => box.querySelector('.btn')?.focus(), 0);
}

/* ---------------------------------------------------------------- view loading */
async function route(force) {
  const { route: name0, arg, params } = parseHash();
  const name = VIEWS.has(name0) ? name0 : 'home';
  const key = location.hash + '|' + S.runId;
  const view = document.getElementById('view');
  if (!force && key === lastKey) return;
  lastKey = key;
  const token = ++renderToken;
  if (typeof cleanup === 'function') { try { cleanup(); } catch (e) { console.error(e); } }
  cleanup = null;
  drawTop();
  clear(view);
  document.title = (TABS.find(([k]) => k === (TAB_OF[name] || name)) || ['', 'Balance Lab'])[1] + ' · Balance Lab';
  const ctx = { data: S.data, report: S.report, runs: S.runs, baseline: S.baseline, params, arg, navigate, api, lib, status: S.status, runId: S.runId, reload: () => route(true), reportError: S.reportErr };
  if (S.dataErr && !S.data) { view.append(lib.errorState({ message: 'Could not load the game data', hint: S.dataErr.message })); return; }
  let mod;
  try { mod = await import('./views/' + name + '.js'); }
  catch (e) {
    if (token !== renderToken) return;
    console.warn('view ' + name + ' not available', e);
    view.append(lib.comingSoon(name)); return;
  }
  if (token !== renderToken) return;
  try { cleanup = await mod.render(view, ctx); }
  catch (e) { console.error(e); clear(view); view.append(lib.errorState({ message: 'This screen hit a problem', hint: String(e && e.message || e) })); }
  window.scrollTo(0, 0);
}

addEventListener('hashchange', () => route());
document.addEventListener('keydown', (e) => { if (e.key === 'Escape' && S.menuOpen) { S.menuOpen = false; drawTop(); } });
document.addEventListener('click', (e) => { if (S.menuOpen && !e.target.closest('.runwrap')) { S.menuOpen = false; drawTop(); } });
boot().catch((e) => { console.error(e); document.getElementById('view').replaceChildren(lib.errorState(e)); });
