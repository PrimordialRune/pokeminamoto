"""Engine: ctypes wrapper around build/host/libminasim.so, the real battle code host-compiled
(tools/balance_sim/host/, `make balance-lib`). See CONTRACT.md "Engine API" and README.md.

    eng = Engine()
    r = eng.battle(seed=1, player=[Mon("CHARMANDER", 14)], foe=[Mon("PIDGEY", 12)],
                   player_policy="greedy", foe_policy="rom")

One Engine per process (the game code is full of globals; runner.py uses a process pool).
"""
import ctypes as C
import os
import re
import subprocess

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
LIB_PATH = os.path.join(ROOT, "build", "host", "libminasim.so")

HB_PARTY, HB_MAXLEGAL, HB_BAG, HB_USED, HB_LOG_MAX = 6, 128, 16, 32, 2048
ACT_MOVE, ACT_SWITCH, ACT_ITEM, ACT_TALK = 1, 2, 3, 4
POLICY_CALLBACK, POLICY_RANDOM, POLICY_GREEDY, POLICY_ROM = 0, 1, 2, 3
C_POLICIES = {"random": POLICY_RANDOM, "greedy": POLICY_GREEDY, "rom": POLICY_ROM}
SPECIAL_SLOT = 4
STAGE_KEYS = ["hp", "atk", "def", "spe", "spa", "spd", "acc", "eva"]  # gBattleMons.statStages order
TYPE_NAMES = ["NORMAL", "FIGHTING", "FLYING", "POISON", "GROUND", "ROCK", "BUG", "GHOST", "STEEL", "MYSTERY",
              "FIRE", "WATER", "GRASS", "ELECTRIC", "PSYCHIC", "ICE", "DRAGON", "DARK"]
B_WINNER = {0: "player", 1: "foe", 2: "draw"}


# ------------------------------------------------------------------ constants parsed from the repo
def _parse_defines(paths, prefix):
    raw = {}
    for p in paths:
        try:
            with open(os.path.join(ROOT, p), encoding="utf-8", errors="replace") as f:
                for line in f:
                    m = re.match(r"\s*#define\s+(%s\w*)\s+(.+?)\s*(?://.*|/\*.*)?$" % prefix, line)
                    if m:
                        raw[m.group(1)] = m.group(2)
        except OSError:
            pass
    vals = {}
    for _ in range(6):
        for k, e in raw.items():
            if k in vals:
                continue
            ex = e.replace("(u8)", "").replace("(u16)", "")
            try:
                vals[k] = int(eval(ex, {"__builtins__": {}}, vals))
            except Exception:
                pass
    return vals


class Consts:
    """Name <-> id tables, names without the SPECIES_/MOVE_/ITEM_/TRAINER_ prefix."""

    def __init__(self):
        sp = _parse_defines(["include/constants/species.h"], "SPECIES_")
        mv = _parse_defines(["include/constants/moves.h"], "MOVE_")
        it = _parse_defines(["include/constants/items.h"], "ITEM_")
        tr = _parse_defines(["include/constants/opponents.h"], "TRAINER_")
        tc = _parse_defines(["include/constants/trainers.h"], "TRAINER_CLASS_")
        ai = _parse_defines(["include/constants/battle_ai.h"], "AI_SCRIPT_")
        nat = _parse_defines(["include/constants/pokemon.h"], "NATURE_")
        strip = lambda d, p: {k[len(p):]: v for k, v in d.items()}
        self.species, self.moves, self.items = strip(sp, "SPECIES_"), strip(mv, "MOVE_"), strip(it, "ITEM_")
        self.trainers, self.classes, self.ai = strip(tr, "TRAINER_"), strip(tc, "TRAINER_CLASS_"), strip(ai, "AI_SCRIPT_")
        self.natures = strip(nat, "NATURE_")
        rev = lambda d: {v: k for k, v in reversed(list(d.items()))}
        self.species_name, self.move_name, self.item_name = rev(self.species), rev(self.moves), rev(self.items)

    def sp(self, x): return _lookup(self.species, x, "SPECIES_", "species")
    def mv(self, x): return _lookup(self.moves, x, "MOVE_", "move")
    def it(self, x): return _lookup(self.items, x, "ITEM_", "item")


