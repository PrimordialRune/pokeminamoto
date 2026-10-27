'use strict';
/* Minamoto data editor: four editors (learnsets, trainers, encounters, arenas) and the simulate buttons.
   Plain JS, no libraries. Everything it shows comes from /api (tools/editor/medit/server.py). */

const $ = (s, r = document) => r.querySelector(s);
function h(tag, attrs, ...kids) {
  const e = document.createElement(tag);
  for (const [k, v] of Object.entries(attrs || {})) {
    if (v == null || v === false) continue;
    if (k === 'class') e.className = v;
    else if (k.startsWith('on')) e.addEventListener(k.slice(2), v);
    else if (k === 'value') e.value = v;
    else if (k === 'checked') e.checked = !!v;
    else e.setAttribute(k, v === true ? '' : v);
  }
  for (const k of kids.flat(9)) if (k != null && k !== false) e.append(k.nodeType ? k : document.createTextNode(String(k)));
  return e;
}
async function api(path, body) {
  const r = await fetch('/api/' + path, body === undefined ? {} : { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(body) });
  const j = await r.json().catch(() => ({ error: 'bad response' }));
  if (!r.ok) { const e = new Error(j.error || r.statusText); e.data = j; throw e; }
  return j;
}
const clone = (x) => JSON.parse(JSON.stringify(x));
const same = (a, b) => JSON.stringify(a) === JSON.stringify(b);
const pct = (x) => Math.round(x * 100) + '%';
const title = (c) => c.replace(/_/g, ' ').toLowerCase().replace(/\b\w/g, (m) => m.toUpperCase());
function debounce(fn, ms) { let t; return (...a) => { clearTimeout(t); t = setTimeout(() => fn(...a), ms); }; }
function toast(msg, cls) {
  const t = h('div', { class: 'toast card' }, h('div', { class: 'cardb ' + (cls || '') }, msg));
  document.body.append(t); setTimeout(() => t.remove(), 4500);
}

const S = { lists: null, status: null };
const TABS = [['species', 'Learnsets'], ['trainers', 'Trainers'], ['encounters', 'Encounters'], ['arena', 'Arenas'], ['saved', 'Saved edits']];

/* ------------------------------------------------------------------ shell */
function drawTop(route) {
  const t = $('#top'); t.textContent = '';
  t.append(h('div', { class: 'logo' }, h('i'), 'Minamoto Data Editor'),
    h('nav', { class: 'tabs' }, TABS.map(([k, l]) => h('a', { class: 'tab' + (k === route ? ' on' : ''), href: '#/' + k }, l,
      k === 'saved' && S.status && S.status.pending.length ? h('span', { class: 'chip c-amb' }, S.status.pending.length) : null))),
    h('div', { class: 'runpick mut', title: 'Saves go here; the repo is untouched unless the server runs with --in-place' },
      S.status ? (S.status.inPlace ? 'writes the repo' : 'saves to build/editor/out') : ''));
}
async function refreshStatus() { S.status = await api('status'); }

