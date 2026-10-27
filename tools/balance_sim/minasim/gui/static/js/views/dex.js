import {
  esc, nice, ensureCss, getBand, setBand, seg, bindSeg, index, icon, front, typeChip, typeUrl, strength,
  stageName, pageHead, noReport, store, uniqTypes, half, clamp, debounce,
} from './_util2.js';

// Stages that have species rows in the report, in game order.
export function speciesStages(report) {
  const have = new Set((report.species || []).map((r) => r.stage));
  const all = (report.stages || []).slice().sort((a, b) => a.index - b.index);
  const list = all.filter((s) => have.has(s.id));
  return list.length ? list : all;
}
export function rowMap(report) {
  const m = new Map();
  for (const r of report.species || []) m.set(`${r.species}|${r.stage}|${r.band}`, r);
  return m;
}

export function render(root, ctx) {
  ensureCss();
  const { data, report } = ctx;
  if (!report || !(report.species || []).length) {
    root.innerHTML = pageHead('Species dex', 'All 151 at one point of the game.') + noReport();
    return;
  }
  const ix = index(data);
  const stages = speciesStages(report);
  const rows = rowMap(report);
  const st = { stage: Math.min(Number(store('dexStage')) || 0, stages.length - 1), band: getBand(ctx), type: '', q: '', hover: null };
  if (ctx.params && ctx.params.stage != null) { const i = stages.findIndex((s) => s.id === ctx.params.stage); if (i >= 0) st.stage = i; }
  if (store('dexStage') == null && !(ctx.params && ctx.params.stage)) {
    const cnt = stages.map((x) => (report.species || []).filter((r) => r.stage === x.id && r.band === st.band).length);
    st.stage = Math.max(0, cnt.indexOf(Math.max(...cnt)));
  }
  st.stage = clamp(st.stage, 0, stages.length - 1);
  const types = data.types || [];
  const last = Math.max(stages.length - 1, 1);

  root.innerHTML = `<div class="dexpage">
    ${pageHead('Species dex', 'All 151 at one point of the game. Colour shows how strong each one is against what you meet there.',
      `<span id="bandseg"></span><label class="field" style="min-width:240px"><span class="px">Find</span><input id="q" type="search" placeholder="Name or number" aria-label="Search species by name or number"></label>`)}
    <div class="card" style="margin-bottom:20px"><div class="cardb" style="display:flex;flex-wrap:wrap;gap:20px;align-items:center">
      <div class="col g4" style="flex:1 1 520px;min-width:0">
        <div class="flex jb ac"><span class="px">Point in the game</span><b id="stagename" style="font-family:var(--font-d);font-size:20px"></b></div>
        <input id="stage" type="range" min="0" max="${last}" step="1" value="${st.stage}" aria-label="Stage of the game">
        <div class="stagelabels">${stages.map((s) => `<span title="${esc(s.name)}" ${/\(late\)/.test(s.name) ? 'style="visibility:hidden"' : ''}>${esc(s.name.replace(/^Before /, '').replace(/ \((early|late)\)/, '').split(' ')[0])}</span>`).join('')}</div>
      </div>
      <div class="col g8" style="flex:1 1 380px">
        <div class="px">How to read the colours</div>
        <div class="legendbar" role="img" aria-label="From much too weak (blue) through fair (green) to much too strong (red)"><span style="flex:1;background:#B9B4FF">&lt;-1.4</span><span style="flex:1;background:#D6D3FF">-0.6</span><span style="flex:2;background:#DCEBC3">fair</span><span style="flex:1;background:#F7E3A4">+0.8</span><span style="flex:1;background:#F5A7A3">+1.5</span></div>
        <div class="flex jb mut" style="font-size:12px"><span>Much too weak</span><span>Fair</span><span>Much too strong</span></div>
        <div class="mut" style="font-size:12.5px">Hatched = no data at this point. Every tile prints its strength score; 1500 is a typical mon.</div>
      </div>
    </div></div>
    <div class="flex wrap g8 ac" style="margin-bottom:14px"><span class="px">Type</span>
      ${types.map((t) => `<button type="button" class="tchip" data-type="${esc(t)}" aria-label="${esc(nice(t))}" aria-pressed="false"><img class="spr px-img" src="${typeUrl(t)}" width="32" height="16" alt="" onerror="this.replaceWith(document.createTextNode('${esc(nice(t)).slice(0, 3)}'))"></button>`).join('')}
    </div>
    <div class="dexcols">
      <div class="dexgrid" id="grid" role="list"></div>
      <aside class="card dexprev" id="prev" aria-live="polite"></aside>
    </div></div>`;

  const $ = (s) => root.querySelector(s);
  $('#bandseg').innerHTML = seg('Player level', [['average', 'Average'], ['skilled', 'Skilled']], st.band);

  const sorted = (data.species || []).slice().sort((a, b) => a.index - b.index);
  const hrefOf = (s) => `#/species/${s.id}`;

  // build tiles once, update fill/number in place
  $('#grid').innerHTML = sorted.map((s) => `<a role="listitem" class="tile" data-id="${esc(s.id)}" href="${hrefOf(s)}">
      ${icon(s.id, 2)}<span class="nm">${esc(s.name === s.minamoto ? nice(s.id) : nice(s.id))}</span><span class="rt num"></span></a>`).join('');
  const tiles = new Map([...root.querySelectorAll('.tile')].map((a) => [a.dataset.id, a]));

  function rowFor(id) { return rows.get(`${id}|${stages[st.stage].id}|${st.band}`); }

  function matches(s) {
    if (st.type && !uniqTypes(s).includes(st.type)) return false;
    const q = st.q.trim().toLowerCase();
    if (!q) return true;
    return nice(s.id).toLowerCase().includes(q) || String(s.name || '').toLowerCase().includes(q) ||
      String(s.minamoto || '').toLowerCase().includes(q) || String(s.dex) === q || String(s.dex).padStart(3, '0') === q;
  }

  function paint() {
    $('#stagename').textContent = stages[st.stage].name;
    for (const s of sorted) {
      const a = tiles.get(s.id); const r = rowFor(s.id); const c = r ? strength(r.z) : null;
      a.className = 'tile' + (r ? '' : ' na') + (matches(s) ? '' : ' dim') + (st.hover === s.id ? ' sel' : '');
      a.style.background = c ? c.fill : '';
      a.querySelector('.rt').textContent = r ? Math.round(r.rating) : '--';
      a.setAttribute('aria-label', `${nice(s.id)}, ${r ? Math.round(r.rating) + ' strength, ' + (c ? c.word : '') : 'no data at this stage'}`);
      a.tabIndex = matches(s) ? 0 : -1;
    }
    preview();
  }

  function preview() {
    const id = st.hover || (sorted.find(matches) || sorted[0]).id;
    const s = ix.sp.get(id); const r = rowFor(id); const c = r ? strength(r.z) : null;
    const m = r ? half(r.ci) : null;
    const mk = r ? clamp(50 + r.z * 18, 2, 98) : 50;
    $('#prev').innerHTML = `<div class="cardh cy"><h3>${esc(nice(id))}</h3><span class="px" style="margin-left:auto">#${String(s.dex).padStart(3, '0')}</span></div>
      <div class="cardb col g12">
        <div class="well" style="height:164px">${front(id, 2)}</div>
        <div class="flex jb ac"><span class="flex g4">${uniqTypes(s).map((t) => typeChip(t, 2)).join('')}</span>
          <span class="chip ${c ? c.cls : 'c-dead'}">${esc(c ? c.word : 'No data at this stage')}</span></div>
        ${r ? `<div><span class="num bigrt">${Math.round(r.rating)}</span> <span class="mut">strength</span></div>
          <div class="gauge ${c && (c.word.startsWith('Much') ? 'red' : c.word === 'Strong' ? 'amb' : '')}"><div class="band" style="left:35%;width:30%"></div><div class="mk" style="left:${mk}%"></div></div>
          <div class="gscale"><span>weak</span><span>fair range</span><span>strong</span></div>
          <div class="mut" style="font-size:13px">Wins <b class="num">${Math.round(r.win_rate * 100)}%</b>${m != null ? ` ± ${Math.max(1, Math.round(m * 100))} pts` : ''} of its fights at ${esc(stages[st.stage].name)} (${st.band === 'skilled' ? 'skilled' : 'average'} player, ${r.n} fights).</div>`
          : `<div class="mut">The run has no fights for it at this point of the game.</div>`}
        <a class="btn" href="${hrefOf(s)}">Open species page</a>
      </div>`;
  }

  // events
  $('#stage').addEventListener('input', (e) => { st.stage = Number(e.target.value); store('dexStage', st.stage); paint(); });
  $('#q').addEventListener('input', debounce((e) => { st.q = e.target.value; paint(); }, 80));
  const onBand = (k) => {
    st.band = k; setBand(k);
    $('#bandseg').innerHTML = seg('Player level', [['average', 'Average'], ['skilled', 'Skilled']], k);
    bindSeg($('#bandseg'), onBand); paint();
  };
  bindSeg($('#bandseg'), onBand);
  root.querySelectorAll('.tchip').forEach((b) => b.addEventListener('click', () => {
    st.type = st.type === b.dataset.type ? '' : b.dataset.type;
    root.querySelectorAll('.tchip').forEach((x) => { const on = x.dataset.type === st.type; x.classList.toggle('on', on); x.setAttribute('aria-pressed', on); });
    paint();
  }));
  const hov = (e) => { const a = e.target.closest('.tile'); if (a && a.dataset.id !== st.hover) { st.hover = a.dataset.id; tiles.forEach((t, k) => t.classList.toggle('sel', k === st.hover)); preview(); } };
  $('#grid').addEventListener('mouseover', hov);
  $('#grid').addEventListener('focusin', hov);
  root.addEventListener('keydown', (e) => { if (e.key === 'Escape' && st.hover) { st.hover = null; paint(); } });
  paint();
}
