#!/usr/bin/env python3
"""QA playthrough: a new game in Masara to the Route 3 wall, in segments.

    python3 tools/regress/playthrough.py [--out DIR] [--from SEG] [--to SEG] [--starter squirtle]
                                         [--video OUT.mp4] [--rom ROM --elf ELF]

Needs `make release REGRESS=1 DINFO=1` (pokeemerald_modern_release_regress.gba/.elf). Every segment
starts from a savestate written at the end of the previous one (DIR/states/NN.ss) so a failing
segment can be re-run alone with --from. Real button input does the walking, talking, menus and
battles. The only state injections are logged (REGRESS commands and memory pokes), and they are
confined to what the plan allows: forced wins and grinding, clock setup, and the level-up shortcut.
The final segment (Nibi to the Route 3 wall) uses none.

Writes DIR/report.json (segments, frame counts, injections, checks, coverage evidence).
"""
import argparse
import json
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from qa_lib import *  # noqa: E402,F401

NIBI_MART_DOOR = (28, 18)
STARTERS = {"bulbasaur": 0, "charmander": 1, "squirtle": 2}
BALL = {"bulbasaur": (8, 4), "squirtle": (9, 4), "charmander": (10, 4)}


class Ctx:
    def __init__(self, args):
        self.starter = args.starter
        self.args = args


# --------------------------------------------------------------------------------------------
# segment 1: Masara to the lab battle
# --------------------------------------------------------------------------------------------
def seg_masara_to_lab(b, ctx):
    b.boot_new_game()
    b.check("SYS-000: new game starts in Satoshi's room", b.map_name(), "PalletTown_PlayersHouse_2F")
    b.check("SYS-000: respawn is HEAL_LOCATION_MASARA_HOME", b.respawn()[:2], MAPS["PalletTown_PlayersHouse_1F"])
    b.check("SYS-006: the naming screen gave SATOSHI", decode(b.emu.read(b.sb2(), 8)), "SATOSHI")
    b.cover("SYS-000", "new game in 2F, respawn Masara home")
    b.cover("SYS-006", "SATOSHI name through the naming screen (START on the empty name)")
    b.check("new game: no starter yet", b.flag("FLAG_SYS_POKEMON_GET"), 0)
    b.shot("s1_new_game")
    # 2F -> 1F (MAS-001 plays when the player arrives)
    b.take_exit("PalletTown_PlayersHouse_1F")
    b.settle()
    b.check("MAS-001: Mom's send-off line", b.said("All boys leave home some day"), True)
    b.check("MAS-001: FLAG_MASARA_SENDOFF_PLAYED", b.flag("FLAG_MASARA_SENDOFF_PLAYED"), 1)
    b.cover("MAS-001", "Mom: 'All boys leave home some day'")
    b.shot("s1_sendoff")
    b.take_exit("MasaraTown")
    b.settle()
    b.shot("s1_masara")
    # north to the tall grass: MAS-002
    b.goto(15, 3)
    b.walk_dirs(["UP", "UP"])
    b.settle()
    b.check("MAS-002: Okido stopped the player and led him to the lab", b.map_name(), "PalletTown_ProfessorOaksLab")
    b.check("MAS-002: Okido's R/G warning", b.said("Wild POK"), True)
    b.check("MAS-003: VAR_MASARA_LAB_STATE", b.var("VAR_MASARA_LAB_STATE"), 1)
    b.cover("MAS-002", "Okido stops the player at the tall grass")
    b.cover("MAS-003", "lab intro, three balls, Shigeru 'fed up with waiting'")
    b.shot("s1_lab_intro")
    # the pick: MAS-012
    bx, by = BALL[ctx.starter]
    b.talk(bx, by)
    sp = STARTERS[ctx.starter]
    b.check("MAS-012: VAR_STARTER_MON", b.var("VAR_STARTER_MON"), sp)
    b.check("MAS-012: FLAG_SYS_POKEMON_GET", b.flag("FLAG_SYS_POKEMON_GET"), 1)
    b.check("MAS-012: one party member, Lv 5 starter", (b.party_count(), b.party()[0]["level"]), (1, 5))
    b.check("MAS-012: Shigeru takes the counter starter", b.said("SHIGERU received a"), True)
    b.cover("MAS-012", f"picked {ctx.starter}")
    b.shot("s1_picked")
    # the fight at the choke point: MAS-004, real battle input
    b.goto(6, 7)
    t0 = len(b.seen)
    for _ in range(3):
        b.step("DOWN")
        if b.emu.in_battle() or not b.ready():
            break
    b.settle()
    b.check("MAS-004: Shigeru challenged at the choke point", b.said("I'll take you on"), True)
    b.check("MAS-004: fight over, VAR_MASARA_LAB_STATE 3", b.var("VAR_MASARA_LAB_STATE"), 3)
    b.check("MAS-004: no whiteout, still in the lab", b.map_name(), "PalletTown_ProfessorOaksLab")
    b.check("MAS-004: FLAG_Q_RIVAL1_WIN", b.flag("FLAG_Q_RIVAL1_WIN"), 1)
    b.cover("MAS-004", "rival fight with real battle input, won in "
            f"{b.seg_stats[b.seg]['battle_turns']} turns")
    # the aides (MAS-005 with MAS-107; MAS-106)
    b.talk(11, 10)
    b.cover("MAS-005", "aide 2 line after the fight")
    b.talk(3, 11)
    b.cover("MAS-106", "aide 1 line after the fight")
    b.shot("s1_after_fight")


