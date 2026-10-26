"""Single battles with the ROM's integer damage math plus the Minamoto rules (TP, crit stages, badges).

Every rule a designer might want to turn is in Rules; the defaults match develop + PR #9.
"""
import math
import random
from dataclasses import dataclass, field

from costcurve import TP_MODEL, curve_cost
from gamedata import PHYSICAL_TYPES, exp_for_level

# gStatStageRatios / sAccuracyStageRatios, index 0..12 = stage -6..+6
STAT_RATIOS = [(10, 40), (10, 35), (10, 30), (10, 25), (10, 20), (10, 15), (10, 10),
               (15, 10), (20, 10), (25, 10), (30, 10), (35, 10), (40, 10)]
ACC_RATIOS = [(33, 100), (36, 100), (43, 100), (50, 100), (60, 100), (75, 100), (1, 1),
              (133, 100), (166, 100), (2, 1), (233, 100), (133, 50), (3, 1)]

PINCH_ABILITY = {"BLAZE": "FIRE", "TORRENT": "WATER", "OVERGROW": "GRASS", "SWARM": "BUG"}
HIGH_CRIT_EFFECTS = {"HIGH_CRITICAL", "SKY_ATTACK", "BLAZE_KICK", "POISON_TAIL"}

# effect -> (target, stat, stages) for pure stat moves
STAT_MOVES = {
    "ATTACK_UP": ("self", "atk", 1), "ATTACK_UP_2": ("self", "atk", 2),
    "DEFENSE_UP": ("self", "def", 1), "DEFENSE_UP_2": ("self", "def", 2), "DEFENSE_CURL": ("self", "def", 1),
    "SPEED_UP_2": ("self", "spe", 2), "SPECIAL_ATTACK_UP": ("self", "spa", 1), "SPECIAL_ATTACK_UP_2": ("self", "spa", 2),
    "SPECIAL_DEFENSE_UP_2": ("self", "spd", 2), "EVASION_UP": ("self", "eva", 1), "MINIMIZE": ("self", "eva", 1),
    "ATTACK_DOWN": ("foe", "atk", -1), "ATTACK_DOWN_2": ("foe", "atk", -2),
    "DEFENSE_DOWN": ("foe", "def", -1), "DEFENSE_DOWN_2": ("foe", "def", -2),
    "SPEED_DOWN": ("foe", "spe", -1), "SPEED_DOWN_2": ("foe", "spe", -2),
    "ACCURACY_DOWN": ("foe", "acc", -1), "EVASION_DOWN": ("foe", "eva", -1),
    "SPECIAL_DEFENSE_DOWN_2": ("foe", "spd", -2),
}
# effect -> (stat, stages) applied to the target on a secondary-chance roll
SECONDARY_STAT = {
    "ATTACK_DOWN_HIT": ("atk", -1), "DEFENSE_DOWN_HIT": ("def", -1), "SPEED_DOWN_HIT": ("spe", -1),
    "ACCURACY_DOWN_HIT": ("acc", -1), "SPECIAL_DEFENSE_DOWN_HIT": ("spd", -1), "SPECIAL_ATTACK_DOWN_HIT": ("spa", -1),
}
SECONDARY_STATUS = {"POISON_HIT": "psn", "BURN_HIT": "brn", "PARALYZE_HIT": "par", "FREEZE_HIT": "frz",
                    "TWINEEDLE": "psn", "POISON_FANG": "psn", "POISON_TAIL": "psn", "BLAZE_KICK": "brn",
                    "THUNDER": "par"}
STATUS_MOVES = {"SLEEP": "slp", "POISON": "psn", "TOXIC": "psn", "PARALYZE": "par", "WILL_O_WISP": "brn"}
STATUS_IMMUNE = {"psn": {"POISON", "STEEL"}, "brn": {"FIRE"}, "frz": {"ICE"}, "par": set(), "slp": set()}
# Moves the simulator treats as plain damage (their extra behaviour doesn't matter early on)
PLAIN_HIT = {"HIT", "HIGH_CRITICAL", "QUICK_ATTACK", "ALWAYS_HIT", "GUST", "RAGE", "RAMPAGE", "PAY_DAY",
             "THIEF", "PURSUIT", "RAPID_SPIN", "FALSE_SWIPE", "VITAL_THROW", "SECRET_POWER", "TWISTER",
             "FLINCH_MINIMIZE_HIT", "THAW_HIT", "SKY_UPPERCUT", "BRICK_BREAK", "FACADE", "KNOCK_OFF",
             "RECOIL_IF_MISS", "SEMI_INVULNERABLE", "EARTHQUAKE", "MAGNITUDE", "TRAP", "ROLLOUT",
             "FURY_CUTTER", "OVERHEAT", "SUPERPOWER", "SKULL_BASH", "RAZOR_WIND", "SOLAR_BEAM", "SKY_ATTACK",
             "RECHARGE", "TRI_ATTACK", "ATTACK_UP_HIT", "ALL_STATS_UP_HIT", "DEFENSE_UP_HIT", "MEAN_LOOK"}
