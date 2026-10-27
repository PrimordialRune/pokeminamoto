import * as lib from '../lib.js';
// Shared helpers for the dex / species / tables / duel / compare views (self-contained,
// does not depend on lib.js). All markup uses the classes of css/minasim.css + css/views2.css.

export const esc = (s) => String(s ?? '').replace(/[&<>"']/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]));

// "NIDORAN_F" -> "Nidoran F", "MR_MIME" -> "Mr Mime"
export const nice = (id) => String(id ?? '').split('_').map((w) => (w.length ? w[0] + w.slice(1).toLowerCase() : w)).join(' ');
export const pct = (v, d = 0) => (v == null ? '--' : (v * 100).toFixed(d) + '%');
export const pts = (v, d = 0) => (v == null ? '--' : (v >= 0 ? '+' : '') + (v * 100).toFixed(d) + ' pts');
export const half = (ci) => (ci && ci.length === 2 ? (ci[1] - ci[0]) / 2 : null);
export const clamp = (v, a, b) => Math.max(a, Math.min(b, v));

// ---- storage (convenience only)
export function store(key, val) {
  try {
    if (val === undefined) return localStorage.getItem('minasim.' + key);
    localStorage.setItem('minasim.' + key, val);
  } catch (e) { /* private window */ }
  return null;
}
export function getBand(ctx) { return lib.getBand(ctx && ctx.params); }
export const setBand = (b) => lib.setBand(b);

// ---- sprites (server endpoints)
export const iconUrl = (sp) => `/sprites/pokemon/${encodeURIComponent(sp)}/icon.png`;
export const frontUrl = (sp) => `/sprites/pokemon/${encodeURIComponent(sp)}/front.png`;
export const typeUrl = (t) => `/sprites/type/${encodeURIComponent(t)}.png`;
export const itemUrl = (i) => `/sprites/item/${encodeURIComponent(i)}.png`;
export const trainerUrl = (slug) => `/sprites/trainer/${encodeURIComponent(slug)}.png`;
const hide = `onerror="this.style.visibility='hidden'"`;
export const icon = (sp, s = 1) => `<img class="spr px-img" src="${iconUrl(sp)}" width="${32 * s}" height="${32 * s}" alt="" loading="lazy" ${hide}>`;
export const front = (sp, s = 1) => `<img class="spr px-img" src="${frontUrl(sp)}" width="${64 * s}" height="${64 * s}" alt="" ${hide}>`;
export const typeChip = (t, s = 1) => `<img class="spr px-img ty-img" src="${typeUrl(t)}" width="${32 * s}" height="${16 * s}" alt="${esc(nice(t))}" title="${esc(nice(t))}" ${hide}>`;
export const itemImg = (i, s = 1) => `<img class="spr px-img" src="${itemUrl(i)}" width="${24 * s}" height="${24 * s}" alt="" loading="lazy" ${hide}>`;
export const trainerImg = (slug, s = 1) => `<img class="spr px-img" src="${trainerUrl(slug)}" width="${64 * s}" height="${64 * s}" alt="" ${hide}>`;

// ---- chips / verdicts
export const VERDICT = {
  dominant: ['Too strong', 'c-red'], strong: ['Strong', 'c-amb'], ok: ['Fine', 'c-ok'],
  weak: ['Weak', 'c-info'], dead: ['Never used', 'c-dead'],
};
export const verdictChip = (v) => {
  const x = VERDICT[v] || [v ? nice(v) : 'Not rated', 'c-dead'];
  return `<span class="chip ${x[1]}">${esc(x[0])}</span>`;
};
export const SEV = { red: ['Red flag', 'c-red'], amber: ['Amber', 'c-amb'], info: ['Good to know', 'c-info'] };
export const sevChip = (s) => { const x = SEV[s] || [s, 'c-dead']; return `<span class="chip ${x[1]}">${esc(x[0])}</span>`; };

// ---- strength colours for the dex (fixed pastel fills, fixed dark text, both themes)
export function strength(z) {
  if (z == null) return null;
  if (z < -1.4) return { fill: '#B9B4FF', word: 'Much too weak', cls: 'c-info' };
  if (z < -0.6) return { fill: '#D6D3FF', word: 'Weak', cls: 'c-info' };
  if (z < 0.8) return { fill: '#DCEBC3', word: 'Fair', cls: 'c-ok' };
  if (z < 1.5) return { fill: '#F7E3A4', word: 'Strong', cls: 'c-amb' };
  return { fill: '#F5A7A3', word: 'Much too strong', cls: 'c-red' };
}

// ---- small widgets
export function pips(tp) {
  if (tp == null) return '<span class="mut">--</span>';
  const n = Math.min(tp, 10);
  return `<span class="pips" role="img" aria-label="${tp} TP">${'<i></i>'.repeat(n)}</span>` + (tp > 10 ? `<span class="mut num">+${tp - 10}</span>` : `<span class="mut num"> ${tp}</span>`);
}
// DeltaBar: v and ci are fractions of win rate; scale = value that fills one half
export function deltaBar(v, ci, scale = 0.25) {
  if (v == null) return '<span class="mut">--</span>';
  const w = Math.min(50, (Math.abs(v) / scale) * 50);
  const m = half(ci);
  const style = v >= 0 ? `left:50%;width:${w}%` : `right:50%;width:${w}%`;
  const txt = pts(v) + (m != null ? ` ± ${Math.max(1, Math.round(m * 100))}` : '');
  return `<div class="delta" role="img" aria-label="${esc(txt)}"><span class="zero"></span><b class="${v >= 0 ? 'up' : 'dn'}" style="${style}"></b></div><div class="num mut" style="font-size:12px">${esc(txt)}</div>`;
}
export function bar(frac, color = 'var(--cyan)') {
  return `<div class="bar"><b style="left:0;width:${clamp(frac, 0, 1) * 100}%;background:${color}"></b></div>`;
}
export function seg(label, opts, cur) {
  return `<div class="seg" role="group" aria-label="${esc(label)}">${opts.map(([k, l]) => `<button type="button" data-seg="${esc(k)}" class="${k === cur ? 'on' : ''}" aria-pressed="${k === cur}">${esc(l)}</button>`).join('')}</div>`;
}
export function bindSeg(root, fn) {
  root.querySelectorAll('[data-seg]').forEach((b) => b.addEventListener('click', () => fn(b.dataset.seg, b)));
}
export function lcd(html) { return `<div class="lcdbox">${html}</div>`; }
export function card(title, body, { head = '', extra = '', style = '' } = {}) {
  return `<section class="card" style="${style}"><div class="cardh ${head}"><h3>${esc(title)}</h3>${extra}</div>${body}</section>`;
}
export function empty(title, text) {
  return `<div class="card"><div class="cardb">${lcd(`<b>${esc(title)}</b><br>${text}`)}</div></div>`;
}

// ---- data indexes (cached per data object)
const cache = new WeakMap();
export function index(data) {
  if (cache.has(data)) return cache.get(data);
  const sp = new Map((data.species || []).map((s) => [s.id, s]));
  const mv = new Map((data.moves || []).map((m) => [m.id, m]));
  const it = new Map((data.items || []).map((i) => [i.id, i]));
  const tr = new Map((data.trainers || []).map((t) => [t.id, t]));
  const st = new Map((data.stages || []).map((s) => [s.id, s]));
  const idx = { sp, mv, it, tr, st };
  cache.set(data, idx);
  return idx;
}
export const spName = (ix, id) => { const s = ix.sp.get(id); return s ? nice(s.id) : nice(id); };
export const mvName = (ix, id) => { const m = ix.mv.get(id); return m ? nice(m.id) : nice(id); };
export const itName = (ix, id) => { const i = ix.it.get(id); return i ? nice(i.id) : nice(id); };
export function stageName(ix, rep, id) {
  const s = (rep && (rep.stages || []).find((x) => x.id === id)) || ix.st.get(id);
  return s ? s.name : id || '--';
}
export function uniqTypes(s) { return [...new Set(s.types || [])]; }

// fetch with JSON, never throws
export async function getJSON(url, opt) {
  try {
    const r = await fetch(url, opt);
    let body = null;
    try { body = await r.json(); } catch (e) { /* empty */ }
    return { ok: r.ok, status: r.status, body };
  } catch (e) {
    return { ok: false, status: 0, body: { error: String(e) } };
  }
}

// make sure css/views2.css is loaded
export function ensureCss() {
  if (document.querySelector('link[data-views2]')) return;
  const l = document.createElement('link');
  l.rel = 'stylesheet'; l.href = 'css/views2.css'; l.dataset.views2 = '1';
  document.head.appendChild(l);
}

export function pageHead(title, sub, right = '') {
  return `<div class="flex wrap ac g16 vhead"><div style="margin-right:auto"><h1>${esc(title)}</h1><div class="mut">${sub}</div></div>${right}</div>`;
}
export const noReport = () => empty('No runs yet.', 'Start your first run from the bar above, then come back here.');

// debounce
export function debounce(fn, ms = 120) { let t; return (...a) => { clearTimeout(t); t = setTimeout(() => fn(...a), ms); }; }