def _lookup(table, x, prefix, what):
    if isinstance(x, int):
        return x
    k = str(x).upper().replace(prefix, "", 1) if str(x).upper().startswith(prefix) else str(x).upper()
    if k not in table:
        raise KeyError("unknown %s %r" % (what, x))
    return table[k]


_CONSTS = None
def consts():
    global _CONSTS
    if _CONSTS is None:
        _CONSTS = Consts()
    return _CONSTS


# ------------------------------------------------------------------ Mon
class Mon:
    """species/moves/item are constant names without prefix. ivs: None (random), an int (all six) or six ints
    in the order HP, ATK, DEF, SPE, SPA, SPD. hp/status/exp/tp_spent carry state between battles;
    awakened = the Signature is unsealed."""

    __slots__ = ("species", "level", "moves", "item", "ivs", "nature", "tp_spent", "hp", "status", "exp", "awakened", "special_progress", "friendship")

    def __init__(self, species, level, moves=None, item=None, ivs=None, nature=None, tp_spent=0,
                 hp=None, status=None, exp=None, awakened=False, special_progress=0, friendship=None):
        self.species, self.level, self.moves, self.item, self.ivs = species, int(level), moves, item, ivs
        self.nature, self.tp_spent, self.hp, self.status, self.exp, self.awakened = nature, tp_spent or 0, hp, status, exp, awakened
        self.special_progress, self.friendship = special_progress, friendship

    @classmethod
    def of(cls, x):
        if isinstance(x, Mon):
            return x
        if isinstance(x, dict):
            return cls(**{k: v for k, v in x.items() if k in cls.__slots__})
        raise TypeError("expected Mon or dict, got %r" % type(x))

    def __repr__(self):
        return "Mon(%s, %d%s)" % (self.species, self.level, ", moves=%s" % self.moves if self.moves else "")

    def to_dict(self):
        return {k: getattr(self, k) for k in self.__slots__}


# ------------------------------------------------------------------ ctypes mirrors of hb_api.h
u8, u16, u32, i8, i32 = C.c_uint8, C.c_uint16, C.c_uint32, C.c_int8, C.c_int32


class HbMonIn(C.Structure):
    _fields_ = [("species", u16), ("level", u8), ("nature", u8), ("ivs", u8 * 6), ("moves", u16 * 4), ("item", u16),
                ("has_moves", u8), ("tp_spent", u8), ("awakened", u8), ("has_hp", u8), ("hp", u16), ("pad0", u16),
                ("exp", u32), ("has_exp", u8), ("special_progress", u8), ("friendship", u8), ("has_friendship", u8), ("status", u32)]


class HbIn(C.Structure):
    _fields_ = [("seed", u32), ("trainer", u16), ("n_player", u8), ("n_foe", u8), ("player_policy", u8), ("foe_policy", u8),
                ("log", u8), ("charisma", u8), ("turn_cap", u16), ("use_trainer", u8), ("foe_is_trainer", u8),
                ("trainer_class", u8), ("n_foe_items", u8), ("foe_items", u16 * 4), ("ai_flags", u32),
                ("player", HbMonIn * HB_PARTY), ("foe", HbMonIn * HB_PARTY),
                ("bag_item", u16 * HB_BAG), ("bag_count", u16 * HB_BAG), ("n_bag", u8), ("arena", u8), ("pad", u8 * 2)]


