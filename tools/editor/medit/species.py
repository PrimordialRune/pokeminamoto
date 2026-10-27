"""Species records (src/data/species/<name>.json): learnset, evolutions and TM/HM lists.

Files are in the canonical form tools/speciesgen/speciesgen.py writes, so load -> save is byte-identical:
`save` uses speciesgen's own `canonical()`. A record is kept as a dict; the editor changes only the keys it
shows (levelUpMoves, evolutions, tmhm), the rest passes through untouched.
"""
import json
import os
import sys

from . import paths

EDITABLE = ("levelUpMoves", "evolutions", "tmhm", "tutor", "eggMoves")


def _speciesgen():
    d = os.path.join(paths.REPO, "tools", "speciesgen")
    if d not in sys.path:
        sys.path.insert(0, d)
    import speciesgen
    return speciesgen


class SpeciesStore:
    def __init__(self, store):
        self.store = store

    def names(self):
        txt = self.store.read_repo(paths.SPECIES_DIR + "/species.txt")
        return [l.split("#")[0].strip() for l in txt.split("\n") if l.split("#")[0].strip()]

    def file(self, name):
        return "%s/%s.json" % (paths.SPECIES_DIR, name.lower())

    def read_text(self, name):
        return self.store.read(self.file(name))

    def load(self, name):
        return json.loads(self.read_text(name))

    def render(self, rec):
        return _speciesgen().canonical(rec)

    def save(self, name, rec):
        """Write the canonical text of `rec`; returns the path written."""
        return self.store.write(self.file(name), self.render(rec))

    def summary(self):
        out = []
        for n in self.names():
            r = self.load(n)
            if r.get("info", 1) is None:
                continue
            out.append({"species": r["species"], "name": r.get("name", n), "dex": r.get("nationalDex", 0),
                        "types": list(dict.fromkeys(r.get("types", []))),
                        "evolutions": r.get("evolutions", []) if isinstance(r.get("evolutions"), list) else [],
                        "moves": len(r["levelUpMoves"]) if isinstance(r.get("levelUpMoves"), list) else 0})
        return out

    def apply_edit(self, rec, edit):
        """Merge the editable keys of `edit` into a copy of `rec` (anything else in `edit` is ignored)."""
        new = dict(rec)
        for k in EDITABLE:
            if k in edit:
                new[k] = edit[k]
        return new
