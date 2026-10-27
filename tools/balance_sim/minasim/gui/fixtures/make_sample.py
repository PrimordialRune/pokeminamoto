"""Generates sample_report.json (current run) and sample_report_prev.json (baseline) deterministically.

    python3 -m minasim.gui.fixtures.make_sample      (from tools/balance_sim)
Demo data only: numbers are invented but shaped like real output (see CONTRACT.md).
"""
import json
import math
import random
from pathlib import Path

from .. import repodata

OUT = Path(__file__).resolve().parent

STAGES = [  # id, name, badges, level range, maps
    ("s00", "Masara Town", 0, [5, 6], ["MAP_MASARA_TOWN", "MAP_ROUTE1"]),
    ("s01", "Tokiwa City", 0, [3, 9], ["MAP_TOKIWA_CITY", "MAP_ROUTE22"]),
    ("s02", "Tokiwa Forest", 0, [4, 10], ["MAP_TOKIWA_FOREST", "MAP_ROUTE2"]),
    ("s03", "Before Brock (Nibi)", 0, [8, 14], ["MAP_NIBI_CITY", "MAP_NIBI_CITY_GYM"]),
    ("s04", "Mt. Moon to Hanada", 1, [10, 20], ["MAP_ROUTE3", "MAP_MT_MOON", "MAP_HANADA_CITY"]),
    ("s05", "Kuchiba City", 2, [14, 26], ["MAP_ROUTE5", "MAP_ROUTE6", "MAP_KUCHIBA_CITY"]),
    ("s06", "Rock Tunnel to Shion", 3, [18, 30], ["MAP_ROCK_TUNNEL", "MAP_SHION_TOWN"]),
    ("s07", "Tamamushi City", 3, [20, 34], ["MAP_ROUTE7", "MAP_TAMAMUSHI_CITY"]),
    ("s08", "Yamabuki City", 5, [26, 42], ["MAP_ROUTE16", "MAP_YAMABUKI_CITY"]),
    ("s09", "Sekichiku City", 6, [30, 46], ["MAP_ROUTE15", "MAP_SEKICHIKU_CITY"]),
    ("s10", "Guren Town", 7, [34, 52], ["MAP_ROUTE19", "MAP_GUREN_TOWN"]),
    ("s11", "Victory Road", 8, [40, 58], ["MAP_ROUTE23", "MAP_VICTORY_ROAD"]),
    ("s12", "Sekiei Plateau (Indigo)", 8, [48, 66], ["MAP_SEKIEI_PLATEAU"]),
]

