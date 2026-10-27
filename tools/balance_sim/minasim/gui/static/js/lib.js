/*
 * minasim shared helpers (ES module).  import * as lib from '../lib.js'  (views also get it as ctx.lib)
 *
 * Every view module exports  render(root, ctx)  with
 *   ctx = {data, report, runs, baseline, params, navigate(hash, {replace}), api, lib, status, runId, reload()}
 * data = /api/data, report = the selected report.json (or null), baseline = its baseline report or null,
 * params = query part of the hash as an object (#/dex?band=skilled -> {band:'skilled'}).
 * render() may return a cleanup function (called when the user leaves the view).
 *
 * DOM
 *   el(tag, props, ...children)   props: class, style (string|object), on<event>, data-*, aria-*, any attribute;
 *                                 children: string|number|Node|array|null/false (skipped). Returns the element.
 *   svg(tag, attrs, ...children)  same for SVG elements.   clear(node)   empty a node.   frag(...children)
 *   toast(msg)                    small "Copied" style message.   copyText(text)  clipboard + toast.
 * API
 *   api(path, {method, body})     JSON in/out; throws ApiError {status, body, message, hint}.
 *   runDuel(body)                 POST /api/duel -> {ok:true, ...result} | {ok:false, engineMissing, message, hint}
 *   startRun(opts)                POST /api/run -> same shape;  pollProgress(url, onTick) -> {stop()}
 * Sprites (always pixelated, integer scales)
 *   spriteUrl(kind, name, variant)             kind = pokemon|item|trainer|type
 *   monImg(species, variant='front', scale=1, opts)   native sizes: front/back 64, icon 32
 *   itemImg(item, scale), typeImg(type, scale)  items 24, type chips 32x16
 *   trainerPic(trainerId, data)  -> front_pics name or null;   trainerImg(trainerId, data, scale, opts) ("?" tile fallback)
 *   well(child, size, opts)      sprite well box.     typeChips(types)  row of type images.
 * Components (return elements)
 *   card({title, tone:'coral'|'lcd'|'cy', right, style, class}, ...body)    lcdBox(...c)    btn(label, {kind:'go'|'sec2'|'', sm, href, onclick})
 *   chip(text, tone)  tone = red|amb|info|ok|dead       sevChip(sev)   verdictChip(v)   changeChip(change)   fightVerdictChip(v)
 *   gauge({value, ci, target, tone, aria, scale})   value/ci/target on 0..1
 *   seg({options:[{value,label,node}], value, onChange, label})    bandSeg(value, onChange)   (Average / Skilled)
 *   emptyState({title, text, action})   errorState(err)   comingSoon(name)
 *   bar(frac, color)   deltaBar(delta, ci, max)   pips(n)
 *   sortableTable({columns:[{key,label,value(row),render(row),num}], rows, sort:{key,desc}, onRow(row), id(row)}) -> element
 *   lineChart({series:[{name,color,dash,points:[{x,y,label,dot,onPick}]}], band:[{x,lo,hi}], xDomain, yDomain, yTicks, height, minWidth}) -> {el, at(x,y)}
 * Wording / format
 *   W (wording table), pct(0.314)->"31%", pctN(), margin(ci)->2 (pts), pm(ci)->"± 2 pts", pts(0.18)->"+18 pts", range([a,b])->"55 to 75%"
 *   nice('LEECH_SEED')->"Leech Seed", fmtDate(iso), shortRun(run)
 *   verdictOf(value, target) -> 'low'|'ok'|'high'     ratingColor(z) -> fill color for strength tiles
 * State
 *   getBand(params) / setBand(b)   Average|Skilled choice shared across screens (localStorage + hash)
 *   store.get(key, dflt) / store.set(key, v)           safe localStorage
 *   hashOf(route, params) -> '#/route?x=y'             stageName(data, report, id)
 */

/* ------------------------------------------------------------------ DOM */
export function el(tag, props, ...children) {
  const n = document.createElement(tag);
  applyProps(n, props);
  append(n, children);
  return n;
}
const SVGNS = 'http://www.w3.org/2000/svg';
export function svg(tag, attrs, ...children) {
  const n = document.createElementNS(SVGNS, tag);
  for (const [k, v] of Object.entries(attrs || {})) {
    if (v == null || v === false) continue;
    if (k.startsWith('on') && typeof v === 'function') n.addEventListener(k.slice(2), v);
    else n.setAttribute(k, v === true ? '' : v);
  }
  append(n, children);
  return n;
}
function applyProps(n, props) {
  if (!props) return;
  for (const [k, v] of Object.entries(props)) {
    if (v == null || v === false) continue;
    if (k === 'class') n.className = v;
    else if (k === 'style') {
      if (typeof v === 'string') n.style.cssText = v;
      else for (const [sk, sv] of Object.entries(v)) sk.startsWith('--') ? n.style.setProperty(sk, sv) : (n.style[sk] = sv);
    } else if (k.startsWith('on') && typeof v === 'function') n.addEventListener(k.slice(2).toLowerCase(), v);
    else if (k === 'value' || k === 'checked' || k === 'disabled' || k === 'selected') n[k] = v;
    else if (k === 'html') n.innerHTML = v;
    else n.setAttribute(k, v === true ? '' : v);
  }
}
function append(n, children) {
  for (const c of children) {
    if (c == null || c === false) continue;
    if (Array.isArray(c)) append(n, c);
    else n.appendChild(c instanceof Node ? c : document.createTextNode(String(c)));
  }
}
export function frag(...children) { const f = document.createDocumentFragment(); append(f, children); return f; }
export function clear(n) { while (n.firstChild) n.removeChild(n.firstChild); return n; }

