#!/usr/bin/env python3
"""QA systemic checks for the Masara-to-Nibi slice (ROM: make release REGRESS=1 DINFO=1).

    qa_systemic.py SECTION [--states DIR] [--out DIR]

States come from a playthrough.py run (DIR/NN.ss; default /tmp/qa_full/states). Sections:

  whiteout    a forced loss (lead at 1 HP, weakest move, no items) on Route 1 before any inn goes home; after the
              Tokiwa rest it goes to the Tokiwa inn; after the Nibi rest it goes to the Nibi inn (SYS-002 lines)
  noheal      Mom and the Route 1 vendor never heal HP or TP
  savereload  every slice map: warp in, save from the start menu, reset, Continue, and the same map, cell and
              party come back
  npcs        every NPC on every map: talk, leave mid-box (hold a direction, press B), the player must be free
              again; an NPC the player cannot get next to is reported
  coords      every coord-event cell on every map, stepped on twice: the player must be free afterwards
              (the softlock class of QA-001)

Warps are only used to put the player on a map (logged as test setup). Results are written to OUT/systemic_<section>.json.
"""
import argparse
import json
import os
import sys
import traceback

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import qa_lib as Q


def arrival_cell(name):
    """A cell to warp into for `name`: its first warp event (a door mat), else the map's first object."""
    mj = Q.mapjson(name)
    for w in mj.get("warp_events", []):
        return w["x"], w["y"]
    for o in mj.get("object_events", []):
        return o["x"], o["y"] + 1
    return 5, 5


def set_party_hp(b, hp, why):
    b.inject("hp", f"all party HP -> {hp} ({why})")
    for i in range(b.party_count()):
        m = b.mon(i)
        b.write_u16(m["addr"] + 0x56, min(hp, m["maxhp"]))


def grass_cell(b):
    m = b.snapshot_map()
    mw, mh = b.map_size()
    best = None
    for y in range(mh):
        for x in range(mw):
            ok, beh = b.cell(m, x, y)
            if ok and beh in Q.GRASS:
                nb = sum(1 for dx, dy in Q.DIRV.values()
                         if b.cell(m, x + dx, y + dy)[0] and b.cell(m, x + dx, y + dy)[1] in Q.GRASS)
                if nb >= 3:
                    return x, y
                best = best or (x, y)
    return best


def force_loss(b, map_name, why):
    """Lose a real wild battle on `map_name`: lead at 1 HP, weakest move, no items, no running."""
    ax, ay = arrival_cell(map_name)
    b.warp(map_name, ax, ay, f"test setup: {why}", settle=True)
    gx, gy = grass_cell(b)
    b.warp(map_name, gx, gy, f"test setup: grass cell for {why}", settle=True)
    set_party_hp(b, 1, why)
    b.lose_mode = True
    start = b.loc()
    dirs = ["LEFT", "RIGHT", "UP", "DOWN"]
    try:
        for i in range(400):
            if b.loc() != start:
                break
            if b.emu.in_battle():
                b.run_battle()
                b.settle()
                continue
            if not b.ready():
                b.settle()
                continue
            m = b.snapshot_map()
            px, py = b.pos()
            for k in range(4):
                d = dirs[(i + k) % 4]
                dx, dy = Q.DIRV[d]
                if b.cell(m, px + dx, py + dy)[0] and b.cell(m, px + dx, py + dy)[1] in Q.GRASS:
                    b.step(d)
                    break
    finally:
        b.lose_mode = False
    b.settle()


# ---------------------------------------------------------------------------------------------
def sec_whiteout(b, a):
    res = []
    cases = [
        ("Route1 before any inn", "02", "Route1", "PalletTown_PlayersHouse_1F", "Masara home"),
        ("Route1 after the Tokiwa rest", "04", "Route1", "ViridianCity_PokemonCenter_1F", "Tokiwa inn"),
        ("Route2 after the Nibi rest", "06", "Route2", "PewterCity_PokemonCenter_1F", "Nibi inn"),
    ]
    for label, st, mp, want_map, want_name in cases:
        b.load_state(os.path.join(a.states, f"{st}.ss"))
        b.emu.run(30)
        b.seen.clear()
        money0 = b.money()
        resp0 = b.respawn()
        force_loss(b, mp, label)
        got = b.map_name()
        texts = " | ".join(b.seen[-8:])
        b.check(f"whiteout [{label}]: lands in the {want_name}", got, want_map)
        mons = b.party()
        b.check(f"whiteout [{label}]: party healed (HP)", all(m["hp"] == m["maxhp"] for m in mons), True)
        b.check(f"whiteout [{label}]: money did not grow", b.money() <= money0, True)
        b.check(f"whiteout [{label}]: player is free", b.ready(), True)
        res.append(dict(case=label, respawn_before=resp0, landed=got, want=want_map, ok=got == want_map,
                        tp_after=[m["tp"] for m in mons], money=(money0, b.money()), texts=texts[-400:]))
        b.say(f"  whiteout case [{label}]: landed {got}; TP spent after {[m['tp'] for m in mons]}; texts: {texts[-220:]}")
        b.shot("whiteout_" + st)
    return res