class HbMonOut(C.Structure):
    _fields_ = [("species", u16), ("level", u8), ("tp", u8), ("tp_max", u8), ("alive", u8), ("special_awake", u8), ("tp_spent", u8),
                ("hp", u16), ("max_hp", u16), ("status", u32), ("exp", u32), ("exp_gained", u32), ("moves", u16 * 4),
                ("item", u16), ("special_progress", u8), ("friendship", u8)]


class HbLogEv(C.Structure):
    _fields_ = [("turn", u16), ("side", u8), ("slot", u8), ("action", u8), ("crit", u8), ("miss", u8), ("eff", u8),
                ("ko", u8), ("pad", u8), ("move", u16), ("damage", u16), ("hp_after", u16), ("target_slot", u16)]


class HbOut(C.Structure):
    _fields_ = [("winner", i8), ("outcome", u8), ("turns", u16), ("frames", u32),
                ("n_player", u8), ("n_foe", u8), ("kind", u8), ("error", u8),
                ("player", HbMonOut * HB_PARTY), ("foe", HbMonOut * HB_PARTY),
                ("moves_used_id", (u16 * HB_USED) * 2), ("moves_used_n", (u16 * HB_USED) * 2),
                ("n_moves_used", u8 * 2), ("pad0", u8 * 2),
                ("items_used_id", u16 * 8), ("items_used_n", u16 * 8), ("n_items_used", u8), ("pad1", u8),
                ("talk_used", u16), ("special_used", u16 * 2), ("struggles", u16 * 2), ("min_tp", u16 * 2),
                ("illegal", u16), ("switches", u16 * 2),
                ("log_n", u16), ("leaked_blocks", i32), ("rng_end", u32)]


class _PartyEntry(C.Structure):
    _fields_ = [("species", u16), ("level", u8), ("tp", u8), ("hp", u16), ("max_hp", u16), ("status", u32),
                ("alive", u8), ("active", u8), ("pad", u8 * 2)]


class HbSideState(C.Structure):
    _fields_ = [("species", u16), ("level", u8), ("tp", u8), ("tp_max", u8), ("party_idx", u8), ("n_moves", u8), ("pad0", u8),
                ("hp", u16), ("max_hp", u16), ("status", u32), ("stages", i8 * 8), ("moves", u16 * 4), ("tp_cost", u8 * 4),
                ("special", u16), ("types", u8 * 2), ("item", u16), ("pad1", u16)]


class HbState(C.Structure):
    _fields_ = [("side", u8), ("kind", u8), ("forced", u8), ("turn", u8), ("me", HbSideState), ("foe", HbSideState),
                ("party", _PartyEntry * HB_PARTY), ("n_party", u8), ("n_legal", u8), ("n_items", u8), ("pad2", u8),
                ("item_id", u16 * HB_BAG), ("item_count", u16 * HB_BAG), ("legal", u32 * HB_MAXLEGAL)]


POLICY_FN = C.CFUNCTYPE(C.c_int32, C.POINTER(HbState))
STATUS_BITS = [(0x7, "SLP"), (0x8, "PSN"), (0x10, "BRN"), (0x20, "FRZ"), (0x40, "PAR"), (0x80, "TOX")]


def status_name(raw):
    for mask, name in STATUS_BITS:
        if raw & mask:
            return name
    return None


# ------------------------------------------------------------------ build
def build_lib(quiet=True):
    """`make balance-lib` from the repo root."""
    r = subprocess.run(["make", "balance-lib"], cwd=ROOT, capture_output=quiet, text=True)
    if r.returncode != 0:
        raise RuntimeError("make balance-lib failed:\n" + (r.stdout or "") + (r.stderr or ""))


def lib_is_stale():
    if not os.path.exists(LIB_PATH):
        return True
    r = subprocess.run(["make", "-q", "-f", "tools/balance_sim/host/Makefile"], cwd=ROOT, capture_output=True)
    return r.returncode != 0


class EngineNotBuilt(RuntimeError):
    pass


