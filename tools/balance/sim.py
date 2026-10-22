#!/usr/bin/env python3
"""Minamoto balance model: Monte Carlo battles and early-game playthroughs on the ROM's own data.

  sim.py duel CHARMANDER:12 "ISITSUBUTE:11,IWARK:14" --boss --badges 1
  sim.py run scenarios/early_game.yaml --starter BULBASAUR --n 300
  sim.py tp CHARMANDER SQUIRTLE BULBASAUR PIDGEY --cost 10
  sim.py ttk CHARMANDER:12 IWARK:14
  any command: --set tp_regen_base=3 --set flat_plus2=true --set tp_formula="round5(power**2/200)"

See README.md for the rules, the assumptions and how to read the output.
"""
import argparse
import json
import random
import re
import statistics
import sys

import yaml

from engine import Battle, Mon, Rules, level_up
from gamedata import GameData

COUNTER = {"BULBASAUR": "CHARMANDER", "CHARMANDER": "SQUIRTLE", "SQUIRTLE": "BULBASAUR"}


class Context:
    def __init__(self, args):
        self.data = GameData()
        self.rules = Rules()
        for kv in args.set or []:
            k, v = kv.split("=", 1)
            self.rules.set(k, v)
        self.starter = self.data.resolve(getattr(args, "starter", None) or "CHARMANDER")
        self.proxies = {}
        self.trainer_ai = getattr(args, "foe_ai", None) or "trainer"
        self.battles_model = getattr(args, "battles", None) or "scenario"
        self.no_evolve = getattr(args, "no_evolve", False)

    def species(self, name):
        name = name.strip()
        if name.upper() == "STARTER":
            return self.starter
        if name.upper() == "RIVAL_STARTER":
            return COUNTER.get(self.starter, "SQUIRTLE")
        key = re.sub(r"[^A-Z0-9]", "", name.upper().replace("♂", "M").replace("♀", "F"))
        for alias, target in self.proxies.items():
            if re.sub(r"[^A-Z0-9]", "", alias.upper()) == key:
                name = target
        return self.data.resolve(name)

    def evolved(self, species, level):
        """Trainer mons appear in the form their level implies (rival starters, Pidgeotto)."""
        evo = self.data.evolutions.get(species)
        while evo and level >= evo[0] and evo[1] in self.data.species:
            species = evo[1]
            evo = self.data.evolutions.get(species)
        return species

    def mon(self, spec, owner="trainer", ivs=None):
        """SPECIES:LEVEL[MOVE/MOVE]@ITEM"""
        m = re.match(r"\s*([^:\[@]+):(\d+)(?:\[([^\]]*)\])?(?:@(\w+))?\s*$", spec)
        if not m:
            raise ValueError("bad mon spec %r (want SPECIES:LEVEL[MOVE/MOVE]@ITEM)" % spec)
        level = int(m.group(2))
        sp = self.species(m.group(1))
        if m.group(1).strip().upper() == "RIVAL_STARTER":
            sp = self.evolved(sp, level)
        moves = [self.data.move(x)["name"] for x in m.group(3).split("/")] if m.group(3) else None
        return Mon(self.data, sp, level, moves=moves, ivs=ivs, item=m.group(4), owner=owner)

    def team(self, specs, owner="trainer", ivs=None):
        if isinstance(specs, str):
            specs = specs.split(",")
        return [self.mon(s, owner, ivs) for s in specs]


def pct(x):
    return "%5.1f%%" % (100 * x)