FIGHTS = [  # trainer, name, class, stage, party, target
    ("TRAINER_RIVAL_1_S", "Rival (Route 1)", "rival", "s01", [("PIDGEY", 9)], .8),
    ("TRAINER_YOUNGSTER_JOEY", "Youngster Joey", "trainer", "s01", [("RATTATA", 4)], .95),
    ("TRAINER_BUG_CATCHER_RICK", "Bug Catcher Rick", "trainer", "s02", [("WEEDLE", 6), ("CATERPIE", 6)], .93),
    ("TRAINER_BUG_CATCHER_DOUG", "Bug Catcher Doug", "trainer", "s02", [("WEEDLE", 7), ("KAKUNA", 7), ("WEEDLE", 7)], .93),
    ("TRAINER_LASS_JANICE", "Lass Janice", "trainer", "s02", [("PIDGEY", 9), ("PIDGEY", 9)], .93),
    ("TRAINER_BROCK", "Brock", "gym", "s03", [("GEODUDE", 12), ("ONIX", 14)], .65),
    ("TRAINER_RIVAL_2_S", "Rival (Nibi)", "rival", "s03", [("PIDGEOTTO", 13), ("RATTATA", 11), ("SQUIRTLE", 12)], .7),
    ("TRAINER_YOUNGSTER_BEN", "Youngster Ben", "trainer", "s04", [("EKANS", 11), ("SANDSHREW", 11)], .93),
    ("TRAINER_SUPER_NERD_MIGUEL", "Super Nerd Miguel", "trainer", "s04", [("GRIMER", 11), ("VOLTORB", 11), ("KOFFING", 11)], .9),
    ("TRAINER_HIKER_MARCOS", "Hiker Marcos", "trainer", "s04", [("GEODUDE", 10), ("ONIX", 10)], .9),
    ("TRAINER_MISTY", "Misty", "gym", "s04", [("STARYU", 18), ("STARMIE", 21)], .65),
    ("TRAINER_RIVAL_3_S", "Rival (Hanada Bridge)", "rival", "s04", [("PIDGEOTTO", 19), ("ABRA", 16), ("RATTATA", 16), ("SQUIRTLE", 18)], .7),
    ("TRAINER_SAILOR_DWAYNE", "Sailor Dwayne", "trainer", "s05", [("MACHOP", 18), ("SHELLDER", 17)], .9),
    ("TRAINER_ENGINEER_BAILY", "Engineer Baily", "trainer", "s05", [("MAGNEMITE", 21), ("MAGNEMITE", 21), ("MAGNETON", 21)], .9),
    ("TRAINER_LT_SURGE", "Lt. Surge", "gym", "s05", [("VOLTORB", 21), ("PIKACHU", 18), ("RAICHU", 24)], .65),
    ("TRAINER_ROCKET_GRUNT_1", "Rocket Grunt", "trainer", "s06", [("RATTATA", 20), ("ZUBAT", 20)], .88),
    ("TRAINER_HIKER_ANTHONY", "Hiker Anthony", "trainer", "s06", [("MACHOP", 25), ("GEODUDE", 25), ("ONIX", 25)], .88),
    ("TRAINER_RIVAL_4_S", "Rival (Shion Tower)", "rival", "s06", [("PIDGEOTTO", 25), ("GROWLITHE", 23), ("EXEGGCUTE", 22), ("KADABRA", 20), ("SQUIRTLE", 25)], .7),
    ("TRAINER_ERIKA", "Erika", "gym", "s07", [("VICTREEBEL", 29), ("TANGELA", 24), ("VILEPLUME", 29)], .65),
    ("TRAINER_CAMPER_JEROME", "Camper Jerome", "trainer", "s07", [("NIDORINO", 28), ("PARASECT", 28)], .9),
    ("TRAINER_ROCKET_ADMIN_1", "Rocket Boss Hideout", "boss", "s07", [("RHYHORN", 25), ("DROWZEE", 26), ("KANGASKHAN", 29)], .5),
    ("TRAINER_SABRINA", "Sabrina", "gym", "s08", [("KADABRA", 38), ("MR_MIME", 37), ("VENOMOTH", 38), ("ALAKAZAM", 43)], .65),
    ("TRAINER_JUGGLER_NELSON", "Juggler Nelson", "trainer", "s08", [("DROWZEE", 41), ("HYPNO", 43)], .88),
    ("TRAINER_KOGA", "Koga", "gym", "s09", [("KOFFING", 37), ("MUK", 39), ("KOFFING", 37), ("WEEZING", 43)], .65),
    ("TRAINER_TAMER_EDGAR", "Tamer Edgar", "trainer", "s09", [("TAUROS", 43), ("PRIMEAPE", 43), ("RHYHORN", 45)], .88),
    ("TRAINER_RIVAL_5_S", "Rival (Silph)", "rival", "s09", [("PIDGEOT", 37), ("GYARADOS", 38), ("GROWLITHE", 35), ("EXEGGUTOR", 38), ("ALAKAZAM", 35), ("BLASTOISE", 40)], .7),
    ("TRAINER_BLAINE", "Blaine", "gym", "s10", [("GROWLITHE", 42), ("PONYTA", 40), ("RAPIDASH", 42), ("ARCANINE", 47)], .65),
    ("TRAINER_BURGLAR_ARNIE", "Burglar Arnie", "trainer", "s10", [("GROWLITHE", 36), ("VULPIX", 36), ("NINETALES", 40)], .88),
    ("TRAINER_GIOVANNI", "Giovanni", "gym", "s10", [("RHYHORN", 45), ("DUGTRIO", 42), ("NIDOQUEEN", 44), ("NIDOKING", 45), ("RHYDON", 50)], .6),
    ("TRAINER_RIVAL_6_S", "Rival (Victory Road)", "rival", "s11", [("PIDGEOT", 47), ("ALAKAZAM", 50), ("RHYDON", 45), ("ARCANINE", 47), ("EXEGGUTOR", 45), ("BLASTOISE", 50)], .7),
    ("TRAINER_COOLTRAINER_DARYL", "Cooltrainer Daryl", "trainer", "s11", [("CHARIZARD", 52), ("ELECTRODE", 51)], .88),
    ("TRAINER_LORELEI", "Lorelei", "gym", "s12", [("DEWGONG", 54), ("CLOYSTER", 53), ("SLOWBRO", 54), ("JYNX", 56), ("LAPRAS", 56)], .5),
    ("TRAINER_BRUNO", "Bruno", "gym", "s12", [("ONIX", 53), ("HITMONCHAN", 55), ("HITMONLEE", 55), ("ONIX", 56), ("MACHAMP", 58)], .5),
    ("TRAINER_AGATHA", "Agatha", "gym", "s12", [("GENGAR", 56), ("GOLBAT", 56), ("HAUNTER", 55), ("ARBOK", 58), ("GENGAR", 60)], .5),
    ("TRAINER_LANCE", "Lance", "gym", "s12", [("GYARADOS", 58), ("DRAGONAIR", 56), ("DRAGONAIR", 56), ("AERODACTYL", 60), ("DRAGONITE", 62)], .5),
    ("TRAINER_RIVAL_CHAMPION_S", "Rival (Champion)", "rival", "s12", [("PIDGEOT", 61), ("ALAKAZAM", 59), ("RHYDON", 61), ("ARCANINE", 61), ("EXEGGUTOR", 63), ("BLASTOISE", 65)], .5),
    ("WILD_ROUTE22_NIDORAN", "Wild Nidoran (Route 22)", "wild", "s01", [("NIDORAN_M", 4)], .97),
    ("WILD_FOREST_PIKACHU", "Wild Pikachu (Tokiwa Forest)", "wild", "s02", [("PIKACHU", 5)], .97),
    ("WILD_MT_MOON_ZUBAT", "Wild Zubat swarm (Mt. Moon)", "wild", "s04", [("ZUBAT", 9)], .97),
    ("WILD_ROCK_TUNNEL_ONIX", "Wild Onix (Rock Tunnel)", "wild", "s06", [("ONIX", 13)], .97),
]