def heal_if_hurt(b, frac=0.6):
    """Use a Potion from the bag (real menus) when the lead is below `frac` of its HP."""
    for slot, m in enumerate(b.party()):
        if m["hp"] and m["hp"] < m["maxhp"] * frac and b.bag_count("ITEM_POTION"):
            ok = b.use_item_on_mon("ITEM_POTION", slot)
            b.say(f"  potion on slot {slot}: {'ok' if ok else 'FAILED'} -> hp {b.mon(slot)['hp']}/{b.mon(slot)['maxhp']}")
            return ok
    return False


# --------------------------------------------------------------------------------------------
# segment 2: Route 1 and the Parcel
# --------------------------------------------------------------------------------------------
def seg_route1_parcel(b, ctx):
    b.take_exit("MasaraTown")
    b.settle()
    b.goto(15, 2)
    b.exit_through(15, 0, "UP")
    b.settle()
    b.check("Masara to Route 1 by the fade warp", b.map_name(), "Route1")
    # the vendor's sample (R01-101), then a Potion on the lead (real bag menus)
    b.talk(6, 28)
    b.check("R01-101: the vendor gave one Potion", b.bag_count("ITEM_POTION"), 1)
    b.check("R01-101: VAR_Q_VENDOR", b.var("VAR_Q_VENDOR"), 1)
    b.cover("R01-101", "free sample: one POTION")
    heal_if_hurt(b, 0.9)
    # the route sign (R01-001)
    b.talk(9, 31)
    b.check("R01-001: route sign read", b.said("TOKIWA") or b.said("ROUTE 1"), True)
    b.cover("R01-001", "ROUTE 1 sign")
    b.cross("UP")
    b.settle()
    b.check("TOK-001: arrived in Tokiwa", b.map_name(), "ViridianCity")
    b.check("SYS-003: FLAG_VISITED_TOKIWA", b.flag("FLAG_VISITED_TOKIWA"), 1)
    b.check("SYS-003: Fiora left Masara (FLAG_HIDE_MASARA_FIORA)", b.flag("FLAG_HIDE_MASARA_FIORA"), 1)
    b.cover("TOK-001", "first entry to Tokiwa")
    b.cover("SYS-003", "FLAG_VISITED_TOKIWA and FLAG_HIDE_MASARA_FIORA on the first entry")
    b.shot("s2_tokiwa")
    # the CVS: door at (36,19), the parcel triggers inside (TOK-002)
    b.goto(36, 19)
    b.settle()
    b.check("TOK-002: inside the CVS", b.map_name(), "ViridianCity_Mart")
    b.walk_dirs(["UP", "UP", "UP"])
    b.settle()
    b.check("TOK-002: OKIDO PARCEL given", b.has_key_item("ITEM_OAKS_PARCEL"), True)
    b.check("TOK-002: FLAG_Q_PARCEL_GIVEN", b.flag("FLAG_Q_PARCEL_GIVEN"), 1)
    b.cover("TOK-002", "Tanabe gives the OKIDO PARCEL")
    b.shot("s2_parcel")
    # QA-001 (open): stepping back onto the trigger row after the scene locks the player for good.
    # The only way out of the counter nook crosses that row, so warp out instead.
    # QA-001 re-verify (round 2): leave the nook by the trigger row with real input; no workaround warp.
    b.walk_dirs(["DOWN"])
    b.settle()
    b.check("QA-001 fixed: DOWN onto trigger row (4,4) is not a softlock", b.ready(), True)
    b.walk_dirs(["DOWN", "RIGHT", "RIGHT"])
    b.settle()
    b.check("QA-001 fixed: the player left the nook and can walk", b.pos()[0] > 4, True)


def walk_to_masara(b):
    """Tokiwa (anywhere) or Route 1 to Masara's north edge, by the fade warp."""
    if b.map_name() == "ViridianCity":
        b.cross("DOWN")
        b.settle()
    if b.map_name() == "Route1":
        b.goto_any([(12, 39), (13, 39)])
        b.walk_dirs(["DOWN"])
        b.settle()


