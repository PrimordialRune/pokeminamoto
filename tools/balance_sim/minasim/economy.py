"""The slice's money and shop model (slice-plan 3.2, 4 and 7.3), read from the repo's own data where it exists.

Facts read from the sources (never copied):
  - prize money: `4 * last-mon level * class value` from gTrainerMoneyTable (src/battle_main.c), single battles
  - shop prices: `.price` of the item in src/data/items.h
  - starting money: SetMoney(..., N) in src/new_game.c

Rest-is-a-place rule (plan pillar 3): Mom does not heal, nothing heals outside an inn except the lab's one free
Potion sample on Route 1 (the vendor, once, no shop), the Welcome Kit and quest Potions. Only an inn restores HP
and TP. A whiteout returns the player to the last inn rested (full HP and TP) and costs money. A no-whiteout fight
(lab Shigeru, Ken, Route 22 Shigeru) restores HP and status but not TP after a loss, and costs nothing.

Everything the sources do not say is a PLACEHOLDER constant below, with the existing design thread that owns it.
"""
import os
import re

from .gamedata import ROOT, _read

# ---- PLACEHOLDERS (owner thread "Make TP need rest, with town toolkit" unless noted) ---------------------------
OKIDO_BALLS = 5            # Okido's gift (slice-plan 4.3 question 16): canon from Omar's answers
KIT_BALLS = 3              # Welcome Kit: the script list's proposal, Tokiwa v5 only says "a limited number"
KIT_POTIONS = 1
FOREST_ITEM_BALL = 1       # the forest item ball before the Nibi Market
VENDOR_POTIONS = 1         # Route 1 vendor, once, the R/G free sample (no shop, no repeat)
MUSEUM_TICKET = 50         # Nibi museum ticket, plan 3.2 (refuses at 0)
SANDSHREW_SALE = 3500      # Omura's offer for the Route 22 Sandshrew (optional branch), plan 4.2 row 6
WHITEOUT_LOSS = 0.5        # share of money lost on a whiteout (Emerald's rule; no slice rule written): PLACEHOLDER
BALL_CATCH_CHANCE = 0.5    # chance one thrown ball catches (a weakened wild mon, Gen 3 rates): PLACEHOLDER
TRAINER_CLASS_DEFAULT = {"trainer": "Youngster", "rival": "Rival", "gym": "Leader"}


def _items_h():
    return _read("src/data/items.h")


def item_price(item):
    """Shop price of an item const name ('POTION', 'POKE_BALL'), from src/data/items.h."""
    m = re.search(r"\[ITEM_%s\]\s*=\s*\{.*?\.price\s*=\s*(\d+)" % re.escape(item), _items_h(), re.S)
    return int(m.group(1)) if m else 0


def start_money():
    m = re.search(r"SetMoney\([^,]+,\s*(\d+)\)", _read("src/new_game.c"))
    return int(m.group(1)) if m else 3000


def class_values():
    """{class const without TRAINER_CLASS_: value} from gTrainerMoneyTable; '_default' for unlisted classes."""
    src = _read("src/battle_main.c")
    blk = src.split("gTrainerMoneyTable[]", 1)[1].split("};", 1)[0]
    out = {m.group(1): int(m.group(2)) for m in re.finditer(r"\{TRAINER_CLASS_(\w+),\s*(\d+)\}", blk)}
    d = re.search(r"\{0xFF,\s*(\d+)\}", blk)
    out["_default"] = int(d.group(1)) if d else 5
    return out


_CV = {}


def prize(trainer_class, last_level):
    """Prize money for beating a single-battle trainer of `trainer_class` ('Bird Keeper') whose last mon is `last_level`."""
    if not _CV:
        _CV.update(class_values())
    key = re.sub(r"[^A-Z0-9]+", "_", (trainer_class or "").upper()).strip("_")
    return 4 * last_level * _CV.get(key, _CV["_default"])


def balls_to_catch(rng, have):
    """(balls spent, caught) for one catch attempt with `have` balls: throws until one sticks or the supply runs out."""
    n = 0
    while n < have:
        n += 1
        if rng.random() < BALL_CATCH_CHANCE:
            return n, True
    return n, False
