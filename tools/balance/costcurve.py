"""A continuous TP cost curve: cost = base + k * value, rounded.

`value` is a move's worth in expected base power per use, so a move that hits twice as hard costs
about twice the TP above the base, and stronger moves are pricier but not worse value.

  damaging move: power x accuracy x hits, x0.6 for two-turn or recharge moves, x1.1 for high crit
                 or priority, x0.85 for recoil, x1.25 for drain, plus chance x the secondary's value
  status move:   the turns of value it buys, in the same power units (table below), x accuracy

Run `python3 costcurve.py` for the table of early-game moves.
"""

# Status and secondary effects, in "base power" units: what a turn spent on them is worth.
# One attacking turn of an early move is ~40. Sleep costs the foe 2-5 turns, so ~2 attacks.
STATUS_VALUE = {
    "SLEEP": 80, "PARALYZE": 50, "POISON": 45, "TOXIC": 60, "WILL_O_WISP": 50, "CONFUSE": 45,
    "LEECH_SEED": 55, "RESTORE_HP": 60, "REST": 60, "SOFTBOILED": 60,
    "ATTACK_DOWN": 25, "DEFENSE_DOWN": 25, "SPEED_DOWN": 20, "ACCURACY_DOWN": 30, "EVASION_DOWN": 15,
    "ATTACK_DOWN_2": 40, "DEFENSE_DOWN_2": 40, "SPEED_DOWN_2": 30, "SPECIAL_DEFENSE_DOWN_2": 40,
    "ATTACK_UP": 25, "DEFENSE_UP": 20, "DEFENSE_CURL": 20, "SPECIAL_ATTACK_UP": 25, "EVASION_UP": 30,
    "MINIMIZE": 30, "ATTACK_UP_2": 45, "DEFENSE_UP_2": 35, "SPEED_UP_2": 35, "SPECIAL_ATTACK_UP_2": 45,
    "SPECIAL_DEFENSE_UP_2": 35, "FOCUS_ENERGY": 25, "DISABLE": 35, "MIST": 20, "LIGHT_SCREEN": 35,
    "REFLECT": 35, "HAZE": 30, "SUBSTITUTE": 40, "ROAR": 20, "METRONOME": 45, "MIMIC": 30,
    "MIRROR_MOVE": 40, "TELEPORT": 10, "SPLASH": 0,
}
SECONDARY_VALUE = {
    "BURN_HIT": 50, "PARALYZE_HIT": 50, "POISON_HIT": 45, "FREEZE_HIT": 70, "FLINCH_HIT": 30,
    "CONFUSE_HIT": 45, "THUNDER": 50, "TWINEEDLE": 45, "TRAP": 15,
    "ATTACK_DOWN_HIT": 25, "DEFENSE_DOWN_HIT": 25, "SPEED_DOWN_HIT": 20, "ACCURACY_DOWN_HIT": 30,
    "SPECIAL_DEFENSE_DOWN_HIT": 25, "SPECIAL_ATTACK_DOWN_HIT": 25,
}
# Moves whose damage doesn't come from power: an equivalent power for the early game
FIXED_POWER = {"LEVEL_DAMAGE": 60, "DRAGON_RAGE": 60, "SONICBOOM": 40, "SUPER_FANG": 70, "PSYWAVE": 50,
               "LOW_KICK": 50, "OHKO": 150}
HITS = {"MULTI_HIT": 3.0, "DOUBLE_HIT": 2.0, "TWINEEDLE": 2.0, "TRIPLE_KICK": 3.0}
SLOW = {"RAZOR_WIND", "SOLAR_BEAM", "SKY_ATTACK", "SKULL_BASH", "SEMI_INVULNERABLE", "RECHARGE"}
SHARP = {"HIGH_CRITICAL", "QUICK_ATTACK", "SKY_ATTACK", "BLAZE_KICK", "POISON_TAIL"}


def move_value(m):
    """Worth of one use in base-power units (see module docstring)."""
    eff, acc = m["effect"], (m["acc"] or 100) / 100
    if m["power"] == 0:
        return STATUS_VALUE.get(eff, 30) * acc
    power = FIXED_POWER.get(eff, m["power"])
    v = power * acc * HITS.get(eff, 1.0)
    if eff in SLOW:
        v *= 0.6
    if eff in SHARP or m.get("priority", 0) > 0:
        v *= 1.1
    if eff in ("RECOIL", "DOUBLE_EDGE"):
        v *= 0.85
    if eff in ("ABSORB", "DREAM_EATER"):
        v *= 1.25
    if eff == "EXPLOSION":
        v *= 0.5
    if m["acc"] == 0:
        v *= 1.05
    v += m.get("chance", 0) / 100 * SECONDARY_VALUE.get(eff, 0)
    return v


def curve_cost(m, base, k):
    return max(1, int(round(base + k * move_value(m))))


if __name__ == "__main__":
    import argparse
    from gamedata import GameData

    ap = argparse.ArgumentParser(description="TP cost table for the early-game moves")
    ap.add_argument("--base", type=float, default=1.0)
    ap.add_argument("--k", type=float, default=0.13)
    ap.add_argument("--markdown", action="store_true")
    a = ap.parse_args()
    d = GameData()
    early = ["BULBASAUR", "IVYSAUR", "CHARMANDER", "CHARMELEON", "SQUIRTLE", "WARTORTLE", "PIDGEY",
             "PIDGEOTTO", "NIDORAN_M", "NIDORINO", "NIDORAN_F", "NIDORINA", "PIKACHU"]
    names = {mv for s in early for l, mv in d.learnsets.get(s, []) if l <= 32}
    rows = sorted((curve_cost(d.moves[n], a.base, a.k), move_value(d.moves[n]), n) for n in names)
    fmt = "| %s | %d | %d | %d | %s |" if a.markdown else "%-14s %4d %4d %4d  %s"
    if a.markdown:
        print("| Move | Power | Value | TP | TP per 10 value |\n|---|---|---|---|---|")
    for cost, v, n in rows:
        m = d.moves[n]
        print(fmt % (n.replace("_", " ").title(), m["power"], round(v), cost, "%.1f" % (10 * cost / v) if v else "-"))