let toastTimer;
export function toast(msg) {
  let t = document.getElementById('toast');
  if (!t) { t = el('div', { id: 'toast', role: 'status', 'aria-live': 'polite' }); document.body.appendChild(t); }
  t.textContent = msg; t.classList.add('show');
  clearTimeout(toastTimer); toastTimer = setTimeout(() => t.classList.remove('show'), 1600);
}
export async function copyText(text) {
  try { await navigator.clipboard.writeText(text); }
  catch (e) {
    const ta = el('textarea', { style: 'position:fixed;left:-999px' }); ta.value = text; document.body.appendChild(ta);
    ta.select(); try { document.execCommand('copy'); } catch (e2) { /* ignore */ } ta.remove();
  }
  toast('Copied');
}

/* ------------------------------------------------------------------ API */
export class ApiError extends Error {
  constructor(status, body) {
    super((body && (body.error || body.message)) || ('HTTP ' + status));
    this.status = status; this.body = body; this.hint = body && body.hint;
  }
}
export async function api(path, opts = {}) {
  const init = { method: opts.method || 'GET', headers: {} };
  if (opts.body !== undefined) { init.method = opts.method || 'POST'; init.body = JSON.stringify(opts.body); init.headers['Content-Type'] = 'application/json'; }
  if (opts.signal) init.signal = opts.signal;
  let res;
  try { res = await fetch(path, init); } catch (e) { throw new ApiError(0, { error: 'The server is not answering', hint: 'Is `python3 -m minasim gui` still running?' }); }
  let body = null;
  try { body = await res.json(); } catch (e) { /* not JSON */ }
  if (!res.ok) throw new ApiError(res.status, body);
  return body;
}
function friendly(e) {
  const engineMissing = e.status === 503;
  return {
    ok: false, engineMissing, status: e.status,
    message: engineMissing ? 'The simulator is not built yet.' : (e.message || 'Something went wrong'),
    hint: engineMissing ? 'Run `make balance-lib`, then try again.' : (e.hint || ''),
  };
}
export async function runDuel(body, opts) {
  try { return Object.assign({ ok: true }, await api('/api/duel', { body, signal: opts && opts.signal })); }
  catch (e) { return friendly(e); }
}
export async function startRun(opts) {
  try { return Object.assign({ ok: true }, await api('/api/run', { body: opts || {} })); }
  catch (e) { return friendly(e); }
}
/** Poll a progress url every `every` ms until state is done/error. onTick(progress). Returns {stop()}. */
export function pollProgress(url, onTick, every = 1000) {
  let stopped = false, timer;
  const tick = async () => {
    if (stopped) return;
    try {
      const p = await api(url);
      onTick(p);
      if (p.state === 'done' || p.state === 'error' || p.phase === 'done') return;
    } catch (e) { onTick({ state: 'error', error: e.message }); return; }
    timer = setTimeout(tick, every);
  };
  tick();
  return { stop() { stopped = true; clearTimeout(timer); } };
}

/* ------------------------------------------------------------------ storage + params */
export const store = {
  get(k, d) { try { const v = localStorage.getItem('minasim:' + k); return v == null ? d : v; } catch (e) { return d; } },
  set(k, v) { try { localStorage.setItem('minasim:' + k, v); } catch (e) { /* private window */ } },
};
export function getBand(params) {
  const b = (params && params.band) || store.get('band', 'average');
  return b === 'skilled' ? 'skilled' : 'average';
}
export function setBand(b) { store.set('band', b); }
export function hashOf(route, params) {
  const q = Object.entries(params || {}).filter(([, v]) => v != null && v !== '' && v !== 'all').map(([k, v]) => encodeURIComponent(k) + '=' + encodeURIComponent(v)).join('&');
  return '#/' + route + (q ? '?' + q : '');
}