# --------------------------------------------------------------------------------------------
# segment 3: the Index and Mom's seal
# --------------------------------------------------------------------------------------------
def seg_index_and_seal(b, ctx):
    b.take_exit("ViridianCity")
    b.settle()
    walk_to_masara(b)
    b.check("back in Masara", b.map_name(), "MasaraTown")
    b.shot("s3_masara")
    # the lab door; the parcel trigger sits on the choke row
    b.goto(27, 19)
    b.settle()
    b.check("in the lab", b.map_name(), "PalletTown_ProfessorOaksLab")
    b.goto(6, 8)
    b.settle()
    b.check("MAS-006: parcel handed over", b.has_key_item("ITEM_OAKS_PARCEL"), False)
    b.check("MAS-007: FLAG_SYS_POKEDEX_GET (the Index)", b.flag("FLAG_SYS_POKEDEX_GET"), 1)
    b.check("MAS-007: Okido's R/G request", b.said("complete guide on all the POK"), True)
    b.check("MAS-003..: VAR_MASARA_LAB_STATE 4", b.var("VAR_MASARA_LAB_STATE"), 4)
    b.check("MAS-009: VAR_Q_TAG = 1", b.var("VAR_Q_TAG"), 1)
    b.check("MAS-008: five POKE BALLS", b.bag_count("ITEM_POKE_BALL"), 5)
    b.check("TOK-003: PMC FORM given", b.has_key_item("ITEM_PMC_FORM"), True)
    for r, n in (("MAS-006", "parcel to Okido"), ("MAS-007", "the Index and Okido's request"), ("MAS-009", "tag errand VAR_Q_TAG=1"),
                 ("MAS-008", "five Poke Balls"), ("TOK-003", "PMC FORM")):
        b.cover(r, n)
    b.shot("s3_index")
    # Nanami's TOWN MAP (MAS-010) in the Okido house
    b.take_exit("MasaraTown")
    b.settle()
    b.goto(24, 10)
    b.settle()
    b.check("in the Okido house", b.map_name(), "PalletTown_RivalsHouse")
    b.talk(*obj_xy("PalletTown_RivalsHouse", "EventScript_Nanami"))
    b.check("MAS-010: FLAG_MASARA_TOWNMAP_GIVEN", b.flag("FLAG_MASARA_TOWNMAP_GIVEN"), 1)
    b.check("MAS-010: the TOWN MAP is in the bag", b.bag_count("ITEM_TOWN_MAP") + b.bag_count("ITEM_TOWN_MAP", ((0x5d8, 30),)) >= 1, True)
    b.cover("MAS-010", "Nanami gives the TOWN MAP")
    b.shot("s3_townmap")
    b.talk(*obj_xy("PalletTown_RivalsHouse", "EventScript_Nanami"))
    b.check("MAS-301: Nanami asks for the herb (VAR_Q_HERB 1)", b.var("VAR_Q_HERB"), 1)
    b.cover("MAS-301", "Nanami's herb errand asked")
    b.talk(*obj_xy("PalletTown_RivalsHouse", "MAS112_Ken"))
    b.check("MAS-302: Ken gave the TREASURE MAP", b.flag("FLAG_MASARA_KEN_MAP_GIVEN"), 1)
    b.cover("MAS-112", "Ken's Pidgey line")
    b.cover("MAS-302", "Ken gives the TREASURE MAP")
    # Mom's seal on the next 1F entry (MAS-011)
    b.take_exit("MasaraTown")
    b.settle()
    b.goto(5, 7)
    b.settle()
    b.check("in the home 1F", b.map_name(), "PalletTown_PlayersHouse_1F")
    b.check("MAS-011: FLAG_Q_FORM_SEALED", b.flag("FLAG_Q_FORM_SEALED"), 1)
    b.check("MAS-011: Mom's seal line", b.said("seal"), True)
    b.cover("MAS-011", "Mom seals the PMC FORM")
    b.cover("MAS-010", "Mom/Okido house scene complete")
    b.shot("s3_sealed")


def to_map(b, target, tries=6):
    """Walk (real input) to `target` along the spine. Handles a whiteout on the way by starting again from home."""
    chain = {  # (from map) -> action that moves one leg towards Masara/Tokiwa
    }
    for _ in range(tries):
        here = b.map_name()
        if here == target:
            return True
        if target == "ViridianCity":
            if here == "MasaraTown":
                b.goto_any([(15, 0), (16, 0)])
                b.walk_dirs(["UP"])
                b.settle()
            elif here == "Route1":
                b.cross("UP")
            elif here.startswith("PalletTown") or here == "MasaraTown_OkadaShop":
                leave_interior(b)
            elif here.startswith("ViridianCity_"):
                b.take_exit("ViridianCity")
            else:
                raise BlockedPath(f"to_map: no route from {here} to {target}")
        elif target == "MasaraTown":
            if here in ("ViridianCity",):
                b.cross("DOWN")
            elif here == "Route1":
                b.goto_any([(12, 39), (13, 39)])
                b.walk_dirs(["DOWN"])
                b.settle()
            elif here.startswith("ViridianCity_"):
                b.take_exit("ViridianCity")
            elif here.startswith("PalletTown") or here == "MasaraTown_OkadaShop":
                leave_interior(b)
            else:
                raise BlockedPath(f"to_map: no route from {here} to {target}")
        else:
            raise BlockedPath(f"to_map: unknown target {target}")
    return b.map_name() == target


def leave_interior(b):
    here = b.map_name()
    if here == "PalletTown_PlayersHouse_2F":
        b.take_exit("PalletTown_PlayersHouse_1F")
    else:
        b.take_exit("MasaraTown")
    b.settle()


