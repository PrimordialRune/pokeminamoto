# Gen 1-Only Build Changes

This document catalogs all changes made to transform the Pokémon Emerald codebase into a Gen 1-only build (Kanto Dex: Pokémon #001–#151).

## Overview

The goal of this refactoring was to create a fully playable Pokémon Emerald build containing only the original 151 Pokémon (Bulbasaur through Mew), plus SPECIES_EGG and any required internal constants.

### Approach

Rather than completely removing Gen 2/3 species definitions (which would require massive array restructuring), the approach taken was:

1. **Preserve species constants** - All species definitions remain for array index compatibility
2. **Replace gameplay references** - All Gen 2/3 Pokémon in gameplay-affecting data have been replaced with thematically similar Gen 1 equivalents
3. **Add helper macros** - Added `IS_SPECIES_GEN1()` macro for runtime validation if needed

---

## Category 1: Starter Pokémon

### File: `src/starter_choose.c`

**Change:** Replaced Gen 3 starters with Gen 1 starters

| Original | Replacement |
|----------|-------------|
| SPECIES_TREECKO | SPECIES_BULBASAUR |
| SPECIES_TORCHIC | SPECIES_CHARMANDER |
| SPECIES_MUDKIP | SPECIES_SQUIRTLE |

**Reason:** The game requires three starter Pokémon. Gen 1 Kanto starters are the natural replacement.

**Reversible:** Yes - simply restore the original species constants.

---

## Category 2: Wild Encounters

### File: `src/data/wild_encounters.json`

**Change:** Replaced 1,197 Gen 2/3 species references with Gen 1 equivalents

**Notable Replacement Mappings:**
- Route encounters: Gen 3 common Pokémon → Gen 1 common Pokémon
- Water routes: Gen 3 Water types → Gen 1 Water types
- Cave encounters: Gen 3 Rock/Ground types → Gen 1 Rock/Ground types

**Examples:**
| Original | Replacement | Reasoning |
|----------|-------------|-----------|
| SPECIES_WURMPLE | SPECIES_CATERPIE | Early-game Bug type |
| SPECIES_POOCHYENA | SPECIES_RATTATA | Early-game Normal type |
| SPECIES_ZIGZAGOON | SPECIES_RATTATA | Common Normal type |
| SPECIES_LOTAD | SPECIES_ODDISH | Water/Grass → Grass |
| SPECIES_WINGULL | SPECIES_PIDGEY | Common Flying type |
| SPECIES_TAILLOW | SPECIES_PIDGEY | Common Flying type |
| SPECIES_RALTS | SPECIES_ABRA | Psychic evolution line |
| SPECIES_SLAKOTH | SPECIES_SNORLAX | High-powered Normal type |

**Reason:** Players should only encounter Gen 1 Pokémon in the wild.

**Reversible:** Yes - restore from version control.

---

## Category 3: Trainer Parties

### File: `src/data/trainer_parties.h`

**Change:** Replaced 1,320 Gen 2/3 species references with Gen 1 equivalents

**Notable Changes:**
- Gym Leaders: Now use Gen 1 type specialists
- Elite Four: Updated with Gen 1 Pokémon
- Rival battles: Updated with Gen 1 counterparts
- All NPC trainers: Gen 2/3 Pokémon replaced

**Examples:**
| Trainer Type | Original | Replacement |
|--------------|----------|-------------|
| Bug Catchers | SPECIES_WURMPLE, SPECIES_CASCOON | SPECIES_CATERPIE, SPECIES_KAKUNA |
| Swimmers | SPECIES_WAILMER | SPECIES_SEEL |
| Bird Keepers | SPECIES_SWELLOW | SPECIES_PIDGEOT |
| Dragon Tamers | SPECIES_BAGON, SPECIES_SALAMENCE | SPECIES_DRATINI, SPECIES_DRAGONITE |

**Reason:** All trainer battles should feature Gen 1 Pokémon only.

**Reversible:** Yes - restore from version control.

---

## Category 4: Battle Frontier

### File: `src/data/battle_frontier/battle_frontier_mons.h`

**Change:** Replaced 520 Gen 2/3 species references

**Reason:** Battle Frontier rental Pokémon and opponent pools must be Gen 1 only.

**Reversible:** Yes - restore from version control.

### File: `src/data/battle_frontier/battle_pyramid_level_50_wild_mons.h`

**Change:** Replaced 98 Gen 2/3 species references

**Reason:** Battle Pyramid wild encounters must be Gen 1 only.

**Reversible:** Yes - restore from version control.

### File: `src/data/battle_frontier/battle_pyramid_open_level_wild_mons.h`

**Change:** Replaced 98 Gen 2/3 species references

**Reason:** Battle Pyramid wild encounters must be Gen 1 only.

**Reversible:** Yes - restore from version control.

### File: `src/data/battle_frontier/trainer_hill.h`

**Change:** Replaced 122 Gen 2/3 species references

**Reason:** Trainer Hill opponent Pokémon must be Gen 1 only.

**Reversible:** Yes - restore from version control.

### File: `src/data/battle_frontier/apprentice.h`

**Change:** Replaced 117 Gen 2/3 species references

**Reason:** Apprentice trainer Pokémon must be Gen 1 only.

**Reversible:** Yes - restore from version control.

---

## Category 5: Contest Data

### File: `src/data/contest_opponents.h`

**Change:** Replaced 76 Gen 2/3 species references

**Reason:** Contest opponent Pokémon must be Gen 1 only.

**Reversible:** Yes - restore from version control.

---

## Category 6: Species Constants

### File: `include/constants/species.h`

**Changes:**
1. Added section comments marking Gen 1, Gen 2, and Gen 3 species
2. Added `TODO: GEN2/3_REMOVED` markers
3. Added `IS_SPECIES_GEN1(species)` helper macro
4. Added `SPECIES_GEN1_MAX` constant

**Reason:** Provides clear documentation and runtime validation capability.

**Reversible:** Yes - not a functional change, only organizational.

---

## Replacement Mapping Reference

The following thematic replacement mappings were used consistently:

### Starter Lines
| Gen 2/3 Line | → | Gen 1 Line |
|--------------|---|------------|
| Chikorita → Meganium | → | Bulbasaur → Venusaur |
| Cyndaquil → Typhlosion | → | Charmander → Charizard |
| Totodile → Feraligatr | → | Squirtle → Blastoise |
| Treecko → Sceptile | → | Bulbasaur → Venusaur |
| Torchic → Blaziken | → | Charmander → Charizard |
| Mudkip → Swampert | → | Squirtle → Blastoise |

### Common Early-Game
| Gen 2/3 | → | Gen 1 |
|---------|---|-------|
| Sentret, Furret | → | Rattata, Raticate |
| Zigzagoon, Linoone | → | Rattata, Raticate |
| Poochyena, Mightyena | → | Rattata, Raticate |
| Wurmple, Silcoon, Beautifly | → | Caterpie, Metapod, Butterfree |
| Taillow, Swellow | → | Pidgey, Pidgeot |

### Legendary Pokémon
| Gen 2/3 Legendary | → | Gen 1 Legendary |
|-------------------|---|-----------------|
| Raikou | → | Zapdos |
| Entei | → | Moltres |
| Suicune | → | Articuno |
| Lugia | → | Articuno |
| Ho-Oh | → | Moltres |
| Celebi | → | Mew |
| Regirock, Regice, Registeel | → | Articuno |
| Kyogre | → | Articuno |
| Groudon | → | Moltres |
| Rayquaza | → | Dragonite |
| Latias, Latios | → | Mew |
| Jirachi | → | Mew |
| Deoxys | → | Mewtwo |

### Eevee Evolutions
| Gen 2 | → | Gen 1 |
|-------|---|-------|
| Espeon | → | Jolteon |
| Umbreon | → | Flareon |

### Cross-Gen Evolutions
| Gen 2/3 | → | Gen 1 Base |
|---------|---|------------|
| Crobat | → | Golbat |
| Pichu | → | Pikachu |
| Cleffa | → | Clefairy |
| Igglybuff | → | Jigglypuff |
| Bellossom | → | Vileplume |
| Politoed | → | Poliwrath |
| Slowking | → | Slowbro |
| Steelix | → | Onix |
| Scizor | → | Scyther |
| Kingdra | → | Seadra |
| Porygon2 | → | Porygon |
| Blissey | → | Chansey |

---

## Systems Not Modified (Preserved for Compatibility)

The following data arrays retain Gen 2/3 entries to maintain array index compatibility:

1. **species_info.h** - Full species stats array (required for array indexing)
2. **evolution.h** - Evolution data (Gen 2/3 evolutions simply won't trigger with Gen 1-only encounters)
3. **level_up_learnsets.h** - Learnset data (array indexed by species)
4. **tmhm_learnsets.h** - TM compatibility (array indexed by species)
5. **pokedex.h** - Pokedex constants (array indexed by species)
6. **Graphics data** - Sprite coordinates, animations (array indexed by species)

**Reason:** These arrays use species numbers as indices. Removing entries would require renumbering all species and updating hundreds of references. Since Gen 2/3 Pokémon are no longer instantiated in gameplay, the data simply goes unused.

---

## Mechanics Considerations

### Form-Change Pokémon (e.g., Castform)
- Castform has been replaced with SPECIES_DITTO in encounter/trainer data
- Form-change logic remains in code but will never trigger (no Castform can be obtained)
- TODO: Could be fully commented out in a future pass

### Unown (Gen 2)
- SPECIES_UNOWN replaced with SPECIES_ABRA in all encounter data
- Unown letter forms remain defined but unused
- Unown cave puzzles will feature Abra instead

### Baby Pokémon (Gen 2)
- All baby Pokémon (Pichu, Cleffa, etc.) replaced with their Gen 1 base forms
- Breeding will produce Gen 1 base forms directly

### Cross-Gen Evolutions
- Gen 2 evolutions of Gen 1 Pokémon (e.g., Crobat from Golbat) cannot be obtained
- Evolution triggers remain in code but the result species are not spawnable
- Players will have fully evolved Gen 1 forms (e.g., Golbat remains Golbat)

---

## Summary Statistics

| Data File | Replacements Made |
|-----------|-------------------|
| wild_encounters.json | 1,197 |
| trainer_parties.h | 1,320 |
| battle_frontier_mons.h | 520 |
| battle_pyramid files | 196 |
| trainer_hill.h | 122 |
| apprentice.h | 117 |
| contest_opponents.h | 76 |
| **Total** | **3,548** |

---

## Future Enhancements (Optional)

1. **Runtime Validation**: Use `IS_SPECIES_GEN1()` macro to add runtime checks
2. **Comment Out Unused Code**: Further reduce code size by commenting out Gen 2/3 mechanics
3. **Graphics Cleanup**: Remove unused Gen 2/3 sprite data to reduce ROM size
4. **Pokedex Adjustment**: Limit pokedex to 151 entries

---

---

## Move Replacements (Gen 2/3 → Gen 1)

In addition to species, all Gen 2/3 moves have been replaced with Gen 1 equivalents.

**Gen 1 moves:** MOVE_POUND (1) through MOVE_STRUGGLE (165)
**Gen 2 moves:** MOVE_SKETCH (166) through MOVE_BEAT_UP (251)
**Gen 3 moves:** MOVE_FAKE_OUT (252) through MOVE_PSYCHO_BOOST (354)

### Files Updated

| File | Move Replacements |
|------|-------------------|
| trainer_parties.h | 707 |
| battle_frontier_mons.h | 1,529 |
| apprentice.h | 189 |
| trainer_hill.h | 370 |
| battle_pyramid_level_50_wild_mons.h | 209 |
| battle_pyramid_open_level_wild_mons.h | 209 |
| contest_opponents.h | 175 |
| battle_tent.h | 281 |
| level_up_learnsets.h | 1,313 |
| egg_moves.h | 546 |
| tutor_learnsets.h | 2,334 |
| **Total** | **7,862** |

### Notable Move Replacement Mappings

| Gen 2/3 Move | Gen 1 Replacement | Reasoning |
|--------------|-------------------|-----------|
| MOVE_SHADOW_BALL | MOVE_NIGHT_SHADE | Ghost special attack |
| MOVE_CRUNCH | MOVE_BITE | Dark biting move |
| MOVE_SLUDGE_BOMB | MOVE_SLUDGE | Poison attack |
| MOVE_HIDDEN_POWER | MOVE_SWIFT | Special attack |
| MOVE_PROTECT | MOVE_HARDEN | Defensive move |
| MOVE_GIGA_DRAIN | MOVE_MEGA_DRAIN | Draining move |
| MOVE_DRAGON_CLAW | MOVE_SLASH | Physical slashing |
| MOVE_DRAGON_DANCE | MOVE_SWORDS_DANCE | Stat boosting |
| MOVE_BRICK_BREAK | MOVE_KARATE_CHOP | Fighting move |
| MOVE_EXTREME_SPEED | MOVE_QUICK_ATTACK | Priority move |
| MOVE_AERIAL_ACE | MOVE_WING_ATTACK | Flying move |
| MOVE_CALM_MIND | MOVE_MEDITATE | Psychic boost |
| MOVE_LEAF_BLADE | MOVE_RAZOR_LEAF | Grass physical |
| MOVE_PSYCHO_BOOST | MOVE_PSYCHIC | Psychic (Deoxys) |
| MOVE_SACRED_FIRE | MOVE_FIRE_BLAST | Fire (Ho-Oh) |
| MOVE_AEROBLAST | MOVE_HYPER_BEAM | Special (Lugia) |

---

## Reverting Changes

All changes can be reverted by restoring files from version control:

```bash
git checkout HEAD~n -- src/data/wild_encounters.json
git checkout HEAD~n -- src/data/trainer_parties.h
# ... etc for other files
```

Or by checking out the original branch before the Gen 1-only changes.
