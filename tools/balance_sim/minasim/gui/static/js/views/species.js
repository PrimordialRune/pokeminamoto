import {
  esc, nice, ensureCss, getBand, setBand, seg, bindSeg, index, icon, front, typeChip, strength, stageName,
  pageHead, noReport, uniqTypes, half, clamp, verdictChip, pips, deltaBar, itName, mvName, sevChip, pct, empty,
} from './_util2.js';
import { speciesStages, rowMap } from './dex.js';

const STATS = [['HP', 'hp'], ['ATK', 'attack'], ['DEF', 'defense'], ['SPD', 'speed'], ['SPA', 'spAttack'], ['SPD*', 'spDefense']];

function speciesId(ctx) {
  const p = ctx.params || {};
  let id = p.species || p.id || p.arg || p.name;
  if (!id) { const m = location.hash.match(/#\/species\/([A-Za-z0-9_%.-]+)/); if (m) id = decodeURIComponent(m[1]); }
  return id ? String(id).toUpperCase() : null;
}

// the chain this species belongs to: root -> branches
function family(ix, id) {
  const parent = new Map();
  for (const s of ix.sp.values()) for (const e of s.evolutions || []) parent.set(e.into, { from: s.id, e });
  let root = id; while (parent.has(root)) root = parent.get(root).from;
  const out = [];
  (function walk(sp, via, depth) {
    out.push({ id: sp, via, depth });
    for (const e of (ix.sp.get(sp) || {}).evolutions || []) walk(e.into, e, depth + 1);
  })(root, null, 0);
  return out;
}
const evoLabel = (ix, e) => e.method === 'LEVEL' ? `L${e.param}` : e.method === 'ITEM' ? nice(e.param) : e.method === 'TRADE' ? 'Trade' : nice(e.method);

function stageOfLevel(stages, lvl) {
  for (let i = 0; i < stages.length; i++) { const r = stages[i].level_range; if (r && lvl <= r[1]) return i; }
  return null;
}

export function render(root, ctx) {
  ensureCss();
  const { data } = ctx;
  const report = ctx.report || {};
  const ix = index(data);
  const id = speciesId(ctx);
  const s = id && ix.sp.get(id);
  if (!s) {
    root.innerHTML = pageHead('Species', 'Pick one from the dex.') + empty('No such species.', `${esc(id || '')} is not one of the 151. <a href="#/dex">Back to the dex</a>.`);
    return;
  }
  let band = getBand(ctx);
  const hasRep = (report.species || []).length;
  const stages = hasRep ? speciesStages(report) : [];
  const rows = hasRep ? rowMap(report) : new Map();

  function draw() {
    const mine = (b) => stages.map((st) => rows.get(`${id}|${st.id}|${b}`) || null);
    const cur = mine(band);
    const lastRow = [...cur].reverse().find(Boolean);
    const lastC = lastRow ? strength(lastRow.z) : null;
    const fam = family(ix, id);
    const evoRow = fam.length > 1 ? `<div class="flex ac g8 wrap" aria-label="Evolutions">${fam.map((f, i) => `${f.via ? `<span class="mut">${esc(evoLabel(ix, f.via))} &rarr;</span>` : ''}<a class="evo ${f.id === id ? 'me' : ''}" href="#/species/${f.id}">${icon(f.id)}<b>${esc(nice(f.id))}</b></a>`).join('')}</div>` : '<div class="mut">Does not evolve.</div>';
    const bs = s.base_stats || {};
    const stats = STATS.map(([k, key]) => `<div class="flex ac g8"><span class="px" style="width:38px">${k}</span><div class="bar f1"><b style="left:0;width:${clamp((bs[key] || 0) / 160, 0, 1) * 100}%;background:var(--cyan)"></b></div><b class="num" style="width:30px;text-align:right">${bs[key] ?? '--'}</b></div>`).join('');

    // ---- strength chart
    let chart;
    const pts = cur.map((r, i) => (r ? { i, r } : null)).filter(Boolean);
    if (pts.length < 1) chart = `<div class="lcdbox">No strength data for ${esc(nice(id))} in this run.</div>`;
    else {
      const other = mine(band === 'average' ? 'skilled' : 'average');
      const all = [...cur, ...other].filter(Boolean).map((r) => r.rating);
      // rating <-> z regression to place the fair range
      const R = (report.species || []).filter((r) => r.z != null && r.rating != null).slice(0, 4000);
      let a = 110, b = 1500;
      if (R.length > 5) { const mz = R.reduce((x, r) => x + r.z, 0) / R.length, mr = R.reduce((x, r) => x + r.rating, 0) / R.length; const sxx = R.reduce((x, r) => x + (r.z - mz) ** 2, 0); if (sxx > 0) { a = R.reduce((x, r) => x + (r.z - mz) * (r.rating - mr), 0) / sxx; b = mr - a * mz; } }
      const fairLo = b + a * -0.6, fairHi = b + a * 0.8;
      let lo = Math.min(...all, fairLo) - 40, hi = Math.max(...all, fairHi) + 40;
      lo = Math.floor(lo / 50) * 50; hi = Math.ceil(hi / 50) * 50;
      const n = stages.length, X = (i) => ((i + 0.5) / n) * 600, Y = (v) => 240 - ((v - lo) / (hi - lo)) * 240;
      const path = (arr) => { let d = '', pen = false; arr.forEach((r, i) => { if (!r) { pen = false; return; } d += (pen ? 'L' : 'M') + X(i).toFixed(1) + ',' + Y(r.rating).toFixed(1) + ' '; pen = true; }); return d; };
      const evo = [];
      for (const f of fam) if (f.via && f.via.method === 'LEVEL') {
        const si = stageOfLevel(stages, f.via.param);
        if (si != null) evo.push({ si, label: `L${f.via.param} ${nice(f.id)}` });
      }
      const evoShow = evo.filter((e, k) => evo.findIndex((x) => x.label === e.label) === k);
      const gridY = [lo, (lo + hi) / 2, hi];
      chart = `<div class="chartwrap"><div class="ylab">${gridY.slice().reverse().map((v) => `<span>${Math.round(v)}</span>`).join('')}</div>
        <div class="chartbox"><svg viewBox="0 0 600 240" preserveAspectRatio="none" role="img" aria-label="Strength of ${esc(nice(id))} by stage: ${pts.map((p) => stages[p.i].name + ' ' + Math.round(p.r.rating)).join(', ')}">
          <rect x="0" y="${Y(fairHi).toFixed(1)}" width="600" height="${(Y(fairLo) - Y(fairHi)).toFixed(1)}" fill="var(--band)" opacity=".55"></rect>
          <path d="${path(other)}" fill="none" stroke="${band === 'average' ? '#F2C660' : '#31A2F2'}" stroke-width="2" opacity=".55" vector-effect="non-scaling-stroke" stroke-dasharray="5 4"></path>
          <path d="${path(cur)}" fill="none" stroke="${band === 'average' ? '#31A2F2' : '#F2C660'}" stroke-width="4" vector-effect="non-scaling-stroke" stroke-linejoin="round"></path>
          ${evoShow.map((e) => `<line x1="${X(e.si)}" y1="0" x2="${X(e.si)}" y2="240" stroke="#E15F69" stroke-width="2" stroke-dasharray="5 4" vector-effect="non-scaling-stroke"></line>`).join('')}
        </svg>
        ${pts.map((p) => `<button type="button" class="dot44" style="left:${((p.i + 0.5) / n) * 100}%;top:${(Y(p.r.rating) / 240) * 100}%" title="${esc(stages[p.i].name)}: strength ${Math.round(p.r.rating)}, wins ${pct(p.r.win_rate)}" aria-label="${esc(stages[p.i].name)}: strength ${Math.round(p.r.rating)}, wins ${pct(p.r.win_rate)}"><i style="background:${strength(p.r.z).fill}"></i></button>`).join('')}
        ${evoShow.map((e) => `<span class="chip c-info evochip" style="left:${((e.si + 0.5) / n) * 100}%">${esc(e.label)}</span>`).join('')}
        </div></div>
        <div class="xlab">${stages.map((st) => `<span title="${esc(st.name)}" ${/\(late\)/.test(st.name) ? 'style="visibility:hidden"' : ''}>${esc(st.name.replace(/^Before /, '').replace(/ \((early|late)\)/, '').split(' ')[0])}</span>`).join('')}</div>
        <div class="legend" style="margin-top:8px"><span><i style="background:var(--band)"></i>Fair range for the stage</span><span><i style="background:#E15F69"></i>Evolves here</span><span><i style="background:${band === 'average' ? '#31A2F2' : '#F2C660'}"></i>${band === 'average' ? 'Average' : 'Skilled'} (dashed = other)</span></div>`;
    }

    // ---- matchups
    const mu = lastRow && lastRow.matchups;
    const muRow = (m) => `<div class="flex ac g8">${icon(m.species)}<b style="width:96px">${esc(nice(m.species))}</b><div class="bar f1"><b style="left:0;width:${m.win_rate * 100}%;background:${m.win_rate >= 0.5 ? 'var(--grass)' : 'var(--coral)'}"></b></div><b class="num" style="width:44px;text-align:right">${Math.round(m.win_rate * 100)}%</b></div>`;
    const muBody = (k) => mu && mu[k] && mu[k].length ? mu[k].map(muRow).join('') : `<div class="mut" style="font-size:13px">Not in this report yet. The analysis adds a <code>matchups</code> list to each species row to fill this card.</div>`;
    const fights = (report.fights || []).filter((f) => (f.party || []).some((p) => p.species === id));
    const fightRows = fights.slice(0, 6).map((f) => { const b = (f.by_band || {})[band]; return `<div class="flex ac g8"><b style="flex:1">${esc(f.name || nice(f.trainer))}</b><div class="bar" style="width:90px"><b style="left:0;width:${(b ? b.win_rate : 0) * 100}%;background:var(--cyan)"></b></div><b class="num" style="width:44px;text-align:right">${b ? pct(b.win_rate) : '--'}</b></div>`; }).join('');

    // ---- flags
    const flags = (report.flags || []).filter((f) => f.subject && f.subject.species === id).slice(0, 6);
    const flagsBody = flags.length ? flags.map((f) => `<div class="flex g8 ac">${sevChip(f.severity)}<span>${esc(f.title)}</span></div>`).join('') : '<div class="mut">Nothing flagged on this species in this run.</div>';

    // ---- moves
    const learn = (s.learnset && s.learnset.level) || [];
    const byMove = new Map();
    for (const m of report.moves || []) if (m.species === id) { const prev = byMove.get(m.move); if (!prev || (m.stage || '') > (prev.stage || '')) byMove.set(m.move, m); }
    const lrows = learn.map((l) => ({ lv: l.level, move: l.move, rep: byMove.get(l.move) }));
    for (const [mv, rp] of byMove) if (!learn.some((l) => l.move === mv)) lrows.push({ lv: rp.learn_level, move: mv, rep: rp });
    lrows.sort((a, b) => (a.lv ?? 999) - (b.lv ?? 999));
    const movesBody = lrows.length ? lrows.map((r) => {
      const mi = ix.mv.get(r.move), rp = r.rep;
      return `<tr><td class="num"><b>${r.lv ?? '--'}</b></td><td><b>${esc(mvName(ix, r.move))}</b></td><td>${mi ? typeChip(mi.type) : ''}</td>
        <td>${pips(rp ? rp.tp_cost : mi && mi.tp_cost)}</td><td class="num">${rp ? pct(rp.usage) : '<span class="mut">--</span>'}</td>
        <td style="min-width:200px">${rp ? deltaBar(rp.delta_win, rp.ci) : '<span class="mut">not measured</span>'}</td><td>${rp ? verdictChip(rp.verdict) : '<span class="chip c-dead">Not rated</span>'}</td></tr>`;
    }).join('') : '<tr><td colspan="7" class="mut">No level-up moves.</td></tr>';

    root.innerHTML = `<div class="specpage">
      <div class="flex wrap ac g16" style="margin:20px 0 16px"><a class="btn sec2 sm" href="#/dex">&larr; All species</a><span class="mut">Species &rsaquo; #${String(s.dex).padStart(3, '0')}</span></div>
      <div class="split">
        <section class="card" style="flex:1 1 560px"><div class="cardh"><h3>${esc(nice(id))}</h3><span class="px" style="margin-left:auto">#${String(s.dex).padStart(3, '0')}</span></div>
          <div class="cardb flex wrap g16">
            <div class="well" style="width:176px;height:176px">${front(id, 2)}</div>
            <div class="col g12 f1" style="min-width:260px">
              <div class="flex g8 ac wrap">${uniqTypes(s).map((t) => typeChip(t, 2)).join('')}${lastC ? `<span class="chip ${lastC.cls}">${esc(lastC.word)} late in the game</span>` : ''}</div>
              ${evoRow}<div class="col g4">${stats}</div>
              <div class="mut" style="font-size:12px">Base stats; total ${s.bst ?? '--'}. ${s.minamoto && s.minamoto !== s.name ? `Minamoto name: <b>${esc(nice(s.minamoto))}</b>.` : ''}</div>
            </div></div></section>
        <section class="card" style="flex:2 1 640px"><div class="cardh cy"><h3>Strength over the game</h3><span id="bandseg" style="margin-left:auto"></span></div>
          <div class="cardb">${chart}<div class="mut" style="font-size:13px;margin-top:6px">Strength 1500 is a typical mon. Each dot is a point of the game where the run measured it; the colour matches the dex.</div></div></section>
      </div>
      <div class="split" style="margin-top:20px">
        <section class="card" style="flex:1 1 440px"><div class="cardh lcd"><h3>Beats these easily</h3></div><div class="cardb col g8">${muBody('best')}</div></section>
        <section class="card" style="flex:1 1 440px"><div class="cardh"><h3>Struggles against</h3></div><div class="cardb col g8">${muBody('worst')}</div></section>
        <section class="card" style="flex:1 1 340px"><div class="cardh cy"><h3>Flags on ${esc(nice(id))}</h3></div><div class="cardb col g8">${flagsBody}
          ${fightRows ? `<div class="px mut" style="margin-top:8px">Fights it shows up in (${band})</div>${fightRows}` : ''}
          <a class="btn sm sec2" href="#/">See the red flag board</a></div></section>
      </div>
      <section class="card" style="margin-top:20px"><div class="cardh"><h3>Moves it learns</h3><span class="px" style="margin-left:auto">Bar = win rate change from having the move</span></div>
        <div style="overflow-x:auto"><table class="tbl" style="min-width:820px"><thead><tr><th>Level</th><th>Move</th><th>Type</th><th>TP cost</th><th>Used</th><th>Fights won thanks to it</th><th>Verdict</th></tr></thead><tbody>${movesBody}</tbody></table></div></section>
    </div>`;
    root.querySelector('#bandseg').innerHTML = seg('Player level', [['average', 'Average'], ['skilled', 'Skilled']], band);
    bindSeg(root.querySelector('#bandseg'), (k) => { band = k; setBand(k); draw(); });
  }
  draw();
}
