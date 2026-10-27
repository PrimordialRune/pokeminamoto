"""Signal 3.6: dominant strategies and dead content, derived from the other signals' data (no extra battles)."""
from . import _common as C


def run(ctx):
    sec = C.Section()
    g, cat = ctx.game, ctx.catalog
    flags = []
    # --- data-level dead content: species and moves no encounter, trainer or learnset reaches
    used_sp = set()
    for si in range(18):
        used_sp |= {g.base_form(w[0]) for w in cat.wild[si]}
    for f in cat.fights:
        used_sp |= {g.base_form(m["species"]) for m in cat.party_for(f, "CHARMANDER")}
    used_sp |= {"BULBASAUR", "CHARMANDER", "SQUIRTLE"}
    lines = {g.base_form(s) for s in g.species}
    missing = sorted(lines - used_sp)
    sec.data["dead_species"] = missing
    if missing:
        flags.append(C.flag("dead:species_unreachable", "dead_content", "info", 0.5,
                            "%d of %d evolution lines appear in no encounter, trainer or gift" % (len(missing), len(lines)),
                            "Unreachable so far (data still being written): %s." % ", ".join(s.title() for s in missing[:25]) + (" ..." if len(missing) > 25 else ""),
                            metric={"name": "unreachable_lines", "value": len(missing), "ci": None, "target": [0, 0]}))
    # --- move spam: share of all skilled turns taken by one move, per stage
    use = {}
    for (fid, band, st), rows in getattr(ctx, "fight_rows", {}).items():
        if band != "skilled":
            continue
        stg = ctx.fight_by_id[fid]["stage"]
        for r in rows:
            for mv, n in (r["moves"] or {}).items():
                use.setdefault(stg, {})
                use[stg][mv] = use[stg].get(mv, 0) + n
    gini_rows, spam = [], {}
    for stg, d in sorted(use.items()):
        tot = sum(d.values()) or 1
        top = max(d, key=d.get)
        vals = sorted(d.values())
        n = len(vals)
        gini = sum((2 * (i + 1) - n - 1) * v for i, v in enumerate(vals)) / (n * sum(vals)) if n > 1 else 0
        gini_rows.append({"stage": "s%02d" % stg, "top_move": top, "top_share": round(d[top] / tot, 3), "gini": round(gini, 3), "moves": n})
        if d[top] / tot > 0.6:
            spam.setdefault(top, []).append(stg)
    sec.data["move_usage"] = gini_rows
    for mv, stgs in spam.items():
        if len(stgs) >= 5:
            flags.append(C.flag("dominance:spam:%s" % mv, "dominance", "amber", 1.5, "%s is spammed across %d stages" % (mv.title().replace("_", " "), len(stgs)),
                                "Takes over 60%% of all skilled turns in stages %s." % ", ".join("s%02d" % s for s in sorted(stgs)),
                                band="skilled", subject={"move": mv}, metric={"name": "top_share", "value": 0.6, "ci": None, "target": [0, 0.6]}))
    # --- choice collapse: stages where fewer than 2 moves take more than 10% of turns
    for r in gini_rows:
        d = use[int(r["stage"][1:])]
        tot = sum(d.values())
        if sum(1 for v in d.values() if v / tot > 0.10) < 2 and tot > 200:
            flags.append(C.flag("dominance:collapse:%s" % r["stage"], "dominance", "info", 0.4, "Choice collapse at %s" % r["stage"],
                                "Only one move takes more than 10%% of skilled turns (%s %.0f%%)." % (r["top_move"], 100 * r["top_share"]),
                                stage=r["stage"], band="skilled"))
    # --- starter dominance: a starter that wins far more than the others across all fights (average band)
    tot = {}
    for (fid, band, st), rows in getattr(ctx, "fight_rows", {}).items():
        if band == ctx.bands[0] and ctx.fight_by_id[fid]["class"] != "wild":
            a = tot.setdefault(st, [0, 0])
            a[0] += sum(r["w"] == 0 for r in rows)
            a[1] += len(rows)
    rates = {s: a[0] / a[1] for s, a in tot.items() if a[1]}
    sec.data["starter_win"] = {s: round(v, 3) for s, v in rates.items()}
    if len(rates) == 3 and max(rates.values()) - min(rates.values()) > 0.20:
        hi, lo = max(rates, key=rates.get), min(rates, key=rates.get)
        flags.append(C.flag("dominance:starter", "dominance", "amber", 1.6, "%s beats %s across the whole game" % (hi.title(), lo.title()),
                            "Win rate over every trainer, rival and gym fight (%s band): %s." % (ctx.bands[0], ", ".join("%s %.0f%%" % (s.title(), 100 * v) for s, v in sorted(rates.items()))),
                            band=ctx.bands[0], metric={"name": "starter_spread", "value": round(max(rates.values()) - min(rates.values()), 3), "ci": None, "target": [0, 0.2]}))
    sec.flags = flags
    return sec
