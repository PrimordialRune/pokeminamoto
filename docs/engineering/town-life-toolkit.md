# Town-life toolkit

Script helpers for towns that change with Charisma, the clock and what the player has seen, plus inn rest. The C side is `src/town_life.c`, the constants are in `include/constants/town_life.h`, the macros are at the end of `asm/macros/event.inc`. None of it has RAM or save data of its own: it uses existing flags and vars.

Test content for everything here is in Oldale Town (`data/maps/OldaleTown/scripts.pory`) and the Oldale inn (`data/maps/OldaleTown_House1`).

## Persistent TP and rest

A party mon's TP carries over between battles (`MON_DATA_SAVED_TP`, stored as TP spent so a new mon is full). It only refills when the party rests:

| Rest | Restores |
|---|---|
| Inn (`Common_EventScript_Inn`, `special InnRest`) | HP, status, TP |
| Home (`Common_EventScript_OutOfCenterPartyHeal`), whiteout, Hall of Fame, debug heal | HP, status, TP (`RestPlayerParty`) |
| Nurse, story heals that aren't a rest (`HealPlayerParty`) | HP and status only |

Pokémon Centers are gone from the design (Omar, 2026-10-26): inns are the only place to rest. Oldale's Center map is a Hoenn leftover; its nurse still heals HP and status but not TP. Ethers and Elixirs now work in the field and restore saved TP. Link, recorded and Frontier battles start full and don't write TP back.

## Inns

`data/scripts/inn.pory`. An innkeeper calls the shared script and sets its own respawn point, so a whiteout wakes the player at the last inn rested at:

```
script MyTown_Inn_EventScript_Keeper {
    call(Common_EventScript_Inn)
    if (var(VAR_RESULT) == TRUE) {
        setrespawn(HEAL_LOCATION_MY_TOWN_INN)
    }
    release
    end
}
```

| Room | Opens | Rest |
|---|---|---|
| Ume | always (Trainer Card check comes later) | HP, status, TP |
| Take | Charisma ≥ `INN_TAKE_CHARISMA` (5) or `INN_TAKE_BADGES` (2) badges | as Ume, then a healing item turns up in the room `INN_TAKE_ITEM_STEPS` (100) steps later |
| Matsu | `FLAG_INN_MATSU_OPEN` (placeholder story flag) | as Take, plus `INN_MATSU_FRIENDSHIP` (5) friendship |

The Take item is an object with flag `FLAG_HIDE_INN_TAKE_ITEM`. A Take or Matsu stay sets the flag and `VAR_INN_TAKE_ITEM_STEPS`; each step counts down (`UpdateInnStepCounter`), and at 0 the flag clears, so the item is there next time the room is entered. Every inn shares the one item flag: one item is due at a time.

## Charisma tiers

```
checkcharisma(CHARISMA_TIERS_TOKIWA)
```

`VAR_RESULT` becomes `CHARISMA_TIER_NONE`, `LOW`, `MID` or `HIGH`. Thresholds are a table per place (`sCharismaTiers`); Tokiwa's are 2, 5 and 9 (tokiwa_city_v5 §15). Add a table for another town with new numbers.

## Clock

The hour comes from the cartridge RTC and falls back to play time without a working clock. With `VAR_TIME_OF_DAY_OVERRIDE` set (debug menu), the hour is pinned to one hour of that phase: morning 7, day 12, night 22.

```
checkhour(22, 23)   // VAR_RESULT TRUE when 22:00 <= hour < 23:00; windows can wrap (22, 2)
checktimeofday      // VAR_RESULT TIME_OF_DAY_MORN / DAY / NITE
```

## Timed NPCs

Give the object a hide flag and set it from the map's `MAP_SCRIPT_ON_TRANSITION`, which runs before objects spawn:

```
mapscripts MyTown_MapScripts {
    MAP_SCRIPT_ON_TRANSITION {
        showduringhours(FLAG_HIDE_KIMURA, 22, 23)
        showduringtime(FLAG_HIDE_NIGHT_MAN, TIME_OF_DAY_NITE)
    }
}
```

The check runs when the map loads, so an NPC never vanishes while the player watches; it is gone (or there) the next time the map is entered.

## Examine text that changes after seeing something

No helper, a convention. The first examine sets `FLAG_SEEN_<THING>`, and any other text checks it:

```
script Tokiwa_EventScript_CvsShelf {
    if (flag(FLAG_SEEN_KIMURA_BOWL)) {
        msgbox(format("Cat food. The same brand as the bowl by Kimura's door."), MSGBOX_SIGN)
    } else {
        msgbox(format("Cat food, the cheap kind."), MSGBOX_SIGN)
    }
}
```

Name the flag after what was seen, not where, so any later text can use it.