/* ------------------------------------------------------------------ wording */
export const W = {
  band: { average: 'Average', skilled: 'Skilled', both: 'Both' },
  bandLong: { average: 'Average player: greedy, a level behind', skilled: 'Skilled player: plans ahead, uses TP and Talk' },
  severity: { red: ['Red flag', 'red'], amber: ['Amber', 'amb'], info: ['Good to know', 'info'] },
  verdict: { dominant: ['Too strong', 'red'], strong: ['Strong', 'amb'], ok: ['Fine', 'ok'], weak: ['Weak', 'info'], dead: ['Never used', 'dead'] },
  change: { worse: ['▼', 'red', 'worse'], better: ['▲', 'ok', 'better'], new: ['✳', 'info', 'new this run'], same: ['=', 'dead', 'no change'], resolved: ['✔', 'ok', 'fixed'] },
  kinds: { all: 'All', fight: 'Fights', species: 'Species', move: 'Moves', item: 'Items', tp: 'TP' },
  kindLong: { fight: 'Fight', species: 'Species', move: 'Move', item: 'Item', tp: 'TP', level_curve: 'Levels', special: 'Signature', talk: 'Talk', rng: 'Luck', dominance: 'Dominance', dead_content: 'Unused' },
  metric: { win_rate: 'win rate', usage: 'turns using it', hp_left: 'HP left', tp_left: 'TP left', battles_per_rest: 'fights per rest', delta_win: 'fights won thanks to it', level_gap: 'level gap' },
  cls: { gym: 'Gym leader', rival: 'Rival', trainer: 'Trainer', wild: 'Wild', boss: 'Boss' },
  margin: 'Run it again and the number stays within this much',
};
export function nice(s) {
  if (!s) return '';
  return String(s).replace(/^(TRAINER|SPECIES|MOVE|ITEM)_/, '').toLowerCase().split(/[_ ]+/).map((w) => w.charAt(0).toUpperCase() + w.slice(1)).join(' ');
}
export const pct = (x, d = 0) => (x == null || isNaN(x) ? '–' : (x * 100).toFixed(d) + '%');
export const pctN = (x, d = 0) => (x == null || isNaN(x) ? '–' : (x * 100).toFixed(d));
export function margin(ci) { return ci && ci.length === 2 ? Math.max(0.5, Math.round(((ci[1] - ci[0]) / 2) * 100)) : null; }
export function pm(ci) { const m = margin(ci); return m == null ? '' : '± ' + m + ' pts'; }
export function pts(d) { if (d == null || isNaN(d)) return ''; const v = Math.round(d * 100); return (v > 0 ? '+' : v < 0 ? '−' : '') + Math.abs(v) + ' pts'; }
export function range(t, unit = '%') { return t ? (unit === '%' ? Math.round(t[0] * 100) + ' to ' + Math.round(t[1] * 100) + '%' : t[0] + ' to ' + t[1]) : ''; }
export function fmtDate(iso) {
  if (!iso) return '';
  const d = new Date(iso); if (isNaN(d)) return String(iso);
  return d.toLocaleDateString('en-GB', { day: 'numeric', month: 'short' }) + ' ' + d.toLocaleTimeString('en-GB', { hour: '2-digit', minute: '2-digit' });
}
export const shortRun = (r) => r ? fmtDate(r.created) + ' · ' + (r.commit || '').slice(0, 8) : '';
export function verdictOf(v, t) { if (!t || v == null) return 'ok'; return v < t[0] ? 'low' : v > t[1] ? 'high' : 'ok'; }
export function ratingColor(z) { return z == null ? 'var(--panel2)' : z < -1.4 ? '#B9B4FF' : z < -0.6 ? '#D6D3FF' : z <= 0.8 ? '#DCEBC3' : z <= 1.5 ? '#F7E3A4' : '#F5A7A3'; }
export function stageName(data, report, id) {
  const s = ((report && report.stages) || []).find((x) => x.id === id) || ((data && data.stages) || []).find((x) => x.id === id);
  return s ? s.name : id || '';
}
/** fight verdict from a win rate vs its target band */
export function fightVerdict(v, target) {
  const k = verdictOf(v, target);
  return k === 'low' ? ['Too hard', 'red'] : k === 'high' ? ['Too easy', 'amb'] : ['In range', 'ok'];
}