def sec_noheal(b, a):
    res = []
    b.load_state(os.path.join(a.states, "02.ss"))
    b.emu.run(30)
    mom_map = "PalletTown_PlayersHouse_1F"
    ax, ay = arrival_cell(mom_map)
    b.warp(mom_map, ax, ay, "test setup: Mom", settle=True)
    set_party_hp(b, 3, "Mom must not heal")
    before = [(m["hp"], m["tp"]) for m in b.party()]
    mom = [o for o in Q.mapjson(mom_map)["object_events"] if "Mom" in o.get("script", "")]
    for o in mom:
        b.talk(o["x"], o["y"])
        b.settle()
    after = [(m["hp"], m["tp"]) for m in b.party()]
    b.check("no heal: Mom leaves HP and TP alone", after, before)
    res.append(dict(case="Mom", before=before, after=after, texts=b.seen[-4:]))
    # the vendor on Route 1: a second visit gives no free Potion and no heal
    b.warp("Route1", 6, 29, "test setup: vendor", settle=True)
    n0 = b.bag_count("ITEM_POTION")
    before = [(m["hp"], m["tp"]) for m in b.party()]
    b.talk(6, 28)
    b.settle()
    b.talk(6, 28)
    b.settle()
    after = [(m["hp"], m["tp"]) for m in b.party()]
    b.check("no heal: the vendor leaves HP and TP alone", after, before)
    b.check("no heal: no second free Potion from the vendor", b.bag_count("ITEM_POTION") <= n0, True)
    res.append(dict(case="vendor", before=before, after=after, potions=(n0, b.bag_count("ITEM_POTION"))))
    return res


def continue_game(b):
    e = b.emu
    e.run(3400)
    n = 0
    while not e.in_overworld():
        e.press("A", hold=4, after=8)
        n += 1
        if n > 400:
            raise Q.Fail("Continue never reached the overworld")
    Q.R.wait_field(e)
    e.run(20)


def sec_savereload(b, a):
    res = []
    b.load_state(os.path.join(a.states, "07.ss"))
    b.emu.run(30)
    for name in Q.slice_maps():
        try:
            ax, ay = arrival_cell(name)
            b.warp(name, ax, ay, "test setup: save and reload", settle=True)
            b.settle()
            before = (b.loc(), tuple(b.pos()), b.party_count(), b.flag("FLAG_BADGE02_GET"), b.money())
            saved = b.save_game()
            b.check(f"save in {name}: the game says it saved", saved, True)
            b.emu.cmd("reset")
            continue_game(b)
            after = (b.loc(), tuple(b.pos()), b.party_count(), b.flag("FLAG_BADGE02_GET"), b.money())
            same = before == after
            b.check(f"save/reload in {name}", same, True)
            res.append(dict(map=name, before=before, after=after, ok=same))
        except Exception as ex:
            b.say(f"  save/reload in {name}: EXCEPTION {type(ex).__name__}: {ex}")
            res.append(dict(map=name, error=f"{type(ex).__name__}: {ex}"))
            b.checks.append((f"save/reload in {name}", False, str(ex)))
    return res