const VIEWS = { species: viewSpecies, trainers: viewTrainers, encounters: viewEncounters, arena: viewArena, saved: viewSaved };
async function route() {
  const r = (location.hash.replace(/^#\/?/, '') || 'species').split('/');
  const name = VIEWS[r[0]] ? r[0] : 'species';
  try { if (!S.lists) S.lists = await api('lists'); await refreshStatus(); } catch (e) { $('#view').textContent = 'Server error: ' + e.message; return; }
  drawTop(name);
  const v = $('#view'); v.textContent = '';
  try { await VIEWS[name](v, r[1] && decodeURIComponent(r[1])); } catch (e) { v.append(h('div', { class: 'lcdbox' }, 'Could not load: ' + e.message)); }
}
window.addEventListener('hashchange', route);
route();

/* ------------------------------------------------------------------ shared widgets */
function card(headCls, title_, body, foot) {
  return h('section', { class: 'card' }, h('div', { class: 'cardh ' + (headCls || '') }, h('h3', null, title_)), h('div', { class: 'cardb' }, body), foot || null);
}
function listCol(titleText, items, cur, mk, opts = {}) {
  const q = h('input', { type: 'search', placeholder: 'filter', 'aria-label': 'filter', class: 'w100' });
  const box = h('div', { class: 'cardb' });
  const draw = () => {
    box.textContent = '';
    const f = q.value.trim().toLowerCase();
    let lastGroup = null;
    for (const it of items) {
      const lab = mk(it);
      if (f && !lab.text.toLowerCase().includes(f)) continue;
      if (lab.group !== undefined && lab.group !== lastGroup) { lastGroup = lab.group; box.append(h('div', { class: 'grp px mut' }, lab.group || 'no block')); }
      box.append(h('a', { class: 'li' + (lab.id === cur ? ' on' : ''), href: '#/' + opts.route + '/' + encodeURIComponent(lab.id) },
        lab.icon ? h('img', { src: lab.icon, alt: '', loading: 'lazy' }) : null, h('span', null, lab.title, lab.sub ? h('small', null, lab.sub) : null)));
    }
  };
  q.addEventListener('input', draw); draw();
  return h('aside', { class: 'listcol card' }, h('div', { class: 'cardh lcd' }, h('h3', null, titleText)), h('div', { style: 'padding:8px' }, q), box);
}
function select(options, value, onchange, opts = {}) {
  const s = h('select', { 'aria-label': opts.label || 'choose', onchange: (e) => onchange(e.target.value) });
  const all = options.includes(value) || value == null || value === '' ? options : [value, ...options];
  if (opts.blank) s.append(h('option', { value: '' }, opts.blank));
  for (const o of all) s.append(h('option', { value: o, selected: o === value }, opts.fmt ? opts.fmt(o) : o));
  if (value === '' || value == null) s.value = '';
  return s;
}
function numInput(v, onchange, o = {}) {
  return h('input', { type: 'number', value: v, min: o.min, max: o.max, step: o.step || 1, 'aria-label': o.label || 'number', onchange: (e) => onchange(Number(e.target.value)) });
}
function errorsBox(errs) {
  if (!errs || !errs.length) return h('div', { class: 'okline' }, 'Passes the check_data rules');
  const nw = errs.filter((e) => e.new);
  return h('div', { class: 'errs' },
    h('div', { class: nw.length ? 'dirty' : 'mut' }, nw.length ? nw.length + ' new problem' + (nw.length > 1 ? 's' : '') + ' (same wording as tools/check_data.py)' : 'Only problems the repo already has (tools/check_data_known.json)'),
    errs.map((e) => h('div', { class: 'eerr ' + (e.new ? 'new' : 'known') }, h('b', null, e.new ? 'new' : 'known'), '[' + e.check + '] ', e.message)));
}
function gauge(cell) {
  const [lo, hi] = cell.target;
  const bad = cell.verdict !== 'in band';
  return h('div', { class: 'egauge', role: 'img', 'aria-label': `${pct(cell.win)} won, wanted ${pct(lo)} to ${pct(hi)}` },
    h('div', { class: 'tb', style: `left:${lo * 100}%;width:${(hi - lo) * 100}%` }),
    cell.ci ? h('div', { class: 'ci', style: `left:${cell.ci[0] * 100}%;width:${(cell.ci[1] - cell.ci[0]) * 100}%` }) : null,
    h('div', { class: 'mk' + (bad ? ' bad' : ''), style: `left:${Math.min(100, cell.win * 100)}%` }));
}
function verdictChip(v) {
  const cls = v === 'in band' ? 'c-ok' : v === 'too hard' ? 'c-red' : 'c-amb';
  return h('span', { class: 'chip ' + cls }, v);
}
function bandRows(cells, label) {
  return Object.entries(cells).map(([k, c]) => h('div', { class: 'srow' },
    h('div', null, h('b', null, k.replace('/', ' / ')), h('div', { class: 'mut' }, typeof c.n === 'number' ? 'n = ' + c.n : c.n)),
    gauge(c),
    h('div', null, h('span', { class: 'num' }, pct(c.win)), ' ', c.ci ? h('span', { class: 'mut num' }, `(${pct(c.ci[0])}-${pct(c.ci[1])}) `) : null, verdictChip(c.verdict))));
}
function simBox() {
  const box = h('div', { class: 'sim' });
  return {
    box,
    busy(msg) { box.textContent = ''; box.append(h('div', { class: 'lcdbox' }, msg || 'Simulating with the real battle code...')); },
    fail(e) { box.textContent = ''; box.append(h('div', { class: 'eerr new' }, 'Simulation failed: ' + e.message + (e.data && e.data.hint ? '  (' + e.data.hint + ')' : ''))); },
    show(...kids) { box.textContent = ""; box.append(...kids.flat(9).filter(Boolean)); },
  };
}
async function runSim(sim, fn, render) {
  sim.busy();
  try { const r = await fn(); if (r.error) { sim.fail({ message: r.error, data: r }); return; } sim.show(render(r)); } catch (e) { sim.fail(e); }
}
function actionBar(state, { onSave, onRevert, onSim, simLabel }) {
  const msg = h('span', { class: 'mut' });
  state.setMsg = (m, cls) => { msg.textContent = m; msg.className = cls || 'mut'; };
  return h('div', { class: 'ebar' },
    h('button', { class: 'btn go', onclick: onSave }, 'Save'),
    h('button', { class: 'btn sec2', onclick: onRevert }, 'Revert'),
    h('button', { class: 'btn', onclick: onSim }, simLabel),
    msg);
}
async function doSave(state, path, body, after) {
  try {
    const r = await api(path + '/save', body);
    state.errs = r.errors; state.drawErrs();
    if (r.saved) { state.setMsg('Saved to ' + r.path.replace(/^.*\/build\//, 'build/'), 'okline'); if (after) after(); await refreshStatus(); drawTop(location.hash.split('/')[1] || 'species'); }
    else state.setMsg('Not saved: fix the new problems first', 'dirty');
  } catch (e) { state.setMsg('Save failed: ' + e.message, 'dirty'); }
}

/* ------------------------------------------------------------------ learnsets */
async function viewSpecies(v, name) {
  const list = await api('species');
  const view = h('div', { class: 'layout' });
  v.append(view);
  view.append(listCol('Species', list, name, (s) => ({ id: s.species, text: s.species + ' ' + s.name, title: title(s.species), sub: '#' + s.dex + '  ' + s.types.join('/') + '  ' + s.moves + ' moves', icon: '/sprites/pokemon/' + s.species + '/icon.png' }), { route: 'species' }));
  const col = h('div', { class: 'editcol' }); view.append(col);
  if (!name) { col.append(h('div', { class: 'lcdbox' }, 'Pick a species to edit its level-up moves, evolutions and TM/HM list.')); return; }
  const orig = await api('species/' + name);
  const cur = { levelUpMoves: clone(orig.levelUpMoves || []), evolutions: clone(orig.evolutions || []), tmhm: clone(orig.tmhm || []), tutor: orig.tutor, eggMoves: orig.eggMoves };
  const editKeys = ['levelUpMoves', 'evolutions', 'tmhm'];
  const edit = () => Object.fromEntries(editKeys.filter((k) => typeof cur[k] !== 'string').map((k) => [k, cur[k]]));
  const state = { errs: orig.errors };
  const errHost = h('div'); state.drawErrs = () => { errHost.textContent = ''; errHost.append(errorsBox(state.errs)); };
  const dirtyChip = h('span', { class: 'dirty' });
  const changed = debounce(async () => {
    dirtyChip.textContent = editKeys.some((k) => !same(cur[k], orig[k] || [])) ? 'edited' : '';
    try { state.errs = (await api('species/' + name + '/validate', { edit: edit() })).errors; state.drawErrs(); } catch (e) { /* server stopped */ }
  }, 250);
  const body = h('div');
  const L = S.lists;
  const draw = () => {
    body.textContent = '';
    const linked = typeof cur.levelUpMoves === 'string';
    body.append(h('h3', null, 'Level-up moves'));
    if (linked) body.append(h('p', { class: 'mut' }, 'Shares the learnset of ' + cur.levelUpMoves.slice(1) + ' (an "@" link): edit that species instead.'));
    if (!linked) {
      const t = h('table', { class: 'ed' }, h('tr', null, h('th', null, 'Level'), h('th', null, 'Move'), h('th')));
      cur.levelUpMoves.forEach((row, i) => t.append(h('tr', null,
        h('td', null, numInput(row[0], (x) => { row[0] = x; changed(); }, { min: 1, max: 100, label: 'level' })),
        h('td', null, select(L.moves, row[1], (x) => { row[1] = x; changed(); }, { fmt: title, label: 'move' })),
        h('td', null, h('button', { class: 'btn sm sec2', 'aria-label': 'remove move', onclick: () => { cur.levelUpMoves.splice(i, 1); draw(); changed(); } }, 'Remove')))));
      body.append(t, h('div', { class: 'row' }, h('button', { class: 'btn sm sec2', onclick: () => { cur.levelUpMoves.push([1, L.moves[0]]); draw(); changed(); } }, 'Add move'),
        h('button', { class: 'btn sm sec2', title: 'Order by level (stable)', onclick: () => { cur.levelUpMoves.sort((a, b) => a[0] - b[0]); draw(); changed(); } }, 'Sort by level')));
    }
    body.append(h('h3', { style: 'margin-top:18px' }, 'Evolutions'));
    if (typeof cur.evolutions === 'string') body.append(h('p', { class: 'mut' }, 'Linked: ' + cur.evolutions));
    else {
      cur.evolutions.forEach((ev, i) => body.append(h('div', { class: 'row' },
        select(L.evoMethods, ev.method, (x) => { ev.method = x; changed(); }, { label: 'method' }),
        typeof ev.param === 'number' || ev.method === 'LEVEL' ? numInput(typeof ev.param === 'number' ? ev.param : 16, (x) => { ev.param = x; changed(); }, { min: 1, max: 100, label: 'level' })
          : h('input', { type: 'text', value: ev.param || '', 'aria-label': 'item', placeholder: 'ITEM name', onchange: (e) => { ev.param = e.target.value.toUpperCase(); changed(); } }),
        '→', select(L.species, ev.into, (x) => { ev.into = x; changed(); }, { fmt: title, label: 'evolves into' }),
        h('button', { class: 'btn sm sec2', onclick: () => { cur.evolutions.splice(i, 1); draw(); changed(); } }, 'Remove'))));
      body.append(h('button', { class: 'btn sm sec2', onclick: () => { cur.evolutions.push({ method: 'LEVEL', param: 16, into: L.species[0] }); draw(); changed(); } }, 'Add evolution'));
    }
    body.append(h('h3', { style: 'margin-top:18px' }, 'TM / HM'));
    if (typeof cur.tmhm === 'string') body.append(h('p', { class: 'mut' }, 'Linked: ' + cur.tmhm));
    else body.append(h('div', { class: 'row' }, L.tmhm.map((m) => {
      const on = cur.tmhm.includes(m);
      const cb = h('input', { type: 'checkbox', checked: on, onchange: (e) => { cur.tmhm = e.target.checked ? L.tmhm.filter((x) => cur.tmhm.includes(x) || x === m) : cur.tmhm.filter((x) => x !== m); draw(); changed(); } });
      return h('label', { class: 'chk' + (on ? ' on' : '') }, cb, title(m));
    })));
    body.append(h('p', { class: 'mut', style: 'margin-top:12px' }, 'Not edited here: tutor slots (' + (Array.isArray(cur.tutor) ? cur.tutor.length : 0) + ') and egg moves (' + (Array.isArray(cur.eggMoves) ? cur.eggMoves.length : 0) + ').'));
  };
  draw();
  const sim = simBox();
  const revert = () => { cur.levelUpMoves = clone(orig.levelUpMoves || []); cur.evolutions = clone(orig.evolutions || []); cur.tmhm = clone(orig.tmhm || []); draw(); changed(); };
  const bar = actionBar(state, {
    onSave: () => doSave(state, 'species/' + name, { edit: edit() }, () => Object.assign(orig, clone(edit()))),
    onRevert: revert, simLabel: 'Simulate the chapter',
    onSim: () => runSim(sim, () => api('species/' + name + '/simulate', { edit: edit(), runs: 40 }), renderSpeciesSim),
  });
  state.drawErrs();
  col.append(card('cy', title(name) + ' · #' + (list.find((s) => s.species === name) || {}).dex, [dirtyChip, body, errHost], bar),
    card('lcd', 'Simulate this change', [h('p', { class: 'mut' }, 'Plays the Masara-to-Nibi journey (every starter, both player bands) with and without your edit. Foe moves of edited species follow your learnset.'), sim.box]));
}
function renderSpeciesSim(r) {
  const rows = Object.keys(r.after).map((id) => {
    const b = r.before[id].by, a = r.after[id].by;
    const avg = (by, band) => { const k = Object.keys(by).filter((x) => x.startsWith(band)); return k.reduce((s, x) => s + by[x].win, 0) / (k.length || 1); };
    const cell = (band) => { const d = avg(a, band) - avg(b, band); return h('td', { class: 'num' }, pct(avg(b, band)), ' → ', pct(avg(a, band)), ' ', h('span', { class: 'chip ' + (Math.abs(d) < .05 ? 'c-dead' : d < 0 ? 'c-red' : 'c-ok') }, (d >= 0 ? '+' : '') + Math.round(d * 100) + ' pts')); };
    return h('tr', null, h('td', null, r.after[id].name), cell('average'), cell('skilled'));
  });
  return [h('table', { class: 'ed' }, h('tr', null, h('th', null, 'Fight'), h('th', null, 'Average player (3 starters)'), h('th', null, 'Skilled player')), rows),
    h('p', { class: 'mut' }, 'First-try win rates before → after your edit; the dex of the Balance Lab has the per-starter detail.')];
}

/* ------------------------------------------------------------------ trainers */
async function viewTrainers(v, id) {
  const { trainers, blocks } = await api('trainers');
  const view = h('div', { class: 'layout' }); v.append(view);
  view.append(listCol('Trainers', trainers, id, (t) => ({ id: t.id, text: t.id + ' ' + t.name + ' ' + t.class, group: t.owner || '', title: t.name || t.id.replace('TRAINER_', ''), sub: t.class + (t.levels ? ' · Lv ' + t.levels[0] + (t.levels[1] !== t.levels[0] ? '-' + t.levels[1] : '') : '') + ' · ' + t.mons + ' mon' }), { route: 'trainers' }));
  const col = h('div', { class: 'editcol' }); view.append(col);
  if (!id) { col.append(h('div', { class: 'lcdbox' }, 'Pick a trainer. Blocks (' + blocks.join(', ') + ') are the world workstream\'s BEGIN/END regions of trainers.party; the markers and comments are never touched.')); return; }
  const orig = await api('trainers/' + id);
  const cur = clone(orig);
  const state = { errs: [] };
  const errHost = h('div'); state.drawErrs = () => { errHost.textContent = ''; errHost.append(errorsBox(state.errs)); };
  const dirty = h('span', { class: 'dirty' });
  const changed = debounce(async () => {
    dirty.textContent = same(strip(cur), strip(orig)) ? '' : 'edited';
    try { state.errs = (await api('trainers/' + id + '/validate', { trainer: cur })).errors; state.drawErrs(); } catch (e) { /* ignore */ }
  }, 250);
  const strip = (t) => ({ keys: t.keys, mons: t.mons.map((m) => ({ species: m.species, nick: m.nick, gender: m.gender, item: m.item, fields: m.fields, moves: m.moves })) });
  const L = S.lists;
  const body = h('div');
  const draw = () => {
    body.textContent = '';
    body.append(h('div', { class: 'egrid' }, cur.keys.map((kv) => h('label', { class: 'fld' }, h('span', { class: 'px mut' }, kv[0] || 'line'),
      kv[0] === 'Class' ? h('input', { type: 'text', value: kv[1], list: 'classes', onchange: (e) => { kv[1] = e.target.value; changed(); } })
        : h('input', { type: 'text', value: kv[1], onchange: (e) => { kv[1] = e.target.value; changed(); } })))),
      h('datalist', { id: 'classes' }, L.classes.map((c) => h('option', { value: c }))));
    cur.mons.forEach((m, i) => {
      const lv = m.fields.find((f) => f[0] === 'Level');
      body.append(h('div', { class: 'mon' }, h('h4', null, 'Pokémon ' + (i + 1)),
        h('div', { class: 'row' },
          h('label', { class: 'fld' }, h('span', { class: 'px mut' }, 'Species'), select(L.species.map((s) => title(s)), m.species, (x) => { m.species = x; changed(); }, { label: 'species' })),
          h('label', { class: 'fld' }, h('span', { class: 'px mut' }, 'Level'), numInput(lv ? Number(lv[1]) : 100, (x) => { if (lv) lv[1] = String(x); else m.fields.unshift(['Level', String(x)]); changed(); }, { min: 1, max: 100, label: 'level' })),
          h('label', { class: 'fld' }, h('span', { class: 'px mut' }, 'Held item'), h('input', { type: 'text', value: m.item || '', placeholder: 'none', list: 'items', onchange: (e) => { m.item = e.target.value.trim() || null; changed(); } })),
          h('label', { class: 'fld' }, h('span', { class: 'px mut' }, 'Nickname'), h('input', { type: 'text', value: m.nick || '', onchange: (e) => { m.nick = e.target.value.trim() || null; changed(); } }))),
        h('div', { class: 'row' }, m.fields.filter((f) => f[0] !== 'Level').map((f) => h('label', { class: 'fld' }, h('span', { class: 'px mut' }, f[0] || 'line'), h('input', { type: 'text', value: f[1], onchange: (e) => { f[1] = e.target.value; changed(); } })))),
        h('div', { class: 'px mut' }, 'Moves (none = what its level-up learnset gives)'),
        h('div', { class: 'moves' }, [0, 1, 2, 3].map((j) => select(L.moves.map(title), m.moves[j] || '', (x) => { if (x) m.moves[j] = x; else m.moves.splice(j, 1); m.moves = m.moves.filter(Boolean); changed(); draw(); }, { blank: '(none)', label: 'move ' + (j + 1) }))),
        h('div', { class: 'row' },
          h('button', { class: 'btn sm sec2', disabled: i === 0, onclick: () => { cur.mons.splice(i - 1, 0, cur.mons.splice(i, 1)[0]); draw(); changed(); } }, 'Up'),
          h('button', { class: 'btn sm sec2', onclick: () => { cur.mons.splice(i, 1); draw(); changed(); } }, 'Remove'))));
    });
    body.append(h('datalist', { id: 'items' }, L.items.map((x) => h('option', { value: title(x) }))),
      h('button', { class: 'btn sm sec2', disabled: cur.mons.length >= 6, onclick: () => { cur.mons.push({ species: 'Rattata', nick: null, gender: null, item: null, head: 'Rattata', fields: [['Level', '5']], moves: [] }); draw(); changed(); } }, 'Add Pokémon'));
  };
  draw();
  const sim = simBox();
  const revert = () => { Object.assign(cur, clone(orig)); draw(); changed(); };
  const bar = actionBar(state, {
    onSave: () => doSave(state, 'trainers/' + id, { trainer: cur }, () => { Object.assign(orig, clone(cur)); dirty.textContent = ''; }),
    onRevert: revert, simLabel: 'Simulate this fight',
    onSim: () => runSim(sim, () => api('trainers/' + id + '/simulate', { trainer: cur, n: 40, runs: 40 }), renderTrainerSim),
  });
  changed();
  col.append(card('', (cur.keys[0] ? cur.keys[0][1] : id) + ' · ' + id + (cur.owner ? ' · block ' + cur.owner : ''), [dirty, body, errHost], bar),
    card('lcd', 'Win rates against the band', [h('p', { class: 'mut' }, 'Real battle code, every starter, average and skilled players. The green stretch is the wanted band from targets.yaml.'), sim.box]));
}
function renderTrainerSim(r) {
  const out = [];
  if (r.problems && r.problems.length) out.push(h('div', { class: 'eerr new' }, r.problems.join('; ')));
  if (r.chapter) out.push(h('h3', null, 'In the chapter: ' + r.chapter.name), h('p', { class: 'mut' }, 'The Masara-to-Nibi journey with your party in place of the scenario\'s: the player arrives with the levels and party the journey gives them (arena included for gym fights).'),
    bandRows(Object.fromEntries(Object.entries(r.chapter.by).map(([k, c]) => [k, Object.assign({ verdict: verdictOf(c.win, c.ci, c.target) }, c)]))));
  out.push(h('h3', { style: 'margin-top:14px' }, 'Stand-alone draw (class ' + r.class + ', stage ' + r.stage + ')'),
    h('p', { class: 'mut' }, 'A rough player team for the foe\'s level (no journey history, no arena): useful for any trainer, harsher than the chapter.'), bandRows(r.duel));
  return out;
}
function verdictOf(w, ci, [lo, hi]) { if (ci[1] < lo) return 'too hard'; if (ci[0] > hi) return 'too easy'; return w >= lo && w <= hi ? 'in band' : (w < lo ? 'too hard' : 'too easy'); }

/* ------------------------------------------------------------------ encounters */
async function viewEncounters(v, idx) {
  const { entries, rates } = await api('encounters');
  const view = h('div', { class: 'layout' }); v.append(view);
  view.append(listCol('Maps', entries, idx, (e) => ({ id: String(e.index), text: e.map, title: e.map.replace('MAP_', ''), sub: Object.entries(e.kinds).map(([k, n]) => k.replace('_mons', '') + ' ' + n).join(' · ') }), { route: 'encounters' }));
  const col = h('div', { class: 'editcol' }); view.append(col);
  if (idx == null) { col.append(h('div', { class: 'lcdbox' }, 'Pick a map to edit its wild tables. Slots are weighted by the field\'s encounter_rates (the same for every map).')); return; }
  const i = Number(idx);
  const d = await api('encounters/' + i);
  const orig = clone(d.entry), cur = clone(d.entry);
  const kinds = ['land_mons', 'water_mons', 'rock_smash_mons', 'fishing_mons'].filter((k) => cur[k]);
  const ui = { kind: kinds[0] };
  const state = { errs: d.errors };
  const errHost = h('div'); state.drawErrs = () => { errHost.textContent = ''; errHost.append(errorsBox(state.errs)); };
  const dirty = h('span', { class: 'dirty' });
  const changed = debounce(async () => {
    dirty.textContent = same(cur, orig) ? '' : 'edited';
    try { state.errs = (await api('encounters/' + i + '/validate', { entry: cur })).errors; state.drawErrs(); } catch (e) { /* ignore */ }
  }, 250);
  const body = h('div'); const L = S.lists;
  const draw = () => {
    body.textContent = '';
    body.append(h('div', { class: 'seg', role: 'tablist' }, kinds.map((k) => h('button', { class: ui.kind === k ? 'on' : '', onclick: () => { ui.kind = k; draw(); } }, title(k.replace('_mons', ''))))));
    const blk = cur[ui.kind]; const rt = d.rates[ui.kind] || [];
    const tot = rt.reduce((a, b) => a + b, 0) || 1;
    body.append(h('div', { class: 'row' }, h('label', { class: 'fld' }, h('span', { class: 'px mut' }, 'Encounter rate'), numInput(blk.encounter_rate, (x) => { blk.encounter_rate = x; changed(); }, { min: 0, max: 100, label: 'encounter rate' }))));
    const t = h('table', { class: 'ed' }, h('tr', null, h('th', null, 'Slot'), h('th', null, 'Chance'), h('th', null, 'Species'), h('th', null, 'Min Lv'), h('th', null, 'Max Lv'), h('th')));
    blk.mons.forEach((m, j) => t.append(h('tr', null, h('td', { class: 'num' }, j + 1), h('td', { class: 'num' }, rt[j] != null ? Math.round(rt[j] / tot * 100) + '%' : '?'),
      h('td', null, select(L.species, m.species.replace('SPECIES_', ''), (x) => { m.species = 'SPECIES_' + x; changed(); }, { fmt: title, label: 'species' })),
      h('td', null, numInput(m.min_level, (x) => { m.min_level = x; changed(); }, { min: 1, max: 100, label: 'min level' })),
      h('td', null, numInput(m.max_level, (x) => { m.max_level = x; changed(); }, { min: 1, max: 100, label: 'max level' })),
      h('td', null, h('img', { class: 'px-img', src: '/sprites/pokemon/' + m.species.replace('SPECIES_', '') + '/icon.png', alt: '', width: 32, height: 32 })))));
    body.append(t, h('p', { class: 'mut' }, 'The slot count is fixed by the field definition (' + rt.length + ' slots); edit species and levels, not the number of slots.'));
  };
  draw();
  const sim = simBox();
  const bar = actionBar(state, {
    onSave: () => doSave(state, 'encounters/' + i, { entry: cur }, () => { Object.keys(orig).forEach((k) => delete orig[k]); Object.assign(orig, clone(cur)); dirty.textContent = ''; }),
    onRevert: () => { Object.keys(cur).forEach((k) => delete cur[k]); Object.assign(cur, clone(orig)); draw(); changed(); },
    simLabel: 'Simulate this route',
    onSim: () => runSim(sim, () => api('encounters/' + i + '/simulate', { entry: cur, kind: ui.kind, n: 60, runs: 30 }), renderRouteSim),
  });
  col.append(card('cy', cur.map + ' · ' + (cur.base_label || ''), [dirty, body, errHost], bar),
    card('lcd', 'Simulate this route', [h('p', { class: 'mut' }, 'Wild battles per slot against plausible player teams, then a short journey over this table to count walk-backs to an inn.'), sim.box]));
}
function renderRouteSim(r) {
  const out = [];
  if (r.problems && r.problems.length) out.push(h('div', { class: 'eerr new' }, r.problems.join('; ')));
  out.push(h('h3', null, 'Wild battles (weighted by slot chance)'), bandRows(r.wild));
  out.push(h('table', { class: 'ed', style: 'margin-top:10px' }, h('tr', null, h('th', null, 'Slot'), h('th', null, 'Weight'), h('th', null, 'Average'), h('th', null, 'Skilled')),
    r.slots.map((s) => h('tr', null, h('td', null, title(s.species) + ' Lv ' + s.min + (s.max !== s.min ? '-' + s.max : '')), h('td', { class: 'num' }, s.weight), h('td', { class: 'num' }, pct(s.win.average)), h('td', { class: 'num' }, pct(s.win.skilled))))));
  if (r.journey) out.push(h('h3', { style: 'margin-top:14px' }, 'Walk-backs to an inn over 12 wild battles'),
    h('table', { class: 'ed' }, h('tr', null, h('th', null, 'Player / starter'), h('th', null, 'Walk-backs'), h('th', null, 'Battles per rest'), h('th', null, 'Blackouts')),
      Object.entries(r.journey).map(([k, c]) => h('tr', null, h('td', null, k.replace('/', ' / ')), h('td', { class: 'num' }, c.walk_backs), h('td', { class: 'num' }, c.battles_per_rest), h('td', { class: 'num' }, (r.blackouts || {})[k])))));
  return out;
}

/* ------------------------------------------------------------------ arenas */
const BLOCKS = [['String Shot', 1], ['Obstruct (Smokescreen, powders)', 2], ['Vines (Vine Whip, Leech Seed)', 4], ['Sand Attack', 8]];
async function viewArena(v) {
  const d = await api('arena');
  const orig = Object.fromEntries(d.knobs.map((k) => [k.name, k.value]));
  const cur = clone(orig);
  const state = { errs: d.errors };
  const errHost = h('div'); state.drawErrs = () => { errHost.textContent = ''; errHost.append(errorsBox(state.errs)); };
  const dirty = h('span', { class: 'dirty' });
  const changed = debounce(async () => {
    dirty.textContent = same(cur, orig) ? '' : 'edited';
    try { state.errs = (await api('arena/validate', { values: cur })).errors; state.drawErrs(); } catch (e) { /* ignore */ }
  }, 250);
  const body = h('div');
  const draw = () => {
    body.textContent = '';
    const split = d.knobs.findIndex((k) => k.name === 'rockSpeedPct');
    [['Lucky Strike (Yujirou, Tokiwa)', d.knobs.slice(0, split)], ['Grunge Rock (Takeshi, Nibi)', d.knobs.slice(split)]].forEach(([name, ks]) => {
      body.append(h('h3', { style: 'margin:12px 0 6px' }, name));
      const t = h('table', { class: 'ed' }, h('tr', null, h('th', null, 'Knob'), h('th', null, 'Value'), h('th', null, 'ROM default'), h('th', null, 'What it does')));
      ks.forEach((k) => t.append(h('tr', null, h('td', null, h('code', null, k.name)),
        h('td', null, k.name === 'ventBlockers'
          ? h('div', null, BLOCKS.map(([l, bit]) => h('label', { class: 'chk' + (cur[k.name] & bit ? ' on' : '') }, h('input', { type: 'checkbox', checked: !!(cur[k.name] & bit), onchange: (e) => { cur[k.name] = e.target.checked ? cur[k.name] | bit : cur[k.name] & ~bit; draw(); changed(); } }), l)))
          : numInput(cur[k.name], (x) => { cur[k.name] = x; changed(); draw(); }, { min: 0, max: 65535, label: k.name })),
        h('td', { class: 'num' }, k.default, cur[k.name] !== k.default ? h('button', { class: 'btn sm sec2', style: 'margin-left:8px', onclick: () => { cur[k.name] = k.default; draw(); changed(); } }, 'Reset') : null),
        h('td', { class: 'mut' }, k.doc + (k.comment ? ' (' + k.comment + ')' : '')))));
      body.append(t);
    });
  };
  draw();
  const sim = simBox();
  const bar = actionBar(state, {
    onSave: () => doSave(state, 'arena', { values: cur }, () => { Object.assign(orig, clone(cur)); dirty.textContent = ''; }),
    onRevert: () => { Object.assign(cur, clone(orig)); draw(); changed(); },
    simLabel: 'Simulate the arena fights',
    onSim: () => runSim(sim, () => api('arena/simulate', { values: cur, runs: 60 }), renderArenaSim),
  });
  v.append(h('div', { class: 'editcol', style: 'margin-top:20px' },
    card('cy', 'Arena ecology knobs · gArenaTuning in src/arena_ecology.c', [dirty, body, errHost], bar),
    card('lcd', 'Simulate the arena fights', [h('p', { class: 'mut' }, 'The journey with the ROM defaults and with your knobs, on the fights that have an arena (Yujirou: Lucky Strike, Takeshi: Grunge Rock).'), sim.box])));
}
function renderArenaSim(r) {
  return Object.entries(r.fights).map(([id, f]) => {
    const mk = (by) => Object.fromEntries(Object.entries(by).map(([k, c]) => [k, Object.assign({ target: r.target[k.split('/')[0]] }, c, { verdict: verdictOf(c.win, c.ci, r.target[k.split('/')[0]]) })]));
    return h('div', null, h('h3', { style: 'margin-top:10px' }, f.after.name + ' (' + title(r.arena[id]) + ') with your knobs'), bandRows(mk(f.after.by)),
      h('details', null, h('summary', { class: 'mut' }, 'ROM defaults for comparison'), bandRows(mk(f.before.by))));
  });
}

/* ------------------------------------------------------------------ saved edits */
async function viewSaved(v) {
  const pend = await api('pending');
  const col = h('div', { class: 'editcol', style: 'margin-top:20px' }); v.append(col);
  if (!pend.length) { col.append(h('div', { class: 'lcdbox' }, 'Nothing saved yet. Saves land in ' + S.status.out + '; the repo files stay as they are.')); return; }
  col.append(h('div', { class: 'lcdbox' }, 'Saved copies differ from the repo in ' + pend.length + ' file(s). Copy them over the repo files (or run the editor with --in-place) to keep an edit.'));
  pend.forEach((p) => col.append(card('', p.file, h('pre', { class: 'diff' }, p.diff.split('\n').map((l) => h('div', { class: l.startsWith('+') && !l.startsWith('+++') ? 'add' : l.startsWith('-') && !l.startsWith('---') ? 'del' : '' }, l || ' '))),
    h('div', { class: 'ebar' }, h('button', { class: 'btn sec2', onclick: async () => { await api('discard', { file: p.file }); toast('Discarded the saved copy'); route(); } }, 'Discard saved copy')))));
}
