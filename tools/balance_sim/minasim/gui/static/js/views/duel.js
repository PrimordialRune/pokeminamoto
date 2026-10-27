import {
  esc, nice, ensureCss, index, icon, front, itemImg, pageHead, seg, bindSeg, getJSON, trainerImg, mvName, itName, clamp, debounce, uniqTypes,
} from './_util2.js';

// state survives navigation inside the page session
const S = {
  me: [{ species: 'CHARMANDER', level: 14, moves: null, item: null }],
  foe: [{ species: 'PIDGEY', level: 12, moves: null, item: null }],
  foeMode: 'custom', trainer: '', n: 1000, pp: 'average', fp: 'rom', seed: 1234,
  side: 'me', slot: 0, q: '', running: false, stop: false, res: null, err: null, engine: null, once: false,
};
const NS = [100, 1000, 5000, 20000];

const wilson = (w, n, z = 1.96) => { if (!n) return [0, 1]; const p = w / n, d = 1 + z * z / n, c = p + z * z / (2 * n), a = z * Math.sqrt(p * (1 - p) / n + z * z / (4 * n * n)); return [Math.max(0, (c - a) / d), Math.min(1, (c + a) / d)]; };

function defaultMoves(sp, level) {
  const out = [];
  for (const l of ((sp.learnset || {}).level || [])) if (l.level <= level && !out.includes(l.move)) out.push(l.move);
  return out.slice(-4);
}
function moveChoices(sp, level) {
  const set = new Set(); const ls = sp.learnset || {};
  for (const l of ls.level || []) if (l.level <= level) set.add(l.move);
  for (const m of ls.tmhm || []) set.add(m);
  return [...set];
}

