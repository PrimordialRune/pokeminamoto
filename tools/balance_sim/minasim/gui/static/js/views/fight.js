// Fight detail (#/fight/<TRAINER>) and the fight list (#/fight).
const STARTERS = ['BULBASAUR', 'CHARMANDER', 'SQUIRTLE'];

export function render(root, ctx) {
  const { lib, report, data, navigate } = ctx;
  if (ctx.reportError) return void root.append(lib.errorState({ message: 'Could not load this run', hint: ctx.reportError.message }));
  if (!ctx.arg) return renderList(root, ctx);
  return renderFight(root, ctx);
}

/* ------------------------------------------------------------ list */
function renderList(root, ctx) {
  const { lib, report, data, navigate } = ctx;
  const { el, W } = lib;
  const fights = (report && report.fights) || [];
  root.append(el('div', { class: 'pagehead' }, el('div', { style: 'margin-right:auto' }, el('h1', null, 'Fights'), el('div', { class: 'mut' }, 'Every fight in the run. Pick one to see its team, costs and a sample replay.')),
    lib.btn('← Journey', { kind: 'sec2', sm: true, href: '#/journey' })));
  if (!report || !fights.length) {
    root.append(lib.emptyState({ title: report ? 'No fights in this run.' : 'No runs yet.', text: 'Start a run to see fights.', action: report ? null : lib.btn('Start your first run', { kind: 'go', onclick: () => document.querySelector('#top .btn.go')?.click() }) }));
    return;
  }
  const stIdx = Object.fromEntries(((report && report.stages) || []).map((s, i) => [s.id, i]));
  const rows = fights.map((f) => Object.assign({ avg: f.by_band && f.by_band.average, sk: f.by_band && f.by_band.skilled }, f));
  root.append(lib.card({ title: fights.length + ' fights', bare: true }, lib.sortableTable({
    rows, sort: { key: 'stage' }, id: (r) => r.trainer, onRow: (r) => navigate('#/fight/' + encodeURIComponent(r.trainer)),
    columns: [
      { key: 'stage', label: 'Stage', value: (r) => stIdx[r.stage] ?? 99, render: (r) => lib.stageName(data, report, r.stage) },
      { key: 'name', label: 'Fight', value: (r) => r.name, render: (r) => el('span', { class: 'flex ac g8' }, lib.trainerImg(r.trainer, data, 1, { class: 'sm32' }), el('a', { href: '#/fight/' + encodeURIComponent(r.trainer) }, el('b', null, r.name))) },
      { key: 'party', label: 'Team', render: (r) => el('span', { class: 'flex g4' }, (r.party || []).map((m) => lib.monImg(m.species, 'icon', 1, { title: lib.nice(m.species) + ' L' + m.level }))) },
      { key: 'class', label: 'Kind', value: (r) => r.class, render: (r) => el('span', { class: 'flex ac g8 wrap' }, W.cls[r.class] || r.class, r.class !== 'wild' ? lib.sourceChip(r.source) : null) },
      { key: 'avg', label: 'Average', num: true, value: (r) => r.avg && r.avg.win_rate, render: (r) => r.avg ? lib.pct(r.avg.win_rate) + ' ' : '–' },
      { key: 'sk', label: 'Skilled', num: true, value: (r) => r.sk && r.sk.win_rate, render: (r) => r.sk ? lib.pct(r.sk.win_rate) : '–' },
      { key: 'want', label: 'Wanted', render: (r) => el('span', { style: 'white-space:nowrap' }, lib.range(r.target)) },
      { key: 'verdict', label: 'Verdict', value: (r) => (r.avg ? lib.fightVerdict(r.avg.win_rate, r.target)[0] : ''), render: (r) => r.avg ? lib.fightVerdictChip(r.avg.win_rate, r.target) : '' },
    ] })));
}

/* ------------------------------------------------------------ detail */
function movesFor(data, species, level) {
  const sp = (data.species || []).find((s) => s.id === species);
  if (!sp) return [];
  const names = []; const seen = new Set();
  (sp.learnset.level || []).filter((e) => e.level <= level).reverse().forEach((e) => { if (!seen.has(e.move) && names.length < 4) { seen.add(e.move); names.push(e.move); } });
  return names.reverse();
}