def sec_npcs(b, a):
    res = []
    st = os.path.join(a.states, "07.ss")
    b.load_state(st)
    b.emu.run(30)
    for name in Q.slice_maps():
        mj = Q.mapjson(name)
        npcs = [o for o in mj.get("object_events", [])
                if o.get("script") and o["script"] != "0"
                and o.get("trainer_type", "TRAINER_TYPE_NONE") == "TRAINER_TYPE_NONE"
                and "ITEM_BALL" not in o.get("graphics_id", "") and "CUT_TREE" not in o.get("graphics_id", "")]
        if not npcs:
            continue
        ax, ay = arrival_cell(name)
        b.warp(name, ax, ay, "test setup: NPC scan", settle=True)
        for o in npcs:
            tag = f"{name}:{o['script']}"
            try:
                if b.map_name() != name:
                    b.warp(name, ax, ay, "test setup: back on the map after an NPC moved the player", settle=True)
                try:
                    b.talk(o["x"], o["y"], mash_limit=0)       # A pressed, nothing mashed yet
                except KeyError:                               # a wandering NPC moved while the bot approached: once more
                    b.warp(name, ax, ay, "test setup: retry an NPC that moved", settle=True)
                    b.talk(o["x"], o["y"], mash_limit=0)
                b.emu.run(30)
                started = not b.ready()
                b.emu.hold("DOWN", 20)                          # try to walk away mid-box: nothing may happen
                b.emu.press("B", hold=4, after=10)
                # B closes boxes and answers NO; A would walk into a shop's BUY list and buy (the Nibi Market clerk)
                free = b.mash(key="B", n=80) or b.settle(limit=400)
                res.append(dict(npc=tag, text_started=started, free=free))
                if not free:
                    b.say(f"  NPC SOFTLOCK {tag}")
                    b.shot("softlock_" + o["script"][:30])
                    b.checks.append((f"NPC {tag} frees the player", False, "not free after settle"))
                    b.load_state(st)
                    b.warp(name, ax, ay, "test setup: after a softlock", settle=True)
            except Q.BlockedPath as ex:
                res.append(dict(npc=tag, unreachable=str(ex), at=b.pos()))
                b.say(f"  NPC unreachable {tag}: {ex}")
                b.load_state(st)
                b.warp(name, ax, ay, "test setup: after an unreachable NPC", settle=True)
            except Exception as ex:
                res.append(dict(npc=tag, error=f"{type(ex).__name__}: {ex}"))
                b.say(f"  NPC error {tag}: {type(ex).__name__}: {ex}")
                b.load_state(st)
                b.warp(name, ax, ay, "test setup: after an error", settle=True)
    return res


def sec_coords(b, a):
    res = []
    b.load_state(os.path.join(a.states, "07.ss"))
    b.emu.run(30)
    for name in Q.slice_maps():
        mj = Q.mapjson(name)
        cells = sorted({(c["x"], c["y"]) for c in mj.get("coord_events", [])})
        if not cells:
            continue
        ax, ay = arrival_cell(name)
        for (cx, cy) in cells:
            tag = f"{name}({cx},{cy})"
            try:
                b.warp(name, ax, ay, "test setup: coord-event scan", settle=True)
                m = b.snapshot_map()
                stand = None
                for d, (dx, dy) in Q.DIRV.items():
                    nx, ny = cx - dx, cy - dy
                    ok, beh = b.cell(m, nx, ny)
                    if ok and (nx, ny) not in cells:
                        stand = (nx, ny, d)
                        break
                if stand is None:
                    res.append(dict(cell=tag, skipped="no free cell next to it"))
                    continue
                for rep in range(2):
                    b.warp(name, stand[0], stand[1], "test setup: coord-event scan", settle=True)
                    b.step(stand[2])
                    free = b.settle(limit=400)
                    if not free:
                        b.say(f"  COORD SOFTLOCK {tag} on pass {rep + 1}")
                        b.shot("coord_softlock_" + name + f"_{cx}_{cy}")
                        b.checks.append((f"coord cell {tag} frees the player (pass {rep + 1})", False, "not free"))
                        res.append(dict(cell=tag, pass_=rep + 1, free=False))
                        b.load_state(os.path.join(a.states, "07.ss"))
                        b.emu.run(30)
                        break
                else:
                    res.append(dict(cell=tag, free=True))
            except Exception as ex:
                res.append(dict(cell=tag, error=f"{type(ex).__name__}: {ex}"))
                b.say(f"  coord error {tag}: {type(ex).__name__}: {ex}")
                b.load_state(os.path.join(a.states, "07.ss"))
                b.emu.run(30)
    return res


SECTIONS = dict(whiteout=sec_whiteout, noheal=sec_noheal, savereload=sec_savereload, npcs=sec_npcs, coords=sec_coords)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("section", choices=sorted(SECTIONS))
    ap.add_argument("--states", default="/tmp/qa_full/states")
    ap.add_argument("--out", default=os.path.join(Q.ROOT, "build", "qa", "systemic"))
    ap.add_argument("--rom", default=Q.ROM)
    ap.add_argument("--elf", default=Q.ELF)
    a = ap.parse_args()
    b = Q.Bot(os.path.join(a.out, a.section), rom=a.rom, elf=a.elf)
    b.begin("systemic_" + a.section)
    try:
        res = SECTIONS[a.section](b, a)
    except Exception:
        traceback.print_exc()
        res = dict(error=traceback.format_exc())
    bad = [c for c in b.checks if not c[1]]
    json.dump(dict(section=a.section, result=res, checks=b.checks, injections=b.injections),
              open(os.path.join(a.out, f"systemic_{a.section}.json"), "w"), indent=1, default=str)
    err = isinstance(res, dict) and "error" in res
    print(f"RESULT {a.section}: {'PASS' if not bad and not err else 'FAIL'} "
          f"({len(b.checks)} checks, {len(bad)} failed, {len(b.injections)} test-setup injections)")
    b.close()
    return 1 if bad or err else 0


if __name__ == "__main__":
    sys.exit(main())
