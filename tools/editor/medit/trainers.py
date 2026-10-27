"""src/data/trainers.party: a lossless parser and writer.

The file is a list of chunks: free text (the header comment, `/* BEGIN owner */` / `/* END owner */` markers,
blank lines, any other comment) and trainer blocks. Free text is never touched. A trainer block that was not
edited is written back as its original lines; an edited one is re-rendered from the model, which prints the
same layout the file uses (keys `Key: value`, blank line, Pokemon separated by blank lines, `- Move` last).
`Party.dumps()` of an unedited document is byte-identical to the input (tests/test_roundtrip.py).
"""
import copy
import re

TRAINER_RE = re.compile(r"^===\s*(\w+)\s*===\s*$")
KEY_RE = re.compile(r"^([A-Za-z][A-Za-z ]*):[ ]?(.*)$")
NATURE_RE = re.compile(r"^(\w+) Nature$")
BEGIN_RE = re.compile(r"^/\*\s*BEGIN\s+([\w-]+)\s*\*/\s*$")
END_RE = re.compile(r"^/\*\s*END\s+([\w-]+)\s*\*/\s*$")


def words(const):
    """MOVE/ITEM/SPECIES constant tail -> the words the party file uses (ROCK_THROW -> Rock Throw)."""
    return const.replace("_", " ").title()


def const(word, prefix=""):
    """The way tools/trainerproc spells a word as a constant (without the prefix)."""
    w = word.strip()
    if prefix and w.startswith(prefix + "_"):
        return w[len(prefix) + 1:]
    w = w.replace("♂", " M").replace("♀", " F").replace("'", "").replace("%", "")
    return re.sub(r"[^A-Z0-9]+", "_", w.upper()).strip("_")


def parse_head(head):
    """`[Nick (]Species[)] [(M)/(F)] [@ Item]` -> dict(species, nick, gender, item) with species as a word string."""
    item = None
    if "@" in head:
        head, item = (x.strip() for x in head.split("@", 1))
    gender = None
    m = re.search(r"\s*\((M|F)\)\s*$", head)
    if m:
        gender, head = m.group(1), head[:m.start()]
    nick = None
    m = re.match(r"^(.*?)\s*\(([^()]*)\)\s*$", head)
    if m and len(m.group(2)) > 1:
        nick, head = m.group(1), m.group(2)
    return {"species": head.strip(), "nick": nick, "gender": gender, "item": item}


def make_head(p):
    s = p["species"]
    if p.get("nick"):
        s = "%s (%s)" % (p["nick"], s)
    if p.get("gender"):
        s += " (%s)" % p["gender"]
    if p.get("item"):
        s += " @ %s" % p["item"]
    return s


class Mon:
    def __init__(self, head, fields=None, moves=None):
        self.head = head
        self.fields = fields or []       # [[key, value]]; key None = a line kept verbatim in value
        self.moves = moves or []

    def lines(self):
        out = [self.head]
        for k, v in self.fields:
            if k is None:
                out.append(v)
            elif k == "@nature":
                out.append("%s Nature" % v)
            else:
                out.append("%s: %s" % (k, v) if v != "" else "%s:" % k)
        out += ["- " + m for m in self.moves]
        return out

    def to_json(self):
        d = parse_head(self.head)
        d.update({"head": self.head, "fields": [list(f) for f in self.fields], "moves": list(self.moves)})
        return d

    @staticmethod
    def from_json(d, old=None):
        p = {k: (d.get(k) or None) for k in ("species", "nick", "gender", "item")}
        p["species"] = p["species"] or ""
        if old is not None and parse_head(old.head) == p:
            head = old.head                # unchanged parts keep the original spelling
        else:
            head = make_head(p)
        return Mon(head, [list(f) for f in d.get("fields", [])], list(d.get("moves", [])))

    def field(self, key, default=None):
        for k, v in self.fields:
            if k == key:
                return v
        return default