/* ------------------------------------------------------------------ sprites */
export const spriteUrl = (kind, name, variant) => '/sprites/' + kind + '/' + encodeURIComponent(name) + (variant ? '/' + variant : '') + '.png';
function sprite(src, w, h, alt, opts = {}) {
  const img = el('img', Object.assign({ class: 'spr' + (opts.class ? ' ' + opts.class : ''), src, alt: alt || '', width: w, height: h, loading: 'lazy', decoding: 'async', title: opts.title }, opts.attrs));
  img.style.width = w + 'px'; img.style.height = h + 'px';
  img.addEventListener('error', () => { img.replaceWith(el('span', { class: 'spr missing', style: { width: w + 'px', height: h + 'px' }, title: alt || '', role: 'img', 'aria-label': alt || 'missing sprite' }, '?')); });
  return img;
}
export function monImg(species, variant = 'front', scale = 1, opts) {
  const n = variant === 'icon' ? 32 : 64;
  return sprite(spriteUrl('pokemon', species, variant), n * scale, n * scale, opts && opts.alt !== undefined ? opts.alt : nice(species), opts);
}
export const itemImg = (item, scale = 1, opts) => sprite(spriteUrl('item', item), 24 * scale, 24 * scale, nice(item), opts);
export const typeImg = (type, scale = 1, opts) => sprite(spriteUrl('type', type), 32 * scale, 16 * scale, nice(type), opts);
export function typeChips(types) { return el('span', { class: 'flex g4' }, [...new Set(types || [])].map((t) => typeImg(t, 1))); }

// Trainer id -> front_pics name. Kanto leaders have no art yet: placeholders from the Hoenn set (DESIGN.md).
const PIC = {
  BROCK: 'leader_roxanne', MISTY: 'leader_juan', LT_SURGE: 'leader_wattson', ERIKA: 'leader_winona', KOGA: 'leader_brawly',
  SABRINA: 'leader_tate_and_liza', BLAINE: 'leader_flannery', GIOVANNI: 'leader_norman', LORELEI: 'elite_four_glacia',
  BRUNO: 'elite_four_sidney', AGATHA: 'elite_four_phoebe', LANCE: 'elite_four_drake',
  YOUNGSTER: 'youngster', LASS: 'lass', BUG_CATCHER: 'bug_catcher', HIKER: 'hiker', SUPER_NERD: 'school_kid_m', SAILOR: 'sailor',
  ENGINEER: 'collector', ROCKET_GRUNT: 'magma_grunt_m', ROCKET_ADMIN: 'magma_admin', CAMPER: 'camper', JUGGLER: 'ninja_boy',
  TAMER: 'dragon_tamer', BURGLAR: 'ruin_maniac', COOLTRAINER: 'cooltrainer_m', RIVAL: 'brendan', BLACK_BELT: 'black_belt',
  PICNICKER: 'picnicker', FISHERMAN: 'fisherman', GENTLEMAN: 'gentleman', PSYCHIC: 'psychic_m', BIRD_KEEPER: 'bird_keeper',
};
let FIGHT_NAMES = {};
/** app.js registers the report's fights so trainer art can be guessed from fight names ("Bug Catcher 2") */
export function setFights(list) { FIGHT_NAMES = Object.fromEntries((list || []).map((f) => [f.trainer, f.name])); GYM_NO = Object.fromEntries((list || []).filter((f) => f.gym_no).map((f) => [f.trainer, f.gym_no])); }
let GYM_NO = {};
const GYM_PICS = ['leader_roxanne', 'leader_juan', 'leader_wattson', 'leader_winona', 'leader_brawly', 'leader_tate_and_liza', 'leader_flannery', 'leader_norman', 'elite_four_glacia', 'elite_four_sidney', 'elite_four_phoebe', 'elite_four_drake'];
export function trainerPic(id, data) {
  if (!id) return null;
  if (GYM_NO[id]) return GYM_PICS[(GYM_NO[id] - 1) % GYM_PICS.length];
  const nm = FIGHT_NAMES[id];
  if (nm) {
    const k = nm.toUpperCase().replace(/\s*\d+$/, '').replace(/[^A-Z]+/g, '_').replace(/^_|_$/g, '');
    if (PIC[k]) return PIC[k];
    if (/^(JIRO|BROCK)$/.test(k)) return PIC.BROCK;
    if (/SHIGERU|RIVAL/.test(k)) return PIC.RIVAL;
  }
  const t = ((data && data.trainers) || []).find((x) => x.id === id);
  if (t && t.pic_slug && t.pic_ok) return t.pic_slug;
  if (/^WILD_/.test(id)) return null;
  const core = id.replace(/^TRAINER_/, '').replace(/_S$/, '').replace(/_(\d+|CHAMPION)$/, '');
  if (PIC[core]) return PIC[core];
  const hit = Object.keys(PIC).filter((k) => core.startsWith(k)).sort((a, b) => b.length - a.length)[0];
  return hit ? PIC[hit] : null;
}
/** art that stands in for a trainer with no sprite of their own (Kanto leaders, rivals) */
export function isStandIn(id) {
  if (!id) return false;
  if (GYM_NO[id]) return true;
  const nm = FIGHT_NAMES[id] || '';
  return /^(JIRO|BROCK)\b/i.test(nm) || /SHIGERU|RIVAL/i.test(nm) || /^(rival|gym):/.test(id);
}
export function trainerImg(id, data, scale = 1, opts) {
  const w = /^wild:[^:]*:(.+)$/.exec(id || '');
  if (w) return monImg(w[1], scale > 1 ? 'front' : 'icon', scale > 1 ? 1 : 2, Object.assign({ title: 'Wild ' + nice(w[1]) }, opts));
  const pic = trainerPic(id, data);
  if (pic && isStandIn(id)) { const o = Object.assign({}, opts); o.class = ((o.class || '') + ' standin').trim(); o.title = (o.title || nice(id)) + ' (stand-in art: this trainer has no sprite yet)'; opts = o; }
  if (!pic) return el('span', { class: 'spr missing', style: { width: 64 * scale + 'px', height: 64 * scale + 'px' }, role: 'img', 'aria-label': 'no art yet' }, '?');
  return sprite(spriteUrl('trainer', pic), 64 * scale, 64 * scale, nice(id), opts);
}
export function well(child, size = 112, opts = {}) {
  return el('div', { class: 'well' + (opts.class ? ' ' + opts.class : ''), style: { width: size + 'px', height: size + 'px', flex: 'none' } }, child);
}