function renderFight(root, ctx) {
  const { lib, report, data, navigate } = ctx;
  const { el, W } = lib;
  const id = ctx.arg;
  const f = ((report && report.fights) || []).find((x) => x.trainer === id);
  const tdata = (data.trainers || []).find((t) => t.id === id);
  if (!f && !tdata) {
    root.append(el('div', { class: 'pagehead' }, lib.btn('← Journey', { kind: 'sec2', sm: true, href: '#/journey' })),
      lib.emptyState({ title: 'Unknown fight.', text: lib.nice(id) + ' is not in this run. It may have been added or renamed since.', action: lib.btn('See all fights', { href: '#/fight' }) }));
    return;
  }
  const fight = f || { trainer: id, name: tdata.name, class: tdata.kind || 'trainer', party: tdata.party, by_band: {}, by_starter: {}, target: null };
  const a = fight.by_band && fight.by_band.average, s = fight.by_band && fight.by_band.skilled;
  const stage = ((report && report.stages) || []).find((x) => x.id === fight.stage);
  const place = fight.map ? lib.nice(fight.map.replace(/^MAP_/, '')) : '';
  const [vt, vc] = a ? lib.fightVerdict(a.win_rate, fight.target) : ['No result yet', 'dead'];

  root.append(el('div', { class: 'pagehead' },
    lib.btn('← Journey', { kind: 'sec2', sm: true, href: '#/journey' }),
    el('div', { class: 'flex ac g16 f1', style: 'min-width:300px' }, lib.well(lib.trainerImg(fight.trainer, data, 1), 96),
      el('div', null, el('h1', null, fight.name), el('div', { class: 'mut' }, [place, stage && 'Stage: ' + stage.name, stage && stage.badges + ' badges', W.cls[fight.class] || fight.class].filter(Boolean).join(' · ') + (fight.class === 'gym' && !lib.trainerPic(fight.trainer, data) ? '' : '')))),
    el('div', { class: 'flex g8 ac wrap' }, lib.sourceChip(fight.source), a ? lib.chip(vt + (vc === 'red' ? ' for an average player' : ''), vc, { style: 'font-size:15px;min-height:34px' }) : null),
    lib.btn('Run duel now', { kind: 'go', onclick: () => document.getElementById('duelpanel')?.scrollIntoView({ block: 'center' }) })));

  const left = el('div', { class: 'col', style: 'flex:2 1 720px;gap:20px;min-width:0' });
  const right = el('div', { class: 'col', style: 'flex:1 1 400px;gap:20px;min-width:0' });
  root.append(el('div', { style: 'display:flex;flex-wrap:wrap;gap:20px' }, left, right));

  // team
  left.append(lib.card({ title: fight.name + "'s team" },
    el('div', { class: 'flex wrap g16' }, (fight.party || []).map((m) => {
      const sp = (data.species || []).find((x) => x.id === m.species);
      const mv = (m.moves && m.moves.length ? m.moves : movesFor(data, m.species, m.level));
      return el('div', { class: 'partycard' }, lib.monImg(m.species, 'front', 2), el('b', { style: 'font-family:var(--font-d);font-size:18px' }, lib.nice(m.species)), el('span', { class: 'mut' }, 'Level ' + m.level),
        el('div', { class: 'flex g4' }, sp ? [...new Set(sp.types)].map((t) => lib.typeImg(t, 2)) : null),
        el('div', { class: 'flex g4 wrap', style: 'justify-content:center' }, mv.map((x) => lib.chip(lib.nice(x), 'info', { style: 'min-height:22px;font-size:11.5px' }))));
    }), el('div', { class: 'lcdbox f1', style: 'min-width:220px;align-self:flex-start;font-size:14px' }, 'Parties come from trainers.party. Edit the file, rebuild, run again to see the change.'))));

  // win rate
  const bandRow = (label, b) => {
    if (!b) return el('div', null, el('b', null, label), el('div', { class: 'mut' }, 'No result for this player level.'));
    const [t, c] = lib.fightVerdict(b.win_rate, fight.target);
    return el('div', null,
      el('div', { class: 'flex ac jb wrap', style: 'margin-bottom:6px;gap:4px 12px' }, el('b', null, label + ' player'), el('span', null, el('b', { class: 'num', style: 'font-size:24px' }, lib.pct(b.win_rate)), ' ', el('span', { class: 'mut', title: W.margin }, lib.pm(b.ci)), ' ', lib.chip(t, c))),
      lib.gauge({ value: b.win_rate, ci: b.ci, target: fight.target, ticks: false, aria: `${label} player wins ${lib.pct(b.win_rate)} ${lib.pm(b.ci)}, wanted ${lib.range(fight.target)}` }));
  };
  const starters = Object.entries(fight.by_starter || {});
  const rates = starters.map(([, v]) => v), spread = rates.length ? Math.max(...rates) - Math.min(...rates) : 0;
  const hi = starters.slice().sort((x, y) => y[1] - x[1])[0], lo = starters.slice().sort((x, y) => x[1] - y[1])[0];
  left.append(lib.card({ title: 'Chance to win on the first try', tone: 'cy', right: '± = margin of error' },
    el('div', { class: 'col g16' }, bandRow('Average', a), bandRow('Skilled', s),
      fight.target ? el('div', { class: 'gscale' }, el('span', null, '0%'), el('span', null, 'wanted ' + lib.range(fight.target)), el('span', null, '100%')) : null,
      starters.length ? el('h3', { style: 'font-size:16px' }, 'By starter (average player)') : null,
      starters.map(([sp, v]) => { const [t, c] = lib.fightVerdict(v, fight.target);
        return el('div', { class: 'flex ac g12' }, lib.well(lib.monImg(sp, 'front', 1), 72),
          el('div', { class: 'f1' }, el('div', { class: 'flex jb' }, el('b', null, lib.nice(sp)), el('span', { class: 'num' }, el('b', null, lib.pct(v)), ' ', lib.chip(t, c))),
            lib.gauge({ value: v, target: fight.target, ticks: false, aria: `${lib.nice(sp)} wins ${lib.pct(v)}` }))); }),
      starters.length > 1 ? el('div', { class: 'legend' }, el('span', null, `${lib.nice(lo[0])} is ${Math.round(spread * 100)} points behind ${lib.nice(hi[0])}. Wanted gap: under 15.`)) : null)));

  // cost
  if (a) {
    const costs = [
      ['HP lost', a.hp_cost, lib.pct(a.hp_cost), 'of team HP', 'var(--coral)', lib.itemImg('POTION')],
      ['TP used', a.tp_cost, lib.pct(a.tp_cost), 'of team TP', 'var(--cyan)', lib.itemImg('ETHER')],
      ['Items used', Math.min(1, (a.items_used || 0) / 3), (a.items_used ?? 0).toFixed(1), 'per fight', 'var(--lcd)', lib.itemImg('FULL_HEAL')],
      ['Blackout', a.blackout, lib.pct(a.blackout), 'of tries', 'var(--ink)', lib.monImg('GASTLY', 'icon', 1, { alt: '' })],
    ];
    left.append(lib.card({ title: 'What the fight costs', tone: 'lcd', right: 'average of all tries' },
      el('div', { style: 'display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:16px' }, costs.map(([l, fr, v, note, col, ic]) =>
        el('div', { class: 'col g8' }, el('div', { class: 'flex ac g8' }, ic, el('b', null, l)), lib.bar(fr ?? 0, col), el('div', { class: 'num' }, el('b', { style: 'font-size:20px' }, v), ' ', el('span', { class: 'mut' }, note)))))));
  }

  right.append(replayCard(ctx, fight, stage), duelCard(ctx, fight, stage, a));
}