def parse_mon(lines):
    mon = Mon(lines[0])
    for ln in lines[1:]:
        if ln.startswith("- "):
            mon.moves.append(ln[2:])
        elif NATURE_RE.match(ln):
            mon.fields.append(["@nature", NATURE_RE.match(ln).group(1)])
        elif KEY_RE.match(ln):
            m = KEY_RE.match(ln)
            mon.fields.append([m.group(1), m.group(2)])
        else:
            mon.fields.append([None, ln])
    return mon


class Trainer:
    def __init__(self, id, keys, mons, raw, owner=None):
        self.id, self.keys, self.mons, self.raw, self.owner = id, keys, mons, raw, owner
        self.pristine = self.snapshot()

    def snapshot(self):
        return (self.id, [list(k) for k in self.keys], [(m.head, [list(f) for f in m.fields], list(m.moves)) for m in self.mons])

    @property
    def dirty(self):
        return self.snapshot() != self.pristine

    def render_lines(self):
        out = ["=== %s ===" % self.id]
        for k, v in self.keys:
            if k is None:
                out.append(v)
            else:
                out.append("%s: %s" % (k, v) if v != "" else "%s:" % k)
        for m in self.mons:
            out.append("")
            out += m.lines()
        return out

    @property
    def lossless(self):
        return self.render_lines() == self.raw

    def lines(self):
        return self.raw if not self.dirty else self.render_lines()

    def key(self, k, default=None):
        for kk, v in self.keys:
            if kk == k:
                return v
        return default

    def to_json(self):
        return {"id": self.id, "owner": self.owner, "keys": [list(k) for k in self.keys],
                "mons": [m.to_json() for m in self.mons], "dirty": self.dirty}

    def set_from_json(self, d):
        self.keys = [list(k) for k in d["keys"]]
        olds = self.mons
        self.mons = [Mon.from_json(m, olds[i] if i < len(olds) else None) for i, m in enumerate(d["mons"])]


def parse_trainer(lines, owner=None):
    m = TRAINER_RE.match(lines[0])
    i = 1
    keys = []
    while i < len(lines) and lines[i].strip() != "":
        km = KEY_RE.match(lines[i])
        keys.append([km.group(1), km.group(2)] if km else [None, lines[i]])
        i += 1
    mons, cur = [], []
    for ln in lines[i:]:
        if ln.strip() == "":
            if cur:
                mons.append(parse_mon(cur))
                cur = []
        else:
            cur.append(ln)
    if cur:
        mons.append(parse_mon(cur))
    return Trainer(m.group(1), keys, mons, list(lines), owner)


class Party:
    """chunks: ('text', [lines]) or ('trainer', Trainer). The last text chunk ends the file."""

    def __init__(self, text):
        lines = text.split("\n")
        self.chunks = []
        owner = None
        buf = []
        i = 0
        while i < len(lines):
            ln = lines[i]
            if TRAINER_RE.match(ln):
                if buf:
                    self.chunks.append(["text", buf])
                    buf = []
                j = i + 1
                while j < len(lines) and not TRAINER_RE.match(lines[j]) and not lines[j].startswith("/*"):
                    j += 1
                k = j
                while k > i + 1 and lines[k - 1].strip() == "":
                    k -= 1
                self.chunks.append(["trainer", parse_trainer(lines[i:k], owner)])
                buf = lines[k:j]
                i = j
                continue
            b, e = BEGIN_RE.match(ln), END_RE.match(ln)
            if b:
                owner = b.group(1)
            elif e:
                owner = None
            buf.append(ln)
            i += 1
        if buf:
            self.chunks.append(["text", buf])

    def trainers(self):
        return [c[1] for c in self.chunks if c[0] == "trainer"]

    def get(self, tid):
        for t in self.trainers():
            if t.id == tid:
                return t
        return None

    def dumps(self):
        out = []
        for kind, c in self.chunks:
            out += c if kind == "text" else c.lines()
        return "\n".join(out)

    def blocks(self):
        """Owner names of the BEGIN/END blocks, in file order."""
        seen = []
        for t in self.trainers():
            if t.owner and t.owner not in seen:
                seen.append(t.owner)
        return seen
