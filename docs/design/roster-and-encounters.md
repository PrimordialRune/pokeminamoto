# Roster, Gyms, Trainers and Encounters

Sources: Notion PKMN INDEX / TRAINERS / BOSS BATTLES / Moves, arena_ecology_v2_1, GDD v1.

## The roster (Notion PKMN INDEX, "Index" data source)

- Goal in GDD: a full Pocket Monster index; the project is a Gen-1 (151) redesign. Species are named with Minamoto/Japanese-style names (property "Pocket Monster") and a "Localized Name" (the familiar name).
- Notion rows found: **180** (more than 151; likely includes pre-release/beta-only species, alternate lines, and placeholders such as `[NYAKO]`, `[TORA1-3]`). Not reconciled with 151 (see open-questions).
- Columns: Index (number), Pocket Monster (title), Localized Name, Type (multi-select, 15 types incl. NORMAL..DRAGON), Habitat (select), Release (ALPHA / ALPHA v2 / BETA), Wild Nature (checkbox), Encounters (relation to MAPS), Evolves Into (relation), Signature Move (relation to Moves), and an unnamed formula column.
- Example rows (Index, name, localized, type, habitat, release):
  - 1 FLAMIE (Growlithe), FIRE, GRASSLANDS, BETA
  - 5 NYARTH (Meowth), NORMAL, GRASSLANDS, BETA; 4 `[NYAKO]` (Miako), 6 PERSIAN
  - 9-11 `[TORA1]`, `[TORA2]`, `[TORA3]` (Kotora, Raitora, unnamed), NORMAL/ELECTRIC, GRASSLANDS, BETA (non-vanilla beta line)
  - 88 TRANSEL (Metapod), BUG, FOREST, ALPHA, has a signature move
  - 99 RATTA (Raticate), NORMAL, FOREST, ALPHA; 108 GOLDY (Goldeen), WATER, SEA, BETA
- Release tiers (row counts): ALPHA 30, ALPHA v2 10, BETA 140. Interpreted as content shipped per development milestone (inferred).
- Habitat counts (all releases): Grasslands 51, Sea 25, Water's Edge 19, Mountain 16, Forest 19, Skies 10, Urban 11, Mysterious 12, Mirage 12, Fossil 5.
- Every sampled Pokemon has at least one **Signature Move** relation. Moves DB has 150 SIGNATURE entries.

## Taxonomy (arena_ecology_v2_1)

**Habitat (mutually exclusive, 10):** Grasslands, Water's Edge, Mountain, Forest, Sea, Skies, Urban, Mysterious, Fossil, Mirage. (Fossil = revived extinct species: Omanyte/Omastar, Kabuto/Kabutops, Aerodactyl. Mirage = legendary/semi-mythical: Articuno, Zapdos, Moltres, Mew, Mewtwo.)

**Characteristic (overlapping, 7):** Aerial, Swimmer, Terrain-dweller, Ground-bound (default), Subterranean, Liminal, Volatile (battle state, not permanent).

**Liminal species (design-authoritative):** Gastly/Haunter/Gengar (Spirit/Spectre/Phantom), Cubone/Marowak (Orphon/Garagara), Vulpix/Ninetales (Foxfire/Mikon), Jynx (Rougela), Drowzee/Hypno (Sleeper/Sleepe), Omanyte/Omastar/Kabuto/Kabutops/Aerodactyl. Ghost type != automatically Liminal.