class Engine:
    def __init__(self, build="auto", lib=None):
        """build: "auto" runs `make balance-lib` when the library is missing or older than the sources,
        False never builds (workers), True always runs make."""
        path = lib or LIB_PATH
        if lib is None:
            if build is True or (build == "auto" and lib_is_stale()):
                build_lib()
        if not os.path.exists(path):
            raise EngineNotBuilt("engine not built: run `make balance-lib`")
        self.lib = C.CDLL(path)
        self.lib.hb_battle.argtypes = [C.POINTER(HbIn), C.POINTER(HbOut)]
        self.lib.hb_battle.restype = C.c_int
        self.lib.hb_set_callback.argtypes = [POLICY_FN]
        self.lib.hb_log.restype = C.POINTER(HbLogEv)
        sizes = (C.c_int * 16)()
        self.lib.hb_abi(sizes)
        mine = [C.sizeof(HbIn), C.sizeof(HbOut), C.sizeof(HbState), C.sizeof(HbLogEv), C.sizeof(HbMonIn), C.sizeof(HbMonOut)]
        if list(sizes)[:6] != mine:
            raise RuntimeError("engine ABI mismatch (lib %s, python %s): update minasim/engine.py or rebuild" % (list(sizes)[:6], mine))
        self.pokemon_size = sizes[6]
        self.c = consts()
        self._in, self._out = HbIn(), HbOut()
        self._cb_refs = None
        self._pcb = [None, None]
        self.battles = 0

    # -------------------------------------------------------------- inputs
    def _fill_mon(self, m, mon):
        c = self.c
        mon = Mon.of(mon)
        C.memset(C.addressof(m), 0, C.sizeof(m))
        m.species = c.sp(mon.species)
        m.level = mon.level
        m.nature = 0xFF if mon.nature is None else (mon.nature if isinstance(mon.nature, int) else c.natures[str(mon.nature).upper()])
        iv = mon.ivs
        for i in range(6):
            m.ivs[i] = 0xFF if iv is None else (iv if isinstance(iv, int) else iv[i])
        if mon.moves is not None:
            m.has_moves = 1
            for i, mv in enumerate(list(mon.moves)[:4]):
                m.moves[i] = c.mv(mv)
        if mon.item:
            m.item = c.it(mon.item)
        m.tp_spent = mon.tp_spent
        m.awakened = 1 if mon.awakened else 0
        m.special_progress = mon.special_progress or 0
        if mon.friendship is not None:
            m.has_friendship, m.friendship = 1, mon.friendship
        if mon.hp is not None:
            m.has_hp, m.hp = 1, mon.hp
        if mon.exp is not None:
            m.has_exp, m.exp = 1, mon.exp
        if mon.status:
            m.status = mon.status if isinstance(mon.status, int) else {n: b for b, n in STATUS_BITS}[str(mon.status).upper()] if str(mon.status).upper() != "SLP" else 3

    def _resolve_policy(self, p, side):
        if isinstance(p, str) and p in C_POLICIES:
            return C_POLICIES[p], None
        if isinstance(p, str):
            try:
                from .policies import get_policy
            except ImportError as e:
                raise KeyError("policy %r needs minasim.policies (%s)" % (p, e))
            p = get_policy(p)
            if isinstance(p, str):
                return C_POLICIES[p], None
        if callable(p):
            return POLICY_CALLBACK, p
        raise TypeError("bad policy %r" % (p,))

    def _prepare(self, seed, player, foe, trainer, player_policy, foe_policy, log, bag, charisma, turn_cap,
                 is_trainer, trainer_class, ai_flags, foe_items):
        c, hin = self.c, self._in
        C.memset(C.addressof(hin), 0, C.sizeof(hin))
        hin.seed = seed & 0xFFFFFFFF
        player = [Mon.of(m) for m in player]
        if not 1 <= len(player) <= HB_PARTY:
            raise ValueError("player party must have 1-6 mons")
        hin.n_player = len(player)
        for i, m in enumerate(player):
            self._fill_mon(hin.player[i], m)
        if trainer is not None:
            hin.use_trainer = 1
            hin.trainer = trainer if isinstance(trainer, int) else _lookup(c.trainers, trainer, "TRAINER_", "trainer")
        else:
            foe = [Mon.of(m) for m in (foe or [])]
            if not 1 <= len(foe) <= HB_PARTY:
                raise ValueError("foe party must have 1-6 mons (or pass trainer=)")
            hin.n_foe = len(foe)
            for i, m in enumerate(foe):
                self._fill_mon(hin.foe[i], m)
            if is_trainer is None:
                is_trainer = len(foe) > 1  # wild battles have a single foe
            hin.foe_is_trainer = 1 if is_trainer else 0
            if is_trainer:
                hin.trainer_class = 0 if trainer_class is None else (trainer_class if isinstance(trainer_class, int) else _lookup(c.classes, trainer_class, "TRAINER_CLASS_", "trainer class"))
                if ai_flags is None:
                    ai_flags = ["CHECK_BAD_MOVE"]
                if isinstance(ai_flags, int):
                    hin.ai_flags = ai_flags
                else:
                    for f in ai_flags:
                        hin.ai_flags |= f if isinstance(f, int) else _lookup(c.ai, f, "AI_SCRIPT_", "ai flag")
                for i, it in enumerate((foe_items or [])[:4]):
                    hin.foe_items[i] = c.it(it)
                hin.n_foe_items = min(4, len(foe_items or []))
        pp, pcb = self._resolve_policy(player_policy, 0)
        fp, fcb = self._resolve_policy(foe_policy, 1)
        hin.player_policy, hin.foe_policy = pp, fp
        self._pcb = [pcb, fcb]
        hin.log = 1 if log else 0
        hin.charisma = min(255, int(charisma))
        hin.turn_cap = min(250, int(turn_cap))
        n = 0
        for name, cnt in (bag or {}).items():
            if cnt > 0 and n < HB_BAG:
                hin.bag_item[n], hin.bag_count[n] = c.it(name), cnt
                n += 1
        hin.n_bag = n
        self._kind_trainer = bool(trainer is not None or is_trainer)
        return hin

    # -------------------------------------------------------------- callback
    def _install_callback(self):
        if self._pcb[0] is None and self._pcb[1] is None:
            if self._cb_refs is not None:
                self.lib.hb_set_callback(C.cast(None, POLICY_FN))
                self._cb_refs = None
            return
        if self._cb_refs is None:
            self._cb_refs = POLICY_FN(self._on_decision)
            self.lib.hb_set_callback(self._cb_refs)

    def _on_decision(self, stp):
        s = stp.contents
        pol = self._pcb[s.side]
        if pol is None:
            return -1
        try:
            action = pol(self._state_dict(s))
        except Exception:  # a policy bug must not take the worker down; counted as an illegal action
            import traceback
            traceback.print_exc()
            return -1
        return _action_code(action, self.c)

    def _state_dict(self, s):
        c = self.c
        mv = lambda x: c.move_name.get(x) if x else None

        def side(ss, own):
            d = {"species": c.species_name.get(ss.species), "level": ss.level, "hp": ss.hp, "max_hp": ss.max_hp,
                 "status": status_name(ss.status), "stages": {STAGE_KEYS[i]: ss.stages[i] for i in range(1, 8)},
                 "types": [TYPE_NAMES[t] for t in ss.types if t < len(TYPE_NAMES)]}
            if own:
                d.update(tp=ss.tp, tp_max=ss.tp_max, moves=[mv(m) for m in ss.moves], tp_costs=list(ss.tp_cost),
                         special=mv(ss.special), item=c.item_name.get(ss.item) if ss.item else None)
            return d

        party = [{"species": c.species_name.get(p.species), "level": p.level, "hp": p.hp, "max_hp": p.max_hp, "tp": p.tp,
                  "status": status_name(p.status), "alive": bool(p.alive), "active": bool(p.active)}
                 for p in s.party[:s.n_party]]
        legal = []
        for code in s.legal[:s.n_legal]:
            t, a, a2 = code & 0xFF, (code >> 8) & 0xFFFF, code >> 24
            if t == ACT_MOVE:
                legal.append({"type": "special"} if a == SPECIAL_SLOT else {"type": "move", "slot": a, "move": mv(s.me.moves[a]) if a < 4 else None})
            elif t == ACT_SWITCH:
                legal.append({"type": "switch", "slot": a})
            elif t == ACT_ITEM:
                legal.append({"type": "item", "item": c.item_name.get(a), "target": a2})
            elif t == ACT_TALK:
                legal.append({"type": "talk", "kind": "provoke" if a == 0 else "unnerve"})
        return {"side": s.side, "turn": s.turn, "seed": self._in.seed, "kind": "trainer" if s.kind else "wild",
                "forced": bool(s.forced), "me": side(s.me, True), "foe": side(s.foe, False), "party": party,
                "items": {c.item_name.get(s.item_id[i]): s.item_count[i] for i in range(s.n_items)}, "legal": legal}

    # -------------------------------------------------------------- run
    def battle(self, seed=1, player=None, foe=None, trainer=None, player_policy="greedy", foe_policy="rom", log=False,
               bag=None, charisma=0, turn_cap=200, is_trainer=None, trainer_class=None, ai_flags=None, foe_items=None, max_turns=None,
               arena=0):
        """One battle. Returns the dict described in CONTRACT.md plus: "exp" per player mon (exp_gained),
        "stats" (per-side moves_used, items_used, talk_used, special_used, struggles, min_tp, switches),
        "error" (None, "frame cap", "policy loop"), "illegal" (illegal actions replaced), "leaked_blocks"."""
        if max_turns is not None:
            turn_cap = max_turns
        hin = self._prepare(seed, player or [], foe, trainer, player_policy, foe_policy, log, bag, charisma, turn_cap,
                            is_trainer, trainer_class, ai_flags, foe_items)
        hin.arena = arena                  # ARENA_* (minasim/arena.py)
        self._install_callback()
        out = self._out
        self.lib.hb_battle(C.byref(hin), C.byref(out))
        self.battles += 1
        return self._result(out, log)

    def run_seeds(self, seeds, **kw):
        """Fast path for many battles with the same set-up and only the seed changing: returns
        [(winner, turns, player_hp_total_left)] without building result dicts."""
        hin = self._prepare(0, kw.pop("player"), kw.pop("foe", None), kw.pop("trainer", None), kw.pop("player_policy", "greedy"),
                            kw.pop("foe_policy", "rom"), False, kw.pop("bag", None), kw.pop("charisma", 0),
                            kw.pop("turn_cap", 200), kw.pop("is_trainer", None), kw.pop("trainer_class", None),
                            kw.pop("ai_flags", None), kw.pop("foe_items", None))
        if kw:
            raise TypeError("unexpected arguments %s" % list(kw))
        self._install_callback()
        out, fn, res = self._out, self.lib.hb_battle, []
        for s in seeds:
            hin.seed = s & 0xFFFFFFFF
            fn(C.byref(hin), C.byref(out))
            res.append((out.winner, out.turns, sum(out.player[i].hp for i in range(out.n_player))))
        self.battles += len(res)
        return res

    def _mon_dict(self, m):
        c = self.c
        return {"species": c.species_name.get(m.species), "level": m.level, "hp": m.hp, "max_hp": m.max_hp, "tp": m.tp,
                "tp_max": m.tp_max, "tp_spent": m.tp_spent, "status": status_name(m.status), "exp": m.exp,
                "exp_gained": m.exp_gained, "moves": [c.move_name.get(x) for x in m.moves if x], "alive": bool(m.alive),
                "item": c.item_name.get(m.item) if m.item else None, "special_awake": bool(m.special_awake), "special_progress": m.special_progress, "friendship": m.friendship}

    def _result(self, o, log):
        c = self.c
        res = {"winner": o.winner if o.winner in (0, 1) else 2, "turns": o.turns,
               "player": [self._mon_dict(o.player[i]) for i in range(o.n_player)],
               "foe": [self._mon_dict(o.foe[i]) for i in range(o.n_foe) if o.foe[i].species],
               "exp": [o.player[i].exp_gained for i in range(o.n_player)],
               "error": {0: None, 1: "frame cap", 2: "policy loop", 3: "bad input"}.get(o.error, "error %d" % o.error),
               "illegal": o.illegal, "outcome": o.outcome, "frames": o.frames, "leaked_blocks": o.leaked_blocks, "trainer": bool(o.kind)}
        st = []
        for s in (0, 1):
            st.append({"moves_used": {c.move_name.get(o.moves_used_id[s][i]): o.moves_used_n[s][i] for i in range(o.n_moves_used[s])},
                       "items_used": {c.item_name.get(o.items_used_id[i]): o.items_used_n[i] for i in range(o.n_items_used)} if s == 0 else {},
                       "talk_used": o.talk_used if s == 0 else 0, "special_used": o.special_used[s], "struggles": o.struggles[s],
                       "min_tp": o.min_tp[s] if o.min_tp[s] != 255 else None, "switches": o.switches[s]})
        res["stats"] = st
        res.update(moves_used=st[0]["moves_used"], items_used=st[0]["items_used"], talk_used=st[0]["talk_used"],
                   special_used=st[0]["special_used"], struggles=st[0]["struggles"], min_tp=st[0]["min_tp"])
        if log:
            lg = self.lib.hb_log()
            ev = []
            for i in range(o.log_n):
                e = lg[i]
                a = {ACT_MOVE: "move", ACT_SWITCH: "switch", ACT_ITEM: "item", ACT_TALK: "talk", 5: "move"}.get(e.action, "move")
                if e.action == ACT_MOVE and False:
                    pass
                row = {"turn": e.turn, "side": e.side, "slot": e.slot, "action": a,
                       "move": c.move_name.get(e.move) if e.move and e.action in (ACT_MOVE, 5, ACT_ITEM) and e.action != ACT_ITEM else None}
                if e.action == ACT_ITEM:
                    row["item"] = c.item_name.get(e.move)
                if e.action == ACT_TALK:
                    row["kind"] = "provoke" if e.target_slot == 0 else "unnerve"
                if e.action == ACT_SWITCH:
                    row["to"] = e.target_slot
                if e.action == 5:
                    row.update(damage=e.damage, crit=bool(e.crit), miss=bool(e.miss), effectiveness=("not_very", "normal", "super", "none")[e.eff],
                               ko=bool(e.ko), hp_after=e.hp_after)
                else:
                    row.update(damage=0, crit=False, miss=False, effectiveness=None, ko=False)
                ev.append((e.action == 5, row))
            # decisions (action 1-4) and executed moves (5): keep the executed-move rows, and the other decisions
            res["log"] = [r for is_exec, r in ev if is_exec or r["action"] != "move"]
        return res


def _action_code(a, c):
    if isinstance(a, int):
        return a
    if not isinstance(a, dict):
        return -1
    t = a.get("type")
    if t == "move":
        return ACT_MOVE | (int(a.get("slot", 0)) << 8)
    if t == "special":
        return ACT_MOVE | (SPECIAL_SLOT << 8)
    if t == "switch":
        return ACT_SWITCH | (int(a["slot"]) << 8)
    if t == "item":
        return ACT_ITEM | (c.it(a["item"]) << 8) | (int(a["target"]) << 24)
    if t == "talk":
        return ACT_TALK | ((0 if a.get("kind", "provoke") == "provoke" else 1) << 8)
    return -1
