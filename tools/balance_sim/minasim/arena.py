"""Arena ecology knobs (src/arena_ecology.c, concept in docs/design/boss-battles.md).

The host build makes `gArenaTuning` writable, so the lab can sweep the knobs without a rebuild:
    from minasim import arena
    arena.apply(engine, {"ventFlyerSpeedPct": 150})    # every other knob back to the ROM default
Scenario fights name their arena with `arena: LUCKY_STRIKE`; `minasim journey --arena key=value` overrides knobs.
"""
import ctypes as C

ARENAS = {"NONE": 0, "LUCKY_STRIKE": 1, "GRUNGE_ROCK": 2}
FIELDS = ["ventFlyerSpeedPct", "stillFlyerSpeedPct", "firstMoverCritStages", "firstMoverNeedsVents", "leekAlwaysCrits",
          "leekNeedsVents", "ventBlockTurns", "ventBlockers", "windRestartsVents", "sandAttackStormTurns",
          "rockSpeedPct", "rockDefPct", "rockSpDefPct", "rockDefFlyers", "rockFirePowerPct", "wetTurns",
          "wetEvasionStages", "gustStormTurns", "rockSandAttackStormTurns", "rockSandSpDefPct",
          "seedDrainPct"]
FOR_REST_OF_BATTLE = 0xFF
BLOCK = {"STRING_SHOT": 1, "OBSTRUCT": 2, "VINES": 4, "SAND": 8}
BLOCK_MOVES = {1: ["STRING_SHOT"], 2: ["SMOKESCREEN", "POISON_POWDER", "STUN_SPORE", "SLEEP_POWDER", "SPORE"],
               4: ["VINE_WHIP", "LEECH_SEED"], 8: ["SAND_ATTACK"]}


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
