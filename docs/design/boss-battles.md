# Boss Battles (Notion, pre-Arena-Ecology version)

A full translation of the Notion BOSS BATTLES page: per-gym battle gimmicks, including the move interactions and numbers that `roster-and-encounters.md` only summarises in one line each.

Source:
- Current GDD copy: [BOSS BATTLES](https://app.notion.com/p/31b3d985050a81fa9ff8e9d09130967c), under GAME DESIGN DOCUMENT / Battaglia, last edited 2026-03-17.
- Legacy copy: [BOSS BATTLES](https://app.notion.com/p/31b3d985050a817890a5ccc6a655c4fa), under GAME DESIGN FUNDATIONS (v1 — Legacy) / Battaglia, last edited 2026-03-06.

Read 2026-10-15.

**Are the two copies identical?** Yes in text, layout, gimmick names and leader images (same files, `001e.png` to `008e.png`). The only differences:

- In the legacy copy, the four move links in the JIRO and BROCK sections point to old pages that no longer resolve (404).
- Each copy embeds its own inline team database.

Treat the current copy as canonical.

**Status vs v2.1.** This page predates `arena_ecology_v2_1`, which redesigns every gym (see `roster-and-encounters.md` and `open-questions.md` B, "Boss battle designs"). Keep it as the earlier concept and a source of move-interaction ideas, not as the spec. Italian text is translated; inline move links are resolved to the move names and stats in the Notion Moves database (Power / Accuracy / Category / Type).

**Team rosters.** Under every leader there is an inline, locked, untitled database (probably the leader's team, inferred). The API returns **no schema and no rows** for them. Either they are empty or their content is not readable through the connector, so no boss teams or levels are documented here.

---

## JIRO: "LUCKY STRIKE" (image `001e.png`)

> Whoever attacks first has an increased chance of landing a critical hit. Farfetch'd's **LEEK SLAP** attack ALWAYS lands a critical hit. Flying-type Pokémon get double Speed as long as the air vents keep running.

- Use **STRING SHOT** to block the vents.
- Use **SAND ATTACK** to raise a sandstorm. The sandstorm deals **1/16 of HP per turn** to every Pokémon that doesn't live in a mountain environment, and it lowers accuracy.

Moves referenced: LEEK SLAP (SIGNATURE, NORMAL, Power 60, Acc 100, ★, effect HIGH CRIT CHANCE); STRING SHOT (SIGNATURE, BUG, Acc 100, ★, SPEED-); SAND ATTACK (SIGNATURE, GROUND, Acc 100, ★, ACCURACY-).

## BROCK: "GRUNGE ROCK" (image `002e.png`)

> The rocks on the battlefield make movement hard, so they lower the Speed of all Pokémon except mountain-dwellers and Pokémon that can fly. The Defense of the latter also doubles, because they know how to exploit the rocky ground. Because of the high humidity, Fire-type moves have reduced power.

- **WATER GUN** wets the opponent's side of the field, lowering its evasion.
- **GUST** raises a light sandstorm that hits every Pokémon not living in a mountain habitat for **3 turns**.

Moves referenced: WATER GUN (ELEMENTAL, WATER, Power 40, Acc 100, ★); GUST (ELEMENTAL, FLYING, Power 40, Acc 100, ★).

Note: Brock's gym is Rock-themed here ("Grunge Rock"). In v2.1 it is Ground ("FAULT LINE").

## MISTY: "LITTLE MERMAID" (image `003e.png`)

> The tide changes direction based on the last Water-type move used, and grants **double Speed** and a **50% boost to Water moves**. The match takes place on platforms in the middle of the sea; Pokémon that can't move in water get an accuracy penalty.

- Every Electric attack, such as **THUNDER JOLT** or **ELECTRIC SHOCK**, always hits, but its power is reduced as if it were used as a spread (area) move.
- **EMBER** and **FLAMETHROWER** generate a cloud with the properties of **MIST**.

Moves referenced: THUNDER JOLT (SIGNATURE, ELECTRIC, Power 50, Acc 100, ★, PARALYSIS); ELECTRIC SHOCK (ELEMENTAL, ELECTRIC, Power 40, Acc 100, ★, PARALYSIS); EMBER (ELEMENTAL, FIRE, Power 40, Acc 100, ★, BURN); FLAMETHROWER (MOVE MACHINE, FIRE, Power 95, Acc 100, ★, BURN); MIST (SIGNATURE, ICE, Acc 100, ★, effects EVASION+, SHIELD, TP REGEN).

## LT. SURGE: "GENERAL ELECTRIC" (image `004e.png`)

> Next to each player there is a coil with **3 charges**. Every **non-lethal** Electric move charges the coil. On the third charge the coil explodes, damaging the Pokémon on the corresponding side. When the coil explodes it **paralyses every non-Electric Pokémon**, and **paralysed enemies take more damage**.

No numbers are given for the explosion damage or the paralysis damage bonus. v2.1 replaces the per-side coils with one shared TESLA COIL and sets the bonus at +30%.

## ERIKA: "KODAMA" (image `005e.png`)

> A sacred tree stands between the player and the opponent. It must be **watered every 3 turns with a Water move**; otherwise it **drains 1/8 of HP from every non-Grass Pokémon**. **Fire moves are forbidden.** The tree gives **Grass Pokémon** a barrier that **reduces damage by 50%**.
>
> Using an [ICE] move sends the tree dormant for **2 turns**: the barriers are removed, and it drains **1/4** instead of 1/8 while the frost lasts.

Erika is Grass-themed here. v2.1 makes her Ghost-type with SPIRIT PRESSURE but keeps the name "KODAMA".

## KOGA: "PURPLE HAZE" (image `006e.png`)

> The surrounding area is filled with air full of poisonous toxins: **every non-Poison Pokémon faints after 3 turns**. There are **poison spikes on the ground** that poison Pokémon as they enter the field.

## COLEMAN: "FIRE IN THE HOLE" (image `007e.png`)

This section is written in English, unlike the Italian sections, and is longer and more numeric than them. `open-questions.md` B already flags it as probably an AI-generated draft. Verbatim content:

> The battle takes place in a volcanic cave filled with magma and lava. The intense heat affects the power of Fire moves and the speed of all Pokémon except Fire-type Pokémon. The cave is also filled with smoke that makes it difficult to see, decreasing the accuracy of all moves. The smoke can be cleared by using Water-type moves or moves that create wind.

**Bonus/Malus**
- The intense heat increases the power of Fire moves by 50% and the speed of Fire-type Pokémon by 25%, but decreases the speed of all other Pokémon by 25%.
- The smoke decreases the accuracy of all moves by 25%, but Fire-type moves are not affected.
- The magma and lava on the field can damage non-Fire-type Pokémon for 1/8 of their max HP each turn they spend on the ground.
- Using Fire Punch creates a small explosion that damages all non-Fire-type Pokémon and clears the smoke for 3 turns.

**Move Interactions**
- Flamethrower can be used to melt the magma and lava, creating a pool of hot water that heals Fire-type Pokémon for 1/16 of their max HP for each turn they spend in the water.
- Fire Spin can be used to trap non-Fire-type Pokémon in a ring of fire, preventing them from switching out and damaging them for 1/16 of their max HP each turn they spend in the fire.
- Ember can be used to create a small fire that spreads through the smoke, damaging all non-Fire-type Pokémon and decreasing their accuracy for 2 turns.
- Water Gun can be used to create a cloud of steam that clears the smoke for 3 turns and increases the accuracy of Water-type moves by 25%.

This version does not mention a Double Battle; v2.1 makes Coleman's gym a Double Battle.

## SABRINA: "ESPER" (image `008e.png`)

> Use the AI as clairvoyance, to know in advance which moves the player will use and act accordingly?

This is a question only, not a design. v2.1 excludes Sabrina's gym "pending character confirmation".

---

## What this page adds over `roster-and-encounters.md`

- The specific counter-moves for each gym: STRING SHOT/SAND ATTACK (Jiro), WATER GUN/GUST (Brock), Electric always-hit spread and EMBER/FLAMETHROWER → MIST cloud (Misty), and ICE → dormant tree (Erika).
- The move data those counters reference. Most are ★ rank, Acc 100, and several are categorised SIGNATURE in Notion (e.g. STRING SHOT, SAND ATTACK, MIST), which is odd for widely shared moves. Worth flagging when the Moves DB is cleaned up (inferred).
- Numbers: Jiro sandstorm 1/16; Brock sandstorm 3 turns; Misty x2 Speed and +50% Water; Erika 3-turn watering, 1/8 drain, 50% barrier, 2-turn frost with 1/4 drain; Koga 3-turn KO; Coleman's full numeric list.
- Confirmation that no boss team rosters are readable in Notion.
