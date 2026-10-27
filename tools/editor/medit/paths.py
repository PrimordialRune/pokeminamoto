"""Where the repo is, where reads come from and where saves go.

The editor never writes repo data unless it is started with --in-place. Saves go to an out directory
(default build/editor/out, gitignored) under the same relative path as the repo file, so a save can be diffed
against the repo and copied over by hand. Tests point both at temp copies.
"""
import os

HERE = os.path.dirname(os.path.abspath(__file__))
EDITOR_DIR = os.path.dirname(HERE)
REPO = os.environ.get("MEDIT_REPO") or os.path.dirname(os.path.dirname(EDITOR_DIR))
SIM_ROOT = os.path.join(REPO, "tools", "balance_sim")

SPECIES_DIR = "src/data/species"
TRAINERS = "src/data/trainers.party"
ENCOUNTERS = "src/data/wild_encounters.json"
ARENA = "src/arena_ecology.c"


class Store:
    """Reads files from `root`, writes them to `out` (or back to `root` when in_place)."""

    def __init__(self, root=None, out=None, in_place=False):
        self.root = os.path.abspath(root or REPO)
        self.in_place = in_place
        self.out = self.root if in_place else os.path.abspath(out or os.path.join(self.root, "build", "editor", "out"))

    def path(self, rel):
        return os.path.join(self.root, rel)

    def read(self, rel):
        """Text exactly as on disk (no newline translation); an edited copy in `out` wins over the repo file."""
        for base in (self.out, self.root):
            p = os.path.join(base, rel)
            if os.path.exists(p):
                with open(p, encoding="utf-8", newline="") as f:
                    return f.read()
        raise FileNotFoundError(rel)

    def read_repo(self, rel):
        with open(self.path(rel), encoding="utf-8", newline="") as f:
            return f.read()

    def write(self, rel, text):
        p = os.path.join(self.out, rel)
        os.makedirs(os.path.dirname(p), exist_ok=True)
        with open(p, "w", encoding="utf-8", newline="") as f:
            f.write(text)
        return p