# --------------------------------------------------------------------------------------------
# segment 4: Tokiwa registration to the Wing Badge
# --------------------------------------------------------------------------------------------
def seg_tokiwa_to_b1(b, ctx):
    to_map(b, "ViridianCity")
    b.check("in Tokiwa", b.map_name(), "ViridianCity")
    # the PMC: Aoi registers the sealed form (TOK-005)
    b.goto(36, 10)
    b.settle()
    b.check("in the PMC", b.map_name(), "ViridianCity_Gym")
    balls0, pots0 = b.bag_count("ITEM_POKE_BALL"), b.bag_count("ITEM_POTION")
    b.talk(*obj_xy("ViridianCity_Gym", "EventScript_Aoi"))
    b.check("TOK-005: the sealed form registers (FLAG_Q_CARD)", b.flag("FLAG_Q_CARD"), 1)
    b.check("TOK-005: Welcome Kit 3 balls + 4 Potions",
            (b.bag_count("ITEM_POKE_BALL") - balls0, b.bag_count("ITEM_POTION") - pots0), (3, 4))
    b.check("TOK-005: the form is taken", b.has_key_item("ITEM_PMC_FORM"), False)
    b.cover("TOK-005", "Aoi registers the sealed form, Welcome Kit")
    b.cover("SYS-005", "Trainer Card unlocked (FLAG_Q_CARD)")
    b.shot("s4_registered")
    # the closed arena (TOK-004) and Aoi's pointer (TOK-006)
    b.talk(*obj_xy("ViridianCity_Gym", "EventScript_ArenaClosed"))
    b.check("TOK-004: FLAG_SEEN_ARENA_CLOSED", b.flag("FLAG_SEEN_ARENA_CLOSED"), 1)
    b.cover("TOK-004", "closed-arena barrier read")
    b.talk(*obj_xy("ViridianCity_Gym", "EventScript_Aoi"))
    b.check("TOK-006: Aoi points at the canal", b.said("Yujirou") or b.said("canal"), True)
    b.cover("TOK-006", "Aoi's canal pointer")
    b.shot("s4_closed")
    # Yujirou's flat by the canal (TOK-007)
    b.take_exit("ViridianCity")
    b.settle()
    b.goto(15, 23)
    b.settle()
    b.check("in the flat", b.map_name(), "ViridianCity_YujirouFlat")
    b.goto(4, 6)
    b.settle()
    b.check("TOK-007: FLAG_Q_YUJI_MET", b.flag("FLAG_Q_YUJI_MET"), 1)
    b.cover("TOK-007", "Yujirou's flat meeting")
    b.shot("s4_flat")
    # the inn: Fumiko, the Ume room (TOK-008)
    b.take_exit("ViridianCity")
    b.settle()
    b.goto(26, 26)
    b.settle()
    b.check("in the inn", b.map_name(), "ViridianCity_PokemonCenter_1F")
    r0 = b.var("VAR_REST_COUNT")
    b.talk(*obj_xy("ViridianCity_PokemonCenter_1F", "EventScript_Fumiko"))
    b.check("TOK-008: respawn is the Tokiwa inn", b.respawn()[:2], MAPS["ViridianCity_PokemonCenter_1F"])
    b.check("SYS-001: VAR_REST_COUNT +1", b.var("VAR_REST_COUNT"), r0 + 1)
    b.check("TOK-009: FLAG_Q_YUJI_SLEPT after the rest", b.flag("FLAG_Q_YUJI_SLEPT"), 1)
    lead = b.mon(0)
    b.check("TOK-008: the rest restored HP and TP", (lead["hp"] == lead["maxhp"], lead["tp"]), (True, 0))
    b.cover("TOK-008", "Fumiko's inn rest, respawn set")
    b.cover("SYS-001", "inn rest increments VAR_REST_COUNT")
    b.cover("TOK-009", "FLAG_Q_YUJI_SLEPT opens the arena")
    b.shot("s4_rested")
    # the north exit (TOK-010) before the badge
    b.take_exit("ViridianCity")
    b.settle()
    b.goto(21, 5)
    b.goto(21, 4)
    b.settle()
    b.check("TOK-010: the north exit scene played (VAR_TOKIWA_STATE 1)", b.var("VAR_TOKIWA_STATE"), 1)
    b.check("TOK-010: still in Tokiwa", b.map_name(), "ViridianCity")
    b.cover("TOK-010", "north block scene at the bench")
    b.shot("s4_north_block")
    # grinding: the plan's ~10 min on Routes 1/22, injected (logged); Yujirou's ace is Lv 9
    b.set_party_level(0, 12, "grind before the Tokiwa arena (Yujirou's party is Lv 7-9; the plan has ~10 min of training)")
    # the arena
    b.goto(36, 10)
    b.settle()
    b.check("TOK-009: the barrier is down", b.flag("FLAG_HIDE_TOKIWA_PMC_BARRIER"), 1)
    b.goto(17, 19)
    b.settle()
    b.shot("s4_arena_gate")
    b.talk(*obj_xy("ViridianCity_Gym", "EventScript_Circuit1"))
    b.check("TOK-503: circuit trainer 1 beaten", b.flag(C("TRAINER_FLAGS_START") + C("TRAINER_CIRCUIT_TOKIWA_1")), 1)
    b.cover("TOK-503", "circuit trainer 1, real battle")
    b.talk(*obj_xy("ViridianCity_Gym", "EventScript_Circuit2"))
    b.check("TOK-504: circuit trainer 2 beaten", b.flag(C("TRAINER_FLAGS_START") + C("TRAINER_CIRCUIT_TOKIWA_2")), 1)
    b.cover("TOK-504", "circuit trainer 2, real battle")
    heal_if_hurt(b, 0.5)
    b.save_state(os.path.join(b.out, "states", "pre_yujirou.ss"))
    b.talk(*obj_xy("ViridianCity_Gym", "EventScript_Yujirou"))
    b.check("TOK-501: Yujirou beaten, FLAG_BADGE01_GET", b.flag("FLAG_BADGE01_GET"), 1)
    b.check("SYS-004: TM04 given", b.bag_count("ITEM_TM04"), 1)
    b.check("SYS-004: Charisma +2", b.var("VAR_CHARISMA") >= 2, True)
    b.check("TOK-501: loss flag cleared", b.flag("FLAG_Q_LOST_YUJI"), 0)
    b.cover("TOK-501", "Yujirou, real battle with the arena armed")
    b.cover("SYS-004", "Wing Badge: TM04 and Charisma +2")
    b.shot("s4_badge")