def summarize(values):
    if not values:
        return "-"
    v = sorted(values)
    return "%.1f (p10 %s, p90 %s)" % (statistics.mean(v), v[len(v) // 10], v[(len(v) * 9) // 10])


# ---------------------------------------------------------------- duel
def cmd_duel(args):
    if args.levels:
        lo, _, hi = args.levels.partition("-")
        print("%s vs %s, win rate by level (%d runs each, badges %d%s)" % (args.player, args.foe, args.n, args.badges, ", gym sash" if args.boss else ""))
        print("  level   win    turns  HP left")
        for lv in range(int(lo), int(hi or lo) + 1):
            out = duel_once(args, args.player.replace(":L", ":%d" % lv))
            print("  %5d  %s  %5.1f  %s" % (lv, pct(out["win_rate"]), out["turns"], pct(out["player_hp_left"])))
        return
    out = duel_once(args, args.player)
    if args.json:
        print(json.dumps(out, indent=2))
        return
    print("%s  vs  %s   (%d runs, badges %d%s)" % (args.player, args.foe, args.n, args.badges, ", gym sash" if args.boss else ""))
    print("  win rate          %s" % pct(out["win_rate"]))
    print("  turns             %.1f" % out["turns"])
    print("  player HP left    %s" % pct(out["player_hp_left"]))
    print("  crit share of dmg %s" % pct(out["player_crit_damage_share"]))
    print("  TP: lowest %.0f on average; preferred move unaffordable in %s of battles; Struggle in %s" % (
        out["player_min_tp"], pct(out["battles_tp_limited"]), pct(out["battles_player_struggled"])))
    print("  foe: lowest TP %.0f on average; Struggled in %s of battles" % (out["foe_min_tp"], pct(out["foe_struggled"])))
    if ctx_rules_ranked(args):
        r = out["player_rank_share"]
        print("  player hits by rank 1*/2*/3*: %s / %s / %s; lower rank by choice %s, forced by TP %s" % (
            pct(r[0]), pct(r[1]), pct(r[2]), pct(out["player_lower_by_choice"]), pct(out["player_lower_forced"])))
    if args.boss:
        print("  sash saves        %.2f per battle" % out["sash_saves_per_battle"])


def ctx_rules_ranked(args):
    return any(kv.startswith("rank_costs=") for kv in (args.set or []))


def duel_once(args, player):
    ctx = Context(args)
    wins, turns, hp_left, crit_share, struggles, starved, min_tp, sash = 0, [], [], [], 0, 0, [], 0
    foe_struggles, foe_min_tp, rank_use, lower, forced = 0, [], [0, 0, 0], 0, 0
    for _ in range(args.n):
        p = ctx.team(player, "player")
        f = ctx.team(args.foe, "trainer" if args.trainer or args.boss else "wild", ivs=None if args.wild_ivs else 15)
        b = Battle(ctx.data, ctx.rules, p, f, kind="trainer" if (args.trainer or args.boss) else "wild",
                   badges=range(1, args.badges + 1), foe_ai=args.foe_ai, sash=args.boss)
        res = b.run()
        s = b.stats
        wins += res == "win"
        turns.append(s.turns)
        hp_left.append(sum(m.hp for m in p) / sum(m.maxhp for m in p))
        if s.damage["player"]:
            crit_share.append(s.crit_damage["player"] / s.damage["player"])
        struggles += s.struggles["player"] > 0
        starved += s.tp_starved_turns["player"] > 0
        min_tp.append(s.min_tp["player"])
        sash += s.sash_saves
        foe_struggles += s.struggles["foe"] > 0
        foe_min_tp.append(s.min_tp["foe"])
        rank_use = [a + b for a, b in zip(rank_use, s.rank_use["player"])]
        lower += s.chose_lower["player"]
        forced += s.forced_lower["player"]
    return {"player": player, "foe": args.foe, "runs": args.n, "win_rate": wins / args.n,
           "turns": statistics.mean(turns), "player_hp_left": statistics.mean(hp_left),
           "player_crit_damage_share": statistics.mean(crit_share) if crit_share else 0,
           "battles_player_struggled": struggles / args.n, "battles_tp_limited": starved / args.n,
           "player_min_tp": statistics.mean(min_tp), "sash_saves_per_battle": sash / args.n,
            "foe_struggled": foe_struggles / args.n, "foe_min_tp": statistics.mean(foe_min_tp),
            "player_rank_share": [u / (sum(rank_use) or 1) for u in rank_use],
            "player_lower_by_choice": lower / (sum(rank_use) or 1), "player_lower_forced": forced / (sum(rank_use) or 1)}


# ---------------------------------------------------------------- ttk
def cmd_ttk(args):
    ctx = Context(args)
    a, d = ctx.mon(args.attacker, "player", 15), ctx.mon(args.defender, "trainer", 15)
    b = Battle(ctx.data, ctx.rules, [a], [d], kind="trainer", badges=range(1, args.badges + 1))
    print("%s (HP %d, INT %d, regen %d/turn)  ->  %s (HP %d)" % (a.label(), a.maxhp, ctx.data.intelligence(a.species),
          ctx.rules.regen(ctx.data, a.species), d.label(), d.maxhp))
    print("  %-14s %4s %5s %4s  %-11s %s" % ("move", "pow", "TP", "acc", "dmg range", "hits to KO (no crit)"))
    for mv in a.moves:
        move = ctx.data.moves[mv]
        if move["power"] == 0:
            print("  %-14s %4s %5d %4s  status" % (mv, "-", ctx.rules.tp_cost(move), move["acc"] or "-"))
            continue
        lo = b.full_damage("player", a, d, move, False, 85)
        hi = b.full_damage("player", a, d, move, False, 100)
        print("  %-14s %4d %5d %4d  %3d-%-3d    %s-%s" % (mv, move["power"], ctx.rules.tp_cost(move), move["acc"], lo, hi,
              -(-d.maxhp // max(hi, 1)), -(-d.maxhp // max(lo, 1)) if lo else "inf"))


# ---------------------------------------------------------------- tp
def cmd_tp(args):
    ctx = Context(args)
    r = ctx.rules
    print("TP %d max, regen %d + INT/%d per turn (INT = base Sp.Def placeholder)" % (r.tp_max, r.tp_regen_base, r.tp_int_div))
    print("  %-12s %4s %6s   turns of back-to-back use before TP runs out, by cost" % ("species", "INT", "regen"))
    costs = args.cost or [5, 10, 15, 20, 30, 40]
    print("  %-12s %4s %6s   %s" % ("", "", "", "  ".join("%5d" % c for c in costs)))
    for name in args.species:
        sp = ctx.species(name)
        regen = r.regen(ctx.data, sp)
        cells = []
        for c in costs:
            if c <= regen:
                cells.append("  inf")
                continue
            tp, n = r.tp_max, 0
            while tp >= c and n < 999:
                tp = min(r.tp_max, tp - c + regen)
                n += 1
            cells.append("%5d" % n)
        print("  %-12s %4d %6d   %s" % (sp, ctx.data.intelligence(sp), regen, "  ".join(cells)))
    if args.moves:
        print("\nMove costs (%s):" % ("ROM tpCost" if not r.tp_formula else r.tp_formula))
        for mv in args.moves:
            m = ctx.data.move(mv)
            print("  %-14s pow %3d  TP %3d" % (m["name"], m["power"], r.tp_cost(m)))


# ---------------------------------------------------------------- run
class Playthrough:
    def __init__(self, ctx, scenario, team_mode, grind):
        self.ctx, self.sc, self.team_mode, self.grind = ctx, scenario, team_mode, grind
        self.party = ctx.team(scenario["party"], "player")
        self.battles_model = ctx.battles_model
        self.badges = []
        self.potions = 0
        self.last_wild = None
        self.records = []
        self.log = []
        self.rank_use = {"player": [0, 0, 0], "foe": [0, 0, 0]}
        self.chose_lower = {"player": 0, "foe": 0}
        self.forced_lower = {"player": 0, "foe": 0}
        self.struggle_battles = {"wild": 0, "trainer": 0}

    def lead(self):
        alive = [m for m in self.party if m.alive()]
        return alive

    def wild_mon(self, table):
        entries = [e.rsplit(":", 2) for e in table]
        sp, lv, _ = random.choices(entries, weights=[int(e[2]) for e in entries])[0]
        lo, _, hi = lv.partition("-")
        return Mon(self.ctx.data, self.ctx.species(sp), random.randint(int(lo), int(hi or lo)))

    def patch_up(self):
        for m in self.party:
            while self.potions and m.alive() and m.hp < m.maxhp // 2:
                m.hp = min(m.maxhp, m.hp + 20)
                self.potions -= 1

    def order_party(self):
        """Train the weakest living mon in team mode; the starter leads in solo mode.
        With persistent TP, a mon that can't afford any move goes to the back."""
        rules = self.ctx.rules

        def broke(m):
            return rules.tp_persist and m.tp is not None and all(m.tp < rules.tp_cost(self.ctx.data.moves[mv]) for mv in m.moves if mv != "STRUGGLE")
        if self.team_mode and len(self.party) > 1:
            self.party.sort(key=lambda m: (not m.alive(), broke(m), m.level))
        elif len(self.party) > 1:
            self.party.sort(key=lambda m: (not m.alive(), broke(m)))

    def fight(self, foe, kind, step_name, sash=False, first=False, ai="trainer"):
        self.order_party()
        b = Battle(self.ctx.data, self.ctx.rules, self.party, foe, kind=kind, badges=self.badges,
                   foe_ai=ai, sash=sash, first_battle=first)
        res = b.run()
        for side in ("player", "foe"):
            for i in range(3):
                self.rank_use[side][i] += b.stats.rank_use[side][i]
            self.chose_lower[side] += b.stats.chose_lower[side]
            self.forced_lower[side] += b.stats.forced_lower[side]
        self.struggle_battles[kind] += b.stats.struggles["foe"] > 0
        seg = self.segment
        seg["battles"] += 1
        seg["special"] += b.stats.special_uses
        seg["special_" + kind] = seg.get("special_" + kind, 0) + b.stats.special_uses
        seg["n_" + kind] = seg.get("n_" + kind, 0) + 1
        seg["attacks"] += sum(b.stats.rank_use["player"]) + b.stats.struggles["player"]
        if b.stats.struggles["player"]:
            seg["struggle_battles"] += 1
        if b.stats.tp_starved_turns["player"] or b.stats.struggles["player"]:
            seg["limited_battles"] += 1
            if seg["first_limited"] is None:
                seg["first_limited"] = seg["battles"]
        for m in self.party:
            level_up(m, self.log, cplus=self.ctx.rules.cplus, evolve=not self.ctx.no_evolve)
        if res != "win":
            self.blackouts += 1
            for m in self.party:
                m.heal()
        else:
            self.patch_up()
        return res, b.stats

    def area_battles(self, step):
        """Fixed count from the scenario, or Omar's MathModels rule (newMM/main.py LocationSimulation):
        round(gamma(2, 5)) clamped to 1..64, a mean of 10 battles per area."""
        if self.battles_model == "gamma":
            return int(max(min(round(random.gammavariate(2.0, 5.0)), 64), 1))
        return step.get("battles", 0)

    def new_segment(self, start):
        self.segment = {"start": start, "battles": 0, "attacks": 0, "struggle_battles": 0, "limited_battles": 0,
                        "first_limited": None, "special": 0}

    def close_segment(self, end):
        seg = self.segment
        seg["end"] = end
        seg["tp_left"] = [m.tp if m.tp is not None else self.ctx.rules.tp_max for m in self.party]
        self.segments.append(seg)
        self.new_segment(end)

    def run(self):
        self.segments = []
        self.new_segment(self.sc.get("start", "start"))
        self.blackouts = 0
        self.wild_battles = 0
        self.grind_battles = 0
        for step in self.sc["steps"]:
            if "heal" in step:
                self.close_segment(step["heal"])
                for m in self.party:
                    m.heal()
                self.potions += self.sc.get("potions_per_town", 0)
            elif "area" in step:
                self.last_wild = step["wild"]
                for _ in range(self.area_battles(step)):
                    self.fight([self.wild_mon(step["wild"])], "wild", step["area"], ai="random")
                    self.wild_battles += 1
            elif "catch" in step:
                c = step["catch"]
                if self.team_mode or not c.get("team_only"):
                    self.party.append(Mon(self.ctx.data, self.ctx.species(c["species"]), c["level"], owner="player"))
            elif "trainer" in step or "boss" in step:
                t = step.get("trainer") or step.get("boss")
                is_boss = "boss" in step
                if is_boss:
                    target = step.get("target")
                    arrival = max(m.level for m in self.party)
                    grinded = 0
                    if self.grind and target and self.last_wild:
                        while max(m.level for m in self.party) < target[0] and grinded < 300:
                            self.fight([self.wild_mon(self.last_wild)], "wild", "grind", ai="random")
                            grinded += 1
                        self.grind_battles += grinded
                    for m in self.party:
                        m.heal()
                    levels = sorted((m.level for m in self.party), reverse=True)
                foe = self.ctx.team(t["team"], "trainer", ivs=15)
                res, stats = self.fight(foe, "trainer", t["name"], sash=is_boss and t.get("sash", False),
                                        first=t.get("first_battle", False), ai=self.ctx.trainer_ai)
                rec = {"step": t["name"], "boss": is_boss, "win": res == "win", "turns": stats.turns,
                       "crit_share": stats.crit_damage["player"] / stats.damage["player"] if stats.damage["player"] else 0,
                       "struggled": stats.struggles["player"] > 0, "tp_limited": stats.tp_starved_turns["player"] > 0,
                       "min_tp": stats.min_tp["player"], "foe_crit_share": stats.crit_damage["foe"] / stats.damage["foe"] if stats.damage["foe"] else 0}
                rec["multi_rank"] = stats.multi_rank_turns / max(1, stats.turns)
                rec["rank_choice"] = list(stats.rank_choice)
                rec["species"] = self.party[0].species if self.party else ""
                rec["special"] = stats.special_uses
                if is_boss:
                    rec.update(arrival=arrival, levels=levels, grind=grinded, target=step.get("target"),
                               wild_battles=self.wild_battles)
                    if res == "win":
                        self.badges.append(t["badge"])
                    else:
                        # retry after a heal, as a player would; count the retries
                        tries = 1
                        while res != "win" and tries < 10:
                            for m in self.party:
                                m.heal()
                            res, _ = self.fight(self.ctx.team(t["team"], "trainer", ivs=15), "trainer", t["name"],
                                                sash=t.get("sash", False), ai=self.ctx.trainer_ai)
                            tries += 1
                        rec["tries"] = tries
                        if res == "win":
                            self.badges.append(t["badge"])
                self.records.append(rec)
        self.close_segment("end")
        return self.records


def cmd_run(args):
    ctx = Context(args)
    with open(args.scenario, encoding="utf-8") as f:
        sc = yaml.safe_load(f)
    ctx.proxies = sc.get("proxies", {})
    for kv in sc.get("rules", {}).items():
        ctx.rules.set(*kv)
    runs = []
    finals = []
    for i in range(args.n):
        random.seed(args.seed + i)
        pt = Playthrough(ctx, sc, args.team, args.grind)
        runs.append(pt.run())
        finals.append(pt)
    by_step = {}
    for recs in runs:
        for r in recs:
            by_step.setdefault(r["step"], []).append(r)
    out = []
    print("%s: starter %s, %s, %d runs, rules: TP %d + %d+INT/%d, crit 1/%s, +2 %s, badge x%.2f%s" % (
        sc["name"], ctx.starter, "team" if args.team else "solo starter", args.n, ctx.rules.tp_max, ctx.rules.tp_regen_base,
        ctx.rules.tp_int_div, "/".join(map(str, ctx.rules.crit_table)), "on" if ctx.rules.flat_plus2 else "off",
        ctx.rules.badge_boost, ", grind to target" if args.grind else ""))
    print()
    print("%-28s %7s %6s %6s %6s %7s  %s" % ("battle", "1st win", "turns", "crit%", "TPlim", "Strugl", "lead level on arrival / target"))
    for name, recs in by_step.items():
        n = len(recs)
        row = {"step": name, "first_try_win": sum(r["win"] for r in recs) / n, "turns": statistics.mean(r["turns"] for r in recs),
               "crit_share": statistics.mean(r["crit_share"] for r in recs), "tp_limited": sum(r["tp_limited"] for r in recs) / n,
               "struggled": sum(r["struggled"] for r in recs) / n}
        extra = ""
        if recs[0]["boss"]:
            arr = [r["arrival"] for r in recs]
            row.update(arrival=statistics.mean(arr), target=recs[0]["target"], grind=statistics.mean(r["grind"] for r in recs),
                       tries=statistics.mean(r.get("tries", 1) for r in recs), wild_battles=statistics.mean(r["wild_battles"] for r in recs))
            extra = "%s / %s" % (summarize(arr), recs[0]["target"])
            if args.grind:
                extra += ", +%.1f grind battles" % row["grind"]
            extra += ", %.2f tries" % row["tries"]
        if recs[0]["boss"] or args.all:
            print("%-28s %7s %6.1f %6s %6s %7s  %s" % (name[:28], pct(row["first_try_win"]), row["turns"], pct(row["crit_share"]),
                  pct(row["tp_limited"]), pct(row["struggled"]), extra))
        out.append(row)
    if ctx.rules.cplus:
        print("\nC+ ranks at the gym leaders (player's lead):")
        for name, recs in by_step.items():
            if not recs[0]["boss"]:
                continue
            rc = [sum(r["rank_choice"][i] for r in recs) for i in range(3)]
            tot = sum(rc) or 1
            forms = {}
            for r in recs:
                forms[r["species"]] = forms.get(r["species"], 0) + 1
            print("  %-26s turns with 2+ ranks on offer %s; moves used 1*/2*/3*: %s / %s / %s; lead %s" % (
                name[:26], pct(statistics.mean(r["multi_rank"] for r in recs)), pct(rc[0] / tot), pct(rc[1] / tot), pct(rc[2] / tot),
                ", ".join("%s %s" % (k.title(), pct(v / len(recs)).strip()) for k, v in sorted(forms.items(), key=lambda kv: -kv[1]))))
    if ctx.rules.rank_costs:
        for side in ("player", "foe"):
            use = [sum(pt.rank_use[side][i] for pt in finals) for i in range(3)]
            tot = sum(use) or 1
            lower = sum(pt.chose_lower[side] for pt in finals)
            forced = sum(pt.forced_lower[side] for pt in finals)
            print("%-6s damaging moves by rank: 1*: %s  2*: %s  3*: %s;  lower rank by choice %s, forced by TP %s (of all hits)" % (
                side, pct(use[0] / tot), pct(use[1] / tot), pct(use[2] / tot), pct(lower / tot), pct(forced / tot)))
        print("trainer battles where the foe Struggled: %.2f per playthrough" % statistics.mean(pt.struggle_battles["trainer"] for pt in finals))
    if ctx.rules.tp_persist or args.segments:
        print("\nBetween rests (TP %s):" % ("carries over between battles" if ctx.rules.tp_persist else "resets every battle"))
        print("  %-34s %7s %8s %13s %11s %9s  %s" % ("stretch", "battles", "attacks", "first TP-short", "TP-short", "Struggled", "lead TP left at the rest"))
        nseg = min(len(pt.segments) for pt in finals)
        for i in range(nseg):
            segs = [pt.segments[i] for pt in finals]
            firsts = [sg["first_limited"] for sg in segs if sg["first_limited"] is not None]
            print("  %-34s %7.1f %8.1f %13s %11.1f %9.1f  %.0f" % (
                (segs[0]["start"] + " -> " + segs[0]["end"])[:34], statistics.mean(sg["battles"] for sg in segs),
                statistics.mean(sg["attacks"] for sg in segs),
                ("%.1f (%s)" % (statistics.mean(firsts), pct(len(firsts) / len(segs)).strip())) if firsts else "never",
                statistics.mean(sg["limited_battles"] for sg in segs), statistics.mean(sg["struggle_battles"] for sg in segs),
                statistics.mean(sg["tp_left"][0] for sg in segs)))
    if ctx.rules.special_power:
        print("\nSpecial (power %d, %d TP, from Lv %d, %s):" % (ctx.rules.special_power, ctx.rules.special_cost,
              ctx.rules.special_level, ctx.rules.special_habit))
        for name, recs in by_step.items():
            if recs[0]["boss"]:
                print("  %-26s uses per fight %.2f" % (name[:26], statistics.mean(r["special"] for r in recs)))
        nseg = min(len(pt.segments) for pt in finals)
        for i in range(nseg):
            segs = [pt.segments[i] for pt in finals]
            per = lambda k: sum(sg.get("special_" + k, 0) for sg in segs) / max(1, sum(sg.get("n_" + k, 0) for sg in segs))
            print("  %-34s uses per wild battle %.2f, per trainer %.2f, share of attacks %s" % (
                (segs[0]["start"] + " -> " + segs[0]["end"])[:34], per("wild"), per("trainer"),
                pct(sum(sg["special"] for sg in segs) / max(1, sum(sg["attacks"] for sg in segs)))))
    bl = [pt.blackouts for pt in finals]
    print()
    print("blackouts per playthrough: %s" % summarize(bl))
    print("final party: %s" % ", ".join("%s" % m.label() for m in finals[0].party) + "  (run 1)")
    if args.log:
        print("\nrun 1 log:\n  " + "\n  ".join(finals[0].log))
    if args.json:
        with open(args.json, "w") as f:
            json.dump(out, f, indent=2)


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--set", action="append", metavar="RULE=VALUE", help="override a rule (see engine.Rules)")
    sub = p.add_subparsers(dest="cmd", required=True)

    d = sub.add_parser("duel", help="Monte Carlo of one battle")
    d.add_argument("player", help='e.g. "CHARMANDER:12" or "CHARMANDER:12,PIDGEY:10"')
    d.add_argument("foe", help='e.g. "ISITSUBUTE:11,IWARK:14"')
    d.add_argument("--n", type=int, default=2000)
    d.add_argument("--badges", type=int, default=0, help="number of badges owned (badge types follow gym order)")
    d.add_argument("--boss", action="store_true", help="gym leader: trainer AI, Focus Sash rule")
    d.add_argument("--trainer", action="store_true", help="trainer battle (trainer AI, x1.5 EXP)")
    d.add_argument("--foe-ai", default="trainer", choices=["random", "trainer", "smart", "rank"])
    d.add_argument("--wild-ivs", action="store_true", help="random foe IVs instead of 15")
    d.add_argument("--levels", help='sweep the player level, e.g. 10-20 with player "CHARMANDER:L"')
    d.add_argument("--json", action="store_true")
    d.set_defaults(func=cmd_duel)

    t = sub.add_parser("ttk", help="damage ranges and hits to KO")
    t.add_argument("attacker")
    t.add_argument("defender")
    t.add_argument("--badges", type=int, default=0)
    t.set_defaults(func=cmd_ttk)

    tp = sub.add_parser("tp", help="TP economy per species")
    tp.add_argument("species", nargs="+")
    tp.add_argument("--cost", type=int, action="append")
    tp.add_argument("--moves", nargs="*")
    tp.set_defaults(func=cmd_tp)

    r = sub.add_parser("run", help="Monte Carlo playthrough of a scenario")
    r.add_argument("scenario")
    r.add_argument("--starter", default="CHARMANDER")
    r.add_argument("--n", type=int, default=200)
    r.add_argument("--seed", type=int, default=1)
    r.add_argument("--team", action="store_true", help="catch the team_only mons and train the weakest")
    r.add_argument("--grind", action="store_true", help="grind the last area until the boss target level")
    r.add_argument("--all", action="store_true", help="show every trainer, not only bosses")
    r.add_argument("--foe-ai", default="trainer", choices=["random", "trainer", "smart", "rank"])
    r.add_argument("--log", action="store_true")
    r.add_argument("--battles", choices=["scenario", "gamma"], default="scenario",
                   help="wild battles per area: the scenario's counts, or MathModels' gamma(2,5) draw")
    r.add_argument("--no-evolve", action="store_true", help="the player's mons never evolve (C+ rank caps stay low)")
    r.add_argument("--segments", action="store_true", help="show the stretches between rests even without tp_persist")
    r.add_argument("--json", metavar="FILE")
    r.set_defaults(func=cmd_run)

    args = p.parse_args(argv)
    args.func(args)


if __name__ == "__main__":
    main()