/* ------------------------------------------------------------------ components */
export function card(o, ...body) {
  o = o || {};
  const head = el('div', { class: 'cardh' + (o.tone && o.tone !== 'coral' ? ' ' + o.tone : '') + (o.headClass ? ' ' + o.headClass : ''), style: o.headStyle },
    typeof o.title === 'string' ? el('h3', null, o.title) : o.title, o.right ? el('span', { class: 'px', style: 'margin-left:auto' }, o.right) : null);
  return el('section', { class: 'card' + (o.class ? ' ' + o.class : ''), style: o.style, id: o.id }, o.title || o.right ? head : null, o.bare ? body : el('div', { class: 'cardb' + (o.bodyClass ? ' ' + o.bodyClass : '') }, body));
}
export const lcdBox = (...c) => el('div', { class: 'lcdbox' }, c);
export function btn(label, o = {}) {
  const cls = 'btn' + (o.kind ? ' ' + o.kind : '') + (o.sm ? ' sm' : '');
  if (o.href) return el('a', { class: cls, href: o.href, onclick: o.onclick, title: o.title }, label);
  return el('button', { class: cls, type: 'button', onclick: o.onclick, disabled: o.disabled, title: o.title, 'aria-label': o['aria-label'] }, label);
}
export function chip(text, tone = 'dead', o = {}) { return el('span', { class: 'chip c-' + tone, title: o.title, style: o.style }, text); }
export function sevChip(sev) { const [t, c] = W.severity[sev] || [sev, 'dead']; return chip(t, c); }
export function verdictChip(v) { const [t, c] = W.verdict[v] || [nice(v), 'dead']; return chip(t, c); }
export function fightVerdictChip(v, target) { const [t, c] = fightVerdict(v, target); return chip(t, c); }
/* Where a fight comes from: the ROM, the design doc (planned) or a made-up stand-in. */
export const SOURCE = {
  rom: ['In ROM', 'ok', 'This fight exists in the game today (trainers.party).'],
  design: ['Planned (design doc)', 'info', 'Not in the ROM yet. The team comes from the design doc, so the numbers show what the plan would feel like.'],
  placeholder: ['Placeholder', 'amb', 'Not in the ROM and not in the design doc either. A made-up stand-in team so the journey has something at this point.'],
};
export function sourceChip(src) { const m = SOURCE[src]; return m ? chip(m[0], m[1], { title: m[2] }) : null; }
export function sourceOfTrainer(report, trainer) {
  const f = report && (report.fights || []).find((x) => x.trainer === trainer);
  return f ? f.source : null;
}
/** one banner for home and journey: most Kanto fights are not in the ROM yet */
export function sourceBanner(report) {
  const fights = (report && report.fights) || [];
  const real = fights.filter((f) => f.class !== 'wild');
  const n = { rom: 0, design: 0, placeholder: 0 };
  real.forEach((f) => { if (n[f.source] != null) n[f.source]++; });
  if (!real.length || n.rom === real.length) return null;
  return el('div', { class: 'srcbanner', role: 'note' },
    el('div', null, el('b', null, 'Most Kanto fights are not in the ROM yet.'), ` Only ${n.rom} of ${real.length} trainer fights exist in the game today, so most numbers show how the plan would play, not what the ROM does.`),
    el('div', { class: 'flex wrap ac g12' }, el('span', { class: 'flex ac g8' }, sourceChip('rom'), el('b', null, n.rom)), el('span', { class: 'flex ac g8' }, sourceChip('design'), el('b', null, n.design), el('span', { class: 'mut' }, 'team from the design doc')),
      el('span', { class: 'flex ac g8' }, sourceChip('placeholder'), el('b', null, n.placeholder), el('span', { class: 'mut' }, 'made-up stand-in team'))));
}
export function changeChip(ch) {
  if (!ch || !ch.status) return null;
  const [ic, tone, word] = W.change[ch.status] || ['', 'dead', ch.status];
  const d = ch.delta != null && (ch.status === 'worse' || ch.status === 'better') ? Math.abs(Math.round(ch.delta * 100)) + ' pts ' : '';
  return chip(ic + ' ' + d + word, tone);
}
export function gauge({ value, ci, target, tone, aria, scale = [0, 1], ticks = true }) {
  const [a, b] = scale, f = (x) => Math.max(0, Math.min(100, ((x - a) / (b - a)) * 100));
  const out = target && value != null && (value < target[0] || value > target[1]);
  const t = tone || (out ? 'red' : '');
  const g = el('div', { class: 'gauge ' + (t === 'amb' ? 'amb' : t === 'red' ? 'red' : '') + (out ? ' out' : ''), role: 'img', 'aria-label': aria || '' });
  if (target) g.appendChild(el('div', { class: 'band', style: `left:${f(target[0])}%;width:${f(target[1]) - f(target[0])}%` }));
  if (ci) g.appendChild(el('div', { class: 'ci', style: `left:${f(ci[0])}%;width:${Math.max(0.5, f(ci[1]) - f(ci[0]))}%` }));
  if (value != null) g.appendChild(el('div', { class: 'mk', style: `left:${f(value)}%` }));
  if (!ticks) return g;
  return el('div', null, g, el('div', { class: 'gscale' }, el('span', null, pct(a)), target ? el('span', null, 'wanted ' + range(target)) : el('span'), el('span', null, pct(b))));
}
export function seg({ options, value, onChange, label }) {
  const root = el('div', { class: 'seg', role: 'group', 'aria-label': label || '' });
  const draw = () => {
    clear(root);
    for (const o of options) {
      root.appendChild(el('button', { type: 'button', class: o.value === value ? 'on' : '', 'aria-pressed': o.value === value ? 'true' : 'false', onclick: () => { value = o.value; draw(); onChange && onChange(o.value); } }, o.node || o.label));
    }
  };
  draw();
  return root;
}
export const bandSeg = (value, onChange, withBoth) => seg({ label: 'Player level', value, onChange, options: [].concat(withBoth ? [{ value: 'both', label: 'Both' }] : [], [{ value: 'average', label: 'Average' }, { value: 'skilled', label: 'Skilled' }]) });
export function emptyState({ title, text, action } = {}) {
  return el('div', { class: 'empty' }, el('div', { class: 'lcdbox' }, title ? el('b', null, title) : null, title && text ? el('br') : null, text), action || null);
}
export function errorState(err) {
  const msg = typeof err === 'string' ? err : (err && err.message) || 'Something went wrong';
  const hint = err && err.hint;
  return el('div', { class: 'empty err', role: 'alert' }, el('div', { class: 'lcdbox' }, el('b', null, msg), hint ? el('br') : null, hint || null));
}
export function comingSoon(name) {
  return emptyState({ title: nice(name) + ' is coming soon', text: 'This screen is still being built.' });
}
export function bar(frac, color = 'var(--cyan)') {
  return el('div', { class: 'bar' }, el('b', { style: `left:0;width:${Math.max(0, Math.min(1, frac)) * 100}%;background:${color}` }));
}
/** centred on zero: positive = green right, negative = coral left. max = value at the edge (default 0.3 = 30 pts) */
export function deltaBar(delta, ci, max = 0.3) {
  const w = Math.min(50, Math.abs(delta) / max * 50);
  const d = el('div', { class: 'delta', role: 'img', 'aria-label': pts(delta) + (ci ? ' ' + pm(ci) : '') }, el('div', { class: 'zero' }),
    el('b', { class: delta >= 0 ? 'up' : 'dn', style: delta >= 0 ? `left:50%;width:${w}%` : `left:${50 - w}%;width:${w}%` }));
  return el('div', null, d, el('div', { class: 'num mut', style: 'font-size:12px;margin-top:2px' }, pts(delta) + (ci ? ' ' + pm(ci) : '')));
}
export function pips(n) {
  const k = Math.min(10, n);
  return el('span', { class: 'pips', title: n + ' TP', 'aria-label': n + ' TP' }, Array.from({ length: k }, () => el('i')), n > 10 ? el('span', { class: 'mut', style: 'font-size:11px;margin-left:3px' }, '+' + (n - 10)) : null);
}

