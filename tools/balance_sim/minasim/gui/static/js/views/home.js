// Home / Red flags
const KIND_GROUPS = {
  fight: ['fight', 'talk', 'rng'], species: ['species'], move: ['move', 'dominance', 'special'], item: ['item'], tp: ['tp', 'level_curve'],
};
function kindMatches(f, k) {
  if (k === 'all') return true;
  if (f.kind === 'dead_content') return (k === 'species' && f.subject.species) || (k === 'item' && f.subject.item);
  return (KIND_GROUPS[k] || []).includes(f.kind);
}

export function render(root, ctx) {
  const { lib, report, params, navigate } = ctx;
  const { el, W } = lib;
  const band = params.band || 'both', kind = params.kind || 'all', stage = params.stage || 'all';
  const go = (patch) => navigate(lib.hashOf('home', Object.assign({ band, kind, stage }, patch)), { replace: true });

  if (ctx.reportError) return void root.append(lib.errorState({ message: 'Could not load this run', hint: ctx.reportError.message }));
  if (!report) {
    root.append(lib.emptyState({ title: 'No runs yet.', text: 'Start your first run to see what is too strong or too weak.',
      action: lib.btn('Start your first run', { kind: 'go', onclick: () => document.querySelector('#top .btn.go')?.click() }) }));
    return;
  }
  const flags = (report.flags || []).slice().sort((a, b) => b.score - a.score);
  const meta = report.meta || {};
  const stages = report.stages || [];
  const red = flags.filter((f) => f.severity === 'red').length, amb = flags.filter((f) => f.severity === 'amber').length;
  const fixed = flags.filter((f) => f.change && f.change.status === 'resolved').length;
  const fresh = flags.filter((f) => f.change && f.change.status === 'new').length;
  const battles = meta.battles ? (meta.battles >= 1e6 ? (meta.battles / 1e6).toFixed(1).replace(/\.0$/, '') + 'M' : Math.round(meta.battles / 1000) + 'k') : '?';
  const mins = meta.seconds ? Math.max(1, Math.round(meta.seconds / 60)) : null;

  const latest = lib.card({ title: 'Latest run', right: (meta.branch || '') + ' · ' + (meta.commit || '').slice(0, 8), style: 'flex:1 1 560px' },
    el('div', { class: 'flex wrap g24 ac' },
      el('div', { class: 'col g4', style: 'min-width:130px' }, el('div', { class: 'bignum', style: 'color:var(--red-t)' }, red), lib.chip('red flags', 'red')),
      el('div', { class: 'col g4', style: 'min-width:100px' }, el('div', { class: 'bignum', style: 'color:var(--amb-t)' }, amb), lib.chip('amber', 'amb')),
      el('div', { class: 'col g8 f1', style: 'min-width:240px' },
        lib.lcdBox(battles + ' fights played' + (mins ? ' in ' + mins + ' min' : '') + '.', el('br'),
          report.meta && meta.baseline_run && ctx.baseline ? fixed + ' flags fixed, ' + fresh + ' new since the previous run.' : 'No previous run to compare with.'),
        el('div', { class: 'legend' }, el('span', null, W.bandLong.average), el('span', null, W.bandLong.skilled)))));

  // stage strip
  const count = {}; flags.forEach((f) => { if (f.severity !== 'info') count[f.stage] = (count[f.stage] || 0) + 1; });
  const worst = stages.reduce((m, s) => ((count[s.id] || 0) > (count[m] || 0) ? s.id : m), stages[0] && stages[0].id);
  const strip = lib.card({ title: 'Where it hurts along the journey', tone: 'cy', style: 'flex:1 1 560px' },
    stages.length ? el('div', { class: 'stage-strip', 'aria-label': 'Red and amber flags per stage' }, stages.map((s) => {
      const n = count[s.id] || 0;
      return el('a', { href: '#/journey', title: s.name + ': ' + n + ' flags', 'aria-label': s.name + ', ' + n + ' flags' },
        el('div', { class: 'barbox' },
          el('span', { class: 'num', style: 'font-weight:800;font-size:12px' }, n || ''),
          el('div', { class: 'col-bar', style: `height:${8 + Math.min(n, 7) * 11}px;background:${n >= 5 ? 'var(--coral)' : n >= 2 ? '#F2C660' : n >= 1 ? 'var(--lcd)' : 'var(--panel2)'}` })),
        el('span', { class: 'vlbl' }, shortStage(s.name)));
    })) : lib.emptyState({ text: 'No stages in this report.' }),
    el('div', { class: 'mut', style: 'margin-top:8px;font-size:13px' }, 'Taller bar = more red and amber flags in that stage.' + (count[worst] ? ' Worst spot right now: ' + lib.stageName(ctx.data, report, worst) + '.' : '')));

  const sb = lib.sourceBanner(report); if (sb) root.append(sb);
  root.append(el('section', { class: 'flex wrap', style: 'gap:20px;align-items:stretch' }, latest, strip));

  // filters
  const shown = flags.filter((f) => (band === 'both' || f.band === band || f.band === 'both') && kindMatches(f, kind) && (stage === 'all' || f.stage === stage));
  const stageSel = el('select', { 'aria-label': 'Stage', onchange: (e) => go({ stage: e.target.value }) },
    el('option', { value: 'all' }, 'All stages'), stages.map((s) => el('option', { value: s.id, selected: s.id === stage }, s.name)));
  root.append(el('section', { class: 'flex wrap ac g12', style: 'margin:28px 0 14px' },
    el('h2', { style: 'margin-right:auto' }, 'Things to look at ', el('span', { class: 'mut', style: 'font-size:16px' }, '(' + shown.length + ' shown)')),
    lib.bandSeg(band, (b) => go({ band: b }), true),
    Object.entries(W.kinds).map(([k, label]) => el('button', { type: 'button', class: 'chip btnish' + (kind === k ? ' on' : ''), 'aria-pressed': String(kind === k), onclick: () => go({ kind: k }) }, label)),
    el('label', { class: 'field', style: 'min-width:200px' }, el('span', { class: 'px' }, 'Stage'), stageSel)));

  if (!flags.length) { root.append(lib.emptyState({ title: 'Nothing flagged.', text: 'This run found nothing too strong or too weak. Good news.' })); return; }
  if (!shown.length) { root.append(lib.emptyState({ title: 'No flags match these filters.', text: 'Try Both players and All.', action: lib.btn('Clear filters', { onclick: () => navigate('#/home', { replace: true }) }) })); return; }

  const fightOf = (t) => (report.fights || []).find((x) => x.trainer === t);
  const grid = el('section', { class: 'grid2' });
  const PAGE = 20; let n = Math.min(PAGE, shown.length);
  const more = el('div', { class: 'flex ac g12', style: 'margin:20px 0' });
  const paint = () => {
    lib.clear(grid); shown.slice(0, n).forEach((f, i) => grid.append(flagCard(f, i + 1)));
    lib.clear(more);
    if (n < shown.length) more.append(lib.btn('Show ' + Math.min(PAGE, shown.length - n) + ' more', { kind: 'sec2', onclick: () => { n += PAGE; paint(); } }), el('span', { class: 'mut' }, n + ' of ' + shown.length + ' shown, most important first'));
  };
  paint();
  root.append(grid, more);

  function wellFor(f) {
    const s = f.subject || {};
    const kids = [];
    if (s.trainer) {
      kids.push(lib.trainerImg(s.trainer, ctx.data, 1));
      const fi = fightOf(s.trainer);
      ((fi && fi.party) || []).slice(0, 6).forEach((m) => kids.push(lib.monImg(m.species, 'icon', 1, { title: lib.nice(m.species) + ' L' + m.level })));
    } else if (s.species) kids.push(lib.monImg(s.species, 'front', 2));
    else if (s.item) kids.push(lib.itemImg(s.item, 2));
    else kids.push(el('span', { class: 'bigglyph', 'aria-hidden': 'true' }, f.kind === 'tp' ? 'TP' : f.kind === 'level_curve' ? 'Lv' : '?'));
    return el('div', { class: 'well', style: 'width:136px;min-height:136px;flex:none;gap:2px;flex-wrap:wrap;padding:4px;align-content:center' }, kids);
  }

  function flagCard(f, rank) {
    const mi = lib.metricInfo(f.metric || {});
    const links = lib.flagLinks(f, report, ctx.data);
    const m = f.metric || {};
    const out = m.target && m.value != null && (m.value < m.target[0] || m.value > m.target[1]);
    const [sevText, sevTone] = W.severity[f.severity] || ['', 'dead'];
    const cmdBox = el('div', { class: 'cmd', hidden: true }, el('span', null, (f.repro && f.repro.cmd) || ''), el('button', { type: 'button', class: 'kbd', onclick: () => lib.copyText((f.repro && f.repro.cmd) || '') }, 'Copy'));
    const ch = lib.changeChip(f.change);
    return el('article', { class: 'card', id: 'flag-' + rank },
      el('div', { class: 'cardh hdr-' + sevTone }, el('span', { class: 'px' }, '#' + rank), lib.chip(sevText, sevTone), f.band && f.band !== 'both' ? lib.chip(W.band[f.band], 'dead') : null, f.subject && f.subject.trainer ? lib.sourceChip(lib.sourceOfTrainer(report, f.subject.trainer) !== 'rom' ? lib.sourceOfTrainer(report, f.subject.trainer) : null) : null,
        el('span', { class: 'px', style: 'margin-left:auto' }, lib.stageName(ctx.data, report, f.stage))),
      el('div', { class: 'cardb col g12' },
        el('div', { class: 'flex g16' }, wellFor(f), el('div', { class: 'col g8 f1' }, el('h3', { style: 'font-size:20px' }, f.title), el('div', { class: 'mut' }, f.detail))),
        !mi.has ? null : el('div', null,
          el('div', { class: 'flex jb ac wrap', style: 'margin-bottom:6px;gap:4px 12px' },
            el('span', null, el('b', { class: 'num', style: 'font-size:22px', title: W.margin }, mi.value), ' ', el('span', { class: 'mut', title: W.margin }, mi.margin)),
            el('span', { class: 'mut', style: 'font-size:13px' }, mi.label + (mi.target ? ', wanted ' + mi.target : ''))),
          lib.gauge({ value: m.value, ci: m.ci, target: m.target, scale: mi.scale, tone: out ? (f.severity === 'red' ? 'red' : 'amb') : '', aria: `${mi.label} ${mi.value} ${mi.margin}${mi.target ? ', wanted ' + mi.target : ''}${out ? ', outside the wanted range' : ''}`,
            ticks: true }) ),
        el('div', { class: 'flex g8 ac wrap' }, ch, ch ? el('span', { class: 'mut', style: 'font-size:13px' }, 'vs previous run') : null),
        f.suggestion ? el('div', { class: 'lcdbox', style: 'font-size:14px' }, el('span', { class: 'px', style: 'font-size:9px' }, 'What if'), el('br'), f.suggestion) : null,
        el('div', { class: 'flex g8 wrap' },
          lib.btn('Reproduce', { kind: 'sec2', sm: true, onclick: (e) => { cmdBox.hidden = !cmdBox.hidden; e.currentTarget.setAttribute('aria-expanded', String(!cmdBox.hidden)); } }),
          lib.btn('Try what-if', { sm: true, href: links.whatIf }), lib.btn('Open', { kind: 'sec2', sm: true, href: links.open })),
        cmdBox));
  }
}

function shortStage(n) {
  const m = /^Before (.+?) \((early|late)\)$/.exec(n);
  if (m) return m[1] + ' ' + m[2];
  return n.replace(/^Before /, '');
}
