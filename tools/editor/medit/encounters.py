"""src/data/wild_encounters.json. The file is json.dumps(indent=2) + newline, so load -> save is byte-identical."""
import copy
import json

from . import paths

KINDS = ("land_mons", "water_mons", "rock_smash_mons", "fishing_mons")


def dumps(doc):
    return json.dumps(doc, indent=2, ensure_ascii=False) + "\n"


class Encounters:
    def __init__(self, store):
        self.store = store
        self.text = store.read(paths.ENCOUNTERS)
        self.doc = json.loads(self.text)

    def entries(self):
        for g in self.doc["wild_encounter_groups"]:
            for e in g["encounters"]:
                yield g, e

    def flat(self):
        return [e for _g, e in self.entries()]

    def rates(self, kind):
        """The slot weights (encounter_rates) of a kind, from the group's field definitions."""
        for g in self.doc["wild_encounter_groups"]:
            for f in g.get("fields", []):
                if f["type"] == kind:
                    return f["encounter_rates"]
        return []

    def summary(self):
        out = []
        for i, e in enumerate(self.flat()):
            out.append({"index": i, "map": e["map"], "label": e.get("base_label"),
                        "kinds": {k: len(e[k]["mons"]) for k in KINDS if k in e}})
        return out

    def get(self, index):
        e = self.flat()[index]
        return {"index": index, "entry": e, "rates": {k: self.rates(k) for k in KINDS}}

    def replace(self, index, entry):
        entry = copy.deepcopy(entry)         # `entry` may be the very dict being replaced
        flat = self.flat()
        flat[index].clear()
        flat[index].update(entry)

    def render(self):
        return dumps(self.doc)

    def save(self):
        return self.store.write(paths.ENCOUNTERS, self.render())
