import * as lib from '../lib.js';
import {
  esc, nice, ensureCss, index, front, itemImg, trainerImg, pageHead, empty, getJSON, half, clamp, mvName, itName, store,
} from './_util2.js';

const cache = new Map();
async function load(id, ctx) {
  if (ctx.report && ctx.report.meta && ctx.report.meta.run_id === id) return ctx.report;
  if (ctx.baseline && ctx.baseline.meta && ctx.baseline.meta.run_id === id) return ctx.baseline;
  if (cache.has(id)) return cache.get(id);
  const r = await getJSON('/api/report/' + encodeURIComponent(id));
  if (r.ok) cache.set(id, r.body);
  return r.ok ? r.body : null;
}

const fmtDate = (iso) => { try { const d = new Date(iso); return d.toLocaleDateString('en-GB', { day: 'numeric', month: 'short' }) + ' ' + d.toTimeString().slice(0, 5); } catch (e) { return iso || ''; } };
const runLabel = (r) => `${fmtDate(r.created)} · ${String(r.commit || '').slice(0, 8)}`;
const dist = (v, t) => (t ? Math.max(0, t[0] - v, v - t[1]) : null);
const sevRank = { info: 0, amber: 1, red: 2 };

export function render(root, ctx) {
  ensureCss();
  const runs = (ctx.runs || []).slice().sort((a, b) => String(a.created).localeCompare(String(b.created)));
  if (runs.length < 2) {
    root.innerHTML = pageHead('Compare two runs', 'What got better, what got worse after your changes.') +
      empty('You need two runs to compare.', runs.length ? 'Only one run exists so far. Start another after your next change.' : 'No runs yet. Start your first run from the bar above.');
    return;
  }
  const curId = ctx.report && ctx.report.meta && ctx.report.meta.run_id;
  const st = { after: ctx.params.after || curId || runs[runs.length - 1].run_id, before: ctx.params.before || '', f: 'all' };
  if (!st.before) {
    const base = ctx.baseline && ctx.baseline.meta && ctx.baseline.meta.run_id;
    const idx = runs.findIndex((r) => r.run_id === st.after);
    if (!base && idx === 0 && !ctx.params.after) { st.before = runs[0].run_id; st.after = runs[1].run_id; }  // oldest run open: compare it with the next one
    else st.before = base || (idx > 0 ? runs[idx - 1].run_id : runs.find((r) => r.run_id !== st.after).run_id);
  }
  let alive = true;
  const ix = index(ctx.data);

  const picker = (kind, cur) => `<label class="fld" style="display:block"><select data-pick="${kind}" aria-label="${kind} run" class="runsel">${runs.slice().reverse().map((r) => `<option value="${esc(r.run_id)}" ${r.run_id === cur ? 'selected' : ''}>${esc(runLabel(r))}${r.demo ? ' (demo)' : ''}</option>`).join('')}</select></label>`;
  const runInfo = (id) => { const r = runs.find((x) => x.run_id === id) || {}; return `${r.battles ? (r.battles / 1e6).toFixed(1) + 'M fights. ' : ''}${r.flags_red ?? '?'} red, ${r.flags_amber ?? '?'} amber.`; };

  root.innerHTML = `<div class="comparepage">${pageHead('Compare two runs', 'What got better, what got worse after your changes.')}
    <div class="split" style="align-items:stretch">
      <section class="card" style="flex:1 1 320px"><div class="cardh"><h3>Before</h3><span class="px" style="margin-left:auto">white circle</span></div><div class="cardb">${picker('before', st.before)}<div class="mut" id="binfo" style="margin-top:8px"></div></div></section>
      <section class="card" style="flex:1 1 320px"><div class="cardh cy"><h3>After</h3><span class="px" style="margin-left:auto">blue circle</span></div><div class="cardb">${picker('after', st.after)}<div class="mut" id="ainfo" style="margin-top:8px"></div></div></section>
      <section class="card" style="flex:2 1 460px"><div class="cardh lcd"><h3>What changed in the game</h3></div><div class="cardb col g4" id="changed"></div></section>
    </div>
    <div class="totals" id="totals"></div>
    <div class="flex wrap ac g12" style="margin:8px 0 12px"><h2 style="margin-right:auto">Biggest swings</h2><div class="seg" role="group" aria-label="Show" id="fseg"></div></div>
    <div class="card"><div style="overflow-x:auto"><table class="tbl" style="min-width:900px" id="tbl"></table></div></div>
    <p class="mut" style="font-size:13px;margin-top:14px">Rows only appear when the change is larger than both runs' margins of error. Click a row to open it.</p></div>`;
  const $ = (s) => root.querySelector(s);
  root.querySelectorAll('[data-pick]').forEach((s) => s.addEventListener('change', () => { st[s.dataset.pick] = s.value; go(); }));

  function subjectOf(f) {
    const s = f.subject || {};
    if (s.trainer) { const t = ix.tr.get(s.trainer); const fight = ((ctx.report || {}).fights || []).find((x) => x.trainer === s.trainer); const sp = fight && fight.party && fight.party[0] && fight.party[0].species;
      const slug = lib.trainerPic(s.trainer, ctx.data); const w = /^wild:[^:]*:(.+)$/.exec(s.trainer);
      return { img: w ? front(w[1]) : slug ? trainerImg(slug) : front(sp || 'PIDGEY'), href: '#/fight/' + s.trainer }; }
    if (s.species) return { img: front(s.species), href: '#/species/' + s.species };
    if (s.item) return { img: itemImg(s.item, 2), href: '#/tables' };
    if (s.move) return { img: '<span class="mut px">MOVE</span>', href: '#/tables' };
    const g = f.kind === 'tp' ? 'TP' : f.kind === 'level_curve' ? 'Lv' : 'Flag';
    return { img: `<span class="bigglyph" style="font-size:26px" aria-hidden="true">${g}</span>`, href: f.kind === 'tp' || f.kind === 'level_curve' ? '#/journey' : '#/home' };
  }
  const fmtV = (v, scaleOne) => (scaleOne ? Math.round(v * 100) + '%' : (Math.round(v * 10) / 10).toString());

  async function go() {
    $('#tbl').innerHTML = '<tbody><tr><td><div class="lcdbox">Loading both runs...</div></td></tr></tbody>';
    const [B, A] = await Promise.all([load(st.before, ctx), load(st.after, ctx)]);
    if (!alive) return;
    $('#binfo').textContent = runInfo(st.before); $('#ainfo').textContent = runInfo(st.after);
    if (!A || !B) { $('#tbl').innerHTML = '<tbody><tr><td><div class="lcdbox">One of those reports could not be read.</div></td></tr></tbody>'; return; }
    const bm = B.meta || {}, am = A.meta || {};
    const fb = new Map((B.flags || []).map((f) => [f.id, f])), fa = new Map((A.flags || []).map((f) => [f.id, f]));
    const count = (r, sev) => (r.flags || []).filter((f) => f.severity === sev).length;
    const rows = [];
    let fixed = 0, fresh = 0;
    for (const [id, a] of fa) {
      const b = fb.get(id);
      if (!b) { if (a.severity !== 'info') fresh++; rows.push({ f: a, kind: 'new', av: a.metric && a.metric.value, d: 0, mag: a.score || 0 }); continue; }
      const ma = a.metric, mb = b.metric;
      if (!ma || !mb || ma.value == null || mb.value == null) continue;
      const d = ma.value - mb.value;
      const thr = Math.max(half(ma.ci) || 0, half(mb.ci) || 0);
      if (Math.abs(d) <= thr) continue;
      const da = dist(ma.value, ma.target), db = dist(mb.value, mb.target);
      let worse;
      if (da != null && db != null && da !== db) worse = da > db; else worse = (sevRank[a.severity] || 0) > (sevRank[b.severity] || 0) ? true : (sevRank[a.severity] || 0) < (sevRank[b.severity] || 0) ? false : null;
      if (worse == null) continue;
      rows.push({ f: a, b, kind: worse ? 'worse' : 'better', bv: mb.value, av: ma.value, d, mag: Math.abs(d) });
    }
    for (const [id, b] of fb) if (!fa.has(id)) { if (b.severity !== 'info') fixed++; rows.push({ f: b, kind: 'resolved', bv: b.metric && b.metric.value, d: 0, mag: b.score || 0 }); }
    rows.sort((x, y) => y.mag - x.mag);

    $('#changed').innerHTML = `<div class="flex g8 ac wrap"><span class="kbd">${esc(String(bm.commit || '?').slice(0, 8))}..${esc(String(am.commit || '?').slice(0, 8))}</span><span class="mut">${bm.branch === am.branch ? esc(am.branch || '') : esc(bm.branch + ' to ' + am.branch)}</span></div>
      <div>${esc(((am.battles || 0) / 1e6).toFixed(1))}M fights now, ${esc(((bm.battles || 0) / 1e6).toFixed(1))}M before.</div>
      <div><b>${rows.filter((r) => r.kind === 'worse').length}</b> measurements got worse, <b>${rows.filter((r) => r.kind === 'better').length}</b> got better, <b>${fixed}</b> flags resolved, <b>${fresh}</b> new.</div>
      <div class="mut" style="font-size:12.5px">The exact data edits live in the git log between these commits.</div>`;

    const rb = count(B, 'red'), ra = count(A, 'red'), mb2 = count(B, 'amber'), ma2 = count(A, 'amber');
    const tone = (a, b) => (a < b ? 'c-ok' : a > b ? 'c-red' : 'c-dead');
    const note = (a, b) => (a === b ? 'no change' : Math.abs(a - b) + (a < b ? ' fewer' : ' more'));
    $('#totals').innerHTML = [['Red flags', `${rb} &rarr; ${ra}`, note(ra, rb), tone(ra, rb)], ['Fixed', fixed, 'resolved since before', fixed ? 'c-ok' : 'c-dead'],
      ['New problems', fresh, fresh ? 'look at these first' : 'none', fresh ? 'c-red' : 'c-ok'], ['Amber', `${mb2} &rarr; ${ma2}`, note(ma2, mb2), tone(ma2, mb2)]]
      .map(([l, n, nt, c]) => `<div class="card"><div class="cardb col g4"><span class="px mut">${l}</span><div class="bigtile num">${n}</div><span class="chip ${c}">${nt}</span></div></div>`).join('');

    const draw = () => {
      $('#fseg').innerHTML = [['all', 'Everything'], ['worse', 'Got worse'], ['better', 'Got better']].map(([k, l]) => `<button type="button" data-f="${k}" class="${st.f === k ? 'on' : ''}" aria-pressed="${st.f === k}">${l}</button>`).join('');
      $('#fseg').querySelectorAll('button').forEach((b) => b.addEventListener('click', () => { st.f = b.dataset.f; draw(); }));
      const list = rows.filter((r) => st.f === 'all' || r.kind === st.f || (st.f === 'better' && r.kind === 'resolved'));
      const STAT = { worse: ['Got worse', 'c-red'], better: ['Got better', 'c-ok'], new: ['New', 'c-info'], resolved: ['Resolved', 'c-ok'] };
      $('#tbl').innerHTML = `<thead><tr><th>Who</th><th>What</th><th style="width:340px">Before (white) and after (blue)</th><th>Change</th><th>Status</th></tr></thead><tbody>${list.length ? list.map((r) => {
        const sj = subjectOf(r.f); const m = r.f.metric || {}; const one = [r.av, r.bv, ...(m.target || [])].filter((v) => v != null).every((v) => v <= 1.0001);
        const vals = [r.av, r.bv, ...(m.target || [])].filter((v) => v != null);
        const max = one ? 1 : Math.max(1, ...vals) * 1.1; const P = (v) => clamp((v / max) * 100, 0, 100);
        const t = m.target;
        const track = r.av != null || r.bv != null ? `<div class="dumb">${t ? `<div class="band" style="left:${P(t[0])}%;width:${P(t[1]) - P(t[0])}%"></div>` : ''}${r.av != null && r.bv != null ? `<div class="ln" style="left:${Math.min(P(r.av), P(r.bv))}%;width:${Math.abs(P(r.av) - P(r.bv))}%"></div>` : ''}${r.bv != null ? `<div class="a" style="left:${P(r.bv)}%"></div>` : ''}${r.av != null ? `<div class="b" style="left:${P(r.av)}%"></div>` : ''}</div>
          <div class="flex jb num" style="font-size:12px;margin-top:4px"><span>${r.bv != null ? fmtV(r.bv, one) : '--'}</span><span><b>${r.av != null ? fmtV(r.av, one) : '--'}</b></span></div>` : '<span class="mut">--</span>';
        const dtxt = r.kind === 'worse' || r.kind === 'better' ? (r.d > 0 ? '+' : '') + (one ? Math.round(r.d * 100) + ' pts' : (Math.round(r.d * 10) / 10)) : '--';
        return `<tr class="rowlink" tabindex="0" data-href="${esc(sj.href)}"><td><span class="well" style="width:56px;height:56px">${sj.img}</span></td><td><b>${esc(r.f.title)}</b><div class="mut" style="font-size:12.5px">${esc(nice(r.f.kind))}${r.f.band && r.f.band !== 'both' ? ', ' + esc(r.f.band) + ' player' : ''}</div></td><td>${track}</td><td class="num"><b style="font-size:18px">${dtxt}</b></td><td><span class="chip ${STAT[r.kind][1]}">${STAT[r.kind][0]}</span></td></tr>`;
      }).join('') : '<tr><td colspan="5"><div class="lcdbox">Nothing moved by more than the margins of error.</div></td></tr>'}</tbody>`;
      $('#tbl').querySelectorAll('.rowlink').forEach((tr) => { const g = () => { location.hash = tr.dataset.href; }; tr.addEventListener('click', g); tr.addEventListener('keydown', (e) => { if (e.key === 'Enter') g(); }); });
    };
    draw();
  }
  go();
  return () => { alive = false; };
}
