"""Game stages derived from the repo: the overworld path, map connections/warps and the gym order.

Stage = (badges earned, early|late half of the span to the next gym). 18 stages: s00 .. s17, where
badge span 8 is the postgame / League. The main path is docs/design/data/overworld.csv (order_id);
every other map takes the stage of the nearest path map across connections and warps. Anything the
walk cannot reach is listed in `Stages.lint`. Override any map with `stage_overrides` in targets.yaml.
"""
from .gamedata import Game, ROOT  # noqa: F401

# Gym towns in badge order: (gym number, leader, overworld.csv localized name of the gym town).
# Gym 8 has no town in the design yet (open-questions B): placed between Route 21 and Route 23.
GYMS = [
    (1, "Jiro", "Viridian City"),
    (2, "Brock", "Pewter City"),
    (3, "Misty", "Cerulean City"),
    (4, "Surge", "Vermillion City"),
    (5, "Erika", "Celadon City"),
    (6, "Koga", "Fuchsia City"),
    (7, "Coleman", "Cinnabar Island"),
    (8, "Gym 8", None),
]
GYM8_AFTER_ORDER = 41.0          # Route 23; the unassigned 8th gym sits right after it

# Opening maps that are still Hoenn leftovers (the start of the game until Kanto lands)
OPENING = ("LittlerootTown", "OldaleTown", "Route101", "Route103", "InsideOfTruck", "BattleColosseum", "MasaraTown")


class Stages:
    def __init__(self, game=None, overrides=None):
        self.game = game or Game.get()
        self.lint = []
        self.stages = []
        self.map_stage = {}
        self.row_stage = {}
        self._build(overrides or {})

    # --------------------------------------------------------------
    def _gym_orders(self):
        g = self.game
        by_name = {}
        for r in g.overworld:
            by_name[(r["localized_name"] or r["name"])] = r["order"]
        out = []
        for no, leader, town in GYMS:
            out.append(by_name.get(town, GYM8_AFTER_ORDER) if town else GYM8_AFTER_ORDER + 0.5)
            if town and town not in by_name:
                self.lint.append(("gym_town", town, "gym %d town not in overworld.csv" % no))
        return out

    def _build(self, overrides):
        g = self.game
        orders = self._gym_orders()
        rows = g.overworld
        span_rows = {b: [] for b in range(9)}
        for r in rows:
            b = sum(1 for o in orders if r["order"] > o)
            span_rows[b].append(r)
        for b in range(9):
            lst = span_rows[b]
            half = (len(lst) + 1) // 2
            for i, r in enumerate(lst):
                late = i >= half
                self.row_stage[r["order_id"]] = 2 * b + (1 if late else 0)
        for i in range(18):
            b, late = divmod(i, 2)
            if b < 8:
                nm = "%s to %s: %s" % ("Path" if b else "Start", GYMS[b][1], "late" if late else "early")
                nm = ("Before %s (%s)" % (GYMS[b][1], "late" if late else "early"))
            else:
                nm = "League and postgame (%s)" % ("late" if late else "early")
            self.stages.append({"id": "s%02d" % i, "index": i, "name": nm, "badges": b,
                                "level_range": None, "maps": [], "orders": []})
        for r in rows:
            self.stages[self.row_stage[r["order_id"]]]["orders"].append(r["order_id"])

        # seeds: rows -> maps
        dist = {}
        frontier = []
        for r in rows:
            mid = g.map_for_row(r)
            if mid is None:
                self.lint.append(("row_map", r["name"], "overworld.csv row %s has no map in data/maps" % r["order_id"]))
                continue
            st = self.row_stage[r["order_id"]]
            if mid not in self.map_stage or st < self.map_stage[mid]:
                self.map_stage[mid] = st
            dist[mid] = 0
            frontier.append(mid)
        for mid, m in g.maps.items():
            if m["dir"].startswith(OPENING):
                self.map_stage[mid] = 0
                dist[mid] = 0
                frontier.append(mid)
        # multi-source BFS across connections and warps; ties go to the earlier stage
        while frontier:
            nxt = []
            for mid in frontier:
                for nb in g.neighbours(mid):
                    if nb not in dist:
                        dist[nb] = dist[mid] + 1
                        self.map_stage[nb] = self.map_stage[mid]
                        nxt.append(nb)
                    elif dist[nb] == dist[mid] + 1 and self.map_stage[mid] < self.map_stage[nb]:
                        self.map_stage[nb] = self.map_stage[mid]
            frontier = nxt
        for mid, st in overrides.items():
            self.map_stage[mid] = int(st)
        for mid in g.maps:
            if mid not in self.map_stage:
                self.lint.append(("map_no_stage", mid, "no path from the overworld.csv route reaches this map"))
        for mid, st in self.map_stage.items():
            self.stages[st]["maps"].append(mid)
        for s in self.stages:
            s["maps"].sort()

    # --------------------------------------------------------------
    def of_map(self, map_id):
        return self.map_stage.get(map_id)

    def of_row(self, row):
        return self.row_stage[row["order_id"]]

    def row_by_name(self, name):
        """overworld.csv row by Minamoto or English name (case/punctuation insensitive)."""
        from .gamedata import _norm
        k = _norm(name)
        for r in self.game.overworld:
            if _norm(r["name"]) == k or (r["localized_name"] and _norm(r["localized_name"]) == k):
                return r
        return None

    def get(self, sid):
        return self.stages[int(sid[1:])]

    def gym_stage(self, gym_no):
        """The late stage of the span that ends in gym `gym_no` (arrival at the gym)."""
        return 2 * (gym_no - 1) + 1

    def public(self):
        return [{"id": s["id"], "index": s["index"], "name": s["name"], "badges": s["badges"],
                 "level_range": s["level_range"], "maps": s["maps"][:40]} for s in self.stages]


def api_stages():
    """Stages for GET /api/data (GUI server)."""
    return Stages().public()