/* sortable table: columns [{key,label,value(row)->sortable,render(row)->node|string,num,title}] */
export function sortableTable({ columns, rows, sort, onRow, id, empty }) {
  const state = { key: sort && sort.key, desc: !!(sort && sort.desc) };
  const wrap = el('div', { class: 'tblwrap' });
  const draw = () => {
    clear(wrap);
    const col = columns.find((c) => c.key === state.key);
    let list = rows.slice();
    if (col && col.value) {
      list.sort((a, b) => { const x = col.value(a), y = col.value(b); const r = typeof x === 'string' ? x.localeCompare(y) : (x ?? -Infinity) - (y ?? -Infinity); return state.desc ? -r : r; });
    }
    const head = el('tr', null, columns.map((c) => {
      const on = c.key === state.key;
      return el('th', { 'aria-sort': on ? (state.desc ? 'descending' : 'ascending') : 'none', class: c.num ? 'r' : '' },
        c.value ? el('button', { type: 'button', class: 'thb', title: c.title, onclick: () => { if (state.key === c.key) state.desc = !state.desc; else { state.key = c.key; state.desc = !!c.num; } draw(); } }, c.label, on ? (state.desc ? ' ▾' : ' ▴') : '') : c.label);
    }));
    const body = list.map((r) => el('tr', { class: onRow ? 'click' : '', 'data-id': id ? id(r) : null, tabindex: onRow ? 0 : null, onclick: onRow ? () => onRow(r) : null, onkeydown: onRow ? (e) => { if (e.key === 'Enter') onRow(r); } : null },
      columns.map((c) => el('td', { class: c.num ? 'r num' : '' }, c.render ? c.render(r) : String(c.value(r) ?? '')))));
    wrap.appendChild(el('table', { class: 'tbl' }, el('thead', null, head), el('tbody', null, body)));
    if (!list.length) wrap.appendChild(empty || emptyState({ title: 'Nothing here', text: 'No rows to show.' }));
  };
  draw();
  return wrap;
}