def fight_trainer(b, x, y, name, tries=3, flag=None):
    """Talk to a trainer. A real fight first; a lost one is logged and the rematch is tried again
    (a whiteout returns the player to the respawn point, so this walks back). `flag` is the trainer-beaten flag."""
    for attempt in range(tries):
        lost_before = b.stats.get("losses", 0)
        b.talk(x, y)
        if flag is None or b.flag(flag):
            return True
        b.stats["losses"] = lost_before + 1
        b.say(f"  QA: lost to {name} on attempt {attempt + 1}")
    return False


# --------------------------------------------------------------------------------------------
# segment 5: Route 22, the north exit, Route 2 and the forest
# --------------------------------------------------------------------------------------------
def seg_route2_forest(b, ctx):
    b.check("starting in the PMC after B1", b.flag("FLAG_BADGE01_GET"), 1)
    b.take_exit("ViridianCity")
    b.settle()
    # Route 22 (west of Tokiwa): the rival, R22-501 (no-whiteout fight)
    inn_rest(b, "Tokiwa")      # a player rests after the Arena, before the rival (TP does not refill elsewhere)
    b.set_party_level(0, int(os.environ.get("QA_LV_R22", "15")), "grind on Routes 1/22 before the forest (QA-003 fix: forest trainers are Lv 8-13)")
    b.cross("LEFT")
    b.check("on Route 22", b.map_name(), "Route22")
    b.goto(30, 5)
    b.settle()
    b.check("R22-501: Shigeru met", b.said("Shigeru") or b.said("SHIGERU"), True)
    b.check("R22-501: VAR_ROUTE22_STATE 1 after the fight", b.var("VAR_ROUTE22_STATE"), 1)
    b.cover("R22-501", "Route 22 rival fight, no-whiteout, real battle")
    b.shot("s5_route22")
    # the League gate: guard 1 (R22-101)
    b.goto(8, 5)
    b.settle()
    b.check("in the League gate", b.map_name(), "Route22_NorthEntrance")
    b.talk(*obj_xy("Route22_NorthEntrance", "Guard1"))
    b.cover("R22-101", "gate guard 1: the Boulder Badge line")
    b.take_exit("Route22")
    b.settle()
    # back to Tokiwa, rest at the inn (TP is spent), then north (TOK-011: Hamada steps aside after the badge)
    b.cross("RIGHT")
    b.check("back in Tokiwa", b.map_name(), "ViridianCity")
    inn_rest(b, "Tokiwa")
    b.cross("UP")
    b.settle()
    b.check("TOK-011: through the north exit to Route 2", b.map_name(), "Route2")
    b.cover("TOK-011", "after B1 the north exit is open")
    b.shot("s5_route2")
    # Route 2: the sign (R02-001), then the forest's south gate
    b.talk(7, 73)
    b.cover("R02-001", "Route 2 sign")
    b.set_party_level(0, int(os.environ.get("QA_LV_FOREST", "16")), "grind on Route 2 before the forest (forest passes at Lv 12-14 too (F12/F14 runs); 16 = Wartortle for the Nibi arena)")
    b.goto(5, 51)
    b.settle()
    b.check("in the forest south gate", b.map_name(), "Route2_ViridianForest_SouthEntrance")
    b.goto(7, 1)
    b.settle()
    b.check("in the forest", b.map_name(), "ViridianForest")
    b.check("R22-501 expiry did not fire (Shigeru was fought)", b.var("VAR_ROUTE22_STATE"), 1)
    b.auto_care = True
    b.wild_policy = "run"      # TP is only refilled at an inn: a player in the forest avoids wild fights
    b.say("  bag: " + str([(a, q) for a, q in b.bag_items(0)]))
    b.talk(31, 60)
    b.cover("FOR-001", "south board")
    b.shot("s5_forest_south")
    forest_walk(b)