The doc also gives Minamoto names for many species (e.g. Kamonegi = Farfetch'd, Pigeot = Pidgeot, Iwark = Onix, Hitodeman = Staryu, Koiking = Magikarp, Kabigon = Snorlax).

## Move categories used by arenas (22-category ecology taxonomy)

WIND, GROUND-IMPACT, EXCAVATING, SONIC, SUPPRESSION, TRAP, CONDUCTIVE, THERMAL-HOT, THERMAL-COLD, AQUATIC, SOLAR, BLAST, TOXIC, POWDER/SPORE, DRAIN, SPECTRAL, PSYCHIC-WAVE, MENTAL, CONTACT (subcategorised by body part: BODY, ARMS, FISTS, KICKS, TEETH, CLAWS, HORNS, TAIL), PROJECTILE, DISPLACEMENT, SUPPORT. Moves may belong to several (e.g. Fire Punch = THERMAL-HOT + CONTACT). Arena rules reference categories, never individual move names. Full category table (mechanism, representative moves, arena relevance) is in the source.

## Gym / arena ecology (v2.1)

### Global rules
- **Gym Leader Focus Sash**: every Pokemon of a Gym Leader (also Four Heavenly Kings and Boss encounters) survives one lethal hit at 1 HP, once per Pokemon per battle.
- **Type advantage mitigation**: gym leaders are monotype specialists; the arena is a "third combatant" that makes the correct type effective but costly.
- **Modified crit stages**: +0 6.25%, +1 12.5%, +2 25%, +3 50%, +4 100% (see `mechanics.md`).

### Gyms (progression summary table from the source)

| Gym | Leader | Type | Theme | Core state variables |
|---|---|---|---|---|
| 1 | Jiro ("LUCKY STRIKE") | Flying | Turn order and crits | AIRFLOW STILL/ACTIVE/FULL; SANDSTORM and HUMIDITY overlays |
| 2 | Brock ("FAULT LINE") | Ground | Terrain restraint | TERRAIN INTACT/CRACKED/FRACTURED; MOISTURE DRY/MUDDY; SANDSTORM overlay; field constant -1 SPE for non-Terrain-dwellers |
| 3 | Misty ("LITTLE MERMAID") | Water | Momentum | TIDE NEUTRAL/PLAYER/OPPONENT; CONDITION CLEAR/MIST/FROZEN; CONDUCTIVE BUOYS |
| 4 | Surge ("GENERAL ELECTRIC") | Electric | Risk management | TESLA COIL ACTIVE 0-2 / DISCHARGING / SHORTED / DESTROYED; metal container (Dig and Sand Attack disabled) |
| 5 | Erika ("KODAMA") | Ghost | TP resource exhaustion | SPIRIT PRESSURE 0-4; Liminal Pokemon exempt |
| 6 | Koga ("PURPLE HAZE") | Poison | Attrition and rotation | MIASMA DENSITY 0-3; all non-Poison poisoned on entry |
| 7 | Coleman ("FIRE IN THE HOLE") | Fire, DOUBLE BATTLE | Spread and multi-target management | COMBUSTION 0-4; STEAM overlay; entry burn 1/16 |

Each gym section in the source has: environment model, transition rules, interaction matrix by move category, archetype interactions, strategic layer, type-interaction reframing table, and an AI utilization model. Key signature hooks:
- Jiro: Farfetch'd + Leek (+2 crit stages) + FULL AIRFLOW first-mover (+2) = +4 = guaranteed crit. Mountain-archetype Pokemon (Geodude line) bypass the arena. Pidgeot's Hurricane is the strongest AIRFLOW pusher.
- Brock: terrain only deteriorates; BLAST skips to FRACTURED; WIND dries MUDDY (and may trigger SANDSTORM); Terrain-dwellers ignore penalties.
- Misty: gain TIDE, use THERMAL-COLD to FREEZE, attack at +50%; Misty counters with THERMAL-HOT -> MIST. Whirlpool shifts TIDE and traps.
- Surge: CHARGE builds with non-lethal Conductive moves; at 3 the coil DISCHARGES (fixed damage to non-Electric, non-Volatile on both sides, 2-turn paralysis, +30% damage on paralysed). AQUATIC shorts it for 2 turns; 3 THERMAL-HOT use DESTROYS it; Earthquake gives RESONANCE (-1 CHARGE).
- Erika: SPIRIT PRESSURE raises TP cost (x1.0 to x2.5) and lowers TP recovery (x1.0 to x0.25) for non-Liminal Pokemon; HAZE -2 pressure.
- Koga: per-habitat-layer poison rates (Aerial 1/32-1/16, Mountain 1/16-1/8, Floor 1/16-1/4 per turn); WIND -2 density.
- Coleman: Double Battle; Helping Hand + Thermal-Hot auto-spread; EXPLOSION endgame when a Pokemon falls below 30% HP.

Sabrina's gym is "excluded pending character confirmation" in v2.1. Notion also lists NATSUME (Sabrina) as a trainer and a "SABRINA / ESPER" boss note.

### Older (Notion BOSS BATTLES) versions of the same gyms
The Notion page predates v2.1 and describes simpler concepts that conflict with v2.1 (see open-questions): Jiro Lucky Strike (Farfetch'd move ALWAYS crits; Flying types double speed while vents run), Brock "Grunge Rock" (rocks lower speed, double DEF for mountain/flying), Misty Little Mermaid (tide doubles speed and +50% water), Surge General Electric (a coil per side with 3 charges), Erika Kodama (a sacred tree needing watering every 3 turns, no fire), Koga Purple Haze (non-Poison KO after 3 turns, poison thorns), Coleman Fire in the Hole (volcanic cave, Fire +50%, smoke -25% accuracy), Sabrina "ESPER" (question: use AI as clairvoyance to anticipate player moves?).

## Trainers and bosses

- Four Heavenly Kings and Gym Leaders are the strongest trainers; Boss Battles have special conditions.
- Pokemon Index lists "Pokemon Illusori" (illusory Pokemon) as a high-end challenge gated by Charisma.
- GDD says trainers have themed teams (e.g. fire, swimmers) and that money can be earned from specific trainers only.

### Notion TRAINERS database (45 entries)
Schema: Name (Minamoto name), Localized Name (vanilla class), Release (ALPHA, BETA 1, BETA 2). Entries are class/name stubs with no body content.
- Named characters: KEN (Jr. Rival), SHIGERU (Rival), OKIDO (Professor), NATSUME (Sabrina), SHIZUKA (Erika), TAKESHI (Brock), KASUMI (Misty), YUJIROU (Jiro), PHILIP SURGE (Lt. Surge), OSUGI (Koga), KATSURA, KIKUKO (Agatha), KANNA (Lorelei), SIBA (Bruno), WATARU (Lance), SHINJUKU JACK.
- Generic classes (Minamoto name / vanilla): BEAST TAMER (Tamer), STRANGER (Gentleman), BLACK EMPEROR (Biker), SCIENCE MAN (Super Nerd), MONSTER MANIAC (PokeManiac), ROCKET MEMBER, ROCKET BOSS, KARATE KING (Blackbelt), SKINHEAD (Cue Ball), PSYCHIC, PRAYER (Channeler), BUG BOY (Bug Catcher), ELITE GIRL/BOY (Cooltrainer), FIRE THIEF (Thief), BIRD TAMER (Bird Keeper), GYPSY/JUGGLER, ELECTRIC GROOVE (Rocker), SHORTS BOY (Youngster), SYLPH CHIEF, SYLPH WORKER (Scientist), ADULT LADY (Beauty), FIRE MAN (Firefighter), SWIMMER, SAILOR, MOUNTAIN MAN (Hiker), GIRL/BOY SCOUT (Jr. Trainer), FISHERMAN, MINISKIRT (Lass), GAMBLER.
- Note "SYLPH CHIEF/WORKER" spelled Sylph in Notion vs "Silph" elsewhere.
- Release tag ALPHA only on KEN, SHIGERU, BUG BOY, YUJIROU, SHORTS BOY, MINISKIRT (the Gym 1 / early-game set).
- Gym leader names in Notion (Takeshi = Brock, Kasumi = Misty, Shizuka = Erika, Osugi = Koga, Natsume = Sabrina) vs arena doc (Brock, Misty...) vs Story doc (Katsura at Cinnabar). Mixed naming; see open-questions.

### Encounter design
- Encounters are stored as relations: each map lists Encounters (land), Water Encounters, and Gifts/Trades/Events, each pointing at PKMN INDEX rows. Each Pokemon page links back to its Encounters.
- Wild Pokemon: "Wild Nature" checkbox in the index (meaning not documented, see open-questions).
- Research-quest requests (Story doc) ask for trade-evolution-line Pokemon only (Graveler, Machoke, Kadabra...).
- Mew: catchable in Cerulean Cave post-League.
- Charisma gates the appearance/catching of illusory Pokemon.
- Safari Zone has normally inaccessible deeper areas that open post-League (Story Act Six).