STARTERS = ["BULBASAUR", "CHARMANDER", "SQUIRTLE"]
# win-rate tweak per fight id for the main run (shaped to give interesting flags)
HARD = {"TRAINER_BROCK": -.34, "TRAINER_MISTY": -.12, "TRAINER_SABRINA": -.31, "TRAINER_RIVAL_3_S": -.08,
        "TRAINER_LT_SURGE": +.2, "TRAINER_LANCE": -.2, "TRAINER_KOGA": +.22, "TRAINER_BLAINE": -.1,
        "TRAINER_ROCKET_ADMIN_1": -.22, "TRAINER_HIKER_ANTHONY": -.16, "TRAINER_RIVAL_2_S": -.15}
FOE_STARTER_COUNTER = {"BULBASAUR": -.35, "CHARMANDER": 0.0, "SQUIRTLE": +.05}


def ci(p, n):
    h = 1.96 * math.sqrt(max(p * (1 - p), 1e-4) / n)
    return [round(max(0, p - h), 3), round(min(1, p + h), 3)]


def build(run_id, commit, created, baseline, drift, seed):
    rnd = random.Random(seed)
    d = repodata.load()
    sp = d["species"]
    mv = {m["id"]: m for m in d["moves"]}
    it = {i["id"]: i for i in d["items"]}
    stages = [{"id": s[0], "index": i, "name": s[1], "badges": s[2], "level_range": s[3], "maps": s[4]}
              for i, s in enumerate(STAGES)]
    fights = []
    for tid, name, cls, st, party, target in FIGHTS:
        band_t = {"wild": [.95, 1.0], "trainer": [.85, .97], "rival": [.6, .8], "gym": [.55, .75]}.get(cls, [.4, .6])
        if tid.startswith("TRAINER_ROCKET_ADMIN"):
            band_t = [.4, .6]
        mid = (band_t[0] + band_t[1]) / 2
        wr = {}
        for band, off in (("average", -.04), ("skilled", .1)):
            p = mid + off + HARD.get(tid, 0) * (1 if band == "average" else .6) + drift.get(tid, 0) + rnd.uniform(-.04, .04)
            p = min(.995, max(.03, p))
            n = 600 if cls != "wild" else 300
            wr[band] = {"win_rate": round(p, 3), "ci": ci(p, n), "hp_cost": round(rnd.uniform(.2, .9), 2),
                        "tp_cost": round(rnd.uniform(.05, .7), 2), "items_used": round(rnd.uniform(0, 2.2), 2),
                        "blackout": round(1 - p, 3), "turns": round(rnd.uniform(3, 14), 1), "n": n}
        by_starter = {}
        for s in STARTERS:
            off = 0
            if tid == "TRAINER_BROCK":
                off = {"BULBASAUR": .22, "CHARMANDER": -.34, "SQUIRTLE": .2}[s]
            elif tid == "TRAINER_MISTY":
                off = {"BULBASAUR": .18, "CHARMANDER": -.25, "SQUIRTLE": -.05}[s]
            elif tid.startswith("TRAINER_RIVAL"):
                off = {"BULBASAUR": -.05, "CHARMANDER": 0, "SQUIRTLE": .04}[s]
            by_starter[s] = round(min(.99, max(.02, wr["average"]["win_rate"] + off + rnd.uniform(-.03, .03))), 3)
        fights.append({"trainer": tid, "name": name, "class": cls, "stage": st,
                       "map": {"gym": "MAP_%s_GYM" % name.split()[-1].upper()}.get(cls, "MAP_" + st.upper()),
                       "party": [{"species": s, "level": l} for s, l in party], "by_band": wr,
                       "by_starter": by_starter, "target": band_t})
    # flags ------------------------------------------------------------------------
    flags = []

    def flag(kind, sev, stage, band, subj, title, detail, metric, suggestion, score, repro=None):
        i = len(flags)
        fid = "%s:%s:%s:%d" % (kind, subj.get("trainer") or subj.get("species") or subj.get("move") or subj.get("item") or "x", band, i)
        base = baseline_status(fid, kind, subj, band)
        flags.append({"id": fid, "kind": kind, "severity": sev, "score": round(score, 3), "stage": stage,
                      "band": band, "subject": dict({"species": None, "move": None, "item": None, "trainer": None}, **subj),
                      "title": title, "detail": detail, "metric": metric,
                      "change": base, "suggestion": suggestion,
                      "repro": repro or {"seed": rnd.randint(1, 99999), "cmd": "python3 -m minasim duel --seed %d" % rnd.randint(1, 99999)}})

    def baseline_status(fid, kind, subj, band):
        r = rnd.random()
        if not baseline:
            return {"vs": None, "delta": 0.0, "status": "new"}
        status = "new" if r < .25 else "worse" if r < .45 else "better" if r < .6 else "same" if r < .9 else "resolved"
        return {"vs": baseline, "delta": round(rnd.uniform(-.15, .15) if status != "same" else 0.0, 3), "status": status}

    def fight(tid):
        return next(f for f in fights if f["trainer"] == tid)

    fb = fight("TRAINER_BROCK")["by_band"]["average"]
    flag("fight", "red", "s03", "average", {"trainer": "TRAINER_BROCK"}, "Brock is too hard for an average player",
         "First-try win rate %d%% (target 55-75%%). Onix's Bind plus Rock Tomb outpaces the starter's TP budget; Charmander wins only %d%%." % (
             fb["win_rate"] * 100, fight("TRAINER_BROCK")["by_starter"]["CHARMANDER"] * 100),
         {"name": "win_rate", "value": fb["win_rate"], "ci": fb["ci"], "target": [.55, .75]},
         "Try Onix L12, or let Charmander learn Metal Claw-equivalent coverage earlier", .93)
    flag("fight", "red", "s08", "average", {"trainer": "TRAINER_SABRINA"}, "Sabrina is a wall for average players",
         "Psychic-types resist everything the party has; win rate far under target.",
         {"name": "win_rate", "value": fight("TRAINER_SABRINA")["by_band"]["average"]["win_rate"], "ci": fight("TRAINER_SABRINA")["by_band"]["average"]["ci"], "target": [.55, .75]},
         "Lower Alakazam to L41 or give the player a Ghost/Bug option earlier", .88)
    flag("fight", "amber", "s04", "average", {"trainer": "TRAINER_MISTY"}, "Misty depends heavily on starter choice",
         "Charmander wins %d%%, Bulbasaur %d%%." % (fight("TRAINER_MISTY")["by_starter"]["CHARMANDER"] * 100, fight("TRAINER_MISTY")["by_starter"]["BULBASAUR"] * 100),
         {"name": "starter_spread", "value": .43, "ci": [.38, .48], "target": [0, .25]}, "Add a Route 3 Electric/Grass option or raise Staryu's level", .71)
    flag("fight", "amber", "s03", "average", {"trainer": "TRAINER_RIVAL_2_S"}, "Rival at Nibi hits slightly hard",
         "Pidgeotto's Wing Attack chains with Quick Attack.", {"name": "win_rate", "value": .59, "ci": [.55, .63], "target": [.6, .8]},
         "Drop the second Rattata to L9", .5)
    flag("fight", "red", "s07", "average", {"trainer": "TRAINER_ROCKET_ADMIN_1"}, "Hideout boss under target",
         "Kangaskhan lasts 11 turns on average; items used 2.1 per attempt.", {"name": "win_rate", "value": .27, "ci": [.23, .31], "target": [.4, .6]},
         "Lower Kangaskhan to L27", .8)
    flag("fight", "amber", "s05", "average", {"trainer": "TRAINER_LT_SURGE"}, "Surge is a pushover",
         "Win rate 95% (target 55-75%): Ground-type access trivialises the gym.", {"name": "win_rate", "value": .95, "ci": [.93, .97], "target": [.55, .75]},
         "Add Magnemite L22 or Raichu Thunder Wave", .6)
    flag("fight", "info", "s09", "skilled", {"trainer": "TRAINER_KOGA"}, "Koga easy for skilled players",
         "Skilled band wins 99% with Talk/Unnerve.", {"name": "win_rate", "value": .99, "ci": [.98, 1.0], "target": [.75, .95]}, "Consider a Panic-resistant AI state", .3)
    flag("fight", "amber", "s12", "both", {"trainer": "TRAINER_LANCE"}, "Lance's Dragonite out-damages the whole party",
         "Hyper Beam two-shots most of the average party.", {"name": "win_rate", "value": .31, "ci": [.27, .35], "target": [.4, .6]}, "Lower Dragonite to L60 or add TP drain", .66)
    flag("species", "red", "s04", "average", {"species": "BUTTERFREE"}, "Butterfree dominates stages 4 to 6",
         "Rating 1810 with Sleep Powder plus Psybeam; usage in winning teams 31%.",
         {"name": "z", "value": 3.4, "ci": [2.7, 4.1], "target": [-2, 2]}, "Raise Sleep Powder TP cost 8 to 10", .85)
    flag("species", "amber", "s06", "both", {"species": "KINGLER"}, "Kingler has no good place in the curve",
         "Win rate 42% below the median species.", {"name": "z", "value": -2.6, "ci": [-3.1, -2.1], "target": [-2, 2]}, "Check Crabhammer accuracy and learn level", .52)
    flag("species", "amber", "s08", "skilled", {"species": "ALAKAZAM"}, "Alakazam over-performs for skilled players",
         "Rating 1990, 2.9 sigma above the stage mean.", {"name": "z", "value": 2.9, "ci": [2.2, 3.5], "target": [-2, 2]}, "Increase Recover TP cost", .62)
    flag("species", "info", "s03", "average", {"species": "GEODUDE"}, "Geodude is picked too rarely",
         "Usage 4% across teams despite good matchups.", {"name": "usage", "value": .04, "ci": [.03, .05], "target": [.1, .4]}, "None needed; team policy may under-pick Rock types", .2)
    flag("move", "red", "s02", "both", {"move": "EMBER", "species": "CHARMANDER"}, "Ember is dominant at 7 TP",
         "Damage per TP 3.1 vs median 1.9; delta win 18%.", {"name": "delta_win", "value": .18, "ci": [.15, .21], "target": [-.1, .1]}, "Raise Ember TP to 9 or lower power to 35", .77)
    flag("move", "amber", "s05", "both", {"move": "SURF"}, "Surf out-scales every alternative",
         "Used 83% of turns by Water-types that learn it.", {"name": "usage", "value": .83, "ci": [.8, .86], "target": [.2, .6]}, "Raise TP cost 12 to 14", .58)
    flag("move", "amber", "s07", "average", {"move": "WRAP", "species": "EKANS"}, "Wrap is a dead move",
         "Chosen 0.4% of turns; delta win -3%.", {"name": "usage", "value": .004, "ci": [.001, .008], "target": [.05, 1]}, "Rework Wrap into a TP-drain move", .35)
    flag("item", "amber", "s03", "average", {"item": "POTION"}, "Potion price too low for what it buys",
         "Delta win 0.2 per 1000 yen, double that of the next item.", {"name": "delta_per_1000", "value": .21, "ci": [.17, .25], "target": [.05, .12]}, "Price Potion at 400", .55)
    flag("item", "info", "s09", "both", {"item": "FULL_RESTORE"}, "Full Restore never bought",
         "0% of simulated runs buy it before the Plateau.", {"name": "usage", "value": 0, "ci": [0, .01], "target": [.05, .5]}, "Lower price to 2500", .22)
    flag("tp", "red", "s06", "average", {}, "TP runs out before Rock Tunnel",
         "Average player has 14 battles per rest but only 6 TP-worth of moves (target 8-15 battles).", {"name": "battles_per_rest", "value": 6.1, "ci": [5.6, 6.6], "target": [8, 15]}, "Add an inn at Shion or lower mid-rank move costs", .82)
    flag("tp", "amber", "s03", "average", {}, "Brock arrives with 0.4 TP left on average",
         "tp_left_at_gym 0.40 (target 0.5 to 1.0).", {"name": "tp_left_at_gym", "value": .4, "ci": [.36, .44], "target": [.5, 1]}, "Add a rest point on Route 2", .64)
    flag("level_curve", "amber", "s08", "average", {}, "Player is 4 levels behind the foes",
         "Mean player level 31.5 vs foe 36.0.", {"name": "level_gap", "value": -4.5, "ci": [-5.2, -3.8], "target": [-2, 2]}, "Lower trainer levels or add EXP Share equivalent", .6)
    flag("level_curve", "info", "s04", "skilled", {}, "Skilled players over-level Mt. Moon",
         "Gap +3.2.", {"name": "level_gap", "value": 3.2, "ci": [2.6, 3.8], "target": [-2, 2]}, "Fine", .25)
    flag("special", "amber", "s05", "skilled", {"species": "PIKACHU"}, "Special action never beats a plain attack",
         "Special used in 2% of turns.", {"name": "usage", "value": .02, "ci": [.01, .03], "target": [.1, .4]}, "Revisit Pikachu's special", .4)
    flag("talk", "amber", "s04", "skilled", {"trainer": "TRAINER_MISTY"}, "Provoke wins Misty outright",
         "Resolute state triggers on 71% of Provokes.", {"name": "talk_win_delta", "value": .24, "ci": [.2, .28], "target": [0, .1]}, "Add a Resolute counter-chance for gym leaders", .55)
    flag("rng", "info", "s03", "average", {"trainer": "TRAINER_BROCK"}, "Brock outcome is decided by crits",
         "36% of losses involve two or more critical hits.", {"name": "crit_loss_share", "value": .36, "ci": [.3, .42], "target": [0, .25]}, "Reduce Onix crit chance", .3)
    flag("dominance", "red", "s04", "both", {"species": "BUTTERFREE", "move": "SLEEP_POWDER"}, "Sleep is a free win",
         "Teams with a sleep move win 24% more often across stages 3 to 7.", {"name": "delta_win", "value": .24, "ci": [.2, .28], "target": [-.1, .1]}, "Cap sleep at 2 turns or raise TP cost", .79)
    flag("dead_content", "amber", "s10", "both", {"species": "MAGMAR"}, "Magmar is outclassed everywhere",
         "Only 0.8% pick rate; every stage has a better Fire-type.", {"name": "pick_rate", "value": .008, "ci": [.004, .012], "target": [.03, 1]}, "Give Magmar a signature move", .45)
    flag("dead_content", "info", "s11", "both", {"item": "ESCAPE_ROPE"}, "Escape Rope has no use in the simulator",
         "Never used; the simulator has no dungeon navigation.", {"name": "usage", "value": 0, "ci": [0, 0], "target": [0, 1]}, "Ignore: simulator limitation", .1)
    flag("fight", "amber", "s06", "average", {"trainer": "TRAINER_HIKER_ANTHONY"}, "Hiker Anthony hits above his class",
         "Three Rock/Ground mons in a row; win rate %d%%." % (fight("TRAINER_HIKER_ANTHONY")["by_band"]["average"]["win_rate"] * 100),
         {"name": "win_rate", "value": fight("TRAINER_HIKER_ANTHONY")["by_band"]["average"]["win_rate"], "ci": fight("TRAINER_HIKER_ANTHONY")["by_band"]["average"]["ci"], "target": [.85, .97]}, "Swap Onix for Geodude", .48)
    flags.sort(key=lambda f: -f["score"])

    # species rows -------------------------------------------------------------------
    species = []
    show = ["s00", "s03", "s05", "s08", "s10", "s12"]
    for s in sp:
        base_r = 1400 + (s["bst"] - 400) * .7
        for st in show:
            for band in ("average", "skilled"):
                r = base_r + rnd.gauss(0, 70) + (80 if band == "skilled" else 0)
                if s["id"] in ("BUTTERFREE", "ALAKAZAM", "GYARADOS", "DRAGONITE"):
                    r += 160
                if s["id"] in ("KINGLER", "MAGMAR", "WEEDLE"):
                    r -= 120
                n = rnd.randint(150, 700)
                wr = 1 / (1 + 10 ** ((1500 - r) / 400))
                species.append({"species": s["id"], "stage": st, "band": band, "rating": round(r), "win_rate": round(wr, 3),
                                "ci": ci(wr, n), "z": round((r - 1500) / 110, 2), "n": n})
    # moves --------------------------------------------------------------------------
    pairs = [("EMBER", "CHARMANDER", 7, "s00"), ("SCRATCH", "CHARMANDER", 1, "s00"), ("TACKLE", "BULBASAUR", 1, "s00"),
             ("VINE_WHIP", "BULBASAUR", 10, "s01"), ("SLEEP_POWDER", "BULBASAUR", 15, "s02"), ("WATER_GUN", "SQUIRTLE", 7, "s01"),
             ("BUBBLE", "SQUIRTLE", 10, "s02"), ("THUNDERBOLT", "PIKACHU", 26, "s05"), ("SURF", "BLASTOISE", 36, "s05"),
             ("FLAMETHROWER", "CHARIZARD", 36, "s07"), ("PSYCHIC", "ALAKAZAM", 42, "s08"), ("WRAP", "EKANS", 17, "s07"),
             ("QUICK_ATTACK", "PIDGEY", 12, "s01"), ("WING_ATTACK", "PIDGEOTTO", 21, "s03"), ("ROCK_THROW", "GEODUDE", 11, "s02"),
             ("BODY_SLAM", "SNORLAX", 1, "s09"), ("HYPER_BEAM", "DRAGONITE", 55, "s12"), ("EARTHQUAKE", "RHYDON", 48, "s10"),
             ("BLIZZARD", "LAPRAS", 40, "s11"), ("SWORDS_DANCE", "SCYTHER", 1, "s09"), ("SLEEP_POWDER", "BUTTERFREE", 14, "s04"),
             ("CONFUSION", "ABRA", 1, "s03"), ("LEECH_SEED", "BULBASAUR", 7, "s01"), ("BITE", "RATTATA", 7, "s01"),
             ("SELF_DESTRUCT", "GEODUDE", 21, "s04"), ("DIG", "SANDSHREW", 24, "s05"), ("MEGA_DRAIN", "ODDISH", 22, "s05"),
             ("CRABHAMMER", "KINGLER", 35, "s06"), ("FIRE_BLAST", "ARCANINE", 50, "s10"), ("AGILITY", "RAPIDASH", 41, "s08")]
    moves = []
    for m, spc, lv, st in pairs:
        info = mv.get(m)
        if not info:
            continue
        dpt = (info["power"] or 30) / max(info["tp_cost"], 1)
        verdict = {"EMBER": "dominant", "SURF": "dominant", "SLEEP_POWDER": "dominant", "WRAP": "dead", "SELF_DESTRUCT": "weak",
                   "HYPER_BEAM": "strong", "EARTHQUAKE": "strong", "THUNDERBOLT": "strong", "PSYCHIC": "strong",
                   "AGILITY": "weak", "DIG": "weak", "CRABHAMMER": "weak"}.get(m, "ok")
        dw = {"dominant": .18, "strong": .09, "ok": .02, "weak": -.02, "dead": -.04}[verdict] + rnd.uniform(-.02, .02)
        moves.append({"move": m, "species": spc, "learn_level": lv, "stage": st, "tp_cost": info["tp_cost"],
                      "damage_per_tp": round(dpt * 1.1, 2), "usage": round({"dominant": .7, "strong": .5, "ok": .3, "weak": .1, "dead": .004}[verdict] + rnd.uniform(0, .1), 3),
                      "delta_win": round(dw, 3), "ci": [round(dw - .03, 3), round(dw + .03, 3)], "verdict": verdict})
    items = []
    for iid, st in (("POTION", "s02"), ("POTION", "s05"), ("SUPER_POTION", "s06"), ("SUPER_POTION", "s08"), ("HYPER_POTION", "s10"),
                    ("FULL_RESTORE", "s11"), ("ANTIDOTE", "s03"), ("PARALYZE_HEAL", "s05"), ("AWAKENING", "s08"), ("ETHER", "s07"),
                    ("POKE_BALL", "s00"), ("GREAT_BALL", "s06"), ("REVIVE", "s09"), ("X_ATTACK", "s04"), ("GUARD_SPEC", "s08")):
        i = it.get(iid)
        if not i:
            continue
        price = i["price"] or 300
        dw = round({"POTION": .06, "SUPER_POTION": .07, "HYPER_POTION": .08, "FULL_RESTORE": .02}.get(iid, .015) + rnd.uniform(-.01, .02), 3)
        items.append({"item": iid, "stage": st, "delta_win": dw, "ci": [round(dw - .02, 3), round(dw + .02, 3)], "price": price,
                      "delta_per_1000": round(dw / price * 1000, 3), "verdict": "ok" if iid != "FULL_RESTORE" else "weak"})
    tp, curve = [], []
    for i, s in enumerate(STAGES):
        for band in ("average", "skilled"):
            b = 14 - i * .5 + (3 if band == "skilled" else 0) - (6 if (s[0] == "s06" and band == "average") else 0) + rnd.uniform(-1, 1)
            tp.append({"stage": s[0], "band": band, "battles_per_rest": round(b, 1), "target": [8, 15],
                       "tp_left_at_gym": round(max(0, min(1, .45 + (.3 if band == "skilled" else 0) + rnd.uniform(-.1, .1))), 2)})
            lo, hi = s[3]
            foe = lo + (hi - lo) * .6 + (1 if band == "average" else 0) + (2 if s[0] == "s08" else 0)
            curve.append({"stage": s[0], "band": band, "player_level": round(foe - (1.5 if band == "average" else 0) - (3 if s[0] == "s08" and band == "average" else 0) + rnd.uniform(-.8, .8), 1),
                          "foe_level": round(foe, 1), "exp_level": round(foe - 1, 1)})
    return {"meta": {"run_id": run_id, "commit": commit, "branch": "develop", "created": created, "battles": 6500000 if baseline else 4800000,
                     "seconds": 812, "jobs": 8, "bands": ["average", "skilled"], "baseline_run": baseline, "demo": True},
            "stages": stages, "flags": flags, "species": species, "moves": moves, "fights": fights, "items": items,
            "tp": tp, "level_curve": curve}


def main():
    prev = build("demo-2026-10-20T0900-9f1c2b7e", "9f1c2b7e", "2026-10-20T09:00:00", None, {"TRAINER_BROCK": -.1, "TRAINER_KOGA": -.12}, 7)
    cur = build("demo-2026-10-27T1200-a55a8324", "a55a8324", "2026-10-27T12:00:00", prev["meta"]["run_id"], {}, 11)
    (OUT / "sample_report_prev.json").write_text(json.dumps(prev, indent=1, ensure_ascii=False))
    (OUT / "sample_report.json").write_text(json.dumps(cur, indent=1, ensure_ascii=False))
    print("flags", len(cur["flags"]), "fights", len(cur["fights"]), "species rows", len(cur["species"]))


if __name__ == "__main__":
    main()