/* ------------------------------------------------------------ replay + duel */
function effText(e) {
  if (e == null) return '';
  if (typeof e === 'string') return /super/i.test(e) ? ' It\'s super effective!' : /not|resist/i.test(e) ? ' It\'s not very effective.' : /no ?effect|immune/i.test(e) ? ' It had no effect.' : '';
  return e > 1 ? ' It\'s super effective!' : e === 0 ? ' It had no effect.' : e < 1 ? ' It\'s not very effective.' : '';
}

function buildSteps(res) {
  const log = res.sample_log || [], sm = res.sample || {};
  const side = [sm.player || [], sm.foe || []];
  const hp = side.map((arr) => arr.map((m) => ({ hp: m.max_hp, max: m.max_hp })));
  const active = [0, 0];
  const nameOf = (sd, i) => (side[sd][i] ? lib_nice(side[sd][i].species) : '?');
  const lib_nice = (x) => String(x || '').toLowerCase().replace(/(^|[_ ])(\w)/g, (m, p, c) => (p ? ' ' : '') + c.toUpperCase());
  const steps = [{ txt: 'The fight begins.', tag: 'start', tone: 'dead', snap: snap(), msg: ((side[1][0] ? lib_nice(side[1][0].species).toUpperCase() : 'The foe') + ' appears!') }];
  function snap() { return { hp: hp.map((a) => a.map((m) => ({ ...m }))), active: active.slice(), turn: 0 }; }
  log.forEach((e) => {
    const sd = e.side === 0 || e.side === 'player' ? 0 : 1, tg = 1 - sd;
    if (e.slot != null) active[sd] = e.slot;
    const actor = nameOf(sd, active[sd]).toUpperCase(), target = nameOf(tg, active[tg]).toUpperCase();
    let txt, msg, tag = '', tone = 'info';
    if (e.action === 'switch') { txt = `${actor} comes in.`; msg = `${sd ? 'Foe' : 'Go,'} ${actor}!`; tag = 'switch'; tone = 'dead'; }
    else if (e.action === 'item') { txt = `${sd ? 'Foe' : 'You'} used an item.`; msg = txt; tag = 'item'; tone = 'ok'; }
    else if (e.action === 'talk') { txt = `${sd ? 'Foe' : 'You'} talked: ${e.kind || e.move || ''}`.trim(); msg = txt; tag = 'talk'; tone = 'ok'; }
    else {
      const mv = lib_nice(e.move);
      msg = `${actor} used ${mv.toUpperCase()}!`;
      if (e.miss) { msg += ' But it missed!'; tag = 'miss'; tone = 'dead'; }
      else { msg += effText(e.effectiveness) + (e.crit ? ' A critical hit!' : ''); }
      txt = `${lib_nice(nameOf(sd, active[sd]))} uses ${mv}.` + (e.miss ? ' Missed.' : effText(e.effectiveness).replace(/^ It's/, ' ').replace('!', '.') + (e.crit ? ' Critical hit.' : ''));
    }
    if (e.damage && !e.miss) {
      const m = hp[tg][active[tg]]; if (m) m.hp = Math.max(0, m.hp - e.damage);
      tag = '-' + e.damage; tone = e.crit ? 'red' : sd ? 'amb' : 'info';
    }
    if (e.ko) {
      const m = hp[tg][active[tg]]; if (m) m.hp = 0;
      tag = 'KO'; tone = 'red'; msg += ` ${target} fainted!`; txt += ` ${lib_nice(target)} faints.`;
      const nx = hp[tg].findIndex((m2) => m2.hp > 0); if (nx >= 0) active[tg] = nx;
    }
    const st = snap(); st.turn = e.turn;
    steps.push({ txt, msg, tag, tone, snap: st, turn: e.turn });
  });
  const w = res.sample && res.sample.winner;
  steps.push({ txt: w === 0 ? 'You win the fight.' : w === 1 ? 'Blackout.' : 'Draw (turn cap).', tag: w === 0 ? 'won' : w === 1 ? 'lost' : 'draw', tone: w === 0 ? 'ok' : 'red', snap: snap(), msg: w === 0 ? 'You won the fight!' : w === 1 ? 'You are out of mons. You whited out!' : 'The fight ran out of turns.' });
  return { steps, side };
}

function replayCard(ctx, fight, stage) {
  const { lib } = ctx; const { el } = lib;
  const box = el('div', { class: 'cardb col g12' });
  const c = lib.card({ title: 'Replay a sample fight', bare: true }, box);
  let res = null, i = 0;
  function empty(msg) {
    lib.clear(box);
    box.append(frame(null), lib.lcdBox(msg || 'Run a duel (right) to get a sample fight to step through.'));
  }
  function frame(st, steps) {
    const foe0 = (res && res.sample && res.sample.foe && res.sample.foe[0]) || fight.party[0];
    const pl0 = (res && res.sample && res.sample.player && res.sample.player[0]) || { species: 'CHARMANDER', level: '?' };
    const snap = st ? st.snap : null;
    const hpOf = (sd, k, m) => (snap && snap.hp[sd][snap.active[sd]] ? snap.hp[sd][snap.active[sd]] : { hp: m.max_hp || 1, max: m.max_hp || 1 });
    const foeM = snap ? ((res.sample.foe || [])[snap.active[1]] || foe0) : foe0, plM = snap ? ((res.sample.player || [])[snap.active[0]] || pl0) : pl0;
    const fh = snap ? hpOf(1) : { hp: 1, max: 1 }, ph = snap ? hpOf(0) : { hp: 1, max: 1 };
    const bar = (h) => el('div', { class: 'hp' }, el('b', { style: `width:${(h.hp / (h.max || 1)) * 100}%;background:${h.hp / (h.max || 1) > 0.5 ? '#6DC155' : h.hp > 0 ? '#F2C660' : '#E15F69'}` }));
    return el('div', { class: 'battle', role: 'img', 'aria-label': 'Battle: ' + lib.nice(plM.species) + ' against ' + lib.nice(foeM.species) },
      el('div', { class: 'box', style: 'right:14px;top:10px' }, el('div', { class: 'l' }, el('span', null, lib.nice(foeM.species).toUpperCase()), el('span', null, 'L' + foeM.level)), bar(fh)),
      el('div', { style: 'position:absolute;right:34px;top:48px' }, lib.monImg(foeM.species, 'front', 2, { alt: '' })),
      el('div', { class: 'box', style: 'left:14px;top:12px' }, el('div', { class: 'l' }, el('span', null, lib.nice(plM.species).toUpperCase()), el('span', null, 'L' + plM.level)), bar(ph), el('div', { class: 'l', style: 'font-size:11px' }, el('span', null, snap ? `HP ${ph.hp}/${ph.max}` : ''))),
      el('div', { style: 'position:absolute;left:18px;top:78px' }, lib.monImg(plM.species, 'back', 2, { alt: '' })));
  }
  function draw(r) {
    res = r; i = 0;
    const { steps } = buildSteps(r);
    const body = () => {
      lib.clear(box);
      const st = steps[i];
      const go = (n) => { i = Math.max(0, Math.min(steps.length - 1, n)); body(); };
      box.append(frame(st, steps), lib.lcdBox(st.msg),
        el('div', { class: 'flex ac g8 wrap' },
          lib.btn('←', { kind: 'sec2', sm: true, 'aria-label': 'Previous turn', onclick: () => go(i - 1) }), lib.btn('Next turn', { sm: true, onclick: () => go(i + 1) }),
          lib.btn('New sample', { kind: 'sec2', sm: true, onclick: () => replayRef.newSample && replayRef.newSample() }),
          el('span', { class: 'mut', style: 'margin-left:auto;font-size:13px' }, `Step ${i} of ${steps.length - 1} · seed ${r.seed ?? 1}`)),
        el('div', { style: 'max-height:260px;overflow:auto;position:relative;border:2px solid var(--line);border-radius:6px', role: 'list' }, steps.map((s, k) =>
          el('button', { type: 'button', class: 'logrow' + (k === i ? ' on' : ''), role: 'listitem', 'aria-current': k === i ? 'step' : null, onclick: () => go(k) },
            el('span', { class: 'px', style: 'width:28px' }, s.turn ?? ''), el('span', { class: 'f1' }, s.txt), s.tag ? lib.chip(s.tag, s.tone, { style: 'min-height:22px' }) : null))));
      const on = box.querySelector('.logrow.on'); if (on) { const p = on.parentNode; p.scrollTop = on.offsetTop - p.clientHeight / 2; }
    };
    body();
  }
  c.setSample = draw; c.setEmpty = empty;
  replayRef.card = c;
  empty();
  return c;
}

function duelCard(ctx, fight, stage, a) {
  const { lib, data } = ctx; const { el } = lib;
  const lvls = (fight.party || []).map((m) => m.level);
  const defLvl = Math.max(2, Math.round(lvls.reduce((x, y) => x + y, 0) / Math.max(1, lvls.length)) - 1);
  const st = { starter: 'CHARMANDER', level: defLvl, n: 500, band: lib.getBand(ctx.params), running: false };
  const out = el('div', { style: 'padding:12px;border:3px dashed var(--ink);border-radius:8px' });
  const engineOff = ctx.status && ctx.status.engine === false;
  const engineMsg = () => lib.errorState({ message: 'The simulator is not built yet.', hint: 'Run `make balance-lib`, then reload.' });
  const runBtn = lib.btn('Run now', { kind: 'go', onclick: () => run(st.n) });
  const body = el('div', { class: 'cardb col g12' });
  const c = lib.card({ title: 'Run this duel now', tone: 'cy', id: 'duelpanel', bare: true }, body);
  function paint() {
    lib.clear(body);
    body.append(
      el('div', null, el('div', { class: 'px mut', style: 'margin-bottom:6px' }, 'Your starter'),
        lib.seg({ label: 'Starter', value: st.starter, onChange: (v) => { st.starter = v; }, options: STARTERS.map((s) => ({ value: s, node: el('span', { class: 'flex ac g8' }, lib.monImg(s, 'icon', 1, { alt: '' }), lib.nice(s)) })) })),
      el('div', { class: 'flex g12 wrap' },
        el('label', { class: 'field', style: 'flex:1' }, el('span', { class: 'px' }, 'Level'), el('input', { type: 'number', min: 2, max: 100, value: st.level, 'aria-label': 'Level', onchange: (e) => { st.level = Math.max(2, Math.min(100, +e.target.value || 5)); } })),
        el('label', { class: 'field', style: 'flex:1' }, el('span', { class: 'px' }, 'Fights'), el('select', { 'aria-label': 'Number of fights', onchange: (e) => { st.n = +e.target.value; } },
          [100, 500, 2000, 10000].map((n) => el('option', { value: n, selected: n === st.n }, n.toLocaleString('en-US'))))) ),
      lib.bandSeg(st.band, (b) => { st.band = b; lib.setBand(b); }),
      runBtn, out);
    if (engineOff) { runBtn.disabled = true; showMsg(engineMsg()); }
    else showMsg(el('div', { class: 'mut', style: 'font-size:13px' }, 'Pick a starter and press Run now. Needs the simulator built (make balance-lib).'));
  }
  const showMsg = (n) => { lib.clear(out); out.append(n); };
  async function run(n, sampleOnly) {
    if (st.running) return;
    st.running = true; runBtn.disabled = true;
    showMsg(el('div', null, el('div', { class: 'flex jb' }, el('span', { class: 'px' }, 'Running'), el('span', { class: 'mut', style: 'font-size:13px' }, n.toLocaleString('en-US') + ' fights')), el('div', { class: 'bar indet', style: 'margin:8px 0' }, el('b', { style: 'left:0;width:40%;background:var(--lcd)' }))));
    const seed = sampleOnly ? Math.floor(Math.random() * 1e6) : 1;
    const known = (data.trainers || []).some((t) => t.id === fight.trainer);
    const body_ = { player: [{ species: st.starter, level: st.level }], n, seed, player_policy: st.band === 'skilled' ? 'skilled' : 'average', foe_policy: 'rom' };
    if (known) body_.trainer = fight.trainer; else body_.foe = (fight.party || []).map((m) => ({ species: m.species, level: m.level, moves: m.moves && m.moves.length ? m.moves : undefined }));
    const r = await lib.runDuel(body_);
    st.running = false; runBtn.disabled = engineOff;
    if (!r.ok) { showMsg(r.engineMissing ? engineMsg() : lib.errorState({ message: r.message, hint: r.hint })); return; }
    const wr = r.win_rate ?? (r.wins / r.n);
    const dr = r.draws || 0, ls = r.losses ?? (r.n - r.wins - dr);
    showMsg(el('div', null,
      el('div', { class: 'flex jb ac' }, el('span', { class: 'px' }, 'Result'), el('span', { class: 'mut', style: 'font-size:13px' }, `${r.wins} of ${r.n} fights`)),
      el('div', { class: 'resbar', style: 'margin:8px 0', role: 'img', 'aria-label': `${r.wins} won, ${ls} lost, ${dr} drawn` }, el('b', { style: `width:${r.wins / r.n * 100}%;background:var(--grass)` }), el('b', { style: `width:${dr / r.n * 100}%;background:var(--amb-t)` }), el('b', { style: `width:${ls / r.n * 100}%;background:var(--coral)` })),
      el('div', null, el('b', { class: 'num', style: 'font-size:26px' }, lib.pct(wr)), ' ', el('span', { class: 'mut', title: lib.W.margin }, lib.pm(r.ci) + (r.turns ? ' · avg ' + r.turns + ' turns' : ''))),
      fight.target ? el('div', { class: 'mut', style: 'font-size:13px' }, 'Wanted ' + lib.range(fight.target)) : null));
    r.seed = seed;
    (replayRef.card || {}).setSample && replayRef.card.setSample(Object.assign({}, r, { sample: Object.assign({ winner: r.sample && r.sample.winner }, r.sample) }));
  }
  replayRef.newSample = () => run(1, true);
  paint();
  return c;
}
const replayRef = {};