def inn_rest(b, town):
    """Real input: walk into the inn and talk to the keeper. Leaves the player outside again."""
    if town == "Tokiwa":
        inn, door, keeper = "ViridianCity_PokemonCenter_1F", (26, 26), "EventScript_Fumiko"
    else:
        inn, door, keeper = "PewterCity_PokemonCenter_1F", (17, 25), "EventScript_Fumie"
    b.goto(*door)
    b.settle()
    b.check(f"in the {town} inn", b.map_name(), inn)
    b.talk(*obj_xy(inn, keeper))
    lead = b.mon(0)
    b.check(f"{town} inn rest healed HP and TP", (lead["hp"] == lead["maxhp"], lead["tp"]), (True, 0))
    b.take_exit("ViridianCity" if town == "Tokiwa" else "PewterCity")
    b.settle()


def buy_potions(b, n=8):
    """Real shop input at the Nibi Market (Potion is 300 yen): the clerk is across the counter. Leaves the
    shop and returns to the street. The arena is attrition: a player who has spent the Welcome Kit buys more."""
    b.goto(*NIBI_MART_DOOR)
    b.settle()
    b.check("in the Nibi Market", b.map_name(), "PewterCity_Mart")
    money0, pots0 = b.money(), b.bag_count("ITEM_POTION")
    e = b.emu
    b.talk(*obj_xy("PewterCity_Mart", "EventScript_Clerk"), mash_limit=0)
    e.run(150)
    for _ in range(3):                       # text, menu, BUY
        e.press("A", hold=4, after=100)
    for attempt in range(4):                  # pick POTION, verified by the clerk's own words
        e.cmd(f"write {b.sym['gStringVar4']:#x} ff")
        e.press("DOWN", hold=4, after=30)
        e.press("A", hold=4, after=150)
        t = b.last_text()
        if "POTION" in t:
            break
        e.press("B", hold=4, after=60)        # wrong item: back to the list, try again
        e.press("UP", hold=4, after=30)
    b.check("shop: the clerk offers POTION", "POTION" in b.last_text(), True)
    e.run(100)
    for _ in range(n - 1):
        e.press("UP", hold=4, after=8)
    e.press("A", hold=4, after=100)          # quantity
    e.press("A", hold=4, after=100)          # "And you wanted n?" (finishes the text; YES is the default)
    e.press("A", hold=4, after=150)          # YES: "Here you go!"
    for _ in range(8):                       # out of the list and the menu: B only, never A (A would buy)
        if b.ready():
            break
        e.press("B", hold=4, after=60)
    b.settle()
    b.check("shop: bought Potions with real menus", b.bag_count("ITEM_POTION") - pots0, n)
    b.check("shop: paid 300 yen each", money0 - b.money(), 300 * n)
    b.cover("NIB-117", "Market: bought Potions")
    b.take_exit("PewterCity")
    b.settle()


def forest_walk(b):
    """South gate to Ken: the Bug Catchers (real battles), the Antidote and Potion balls a player picks up
    on the way, then the clearing scene (FOR-003 tag, FOR-002 Ken)."""
    tf = C("TRAINER_FLAGS_START")
    b.talk(49, 60)          # QA-003 fix: the Antidote ball is now at the south entrance, before the first fight
    b.check("FOR item: the Antidote ball (south entrance)", b.bag_count("ITEM_ANTIDOTE") >= 1, True)
    b.settle()
    for (x, y, nm, tr) in [(42, 55, "BC1", "TRAINER_BUG_CATCHER_FOREST_1")]:
        fight_trainer(b, x, y, nm, flag=tf + C(tr))
        b.settle()
    b.talk(21, 34)
    b.check("FOR item: the Poke Ball ball", b.bag_count("ITEM_POKE_BALL") >= 1, True)
    b.settle()
    for (x, y, nm, tr) in [(43, 24, "BC2", "TRAINER_BUG_CATCHER_FOREST_2")]:
        fight_trainer(b, x, y, nm, flag=tf + C(tr))
        b.settle()
    b.talk(40, 21)
    b.settle()
    fight_trainer(b, 7, 22, "BC3", flag=tf + C("TRAINER_BUG_CATCHER_FOREST_3"))
    b.settle()
    b.check("the three Bug Catchers are beaten", all(b.flag(tf + C(t)) for t in
            ("TRAINER_BUG_CATCHER_FOREST_1", "TRAINER_BUG_CATCHER_FOREST_2", "TRAINER_BUG_CATCHER_FOREST_3")), True)
    b.shot("s5_forest_before_ken")
    b.goto(5, 13)
    b.goto(5, 12)          # the clearing: row 12 is the Ken trigger
    b.settle()
    b.check("FOR-002: VAR_FOREST_STATE 1 after Ken", b.var("VAR_FOREST_STATE"), 1)
    b.check("FOR-002: FLAG_Q_KEN1_DONE", b.flag("FLAG_Q_KEN1_DONE"), 1)
    b.cover("FOR-002", "Ken in the clearing, real battle")
    b.check("FOR-003: Ken gave the Old Ear Tag (FLAG_Q_TAG_KEN, VAR_Q_TAG 2)", (b.flag("FLAG_Q_TAG_KEN"), b.var("VAR_Q_TAG")), (1, 2))
    b.check("FOR-003: Old Ear Tag in the bag", b.has_key_item("ITEM_OLD_EAR_TAG"), True)
    b.cover("FOR-003", "Ken hands over the tag")
    b.shot("s5_ken")


