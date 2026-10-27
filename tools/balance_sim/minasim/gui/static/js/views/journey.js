// Journey: difficulty curve across stages, clickable fight dots, selected-fight card, level curve, rest bars.
const METRICS = {
  win: { label: 'Win rate', get: (b) => b.win_rate, note: 'Higher line = easier for the player.' },
  hp: { label: 'HP left', get: (b) => (b.hp_cost == null ? null : 1 - b.hp_cost), note: 'Team HP left after the fight. Higher = easier.' },
  tp: { label: 'TP left', get: (b) => (b.tp_cost == null ? null : 1 - b.tp_cost), note: 'Team TP left after the fight. Higher = easier.' },
};

export function render(root, ctx) {
  const { lib, report, params, navigate, data } = ctx;
  const { el, svg, W } = lib;
  if (ctx.reportError) return void root.append(lib.errorState({ message: 'Could not load this run', hint: ctx.reportError.message }));
  const stages = (report && report.stages) || [];
  const allFights = (report && report.fights) || [];
  const wild = params.wild === '1';
  const fights = wild ? allFights : allFights.filter((f) => f.class !== 'wild');
  if (!report || !stages.length || !allFights.length) {
    root.append(lib.emptyState({ title: report ? 'No fights in this run.' : 'No runs yet.', text: 'The journey chart needs a report with stages and fights.',
      action: report ? null : lib.btn('Start your first run', { kind: 'go', onclick: () => document.querySelector('#top .btn.go')?.click() }) }));
    return;
  }
  const metricKey = METRICS[params.metric] ? params.metric : 'win';
  const M = METRICS[metricKey];
  const band = lib.getBand(params);
  const stIdx = Object.fromEntries(stages.map((s, i) => [s.id, i]));

  // x positions: spread each stage's fights evenly inside its column
  const byStage = {};
  fights.forEach((f) => { (byStage[f.stage] = byStage[f.stage] || []).push(f); });
  const pts = [];
  for (const [sid, list] of Object.entries(byStage)) {
    if (!(sid in stIdx)) continue;
    list.forEach((f, k) => pts.push({ f, x: (stIdx[sid] + (k + 1) / (list.length + 1)) / stages.length }));
  }
  pts.sort((a, b) => a.x - b.x);
  const val = (f, b) => { const bb = f.by_band && f.by_band[b]; return bb ? M.get(bb) : null; };
  const gap = (p) => { const v = val(p.f, 'average'), t = p.f.target; return v == null || !t ? 0 : v < t[0] ? t[0] - v : v > t[1] ? v - t[1] : 0; };
  let selId = params.sel && pts.find((p) => p.f.trainer === params.sel) ? params.sel : pts.slice().sort((a, b) => gap(b) - gap(a))[0].f.trainer;

  const header = el('div', { class: 'pagehead' },
    el('div', { style: 'margin-right:auto' }, el('h1', null, 'Game journey'), el('div', { class: 'mut' }, 'How hard each fight is, from the first town to the end. ' + M.note)),
    lib.seg({ label: 'Metric', value: metricKey, options: Object.entries(METRICS).map(([k, m]) => ({ value: k, label: m.label })), onChange: (k) => navigate(lib.hashOf('journey', Object.assign({}, params, { metric: k, id: undefined })), { replace: true }) }),
    el('label', { class: 'field', style: 'min-height:38px' }, el('input', { type: 'checkbox', checked: wild, onchange: (e) => navigate(lib.hashOf('journey', Object.assign({}, params, { wild: e.target.checked ? '1' : '', id: undefined })), { replace: true }) }), 'Wild fights'),
    el('div', { class: 'legend', style: 'gap:18px' }, el('span', null, el('i', { style: 'background:#31A2F2' }), 'Average player'), el('span', null, el('i', { style: 'background:#F2C660' }), 'Skilled player'),
      metricKey === 'win' ? el('span', null, el('i', { style: 'background:var(--band)' }), 'Wanted range') : null));
  root.append(header);
  { const sb = lib.sourceBanner(report); if (sb) root.append(sb); }

  // chart
  const mkSeries = (b, color) => ({ name: b, color, points: pts.filter((p) => val(p.f, b) != null).map((p) => ({ x: p.x, y: val(p.f, b) })) });
  const dotPts = pts.filter((p) => val(p.f, 'average') != null).map((p) => {
    const v = val(p.f, 'average'), t = p.f.target, out = metricKey === 'win' && t && (v < t[0] || v > t[1]);
    return { x: p.x, y: v, id: p.f.trainer,
      label: `${p.f.name}, average ${M.label.toLowerCase()} ${lib.pct(v)}${out ? ', outside the wanted range' : ''}`,
      dot: { shape: p.f.class === 'gym' ? 'square' : 'round', fill: metricKey === 'win' ? (out ? 'var(--coral)' : 'var(--lcd)') : 'var(--cyan)', selected: p.f.trainer === selId },
      onPick: () => navigate('#/fight/' + encodeURIComponent(p.f.trainer)), onHover: () => select(p.f.trainer) };
  });
  const avgS = mkSeries('average', '#31A2F2'); const skS = mkSeries('skilled', '#F2C660');
  avgS.points = dotPts; // dots ride the average line
  const bandPts = metricKey === 'win' ? pts.filter((p) => p.f.target).map((p) => ({ x: p.x, lo: p.f.target[0], hi: p.f.target[1] })) : null;
  const chart = lib.lineChart({ series: [skS, avgS], band: bandPts, height: 340, minWidth: 1180, aria: 'Difficulty curve: win rate per fight across the game. Use the dots to open a fight.',
    bg: stages.map((s, i) => ({ x0: i / stages.length, x1: (i + 1) / stages.length, fill: i % 2 ? '#8883' : 'transparent' })) });
  const chartin = chart.plot.parentNode; chartin.style.paddingTop = '64px';
  // gym faces
  pts.filter((p) => p.f.class === 'gym' && lib.trainerPic(p.f.trainer, data) && val(p.f, 'average') != null).forEach((p) => {
    const a = chart.at(p.x, val(p.f, 'average'));
    const img = lib.trainerImg(p.f.trainer, data, 1, { title: p.f.name }); img.style.width = '48px'; img.style.height = '48px';
    img.style.cssText += `;position:absolute;left:calc(${a.left}% - 24px);top:calc(${a.top}% - 76px);pointer-events:none`;
    chart.plot.append(img);
  });
  // stage labels
  const prevBadge = (i) => (i ? stages[i - 1].badges : 0);
  chartin.append(el('div', { style: 'display:flex;margin-top:10px' }, stages.map((s, i) => el('div', { style: 'flex:1;min-width:0;text-align:center;padding:0 2px' },
    el('div', { 'aria-hidden': 'true', style: `width:22px;height:22px;margin:0 auto;border:3px solid var(--ink);border-radius:50%;background:${s.badges > prevBadge(i) ? 'var(--coral)' : 'var(--panel2)'};box-shadow:inset -3px -3px 0 rgba(0,0,0,.18)` }),
    el('div', { style: 'font-weight:800;font-size:12.5px;line-height:1.2;margin-top:4px' }, s.name),
    el('div', { class: 'mut', style: 'font-size:11.5px' }, s.level_range ? 'L' + s.level_range[0] + (s.level_range[1] !== s.level_range[0] ? ' to ' + s.level_range[1] : '') : '')))));
  root.append(lib.card({ title: 'Difficulty curve', right: 'point at a dot to preview, click to open the fight', bare: true }, chart.el));

  // selected fight
  const selSlot = el('div', { style: 'flex:1 1 520px;display:flex' });
  function select(id) {
    if (id === selId) return;
    selId = id;
    chart.plot.querySelectorAll('.pt').forEach((b, i) => b.classList.toggle('sel', dotPts[i] && dotPts[i].id === id));
    drawSel();
  }
  function drawSel() {
    const f = fights.find((x) => x.trainer === selId); lib.clear(selSlot);
    if (!f) return;
    const a = f.by_band && f.by_band.average, s = f.by_band && f.by_band.skilled;
    const [vt, vc] = lib.fightVerdict(a && a.win_rate, f.target);
    selSlot.append(lib.card({ title: 'Selected fight', tone: 'lcd', right: W.cls[f.class] || f.class, style: 'width:100%' },
      el('div', { class: 'flex g16 wrap' },
        lib.well(lib.trainerImg(f.trainer, data, 1), 112),
        el('div', { class: 'col g8 f1', style: 'min-width:240px' },
          el('h2', null, f.name),
          el('div', { class: 'flex g12 wrap' }, (f.party || []).map((m) => el('span', { class: 'flex ac g4', title: lib.nice(m.species) }, lib.monImg(m.species, 'icon', 1), el('b', null, 'L' + m.level)))),
          el('div', { class: 'flex g24 wrap' },
            stat('Average', a ? lib.pct(a.win_rate) : '–'), stat('Skilled', s ? lib.pct(s.win_rate) : '–'), stat('Wanted', lib.range(f.target))),
          el('div', { class: 'flex g8 ac wrap' }, lib.chip(vt, vc), lib.sourceChip(f.source), el('span', { class: 'mut', style: 'font-size:13px', title: W.margin }, a ? lib.pm(a.ci) : ''))),
        el('div', { class: 'col g8', style: 'justify-content:flex-end' },
          lib.btn('Open fight', { href: '#/fight/' + encodeURIComponent(f.trainer) }), lib.btn('Duel now', { kind: 'sec2', href: lib.hashOf('duel', { trainer: f.trainer }) })))));
  }
  const stat = (l, v) => el('div', null, el('div', { class: 'px mut' }, l), el('b', { class: 'num', style: 'font-size:24px' }, v));
  drawSel();

  // level + rest cards for the chosen band
  const lc = (report.level_curve || []).filter((r) => r.band === band).sort((a, b) => stIdx[a.stage] - stIdx[b.stage]);
  const tp = (report.tp || []).filter((r) => r.band === band).sort((a, b) => stIdx[a.stage] - stIdx[b.stage]);
  const lvlCard = lib.card({ title: 'Levels: you vs trainers', tone: 'cy', style: 'flex:1 1 380px' }, levelChart(lc));
  const tpCard = lib.card({ title: 'Fights before you must rest', tone: 'cy', style: 'flex:1 1 380px' }, tpChart(tp));
  root.append(el('div', { class: 'flex wrap ac g12', style: 'margin:20px 0 -8px' }, el('span', { class: 'px' }, 'Level and rest cards for'), lib.bandSeg(band, (b) => { lib.setBand(b); navigate(lib.hashOf('journey', Object.assign({}, params, { band: b, id: undefined })), { replace: true }); })));
  root.append(el('div', { style: 'display:flex;flex-wrap:wrap;gap:20px;margin-top:24px' }, selSlot, lvlCard, tpCard));

  function levelChart(rows) {
    if (!rows.length) return lib.emptyState({ text: 'No level data for this player level.' });
    const all = rows.flatMap((r) => [r.player_level, r.foe_level]), lo = Math.floor(Math.min(...all)), hi = Math.ceil(Math.max(...all));
    const X = (i) => (rows.length < 2 ? 200 : (i / (rows.length - 1)) * 390 + 5), Y = (v) => 120 - ((v - lo) / Math.max(1, hi - lo)) * 110;
    const path = (k) => 'M' + rows.map((r, i) => X(i).toFixed(0) + ',' + Y(r[k]).toFixed(0)).join(' L');
    const worst = rows.reduce((m, r) => (r.foe_level - r.player_level > m.foe_level - m.player_level ? r : m), rows[0]);
    return el('div', null,
      svg('svg', { viewBox: '0 0 400 130', style: 'width:100%;height:140px', role: 'img', 'aria-label': 'Player level compared to trainer level across the game, from L' + lo + ' to L' + hi },
        svg('path', { d: path('player_level'), fill: 'none', stroke: '#31A2F2', 'stroke-width': 3 }), svg('path', { d: path('foe_level'), fill: 'none', stroke: '#E15F69', 'stroke-width': 3, 'stroke-dasharray': '6 4' })),
      el('div', { class: 'legend' }, el('span', null, el('i', { style: 'background:#31A2F2' }), 'Player'), el('span', null, el('i', { style: 'background:#E15F69' }), 'Trainers')),
      el('div', { class: 'mut', style: 'font-size:13px;margin-top:6px' }, worst.foe_level > worst.player_level ? 'Biggest gap at ' + lib.stageName(data, report, worst.stage) + ': trainers ' + (worst.foe_level - worst.player_level).toFixed(1) + ' levels ahead.' : 'Player keeps up with trainers.'));
  }
  function tpChart(rows) {
    if (!rows.length) return lib.emptyState({ text: 'No rest data for this player level.' });
    const tgt = rows[0].target || [8, 15], maxV = Math.max(20, ...rows.map((r) => r.battles_per_rest)), H = 120;
    return el('div', null,
      el('div', { class: 'flex ac g4', style: `height:${H}px;align-items:flex-end;position:relative` },
        el('div', { style: `position:absolute;left:0;right:0;bottom:${tgt[0] / maxV * H}px;height:${(tgt[1] - tgt[0]) / maxV * H}px;background:var(--band);opacity:.5;border-radius:3px` }),
        rows.map((r) => el('div', { title: `${lib.stageName(data, report, r.stage)}: ${r.battles_per_rest} fights`, 'aria-label': `${lib.stageName(data, report, r.stage)}: ${r.battles_per_rest} fights before a rest`, role: 'img',
          style: `flex:1;position:relative;height:${r.battles_per_rest / maxV * H}px;border:3px solid var(--ink);border-radius:3px 3px 0 0;background:${r.battles_per_rest < r.target?.[0] || r.battles_per_rest > r.target?.[1] ? 'var(--coral)' : 'var(--cyan)'}` }))),
      el('div', { class: 'legend', style: 'margin-top:8px' }, el('span', null, el('i', { style: 'background:var(--band)' }), 'Wanted ' + tgt[0] + ' to ' + tgt[1]), el('span', null, el('i', { style: 'background:var(--coral)' }), 'Outside it')));
  }
}