/*
 * SVG line chart with HTML dot buttons (44px hit areas).
 * x/y are in data units; xDomain/yDomain default [0,1]. series[].points[].dot = {fill, shape:'round'|'square', selected}
 * band = [{x, lo, hi}] polygon behind. Returns {el, at(x,y)->{left,top} in %}.
 */
export function lineChart({ series = [], band, xDomain = [0, 1], yDomain = [0, 1], yTicks = [1, 0.75, 0.5, 0.25, 0], yFmt = pct, height = 340, minWidth = 0, bg, aria }) {
  const W_ = 1200, H_ = 340, padT = 20, padB = 20;
  const X = (x) => ((x - xDomain[0]) / (xDomain[1] - xDomain[0])) * W_;
  const Y = (y) => padT + (1 - (y - yDomain[0]) / (yDomain[1] - yDomain[0])) * (H_ - padT - padB);
  const path = (pts_) => 'M' + pts_.map((p) => X(p.x).toFixed(1) + ',' + Y(p.y).toFixed(1)).join(' L');
  const s = svg('svg', { viewBox: `0 0 ${W_} ${H_}`, preserveAspectRatio: 'none', 'aria-hidden': 'true', style: 'position:absolute;inset:0;width:100%;height:100%' });
  (bg || []).forEach((r) => s.appendChild(svg('rect', { x: X(r.x0), y: 0, width: X(r.x1) - X(r.x0), height: H_, fill: r.fill, opacity: '.35' })));
  yTicks.forEach((t) => s.appendChild(svg('line', { x1: 0, x2: W_, y1: Y(t), y2: Y(t), stroke: 'var(--ink2)', 'stroke-opacity': '.3', 'stroke-width': 1, 'vector-effect': 'non-scaling-stroke' })));
  if (band && band.length) {
    const top = band.map((p) => X(p.x).toFixed(1) + ',' + Y(p.hi).toFixed(1)).join(' L'), bot = band.slice().reverse().map((p) => X(p.x).toFixed(1) + ',' + Y(p.lo).toFixed(1)).join(' L');
    s.appendChild(svg('path', { d: 'M' + top + ' L' + bot + ' Z', fill: 'var(--band)', opacity: '.6' }));
  }
  series.forEach((se) => {
    const ps = se.points.slice().sort((a, b) => a.x - b.x);
    if (ps.length > 1) s.appendChild(svg('path', { d: path(ps), fill: 'none', stroke: se.color, 'stroke-width': 4, 'stroke-dasharray': se.dash || null, 'vector-effect': 'non-scaling-stroke', 'stroke-linejoin': 'round' }));
  });
  const plot = el('div', { class: 'chartplot', style: { height: height + 'px' }, role: 'img', 'aria-label': aria || '' }, s,
    el('div', { class: 'yax' }, yTicks.map((t) => el('span', null, yFmt(t)))));
  const at = (x, y) => ({ left: (X(x) / W_) * 100, top: (Y(y) / H_) * 100 });
  series.forEach((se) => se.points.forEach((p) => {
    if (!p.dot) return;
    const pos = at(p.x, p.y);
    plot.appendChild(el('button', { type: 'button', class: 'pt ' + (p.dot.shape === 'square' ? 'sq ' : '') + (p.dot.selected ? 'sel' : ''), style: `left:${pos.left.toFixed(2)}%;top:${pos.top.toFixed(2)}%`, 'aria-label': p.label || '',
      onclick: p.onPick, onmouseenter: p.onHover || null, onfocus: p.onHover || null }, el('i', { style: 'background:' + (p.dot.fill || 'var(--panel)') })));
  }));
  const root = el('div', { class: 'chartscroll' }, el('div', { class: 'chartin', style: { minWidth: minWidth + 'px' } }, plot));
  return { el: root, plot, at };
}

