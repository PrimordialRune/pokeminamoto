import {
  esc, nice, ensureCss, getBand, setBand, seg, bindSeg, index, icon, typeChip, itemImg, pageHead, empty, noReport,
  verdictChip, pips, deltaBar, bar, pct, mvName, itName, stageName, debounce,
} from './_util2.js';

const FILTERS = [['all', 'All'], ['dominant', 'Too strong'], ['strong', 'Strong'], ['ok', 'Fine'], ['weak', 'Weak'], ['dead', 'Never used']];

export function render(root, ctx) {
  ensureCss();
  const { data } = ctx;
  const report = ctx.report || {};
  const ix = index(data);
  if (!ctx.report) { root.innerHTML = pageHead('Moves & items', 'Which moves and items win fights.') + noReport(); return; }
  const moves = report.moves || [], items = report.items || [];
  const hasBand = moves.some((m) => m.band) || items.some((i) => i.band);
  const st = { band: getBand(ctx), filter: 'all', mSort: 'd', mDir: -1, iSort: 'd', iDir: -1, q: '' };

  function frame() {
    root.innerHTML = `<div class="tablespage">
      ${pageHead('Moves & items', 'Which moves and items win fights, and which are never worth taking.',
        `${hasBand ? '<span id="bandseg"></span>' : ''}<label class="field" style="min-width:220px"><span class="px">Find</span><input id="q" type="search" placeholder="Move, mon or item" aria-label="Filter moves and items" value="${esc(st.q)}"></label>`)}
      <section class="card"><div class="cardh"><h3>Moves</h3><div class="flex g8 wrap" id="filters" style="margin-left:auto"></div></div>
        <div style="overflow-x:auto"><table class="tbl" id="mt" style="min-width:1080px"></table></div></section>
      <section class="card" style="margin-top:28px"><div class="cardh cy"><h3>Items</h3><span class="px" style="margin-left:auto">value = win rate change per 1000 spent</span></div>
        <div style="overflow-x:auto"><table class="tbl" id="it" style="min-width:1000px"></table></div></section>
      <p class="mut" style="font-size:13px;margin-top:14px">Click a column name to sort. Click a move row to open the species page.</p></div>`;
    if (hasBand) { root.querySelector('#bandseg').innerHTML = seg('Player level', [['average', 'Average'], ['skilled', 'Skilled']], st.band); bindSeg(root.querySelector('#bandseg'), (k) => { st.band = k; setBand(k); frame(); }); }
    root.querySelector('#q').addEventListener('input', debounce((e) => { st.q = e.target.value; paint(); }, 80));
    paint();
  }

  const th = (kind, key, label, extra = '') => {
    const cur = st[kind + 'Sort'] === key, dir = st[kind + 'Dir'];
    return `<th ${cur ? `aria-sort="${dir > 0 ? 'ascending' : 'descending'}"` : ''} ${extra}><button type="button" class="sorth ${cur ? 'on' : ''}" data-kind="${kind}" data-key="${key}">${label}${cur ? (dir > 0 ? ' &#9650;' : ' &#9660;') : ''}</button></th>`;
  };
  const stageText = (id) => esc(stageName(ix, report, id));
  const inBand = (r) => !r.band || r.band === 'both' || r.band === st.band;

  function paint() {
    const q = st.q.trim().toLowerCase();
    // ---- moves
    let rows = moves.filter(inBand).map((m) => ({ m, mi: ix.mv.get(m.move), name: mvName(ix, m.move), who: nice(m.species) }));
    if (st.filter !== 'all') rows = rows.filter((r) => r.m.verdict === st.filter);
    if (q) rows = rows.filter((r) => r.name.toLowerCase().includes(q) || r.who.toLowerCase().includes(q));
    const key = { d: (r) => r.m.delta_win ?? -9, tp: (r) => r.m.tp_cost ?? 0, use: (r) => r.m.usage ?? 0, lv: (r) => r.m.learn_level ?? 0, name: (r) => r.name }[st.mSort];
    rows.sort((a, b) => { const x = key(a), y = key(b); return (x < y ? -1 : x > y ? 1 : 0) * st.mDir; });
    root.querySelector('#filters').innerHTML = FILTERS.map(([k, l]) => `<button type="button" class="chip ${st.filter === k ? 'c-info' : 'c-dead'}" style="cursor:pointer;min-height:32px" data-f="${k}" aria-pressed="${st.filter === k}">${l}</button>`).join('');
    root.querySelector('#mt').innerHTML = `<thead><tr>${th('m', 'name', 'Move')}<th>Who learns it</th><th>Type</th>${th('m', 'lv', 'Level')}${th('m', 'tp', 'TP cost')}${th('m', 'use', 'Used')}${th('m', 'd', 'Fights won thanks to it', 'style="width:280px"')}<th>Verdict</th><th>Stage</th></tr></thead><tbody>${
      rows.length ? rows.map(({ m, mi, name }) => `<tr class="rowlink" tabindex="0" data-sp="${esc(m.species)}">
        <td><b>${esc(name)}</b></td><td><span class="flex ac">${icon(m.species)}${esc(nice(m.species))}</span></td><td>${mi ? typeChip(mi.type) : ''}</td>
        <td class="num">${m.learn_level ?? '--'}</td><td>${pips(m.tp_cost)}</td>
        <td><div class="flex ac g8"><div class="bar" style="width:70px"><b style="left:0;width:${(m.usage || 0) * 100}%;background:${m.usage > 0.6 ? 'var(--coral)' : 'var(--cyan)'}"></b></div><span class="num">${pct(m.usage)}</span></div></td>
        <td>${deltaBar(m.delta_win, m.ci)}</td><td>${verdictChip(m.verdict)}</td><td class="mut">${stageText(m.stage)}</td></tr>`).join('')
        : `<tr><td colspan="9"><div class="lcdbox">${moves.length ? 'No moves match that filter.' : 'This run has no move results yet.'}</div></td></tr>`}</tbody>`;

    // ---- items
    let irows = items.filter(inBand).map((i) => ({ i, name: itName(ix, i.item) }));
    if (q) irows = irows.filter((r) => r.name.toLowerCase().includes(q));
    const ikey = { d: (r) => r.i.delta_win ?? -9, price: (r) => r.i.price ?? 0, per: (r) => r.i.delta_per_1000 ?? -9, name: (r) => r.name, stage: (r) => r.i.stage || '' }[st.iSort];
    irows.sort((a, b) => { const x = ikey(a), y = ikey(b); return (x < y ? -1 : x > y ? 1 : 0) * st.iDir; });
    root.querySelector('#it').innerHTML = `<thead><tr>${th('i', 'name', 'Item')}${th('i', 'stage', 'First sold')}${th('i', 'price', 'Price')}${th('i', 'd', 'Fights won thanks to it', 'style="width:280px"')}${th('i', 'per', 'Per 1000')}<th>Verdict</th></tr></thead><tbody>${
      irows.length ? irows.map(({ i, name }) => `<tr><td><span class="flex ac g8">${itemImg(i.item, 2)}<b>${esc(name)}</b></span></td><td>${stageText(i.stage)}</td>
        <td class="num">${i.price ? i.price + ' &yen;' : '--'}</td><td>${deltaBar(i.delta_win, i.ci, 0.2)}</td>
        <td class="num"><b>${i.delta_per_1000 ? '+' + (i.delta_per_1000 * 100).toFixed(1) + ' pts' : '--'}</b></td><td>${verdictChip(i.verdict)}</td></tr>`).join('')
        : `<tr><td colspan="6"><div class="lcdbox">${items.length ? 'No items match.' : 'This run has no item results yet.'}</div></td></tr>`}</tbody>`;

    root.querySelectorAll('.sorth').forEach((b) => b.addEventListener('click', () => {
      const k = b.dataset.kind, key = b.dataset.key;
      if (st[k + 'Sort'] === key) st[k + 'Dir'] *= -1; else { st[k + 'Sort'] = key; st[k + 'Dir'] = key === 'name' || key === 'lv' || key === 'stage' ? 1 : -1; }
      paint();
    }));
    root.querySelectorAll('[data-f]').forEach((b) => b.addEventListener('click', () => { st.filter = b.dataset.f; paint(); }));
    root.querySelectorAll('.rowlink').forEach((tr) => {
      const go = () => { location.hash = '#/species/' + tr.dataset.sp; };
      tr.addEventListener('click', go);
      tr.addEventListener('keydown', (e) => { if (e.key === 'Enter') go(); });
    });
  }
  frame();
}