export function render(root, ctx) {
  ensureCss();
  const { data } = ctx;
  const ix = index(data);
  const sorted = (data.species || []).slice().sort((a, b) => a.index - b.index);
  const p = ctx.params || {};
  if (p.species && ix.sp.has(String(p.species).toUpperCase())) { S.me[0] = { species: String(p.species).toUpperCase(), level: Number(p.level) || S.me[0].level, moves: null, item: null }; S.side = 'me'; S.slot = 0; }
  let loaded = null;
  if (p.trainer && ix.tr.has(p.trainer)) { S.foeMode = 'trainer'; S.trainer = p.trainer; }
  else if (p.trainer) {
    // a planned / placeholder fight that is not in trainers.party: load its team as a custom team
    const f = ((ctx.report || {}).fights || []).find((x) => x.trainer === p.trainer);
    const party = ((f && f.party) || []).filter((m) => ix.sp.has(m.species));
    if (party.length) { S.foeMode = 'custom'; S.foe = party.slice(0, 6).map((m) => ({ species: m.species, level: m.level, moves: null, item: null })); loaded = f; S.res = null;
      if (f.by_band && f.by_band.average && S.me.length === 1 && !p.species) { const lv = Math.round((f.by_band.average.player_level || 0)); if (lv > 0) S.me[0].level = lv; } }
  }
  if (S.foeMode === 'trainer' && !S.trainer) S.trainer = (data.trainers[0] || {}).id || '';
  let alive = true;

  root.innerHTML = `<div class="duelpage">
    ${pageHead('Duel lab', "Build two teams, fight them thousands of times, see who wins. Changes to the game's data count after a rebuild.")}
    <div id="engine"></div>
    ${loaded ? `<div class="lcdbox" style="margin-bottom:16px" role="note"><b>${esc(loaded.name)}</b> is ${loaded.source === 'design' ? 'planned in the design doc' : 'a placeholder'}, not in the ROM yet. Their team is loaded as a custom team; the moves are the default ones for each level.</div>` : ''}
    <div class="duelcols">
      <section class="card duelcol" id="mecard"></section>
      <div class="col" style="gap:20px;flex:1.3 1 460px;min-width:0">
        <section class="card"><div class="cardh"><h3>Set up</h3></div><div class="cardb col g12" id="setup"></div></section>
        <section class="card" id="rescard"></section>
      </div>
      <section class="card duelcol" id="foecard"></section>
    </div>
    <section class="card" style="margin-top:20px"><div class="cardh"><h3>Pick a Pokemon for <span id="pickfor"></span></h3>
      <label class="field" style="margin-left:auto;min-height:36px;background:var(--panel)"><span class="px">Find</span><input id="pq" type="search" placeholder="Name" aria-label="Search Pokemon" style="width:160px" value="${esc(S.q)}"></label></div>
      <div class="cardb"><div class="picker" id="picker"></div></div></section>
  </div>`;
  const $ = (s) => root.querySelector(s);

  // ---- engine status
  getJSON('/api/status').then((r) => { if (!alive) return; S.engine = r.ok ? !!(r.body && r.body.engine) : null; paintEngine(); paintSetup(); });
  function engineMissing() { return S.engine === false; }
  function paintEngine() {
    $('#engine').innerHTML = engineMissing() ? `<div class="lcdbox" style="margin-bottom:16px" role="status"><b>The simulator is not built yet.</b><br>Run <code>make balance-lib</code>, then reload. You can still build teams; Fight stays off until then.</div>` : '';
  }

  // ---- teams
  function monCard(side, m, i) {
    const sp = ix.sp.get(m.species); const on = S.side === side && S.slot === i;
    const mv = m.moves && m.moves.length ? m.moves : defaultMoves(sp, m.level);
    return `<button type="button" class="slot ${on ? 'on' : ''}" data-side="${side}" data-slot="${i}" aria-label="${esc(nice(m.species))} level ${m.level}, slot ${i + 1}" aria-pressed="${on}">
      <span class="well" style="width:72px;height:72px">${front(m.species)}</span>
      <span class="col f1"><b class="slotname">${esc(nice(m.species))}</b><span class="mut">Level ${m.level}${m.item ? ' &middot; ' + esc(itName(ix, m.item)) : ''}</span>
      <span class="flex g4 wrap">${mv.map((x) => `<span class="chip c-info mvchip">${esc(mvName(ix, x))}</span>`).join('') || '<span class="mut">no moves</span>'}</span></span></button>`;
  }
  function editor(side) {
    const arr = S[side]; if (S.side !== side || !arr[S.slot]) return '';
    const m = arr[S.slot]; const sp = ix.sp.get(m.species); const ch = moveChoices(sp, m.level); const def = defaultMoves(sp, m.level);
    const sel = (k) => `<select data-mv="${k}" aria-label="Move ${k + 1}"><option value="">${m.moves ? '(none)' : def[k] ? 'default: ' + esc(mvName(ix, def[k])) : '(none)'}</option>${ch.map((x) => `<option value="${x}" ${m.moves && m.moves[k] === x ? 'selected' : ''}>${esc(mvName(ix, x))}</option>`).join('')}</select>`;
    const pockets = {}; for (const it of data.items) (pockets[it.pocket] = pockets[it.pocket] || []).push(it);
    const keep = ['ITEMS', 'BERRIES', 'POKE_BALLS'];
    return `<div class="sloted" data-ed="${side}">
      <div class="flex g8 wrap ac"><label class="fld"><span class="px">Level</span><input type="number" min="1" max="100" value="${m.level}" data-lv aria-label="Level"></label>
      <label class="fld"><span class="px">Item</span><select data-item aria-label="Held item"><option value="">none</option>${Object.keys(pockets).filter((k) => !keep.includes(k) || k === 'BERRIES').flatMap((k) => pockets[k]).slice(0, 400).map((it) => `<option value="${it.id}" ${m.item === it.id ? 'selected' : ''}>${esc(nice(it.id))}</option>`).join('')}</select></label>
      <button type="button" class="btn sm sec2" data-remove ${arr.length <= 1 ? 'disabled' : ''}>Remove</button></div>
      <div class="px mut" style="margin:8px 0 4px">Moves (leave default for the last four it learns)</div>
      <div class="mvgrid">${[0, 1, 2, 3].map(sel).join('')}</div></div>`;
  }
  function paintTeams() {
    $('#mecard').innerHTML = `<div class="cardh cy"><h3>Your team</h3></div><div class="cardb col g8">${S.me.map((m, i) => monCard('me', m, i)).join('')}${editor('me')}
      <button type="button" class="btn sec2 sm" data-add="me" ${S.me.length >= 6 ? 'disabled' : ''}>+ Add a mon</button>
      <div class="flex g8 wrap"><span class="chip c-dead">Skill: ${S.pp === 'skilled' ? 'Skilled' : S.pp === 'random' ? 'Random' : 'Average'}</span></div></div>`;
    const t = S.foeMode === 'trainer' ? ix.tr.get(S.trainer) : null;
    let body;
    if (t) {
      body = `<div class="flex ac g12">${trainerImg(t.pic_slug || '', 1)}<div><b style="font-family:var(--font-d);font-size:18px">${esc(t.name)}</b><div class="mut">${esc(t.class || '')}</div></div></div>
        <select id="trsel" aria-label="Trainer">${data.trainers.map((x) => `<option value="${x.id}" ${x.id === S.trainer ? 'selected' : ''}>${esc(nice(x.name))} (${esc(x.class || '')}) - ${x.party.length} mon</option>`).join('')}</select>
        ${t.party.map((m) => `<div class="slot ro"><span class="well" style="width:72px;height:72px">${front(m.species)}</span><span class="col f1"><b class="slotname">${esc(nice(m.species))}</b><span class="mut">Level ${m.level}</span></span></div>`).join('')}
        <div class="mut" style="font-size:12.5px">Loaded from <code>trainers.party</code>; the engine builds the team.</div>`;
    } else body = `${S.foe.map((m, i) => monCard('foe', m, i)).join('')}${editor('foe')}<button type="button" class="btn sec2 sm" data-add="foe" ${S.foe.length >= 6 ? 'disabled' : ''}>+ Add a mon</button>`;
    $('#foecard').innerHTML = `<div class="cardh"><h3>Their team</h3><span id="foemode" style="margin-left:auto"></span></div><div class="cardb col g8">${body}
      <label class="fld"><span class="px">Their AI</span><select id="fp" aria-label="Their AI"><option value="rom" ${S.fp === 'rom' ? 'selected' : ''}>The game's own AI</option><option value="greedy" ${S.fp === 'greedy' ? 'selected' : ''}>Greedy</option><option value="random" ${S.fp === 'random' ? 'selected' : ''}>Random</option></select></label></div>`;
    $('#foemode').innerHTML = seg('Their team', [['custom', 'Custom'], ['trainer', 'A trainer']], S.foeMode);
    bindSeg($('#foemode'), (k) => { S.foeMode = k; if (k === 'trainer' && !S.trainer) S.trainer = data.trainers[0].id; paintTeams(); paintPicker(); });
    const ts = $('#trsel'); if (ts) ts.addEventListener('change', (e) => { S.trainer = e.target.value; paintTeams(); });
    $('#fp').addEventListener('change', (e) => { S.fp = e.target.value; });
    root.querySelectorAll('.slot[data-side]').forEach((b) => b.addEventListener('click', () => { S.side = b.dataset.side; S.slot = Number(b.dataset.slot); paintTeams(); paintPicker(); }));
    root.querySelectorAll('[data-add]').forEach((b) => b.addEventListener('click', () => { const side = b.dataset.add; S[side].push({ species: 'RATTATA', level: S[side][0].level, moves: null, item: null }); S.side = side; S.slot = S[side].length - 1; paintTeams(); paintPicker(); }));
    root.querySelectorAll('.sloted').forEach((ed) => {
      const side = ed.dataset.ed, m = S[side][S.slot];
      ed.querySelector('[data-lv]').addEventListener('change', (e) => { m.level = clamp(Number(e.target.value) || 1, 1, 100); if (m.moves) m.moves = m.moves.filter((x) => moveChoices(ix.sp.get(m.species), m.level).includes(x)); paintTeams(); });
      ed.querySelector('[data-item]').addEventListener('change', (e) => { m.item = e.target.value || null; paintTeams(); });
      ed.querySelector('[data-remove]').addEventListener('click', () => { S[side].splice(S.slot, 1); S.slot = Math.max(0, S.slot - 1); paintTeams(); paintPicker(); });
      ed.querySelectorAll('[data-mv]').forEach((sel) => sel.addEventListener('change', () => {
        const cur = m.moves && m.moves.length ? m.moves.slice() : defaultMoves(ix.sp.get(m.species), m.level);
        cur[Number(sel.dataset.mv)] = sel.value || null;
        m.moves = cur.filter(Boolean); if (!m.moves.length) m.moves = null; paintTeams();
      }));
    });
  }

  // ---- picker
  function paintPicker() {
    const t = S.side === 'foe' && S.foeMode === 'trainer';
    $('#pickfor').textContent = (S.side === 'me' ? 'your' : 'their') + ' slot ' + (S.slot + 1);
    const cur = (S[S.side][S.slot] || {}).species; const q = S.q.trim().toLowerCase();
    $('#picker').innerHTML = t ? '<div class="mut">Their team comes from the trainer. Switch to Custom to pick mons.</div>' :
      sorted.filter((s) => !q || nice(s.id).toLowerCase().includes(q) || String(s.dex) === q).map((s) => `<button type="button" class="pk ${s.id === cur ? 'on' : ''}" data-sp="${s.id}" aria-label="${esc(nice(s.id))}" title="${esc(nice(s.id))}">${icon(s.id)}</button>`).join('') || '<div class="mut">Nobody by that name.</div>';
    root.querySelectorAll('.pk').forEach((b) => b.addEventListener('click', () => { const m = S[S.side][S.slot]; m.species = b.dataset.sp; m.moves = null; paintTeams(); paintPicker(); }));
  }
  $('#pq').addEventListener('input', debounce((e) => { S.q = e.target.value; paintPicker(); }, 80));

  // ---- setup
  function paintSetup() {
    const dis = S.running || engineMissing();
    const nf = (n) => n.toLocaleString('en-US');
    $('#setup').innerHTML = `<div class="flex wrap g16 ac">
        <div><div class="px mut" style="margin-bottom:6px">Number of fights</div>${seg('Fights', NS.map((n) => [n, nf(n)]), S.n)}</div>
        <div><div class="px mut" style="margin-bottom:6px">Your skill</div>${seg('Skill', [['average', 'Average'], ['skilled', 'Skilled'], ['random', 'Random']], S.pp).replace('class="seg"', 'class="seg" id="ppseg"')}</div>
        <label class="field"><span class="px">Seed</span><input id="seed" value="${S.seed}" aria-label="Seed" style="width:70px" inputmode="numeric"></label></div>
      <div class="flex g8 wrap">${S.running ? `<button class="btn" id="stop" type="button" style="flex:1">Stop</button>` : `<button class="btn go bigbtn" id="go" type="button" style="flex:1" ${dis ? 'disabled' : ''}>Fight ${nf(S.n)} times</button>`}
        <button class="btn sec2" id="once" type="button" ${dis ? 'disabled' : ''}>Fight once with log</button></div>`;
    const segs = $('#setup').querySelectorAll('.seg');
    bindSeg(segs[0], (k) => { S.n = Number(k); paintSetup(); });
    bindSeg(segs[1], (k) => { S.pp = k; paintSetup(); paintTeams(); });
    $('#seed').addEventListener('change', (e) => { S.seed = parseInt(e.target.value, 10) || 1; });
    const go = $('#go'); if (go) go.addEventListener('click', () => run(S.n));
    const stop = $('#stop'); if (stop) stop.addEventListener('click', () => { S.stop = true; });
    $('#once').addEventListener('click', () => run(1));
  }

  // ---- running
  const toMon = (m) => { const o = { species: m.species, level: m.level }; if (m.moves && m.moves.length) o.moves = m.moves; if (m.item) o.item = m.item; return o; };
  async function run(total) {
    S.running = true; S.stop = false; S.err = null; S.once = total === 1;
    S.res = { wins: 0, losses: 0, draws: 0, n: 0, total, turns: 0, log: [], sample: null };
    paintSetup(); paintResult();
    const chunk = total <= 100 ? total : Math.max(100, Math.round(total / 20));
    let done = 0;
    while (done < total && !S.stop && alive) {
      const k = Math.min(chunk, total - done);
      const body = { player: S.me.map(toMon), n: k, seed: S.seed + done, player_policy: S.pp, foe_policy: S.fp };
      if (S.foeMode === 'trainer') body.trainer = S.trainer; else body.foe = S.foe.map(toMon);
      const r = await getJSON('/api/duel', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(body) });
      if (!r.ok) {
        S.err = r.status === 503 ? { title: 'The simulator is not built yet.', text: 'Run <code>make balance-lib</code>, then try again.' } : { title: 'The fight did not run.', text: esc((r.body && (r.body.error || r.body.message)) || 'Unknown error') + (r.body && r.body.hint ? ' (' + esc(r.body.hint) + ')' : '') };
        if (r.status === 503) { S.engine = false; paintEngine(); }
        break;
      }
      const b = r.body, R = S.res;
      R.wins += b.wins; R.losses += b.losses || 0; R.draws += b.draws || 0; R.n += b.n; R.turns += b.turns * b.n;
      if (!R.sample) { R.log = b.sample_log || []; R.sample = b.sample || null; }
      done += k;
      if (alive) paintResult();
    }
    S.running = false;
    if (!S.res.n) S.res = null;
    if (alive) { paintSetup(); paintResult(); }
  }

  function logLines(R) {
    const out = [];
    const isP = (s) => s === 0 || s === 'player' || s === 'p' || s === 'you';
    for (const e of R.log || []) {
      const who = isP(e.side) ? 'player' : 'foe';
      const arr = (R.sample && R.sample[who]) || [];
      const mon = (arr[e.slot || 0] || {}).species || ((who === 'player' ? S.me : S.foe)[e.slot || 0] || {}).species || (who === 'player' ? 'Your mon' : 'Their mon');
      let txt;
      if (e.action === 'move') txt = e.miss ? `used ${mvName(ix, e.move)} and missed` : `used ${mvName(ix, e.move)}: ${e.damage ?? 0} damage`;
      else if (e.action === 'switch') txt = 'switched out';
      else if (e.action === 'item') txt = 'used an item';
      else txt = e.action || 'acted';
      const tags = [];
      if (e.crit) tags.push('critical');
      if (e.effectiveness != null && e.effectiveness !== 1 && !e.miss) tags.push(e.effectiveness > 1 ? 'super effective' : e.effectiveness === 0 ? 'no effect' : 'not very effective');
      if (e.ko) tags.push('KO');
      out.push({ turn: e.turn, player: who === 'player', mon: nice(mon), txt, tags, dmg: e.damage });
    }
    return out;
  }

  function paintResult() {
    const R = S.res;
    const head = `<div class="cardh lcd"><h3>${R && S.running ? 'Result so far' : 'Result'}</h3><span class="px" style="margin-left:auto;color:#1F2D12">${R ? `${R.n.toLocaleString('en-US')} of ${R.total.toLocaleString('en-US')}` : ''}</span></div>`;
    if (S.err) { $('#rescard').innerHTML = head + `<div class="cardb"><div class="lcdbox" role="alert"><b>${S.err.title}</b><br>${S.err.text}</div></div>`; return; }
    if (!R) { $('#rescard').innerHTML = head + `<div class="cardb"><div class="lcdbox">${engineMissing() ? 'The simulator is not built yet. Run <code>make balance-lib</code> and your fights will show here.' : 'Set up both teams and press Fight. Results appear here and the margin shrinks as more fights finish.'}</div></div>`; return; }
    const wr = R.wins / R.n, ci = wilson(R.wins, R.n), m = (ci[1] - ci[0]) / 2;
    const lp = (R.losses / R.n) * 100, dp = (R.draws / R.n) * 100;
    const lines = logLines(R);
    $('#rescard').innerHTML = head + `<div class="cardb col g12">
      <div class="flex ac g16" style="justify-content:center">${front(S.me[0].species, 2)}<div class="vs">VS</div>${front(S.foeMode === 'trainer' ? (((ix.tr.get(S.trainer) || {}).party || [{}])[0].species || S.me[0].species) : S.foe[0].species, 2)}</div>
      <div class="flex stack" role="img" aria-label="Your team won ${Math.round(wr * 100)} percent, lost ${Math.round(lp)}, drew ${Math.round(dp)}">
        <span style="width:${wr * 100}%;background:var(--grass);color:#10203A">${wr > 0.18 ? 'You win ' + Math.round(wr * 100) + '%' : ''}</span>
        <span style="width:${lp}%;background:var(--coral);color:#271F3B">${lp > 18 ? 'They win ' + Math.round(lp) + '%' : ''}</span>
        <span style="width:${dp}%;background:var(--panel2)"></span></div>
      <div class="flex g24 wrap">
        <div><div class="px mut">Win rate</div><b class="num bigrt">${Math.round(wr * 100)}%</b> <span class="mut" title="Run it again and the number stays within this much">${R.n > 1 ? `&plusmn; ${Math.max(1, Math.round(m * 100))} pts` : '(one fight only)'}</span></div>
        <div><div class="px mut">Avg turns</div><b class="num bigrt">${(R.turns / R.n).toFixed(1)}</b></div>
        <div><div class="px mut">Wins / losses</div><b class="num bigrt">${R.wins} / ${R.losses}</b>${R.draws ? `<span class="mut"> (${R.draws} draws)</span>` : ''}</div></div>
      ${S.running ? '' : '<!--'}<div class="bar" role="progressbar" aria-valuemin="0" aria-valuemax="${R.total}" aria-valuenow="${R.n}"><b style="left:0;width:${(R.n / R.total) * 100}%;background:var(--cyan)"></b></div>${S.running ? '' : '-->'}
      <div class="mut" style="font-size:12.5px">${S.running ? 'Margin shrinks as more fights finish.' : 'Done. The number is final for this seed.'}</div>
      ${lines.length ? `<details ${S.once ? 'open' : ''} class="logbox"><summary><b>Sample log</b> <span class="mut">(first fight, ${lines.length} actions)</span></summary><ol class="loglist">${lines.map((l) => `<li class="${l.player ? 'lp' : 'lf'}"><span class="kbd">T${l.turn}</span> <b>${esc(l.mon)}</b> ${esc(l.txt)}${l.tags.map((t) => ` <span class="chip ${t === 'KO' ? 'c-red' : t === 'critical' ? 'c-amb' : 'c-info'}" style="min-height:20px;font-size:11px">${t}</span>`).join('')}</li>`).join('')}</ol></details>` : ''}
      <div class="flex g8 wrap"><button class="btn sm sec2" type="button" id="copyset">Copy setup</button><a class="btn sm sec2" href="#/compare">Compare with a run</a></div></div>`;
    const cs = $('#copyset'); if (cs) cs.addEventListener('click', async () => { try { await navigator.clipboard.writeText(JSON.stringify({ player: S.me.map(toMon), foe: S.foeMode === 'trainer' ? S.trainer : S.foe.map(toMon), n: S.n, policy: S.pp, seed: S.seed }, null, 1)); cs.textContent = 'Copied'; } catch (e) { cs.textContent = 'Copy failed'; } });
  }

  paintEngine(); paintTeams(); paintSetup(); paintResult(); paintPicker();
  return () => { alive = false; S.stop = true; };
}