/* ------------------------------------------------------------------ flag metrics + links (shared by home, species, compare) */
export const METRICS = {
  win_rate: { label: 'chance to win', fmt: 'pct' }, usage: { label: 'turns using it', fmt: 'pct' }, pick_rate: { label: 'how often it is picked', fmt: 'pct' },
  starter_spread: { label: 'gap between best and worst starter', fmt: 'pct' }, crit_loss_share: { label: 'losses caused by critical hits', fmt: 'pct' },
  tp_left_at_gym: { label: 'TP left at the gym', fmt: 'pct' }, delta_win: { label: 'fights won thanks to it', fmt: 'pts', scale: [-0.4, 0.4] },
  delta_per_1000: { label: 'fights won per 1000 spent', fmt: 'pts', scale: [-0.1, 0.4] }, talk_win_delta: { label: 'extra wins from Talk', fmt: 'pts', scale: [-0.1, 0.4] },
  band_gap: { label: 'gap between average and skilled players', fmt: 'pct' }, stuck_runs: { label: 'playthroughs that got stuck', fmt: 'num', scale: [0, 20] }, lead_delta: { label: 'levels ahead of trainers', fmt: 'num', scale: [-12, 12] },
  uses_per_fight: { label: 'uses per fight', fmt: 'num', scale: [0, 4] }, unreachable_lines: { label: 'unreachable lines', fmt: 'num', scale: [0, 40] },
  z: { label: 'strength vs peers (0 is typical)', fmt: 'num', scale: [-4, 4] }, battles_per_rest: { label: 'fights before a rest', fmt: 'num', scale: [0, 20] },
  level_gap: { label: 'levels behind (+) or ahead of (-) trainers', fmt: 'num', scale: [-6, 6] },
};
export function metricInfo(m) {
  m = m || {};
  const base = METRICS[m.name] || null;
  const vals = [m.value, ...(m.target || []), ...(m.ci || [])].filter((x) => x != null);
  const big = vals.some((x) => Math.abs(x) > 1.0001);
  const f = base ? base.fmt : big ? 'num' : 'pct';
  const label = base ? base.label : nice(m.name || 'value').toLowerCase();
  const num = (x) => (x == null ? '\u2013' : Number.isInteger(x) ? String(x) : Math.abs(x) >= 10 ? x.toFixed(0) : x.toFixed(1));
  const fmtV = f === 'pct' ? (x) => pct(x) : f === 'pts' ? (x) => pts(x) : num;
  const mg = m.ci && m.ci[0] !== m.ci[1] ? (f === 'num' ? '\u00B1 ' + num((m.ci[1] - m.ci[0]) / 2) : pm(m.ci)) : '';
  const tgt = m.target ? (f === 'pct' ? range(m.target) : f === 'pts' ? pts(m.target[0]) + ' to ' + pts(m.target[1]) : num(m.target[0]) + ' to ' + num(m.target[1])) : '';
  let scale = (base && base.scale) || [0, 1];
  if (f === 'num' && !(base && base.scale)) { const lo = Math.min(0, ...vals), hi = Math.max(...vals); scale = [lo, hi === lo ? lo + 1 : hi * 1.1]; }
  return { label, value: m.value == null ? '' : fmtV(m.value), margin: mg, target: tgt, scale, fmt: f, has: m.value != null };
}
export function flagLinks(f, report, data) {
  const s = f.subject || {};
  let open = '#/journey', label = 'Open';
  if (s.trainer) open = '#/fight/' + encodeURIComponent(s.trainer);
  else if (s.species) open = '#/species/' + s.species;
  else if (s.move) { const mv = ((report && report.moves) || []).find((x) => x.move === s.move); open = mv && mv.species ? '#/species/' + mv.species : '#/tables'; }
  else if (s.item) open = '#/tables?tab=items';
  const q = {}; if (s.trainer) q.trainer = s.trainer; if (s.species) q.species = s.species; if (s.move) q.move = s.move; if (s.item) q.item = s.item;
  return { open, label, whatIf: hashOf('duel', q) };
}