# --------------------------------------------------------------------------------------------
# segment 6: forest north gate, Route 2 north, Nibi, the Boulder Badge
# --------------------------------------------------------------------------------------------
def seg_nibi_b2(b, ctx):
    b.auto_care = True
    b.check("in the forest after Ken", b.map_name(), "ViridianForest")
    b.goto(5, 9)
    b.settle()
    b.check("in the forest north gate", b.map_name(), "Route2_ViridianForest_NorthEntrance")
    b.goto(7, 1)
    b.settle()
    b.check("on Route 2 north of the forest", b.map_name(), "Route2")
    b.cross("UP")
    b.settle()
    b.check("in Nibi", b.map_name(), "PewterCity")
    b.shot("s6_nibi")
    b.check("NIB-005: FLAG_VISITED_NIBI set on first entry", b.flag("FLAG_VISITED_NIBI"), 1)
    b.check("NIB-005: Omura's Sandshrew is closed (var 3 or 7)", b.var("VAR_Q_SANDSHREW") in (3, 7), True)
    b.cover("NIB-005", "first entry flags")
    # the town sign (NIB-001)
    b.talk(20, 30)
    b.cover("NIB-001", "town sign")
    # the inn: rest (TP is empty after the forest)
    inn_rest(b, "Nibi")
    b.cover("SYS-001", "inn rest at Nibi")
    b.check("respawn is the Nibi inn", b.respawn()[:2], MAPS["PewterCity_PokemonCenter_1F"])
    # the east exit: the boy escorts the player to the arena once (NIB-002)
    b.goto(40, 21)
    b.goto(41, 21)
    b.settle()
    b.check("NIB-002: VAR_NIBI_STATE 1 after the escort", b.var("VAR_NIBI_STATE"), 1)
    b.cover("NIB-002", "east-exit boy escorts the player to the arena")
    b.shot("s6_escort_end")
    buy_potions(b, int(os.environ.get("QA_POTIONS", "8")))
    if b.mon(0)["level"] < int(os.environ.get("QA_LV_ARENA", "18")):
        b.set_party_level(0, int(os.environ.get("QA_LV_ARENA", "18")), "grind in Nibi before the arena (Camper Lv 14 x2 with the Fault Line armed)")
    b.goto(15, 16)
    b.settle()
    b.check("in the Nibi arena", b.map_name(), "PewterCity_Gym")
    b.shot("s6_arena")
    tf = C("TRAINER_FLAGS_START")
    fight_trainer(b, *obj_xy("PewterCity_Gym", "EventScript_Camper"), "camper", flag=tf + C("TRAINER_CAMPER_NIBI_PMC"))
    b.settle()
    b.cover("NIB-501", "camper, real battle with the arena armed")
    # a player rests at the Nibi inn between the gate trainer and the Leader (TP only refills at an inn)
    b.take_exit("PewterCity")
    b.settle()
    inn_rest(b, "Nibi")
    b.goto(15, 16)
    b.settle()
    b.save_state(os.path.join(b.out, "states", "pre_takeshi.ss"))
    b.talk(*obj_xy("PewterCity_Gym", "EventScript_Takeshi"))
    b.settle()
    b.check("NIB-502: Takeshi beaten, FLAG_BADGE02_GET", b.flag("FLAG_BADGE02_GET"), 1)
    b.check("SYS-004: TM34 given", b.bag_count("ITEM_TM34"), 1)
    b.cover("NIB-003", "Takeshi's two ideas before the fight")
    b.cover("NIB-502", "Takeshi, real battle")
    b.cover("SYS-004", "Boulder Badge: TM34 and Charisma")
    b.shot("s6_badge2")
    b.take_exit("PewterCity")
    b.settle()
    b.goto(40, 21)
    b.talk(42, 20)
    b.check("NIB-004: the exit is open (VAR_NIBI_STATE 2)", b.var("VAR_NIBI_STATE"), 2)
    b.cover("NIB-004", "B2 opens the east exit")