TWO_TURN = {"RAZOR_WIND", "SOLAR_BEAM", "SKY_ATTACK", "SKULL_BASH", "SEMI_INVULNERABLE"}

STRUGGLE = {"name": "STRUGGLE", "effect": "RECOIL", "power": 50, "type": "MYSTERY", "acc": 0, "tp": 0,
            "chance": 0, "priority": 0}


@dataclass
class Rules:
    tp_max: int = TP_MODEL["tp_max"]                # TP_MAX, TP_REGEN_*: what the ROM ships (tp_model.json)
    tp_regen_base: int = TP_MODEL["tp_regen_base"]
    tp_int_div: int = TP_MODEL["tp_int_div"]        # 0 = no INT term
    tp_persist: bool = True         # TP carries over between battles and only refills on heal (rest), as in the ROM
    cplus: bool = False             # C+ move ranks: a slot holds a family, rank cap rises with evolution stage
    cplus_single_stage: list = field(default_factory=lambda: [20, 35])   # levels for 2 and 3 stars without evolutions
    cplus_wild_rank1: bool = True   # player uses one star against wild mons, highest affordable rank vs trainers
    cplus_level_fallback: list = field(default_factory=list)   # e.g. 22,40: 2 and 3 stars by level if evolution is later
    cplus_star1_cost: int = 0       # e.g. 5: every family's one-star rank costs this, whatever its PP
    tp_formula: str = ""            # "" = the ROM's tpCost; else a Python expression of power/acc/status/rank
    tp_curve: bool = False          # continuous cost curve: round(curve_base + curve_k * value), see costcurve.py
    curve_base: float = TP_MODEL["curve_base"]
    curve_k: float = TP_MODEL["curve_k"]
    special_power: int = 0          # 0 = off; else every player mon gets a STAB Special of this power
    special_cost: int = 30          # TP per use of the Special
    special_level: int = 25         # level the Special is earned at
    special_habit: str = "greedy"   # greedy: used like any move; trainers: kept for trainer battles
    rank_costs: list = field(default_factory=list)          # e.g. 5,15,30: cost by move rank (overrides the rest)
    rank_status_costs: list = field(default_factory=list)   # e.g. 10,20,30 for status moves; defaults to rank_costs
    crit_table: list = field(default_factory=lambda: [16, 8, 4, 2, 1])   # 1 in N per stage
    crit_mult: int = 2
    flat_plus2: bool = True         # Emerald's +2 before the return (PR #9 removed it, Omar restored it)
    badge_boost: float = 1.25
    badge_types: list = field(default_factory=lambda: ["FLYING", "GROUND", "WATER", "ELECTRIC", "GHOST", "POISON", "FIRE", "MYSTERY"])
    badge3_speed: bool = False      # Emerald's x1.1 speed for badge 3 (removed in PR #9)
    stab: float = 1.5
    gym_sash: bool = True           # arena_ecology 0.1, not in the ROM yet
    exp_trainer_mult: float = 1.5
    pinch_abilities: bool = True

    def regen(self, data, species):
        return self.tp_regen_base + (data.intelligence(species) // self.tp_int_div if self.tp_int_div else 0)

    def tp_cost(self, move):
        if move["name"] == "STRUGGLE":
            return 0
        if move.get("special"):
            return self.special_cost
        if self.rank_costs:
            ladder = self.rank_status_costs if move["power"] == 0 and self.rank_status_costs else self.rank_costs
            return ladder[move.get("rank", 1) - 1]
        if self.tp_curve:
            return curve_cost(move, self.curve_base, self.curve_k)
        if not self.tp_formula:
            return move["tp"]
        env = {"power": move["power"], "acc": move["acc"], "status": move["power"] == 0, "rank": move.get("rank", 1),
               "pp": move.get("family_pp", move.get("pp", 0)) if self.cplus else move.get("pp", 0),
               "round5": lambda x: int(5 * round(x / 5)), "math": math, "max": max, "min": min}
        return max(0, int(eval(self.tp_formula, {"__builtins__": {}}, env)))

    def set(self, key, value):
        cur = getattr(self, key)
        if isinstance(cur, list) and not cur:
            cur = [0]
        if isinstance(cur, bool):
            value = str(value).lower() in ("1", "true", "yes", "on")
        elif isinstance(cur, list):
            value = [type(cur[0])(v) for v in str(value).split(",")] if not isinstance(value, list) else value
        elif not isinstance(value, type(cur)):
            value = type(cur)(value)
        setattr(self, key, value)


class Mon:
    def __init__(self, data, species, level, moves=None, ivs=None, item=None, owner="wild"):
        self.data = data
        self.species = species
        self.level = level
        self.ivs = ivs if ivs is not None else random.randint(0, 31)
        self.item = item
        self.owner = owner
        self.exp = exp_for_level(data.species[species]["growth"], level)
        self.moves = list(moves) if moves else data.moves_at(species, level)
        if not self.moves:
            self.moves = ["STRUGGLE"]
        self.calc_stats()
        self.hp = self.maxhp
        self.status = None
        self.tp = None
        self.reset_battle()

    @property
    def base(self):
        return self.data.species[self.species]

    @property
    def types(self):
        return self.base["types"]

    def calc_stats(self):
        b, L, iv = self.base, self.level, self.ivs
        old = getattr(self, "maxhp", None)
        self.maxhp = (2 * b["hp"] + iv) * L // 100 + L + 10
        self.stats = {k: (2 * b[k] + iv) * L // 100 + 5 for k in ("atk", "def", "spa", "spd", "spe")}
        if old is not None:
            self.hp = min(self.maxhp, self.hp + self.maxhp - old)

    def reset_battle(self):
        self.stages = {k: 0 for k in ("atk", "def", "spa", "spd", "spe", "acc", "eva")}
        self.sash_used = False
        self.confused = 0
        self.focus = False
        self.seeded = False
        self.sleep = 0
        self.charging = None

    def alive(self):
        return self.hp > 0

    def heal(self):
        self.tp = None
        self.hp = self.maxhp
        self.status = None
        self.sleep = 0

    def label(self):
        return "%s L%d" % (self.species.title(), self.level)


def stage_stat(value, stage):
    n, d = STAT_RATIOS[stage + 6]
    return value * n // d


class Stats:
    """Counters for one battle (summed across Monte Carlo runs by the caller)."""

    def __init__(self):
        self.turns = 0
        self.struggles = {"player": 0, "foe": 0}
        self.tp_starved_turns = {"player": 0, "foe": 0}   # turns where the preferred move was unaffordable
        self.min_tp = {"player": 100, "foe": 100}
        self.damage = {"player": 0, "foe": 0}
        self.crit_damage = {"player": 0, "foe": 0}
        self.crits = {"player": 0, "foe": 0}
        self.hits = {"player": 0, "foe": 0}
        self.sash_saves = 0
        self.multi_rank_turns = 0
        self.rank_choice = [0, 0, 0]
        self.rank_use = {"player": [0, 0, 0], "foe": [0, 0, 0]}   # damaging moves used, by rank
        self.chose_lower = {"player": 0, "foe": 0}      # used a lower rank while a higher one was affordable
        self.forced_lower = {"player": 0, "foe": 0}     # used a lower rank because the higher was unaffordable
        self.special_uses = 0
        self.unmodelled = set()


class Battle:
    def __init__(self, data, rules, player_party, foe_party, kind="wild", badges=(), foe_ai="random",
                 player_ai="smart", sash=False, first_battle=False):
        self.data, self.rules = data, rules
        self.parties = {"player": player_party, "foe": foe_party}
        self.kind = kind
        self.badges = set(badges)
        self.ai = {"player": player_ai, "foe": foe_ai}
        self.sash = sash and rules.gym_sash
        self.no_crits = first_battle
        self.stats = Stats()
        self.participants = {}
        self.exp_log = []
        for side in self.parties:
            for m in self.parties[side]:
                m.reset_battle()
                if not rules.tp_persist or m.tp is None:
                    m.tp = rules.tp_max
        self.active = {s: self._next_alive(s) for s in self.parties}
        self._on_switch_in("player")
        self._on_switch_in("foe")

    # ---------------------------------------------------------------- helpers
    def _next_alive(self, side):
        for m in self.parties[side]:
            if m.alive():
                return m
        return None

    def _other(self, side):
        return "foe" if side == "player" else "player"

    def _on_switch_in(self, side):
        me, foe = self.active[side], self.active[self._other(side)]
        if me is None or foe is None:
            return
        if side == "foe":
            self.participants.setdefault(id(me), set()).add(id(self.active["player"]))
        else:
            self.participants.setdefault(id(foe), set()).add(id(me))
        if me.base["ability"] == "INTIMIDATE":
            foe.stages["atk"] = max(-6, foe.stages["atk"] - 1)

    def cost(self, move_name, mon=None):
        if mon is not None and self.rules.cplus and self.rules.cplus_star1_cost and move_name in getattr(mon, "star1", ()):
            return self.rules.cplus_star1_cost
        return self.rules.tp_cost(self._move(move_name))

    def _move(self, name):
        return STRUGGLE if name == "STRUGGLE" else self.data.moves[name]

    def speed(self, side):
        m = self.active[side]
        s = stage_stat(m.stats["spe"], m.stages["spe"])
        if side == "player" and self.rules.badge3_speed and self.kind != "link" and 3 in self.badges:
            s = s * 110 // 100
        if m.status == "par":
            s //= 4
        return s

    # ---------------------------------------------------------------- damage
    def base_damage(self, atk_side, attacker, defender, move, crit, power=None):
        r = self.rules
        power = power if power is not None else move["power"]
        mtype = move["type"]
        if r.pinch_abilities and PINCH_ABILITY.get(attacker.base["ability"]) == mtype and attacker.hp <= attacker.maxhp // 3:
            power = power * 150 // 100
        if mtype in PHYSICAL_TYPES or mtype == "MYSTERY":
            a_key, d_key = "atk", "def"
        else:
            a_key, d_key = "spa", "spd"
        a_st, d_st = attacker.stages[a_key], defender.stages[d_key]
        a = stage_stat(attacker.stats[a_key], a_st if not (crit and a_st < 0) else 0)
        d = stage_stat(defender.stats[d_key], d_st if not (crit and d_st > 0) else 0)
        dmg = a * power
        dmg *= 2 * attacker.level // 5 + 2
        dmg //= max(1, d)
        dmg //= 50
        if a_key == "atk" and attacker.status == "brn":
            dmg //= 2
        if r.flat_plus2:
            dmg += 2
        if atk_side == "player" and self.kind != "link" and self._badge_boosted(mtype):
            dmg = int(dmg * r.badge_boost)
        if dmg == 0 and mtype != "MYSTERY":
            dmg = 1
        return dmg

    def _badge_boosted(self, mtype):
        return any(self.rules.badge_types[i - 1] == mtype for i in self.badges if i - 1 < len(self.rules.badge_types))

    def full_damage(self, atk_side, attacker, defender, move, crit, roll):
        dmg = self.base_damage(atk_side, attacker, defender, move, crit) * (self.rules.crit_mult if crit else 1)
        if move["name"] == "STRUGGLE":
            return max(1, dmg * roll // 100)
        if move["type"] in attacker.types:
            dmg = int(dmg * self.rules.stab)
        eff = self.data.effectiveness(move["type"], defender.types)
        if eff == 0:
            return 0
        for t in defender.types:
            dmg = dmg * self.data.type_chart.get((move["type"], t), 10) // 10
        dmg = dmg * roll // 100
        return max(1, dmg)

    def crit_stage(self, attacker, move):
        stage = 2 * attacker.focus + (move["effect"] in HIGH_CRIT_EFFECTS)
        if attacker.item == "LEEK" and attacker.species == "FARFETCHD":
            stage += 2
        return min(stage, len(self.rules.crit_table) - 1)

    def expected_damage(self, atk_side, attacker, defender, move):
        """Mean damage of one use, counting accuracy, crit odds and multi-hit, for move choice."""
        fixed = self._fixed_damage(attacker, defender, move)
        if fixed is not None:
            hit = 1.0
        else:
            if move["power"] == 0:
                return 0.0
            p_crit = 1 / self.rules.crit_table[self.crit_stage(attacker, move)]
            base = self.full_damage(atk_side, attacker, defender, move, False, 92)
            crit = self.full_damage(atk_side, attacker, defender, move, True, 92)
            fixed = (1 - p_crit) * base + p_crit * crit
        hits = {"MULTI_HIT": 3.0, "DOUBLE_HIT": 2, "TWINEEDLE": 2, "TRIPLE_KICK": 3}.get(move["effect"], 1)
        acc = 1.0 if move["acc"] == 0 or move["effect"] == "ALWAYS_HIT" else move["acc"] / 100
        if move["effect"] in TWO_TURN:
            acc /= 2
        return fixed * hits * acc

    def _fixed_damage(self, attacker, defender, move):
        e = move["effect"]
        if self.data.effectiveness(move["type"], defender.types) == 0 and move["power"] > 0:
            return 0
        if e == "SONICBOOM":
            return 20
        if e == "DRAGON_RAGE":
            return 40
        if e == "LEVEL_DAMAGE":
            return attacker.level
        if e == "SUPER_FANG":
            return max(1, defender.hp // 2)
        if e == "PSYWAVE":
            return attacker.level
        return None

    # ---------------------------------------------------------------- choice
    def rank_cap(self, mon):
        """Basic form 1 star, middle form 2, final form 3; single-stage species unlock by level."""
        stage, final = self.data.evo_stage(mon.species)
        if stage == 1 and final:
            lv2, lv3 = self.rules.cplus_single_stage
            cap = 1 + (mon.level >= lv2) + (mon.level >= lv3)
        else:
            cap = 3 if final else stage
        if self.rules.cplus_level_fallback:
            lv2, lv3 = self.rules.cplus_level_fallback
            cap = max(cap, 1 + (mon.level >= lv2) + (mon.level >= lv3))
        return cap

    def slot_options(self, mon, slot):
        """Moves a C+ slot offers this turn, lowest rank first."""
        fam = self.data.slot_family(slot)
        if fam is None:
            return [slot]
        ranks = list(self.data.families[fam])
        if slot not in ranks:
            ranks[0] = slot          # a type-mapped ROM move (Vine Whip) stands in as the one-star rank
        return [mv for mv in ranks[:self.rank_cap(mon)] if mv]

    def choose_cplus(self, side, me, foe):
        st = self.stats
        slots = [self.slot_options(me, s) for s in me.moves if s != "STRUGGLE"]
        me.star1 = {o[0] for o in slots if self.data.slot_family(o[0])}
        st.multi_rank_turns += any(len(o) > 1 for o in slots)
        conservative = self.rules.cplus_wild_rank1 and self.kind == "wild" and self.ai["player"] == "smart"
        wanted, picks = [], []
        for opts in slots:
            want = opts[0] if conservative else opts[-1]
            wanted.append(want)
            pick = next((mv for mv in reversed(opts[:opts.index(want) + 1]) if me.tp >= self.cost(mv, me)), None)
            if pick:
                picks.append(pick)
        ev = lambda mv: self.expected_damage(side, me, foe, self._move(mv))
        if not picks:
            st.struggles[side] += 1
            st.tp_starved_turns[side] += 1
            return "STRUGGLE"
        best_wanted = max(wanted, key=ev)
        choice = max(picks, key=ev)
        if ev(choice) <= 0:
            choice = random.choice(picks)
        if best_wanted != choice and me.tp < self.cost(best_wanted, me):
            st.tp_starved_turns[side] += 1
        st.rank_choice[next((o.index(choice) for o in slots if choice in o), 0)] += 1
        return choice

    def choose(self, side):
        me, foe = self.active[side], self.active[self._other(side)]
        if self.rules.cplus and side == "player":
            return self.choose_cplus(side, me, foe)
        special = self.special_move(side, me)
        moves = me.moves + ([special] if special else [])
        affordable = [mv for mv in moves if mv != "STRUGGLE" and me.tp >= self.cost(mv)]
        if not affordable:
            self.stats.struggles[side] += 1
            return "STRUGGLE"
        policy = self.ai[side]
        scored = sorted(((self.expected_damage(side, me, foe, self._move(mv)), mv) for mv in moves if mv != "STRUGGLE"), reverse=True)
        regular = [mv for d, mv in scored if mv != special]
        if regular and regular[0] not in affordable:
            self.stats.tp_starved_turns[side] += 1
        choice = self._pick(side, policy, me, foe, scored, affordable)
        if special and choice == special:
            self.stats.special_uses += 1
        return choice

    def special_move(self, side, mon):
        """The mon's Special (a STAB hit of special_power), once earned, or None."""
        r = self.rules
        if side != "player" or not r.special_power or mon.level < r.special_level:
            return None
        if r.special_habit == "trainers" and self.kind == "wild":
            return None
        name = "SPECIAL_" + mon.types[0]
        if name not in self.data.moves:
            self.data.moves[name] = {"name": name, "effect": "HIT", "power": r.special_power, "type": mon.types[0],
                                     "acc": 100, "tp": 0, "pp": 5, "chance": 0, "priority": 0, "special": True}
        return name

    def _pick(self, side, policy, me, foe, scored, affordable):
        if policy == "random":
            return random.choice(affordable)
        if policy == "rank":
            # highest affordable rank among moves that can hurt the foe, then the strongest of that rank
            hitting = [mv for mv in affordable if self.expected_damage(side, me, foe, self._move(mv)) > 0]
            if hitting:
                return max(hitting, key=lambda mv: (self._move(mv).get("rank", 1), self.expected_damage(side, me, foe, self._move(mv))))
            return random.choice(affordable)
        dmg = [(d, mv) for d, mv in scored if mv in affordable]
        if policy == "trainer":
            # Emerald trainer AI, roughly: skip moves that can't affect the foe, usually go for the
            # strongest hit, otherwise anything (status moves included)
            useful = [mv for d, mv in dmg if d > 0 or self._move(mv)["power"] == 0] or affordable
            return dmg[0][1] if dmg and dmg[0][0] > 0 and random.random() < 0.75 else random.choice(useful)
        # smart: strongest affordable hit; use the cheapest move that still KOs; status moves only if nothing hits
        if not dmg or dmg[0][0] <= 0:
            return random.choice(affordable)
        killers = [mv for d, mv in dmg if d >= foe.hp]
        if killers:
            return min(killers, key=lambda mv: (self.cost(mv), -self.expected_damage(side, me, foe, self._move(mv))))
        return dmg[0][1]

    # ---------------------------------------------------------------- turn
    def turn(self):
        self.stats.turns += 1
        choices = {}
        for s in ("player", "foe"):
            m = self.active[s]
            choices[s] = m.charging or self.choose(s)
        order = sorted(("player", "foe"), key=lambda s: (self._move(choices[s])["priority"], self.speed(s), random.random()), reverse=True)
        started = dict(self.active)
        moved = set()
        for s in order:
            me, foe = self.active[s], self.active[self._other(s)]
            if me is None or foe is None or not me.alive() or me is not started[s]:
                continue
            self.act(s, choices[s], moved)
            moved.add(s)
            if self.check_faints():
                return self.result()
        self.end_of_turn()
        self.check_faints()
        return self.result()

    def act(self, side, move_name, moved):
        me, foe = self.active[side], self.active[self._other(side)]
        move = self._move(move_name)
        # can't-move checks (no TP is spent, like attackcanceler before ppreduce)
        if me.status == "slp":
            me.sleep -= 1
            if me.sleep > 0:
                return
            me.status = None
        if me.status == "frz":
            if random.random() < 0.2:
                me.status = None
            else:
                return
        if getattr(me, "flinched", False):
            me.flinched = False
            return
        if me.confused:
            me.confused -= 1
            if me.confused and random.random() < 0.5:
                me.hp -= self.base_damage(side, me, me, {"name": "CONFUSION_SELF", "type": "MYSTERY", "power": 40, "effect": "HIT"}, False)
                me.hp = max(0, me.hp)
                return
        if me.status == "par" and random.random() < 0.25:
            return
        # two-turn moves: charge now, fire next turn (TP is paid on the charging turn)
        if move["effect"] in TWO_TURN and me.charging is None:
            me.tp -= min(me.tp, self.cost(move_name, me))
            self._track_tp(side, me)
            me.charging = move_name
            return
        charged = me.charging is not None
        me.charging = None
        if move["power"] and move_name != "STRUGGLE" and not charged:
            self._track_rank(side, me, move)
        if not charged:
            me.tp -= min(me.tp, self.cost(move_name, me))
            self._track_tp(side, me)
        e = move["effect"]
        if e not in PLAIN_HIT and e not in STAT_MOVES and e not in SECONDARY_STAT and e not in SECONDARY_STATUS \
                and e not in STATUS_MOVES and e not in {"MULTI_HIT", "DOUBLE_HIT", "TRIPLE_KICK", "RECOIL", "DOUBLE_EDGE",
                                                       "ABSORB", "SONICBOOM", "DRAGON_RAGE", "LEVEL_DAMAGE", "SUPER_FANG",
                                                       "PSYWAVE", "FOCUS_ENERGY", "CONFUSE", "CONFUSE_HIT", "FLINCH_HIT",
                                                       "LEECH_SEED", "OHKO", "EXPLOSION"}:
            self.stats.unmodelled.add(move_name)
        # accuracy
        if move["acc"] and e != "ALWAYS_HIT" and not self._hits(me, foe, move):
            return
        if move["power"] == 0 and self._fixed_damage(me, foe, move) is None:
            self.status_move(side, me, foe, move)
            return
        if e == "OHKO":
            if me.level >= foe.level:
                self._deal(side, me, foe, foe.hp, False)
            return
        hits = 1
        if e == "MULTI_HIT":
            hits = random.choice([2, 2, 2, 3, 3, 3, 4, 5])
        elif e in ("DOUBLE_HIT", "TWINEEDLE"):
            hits = 2
        dealt = 0
        for _ in range(hits):
            if not foe.alive():
                break
            fixed = self._fixed_damage(me, foe, move)
            crit = False
            if fixed is None:
                crit = not self.no_crits and random.randrange(self.rules.crit_table[self.crit_stage(me, move)]) == 0
                dmg = self.full_damage(side, me, foe, move, crit, 100 - random.randrange(16))
            else:
                dmg = fixed
            if dmg == 0:
                return
            dealt += self._deal(side, me, foe, dmg, crit)
        if e in ("RECOIL", "DOUBLE_EDGE") and dealt:
            me.hp = max(0, me.hp - max(1, dealt // (3 if e == "DOUBLE_EDGE" else 4)))
        if e == "ABSORB" and dealt:
            me.hp = min(me.maxhp, me.hp + max(1, dealt // 2))
        if e == "EXPLOSION":
            me.hp = 0
        if foe.alive() and move["chance"] and random.randrange(100) < move["chance"]:
            if e in SECONDARY_STAT:
                stat, n = SECONDARY_STAT[e]
                foe.stages[stat] = max(-6, min(6, foe.stages[stat] + n))
            elif e in SECONDARY_STATUS:
                self._inflict(foe, SECONDARY_STATUS[e])
            elif e in ("FLINCH_HIT", "FLINCH_MINIMIZE_HIT") and self._other(side) not in moved:
                foe.flinched = True
            elif e == "CONFUSE_HIT" and not foe.confused:
                foe.confused = random.randint(2, 5)

    def _track_rank(self, side, me, move):
        rank = move.get("rank", 1)
        self.stats.rank_use[side][rank - 1] += 1
        higher = [self._move(mv) for mv in me.moves if mv != "STRUGGLE" and self._move(mv)["power"]
                  and self._move(mv).get("rank", 1) > rank]
        if higher:
            # called before this move's TP is paid
            if any(me.tp >= self.rules.tp_cost(h) for h in higher):
                self.stats.chose_lower[side] += 1
            else:
                self.stats.forced_lower[side] += 1

    def _hits(self, me, foe, move):
        buff = max(0, min(12, me.stages["acc"] + 6 - foe.stages["eva"]))
        n, d = ACC_RATIOS[buff]
        return random.randrange(100) + 1 <= move["acc"] * n // d

    def _deal(self, side, me, foe, dmg, crit):
        if self.sash and side == "player" and dmg >= foe.hp and foe.hp > 0 and not foe.sash_used:
            foe.sash_used = True
            self.stats.sash_saves += 1
            dmg = foe.hp - 1
        dmg = min(dmg, foe.hp)
        foe.hp -= dmg
        self.stats.damage[side] += dmg
        self.stats.hits[side] += 1
        if crit:
            self.stats.crits[side] += 1
            self.stats.crit_damage[side] += dmg
        return dmg

    def _inflict(self, mon, status):
        if mon.status or any(t in STATUS_IMMUNE[status] for t in mon.types):
            return False
        mon.status = status
        if status == "slp":
            mon.sleep = random.randint(2, 5)
        return True

    def status_move(self, side, me, foe, move):
        e = move["effect"]
        if e in STAT_MOVES:
            who, stat, n = STAT_MOVES[e]
            tgt = me if who == "self" else foe
            tgt.stages[stat] = max(-6, min(6, tgt.stages[stat] + n))
        elif e in STATUS_MOVES:
            self._inflict(foe, STATUS_MOVES[e])
        elif e == "FOCUS_ENERGY":
            me.focus = True
        elif e == "CONFUSE" and not foe.confused:
            foe.confused = random.randint(2, 5)
        elif e == "LEECH_SEED" and "GRASS" not in foe.types:
            foe.seeded = True

    def _track_tp(self, side, me):
        self.stats.min_tp[side] = min(self.stats.min_tp[side], me.tp)

    def end_of_turn(self):
        for s in ("player", "foe"):
            m, o = self.active[s], self.active[self._other(s)]
            if m is None or not m.alive():
                continue
            m.flinched = False
            if m.status in ("psn", "brn"):
                m.hp = max(0, m.hp - max(1, m.maxhp // 8))
            if m.seeded and o is not None and o.alive():
                d = min(m.hp, max(1, m.maxhp // 8))
                m.hp -= d
                o.hp = min(o.maxhp, o.hp + d)
            if m.alive():
                m.tp = min(self.rules.tp_max, m.tp + self.rules.regen(self.data, m.species))

    def check_faints(self):
        changed = False
        for s in ("foe", "player"):
            m = self.active[s]
            if m is not None and not m.alive():
                if s == "foe":
                    self.award_exp(m)
                self.active[s] = self._next_alive(s)
                self._on_switch_in(s)
                changed = True
        return changed and self.result() is not None

    def award_exp(self, fainted):
        ids = self.participants.get(id(fainted), set())
        winners = [m for m in self.parties["player"] if id(m) in ids and m.alive()]
        if not winners:
            return
        exp = fainted.base["exp_yield"] * fainted.level // 7 // len(winners)
        if self.kind != "wild":
            exp = int(exp * self.rules.exp_trainer_mult)
        for m in winners:
            m.exp += exp
            self.exp_log.append((m, exp))

    def result(self):
        if self.active["foe"] is None:
            return "win"
        if self.active["player"] is None:
            return "loss"
        return None

    def run(self, max_turns=200):
        res = self.result()
        while res is None and self.stats.turns < max_turns:
            res = self.turn()
        return res or "timeout"


def level_up(mon, log=None, cplus=False, evolve=True):
    """Apply pending EXP: levels, new moves (replacing the weakest if full) and level evolutions."""
    data = mon.data
    growth = mon.base["growth"]
    while mon.level < 100 and mon.exp >= exp_for_level(growth, mon.level + 1):
        mon.level += 1
        mon.calc_stats()
        for lvl, mv in data.learnsets.get(mon.species, []):
            if lvl == mon.level and mv not in mon.moves:
                learn(mon, mv, log, cplus)
    evo = data.evolutions.get(mon.species)
    if evolve and evo and mon.level >= evo[0] and evo[1] in data.species:
        if log is not None:
            log.append("%s evolved into %s at L%d" % (mon.species, evo[1], mon.level))
        mon.species = evo[1]
        mon.calc_stats()


def learn(mon, mv, log=None, cplus=False):
    data = mon.data
    new = data.moves[mv]
    if cplus:
        fam = data.slot_family(mv)
        same = [m for m in mon.moves if data.slot_family(m) == fam] if fam else []
        if same:
            # one slot per family; a stronger type-mapped move (Water Gun over Bubble) becomes its one-star rank
            old = same[0]
            if mv not in data.family_of and new["power"] > data.moves[old]["power"] and old not in data.families[fam][1:]:
                mon.moves[mon.moves.index(old)] = mv
            return
    if len(mon.moves) < 4:
        mon.moves.append(mv)
        return
    if new["power"] == 0:
        return
    weakest = min(mon.moves, key=lambda m: (data.moves[m]["power"], -data.moves[m]["tp"]))
    if data.moves[weakest]["power"] < new["power"] or data.moves[weakest]["power"] == 0:
        mon.moves[mon.moves.index(weakest)] = mv
        if log is not None:
            log.append("%s forgot %s for %s" % (mon.species, weakest, mv))
