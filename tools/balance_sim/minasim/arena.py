"""Arena ecology knobs (src/arena_ecology.c, concept in docs/design/boss-battles.md).

The host build makes `gArenaTuning` writable, so the lab can sweep the knobs without a rebuild:
    from minasim import arena
    arena.apply(engine, {"ventFlyerSpeedPct": 150})    # every other knob back to the ROM default
Scenario fights name their arena with `arena: LUCKY_STRIKE`; `minasim journey --arena key=value` overrides knobs.
"""
import ctypes as C

ARENAS = {"NONE": 0, "LUCKY_STRIKE": 1, "GRUNGE_ROCK": 2, "FAULT_LINE": 3}   # GRUNGE_ROCK is retired (Takeshi is FAULT_LINE)
FIELDS = ["ventFlyerSpeedPct", "stillFlyerSpeedPct", "firstMoverCritStages", "firstMoverNeedsVents", "leekAlwaysCrits",
          "leekNeedsVents", "ventBlockTurns", "ventBlockers", "windRestartsVents", "sandAttackStormTurns",
          "rockSpeedPct", "rockDefPct", "rockSpDefPct", "rockDefFlyers", "rockFirePowerPct", "wetTurns",
          "wetEvasionStages", "gustStormTurns", "rockSandAttackStormTurns", "rockSandSpDefPct",
          "seedDrainPct",
          # Fault Line (appended: the C struct is read by position)
          "faultSpeedPct", "faultDefIntactPct", "faultDefCrackedPct", "faultDefFracturedPct", "faultCrackBlows",
          "faultFractureBlows", "faultHeavyPower", "faultRootBlows", "faultMudTurns", "faultMudStopsCracks",
          "faultMudWaterPct", "faultWindStormTurns", "faultFractureStormTurns", "faultDryWaterPct", "faultMudWaterMissStages"]
FOR_REST_OF_BATTLE = 0xFF
BLOCK = {"STRING_SHOT": 1, "OBSTRUCT": 2, "VINES": 4, "SAND": 8}
BLOCK_MOVES = {1: ["STRING_SHOT"], 2: ["SMOKESCREEN", "POISON_POWDER", "STUN_SPORE", "SLEEP_POWDER", "SPORE"],
               4: ["VINE_WHIP", "LEECH_SEED"], 8: ["SAND_ATTACK"]}


# Fault Line knob defaults, mirroring the initializer in src/arena_ecology.c (tests/test_fault_line.py checks them against the
# built library and against include/arena_ecology.h). PLACEHOLDER: owner thread "Design arena ecology for gym battles".
FAULT_LINE_DEFAULTS = {
    "faultSpeedPct": 67, "faultDefIntactPct": 200, "faultDefCrackedPct": 150, "faultDefFracturedPct": 100,
    "faultCrackBlows": 3, "faultFractureBlows": 7, "faultHeavyPower": 80, "faultRootBlows": 1, "faultMudTurns": 4,
    "faultMudStopsCracks": 1, "faultMudWaterPct": 60, "faultWindStormTurns": 3, "faultFractureStormTurns": 0, "faultDryWaterPct": 80, "faultMudWaterMissStages": 1,
}
TERRAIN_INTACT, TERRAIN_CRACKED, TERRAIN_FRACTURED = 0, 1, 2
WIND_MOVES = {"GUST", "WHIRLWIND", "RAZOR_WIND", "WING_ATTACK", "SKY_ATTACK"}
EXPLOSION_MOVES = {"EXPLOSION", "SELF_DESTRUCT"}


class FaultLine:
    """Reference model of the Fault Line state machine, line for line the C in FaultLineAfterMove / ArenaEndTurn /
    ArenaModifyDamage / ArenaModifySpeed. The lab itself runs the C (host build); this class is for tests, docs and
    quick what-ifs. `hit` = the move was a damaging move that landed (power != 0, not missed, no immunity)."""

    def __init__(self, knobs=None):
        self.k = dict(FAULT_LINE_DEFAULTS, **(knobs or {}))
        self.terrain, self.mud, self.blows, self.storm = TERRAIN_INTACT, 0, 0, 0
        self.shown, self.queue = set(), []

    def _say(self, msg):
        if msg not in self.shown:
            self.shown.add(msg)
            self.queue.append(msg)

    def _set_terrain(self, t):
        if t <= self.terrain:
            return
        self.terrain = t
        if t == TERRAIN_CRACKED:
            self._say("GROUND_CRACKS")
        else:
            self._say("FLOOR_FRACTURED")
            if self._storm(self.k["faultFractureStormTurns"]):
                self._say("DUST_STORM")

    def _storm(self, turns):
        if turns == 0 or self.storm:
            return False
        self.storm = turns
        return True

    def after_move(self, move, power, mtype, hit):
        k = self.k
        if move in EXPLOSION_MOVES:
            self._set_terrain(TERRAIN_FRACTURED)
        if hit and not (self.mud and k["faultMudStopsCracks"]):
            blows = 1 + (1 if k["faultHeavyPower"] and power >= k["faultHeavyPower"] else 0) + (k["faultRootBlows"] if mtype == "GRASS" else 0)
            self.blows = min(255, self.blows + blows)
            if self.blows >= k["faultFractureBlows"]:
                self._set_terrain(TERRAIN_FRACTURED)
            elif self.blows >= k["faultCrackBlows"]:
                self._set_terrain(TERRAIN_CRACKED)
        if hit and mtype == "WATER" and k["faultMudTurns"]:
            if self.mud == 0:
                self._say("FLOOR_MUD")
            self.mud = k["faultMudTurns"]
        if move in WIND_MOVES:
            if self.mud:
                self.mud = 0
            elif self._storm(k["faultWindStormTurns"]):
                self._say("DUST_STORM")

    def end_turn(self):
        if self.mud and self.mud != FOR_REST_OF_BATTLE:
            self.mud -= 1
        if self.storm:
            self.storm -= 1

    def defense_pct(self, mountain):
        if not mountain:
            return 100
        return (self.k["faultDefIntactPct"], self.k["faultDefCrackedPct"], self.k["faultDefFracturedPct"])[self.terrain]

    def power_pct(self, mtype):
        if mtype != "WATER":
            return 100
        return self.k["faultMudWaterPct"] if self.mud else self.k["faultDryWaterPct"]

    def speed_pct(self, dweller):
        return 100 if dweller else self.k["faultSpeedPct"]


class ArenaTuning(C.Structure):
    _fields_ = [(f, C.c_uint16) for f in FIELDS]


_DEFAULTS = {}


def tuning(eng):
    fn = eng.lib.hb_arena_tuning
    fn.restype = C.c_void_p
    return ArenaTuning.from_address(fn())


def apply(eng, overrides=None):
    """Reset every knob to the ROM default, then apply `overrides`; returns the knobs in force."""
    t = tuning(eng)
    if not _DEFAULTS:
        _DEFAULTS.update({f: getattr(t, f) for f in FIELDS})
    now = dict(_DEFAULTS)
    for k, v in (overrides or {}).items():
        if k not in now:
            raise KeyError("unknown arena knob %s (known: %s)" % (k, ", ".join(FIELDS)))
        now[k] = int(v)
    for k, v in now.items():
        setattr(t, k, v)
    return now


def blockers(knobs):
    return {m for bit, ms in BLOCK_MOVES.items() if knobs["ventBlockers"] & bit for m in ms}


def parse_overrides(items):
    out = {}
    for it in items or []:
        k, v = it.split("=", 1)
        out[k.strip()] = int(v, 0)
    return out