# --------------------------------------------------------------------------------------------
# segment 7: Route 3 to the wall. Real input only: no injection allowed in this segment.
# --------------------------------------------------------------------------------------------
def seg_route3_wall(b, ctx):
    b.auto_care = True
    n_inj = len(b.injections)
    b.check("Charisma +4 after two badges", b.var("VAR_CHARISMA") >= 4, True)
    b.take_exit("PewterCity") if b.map_name() != "PewterCity" else None
    inn_rest(b, "Nibi")        # after the Arena a player sleeps at the inn (real input)
    b.goto(40, 21)
    b.cross("RIGHT")
    b.settle()
    b.check("on Route 3", b.map_name(), "Route3")
    b.shot("s7_route3_start")
    tf = C("TRAINER_FLAGS_START")
    # the sign at the edge (R03-001) and the first Route 3 trainer (R03-501, a sight trigger on the way)
    b.goto(18, 9)
    b.settle()
    fight_trainer(b, *obj_xy("Route3", "BugCatcher1"), "R03 bug catcher", flag=tf + C("TRAINER_BUG_CATCHER_ROUTE3_1"))
    b.settle()
    b.cover("R03-501", "first Route 3 trainer, real battle")
    t = b.talk(28, 3)
    b.check("R03-001: the edge sign reads", any("Nibi" in x and "Otsukimi" in x for x in t), True)
    b.cover("R03-001", "the edge sign")
    b.talk(30, 4)
    b.cover("R03-001", "the hiker in front of the wall")
    b.shot("s7_hiker")
    # the wall: every cell of column 31 on the passable rows is blocked, and the player cannot get through
    b.goto(30, 5)
    m = b.snapshot_map()
    for y in (3, 4, 5):
        b.check(f"wall: (31,{y}) is blocked", b.cell(m, 31, y)[0], False)
    for _ in range(4):
        b.step("RIGHT")
    b.check("wall: the player is still at x=30", b.pos()[0], 30)
    b.check("FLAG_BADGE01_GET at the wall", b.flag("FLAG_BADGE01_GET"), 1)
    b.check("FLAG_BADGE02_GET at the wall", b.flag("FLAG_BADGE02_GET"), 1)
    b.check("no injection in the last segment", len(b.injections) - n_inj, 0)
    b.cover("R03-WALL", "Route 3 wall holds at x=31")
    b.shot("s7_wall")


SEGMENTS = [
    ("01_masara_to_lab_battle", seg_masara_to_lab),
    ("02_route1_and_parcel", seg_route1_parcel),
    ("03_index_and_seal", seg_index_and_seal),
    ("04_tokiwa_to_b1", seg_tokiwa_to_b1),
    ("05_route22_route2_forest", seg_route2_forest),
    ("06_nibi_to_b2", seg_nibi_b2),
    ("07_route3_wall", seg_route3_wall),
]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=os.path.join(ROOT, "build", "qa", "playthrough"))
    ap.add_argument("--from", dest="from_seg", type=int, default=1)
    ap.add_argument("--to", dest="to_seg", type=int, default=99)
    ap.add_argument("--starter", default="squirtle", choices=sorted(STARTERS))
    ap.add_argument("--video")
    ap.add_argument("--rom", default=ROM)
    ap.add_argument("--elf", default=ELF)
    ap.add_argument("--shots")
    args = ap.parse_args()
    ctx = Ctx(args)
    states = os.path.join(args.out, "states")
    first = args.from_seg
    bot = Bot(args.out, rom=args.rom, elf=args.elf, video=args.video, shots=args.shots, keep=first > 1)
    os.makedirs(states, exist_ok=True)
    report = dict(starter=args.starter, segments=[])
    ok = True
    t0 = time.time()
    try:
        for i, (name, fn) in enumerate(SEGMENTS, 1):
            if i < first or i > args.to_seg:
                continue
            if i > 1 and i == first:
                bot.load_state(os.path.join(states, f"{i - 1:02d}.ss"))
            bot.begin(name)
            n_fail0 = sum(1 for c in bot.checks if not c[1])
            try:
                fn(bot, ctx)
            except Exception as ex:   # a segment that cannot finish stops the run
                import traceback
                traceback.print_exc()
                bot.say(f"SEGMENT FAILED: {type(ex).__name__}: {ex}")
                bot.shot(f"FAIL_{name}")
                ok = False
                report["segments"].append(dict(name=name, status="exception", error=str(ex)))
                break
            fails = sum(1 for c in bot.checks if not c[1]) - n_fail0
            bot.save_state(os.path.join(states, f"{i:02d}.ss"))
            report["segments"].append(dict(name=name, status="ok" if not fails else "checks failed", fails=fails,
                                           **bot.seg_stats[name]))
            if fails:
                ok = False
    finally:
        report["injections"] = bot.injections
        report["checks"] = bot.checks
        report["coverage"] = bot.cov
        report["anomalies"] = bot.anomalies
        report["stats"] = bot.stats
        report["cat_frames"] = bot.cat_frames
        report["wall_seconds"] = round(time.time() - t0, 1)
        report["ok"] = ok
        json.dump(report, open(os.path.join(args.out, "report.json"), "w"), indent=1, default=str)
        bot.close()
    print("RESULT:", "PASS" if ok else "FAIL", f"({report['wall_seconds']} s, {bot.stats['frames']} frames)")
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
