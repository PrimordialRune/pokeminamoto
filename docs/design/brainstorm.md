# Brainstorm and Ideas (Notion)

Every note on Omar's Notion "BRAINSTORM & IDEAS" page, translated from Italian and grouped by topic, each with a one-line status judgement.

Source: [💡 BRAINSTORM & IDEAS](https://app.notion.com/p/31b3d985050a8139901ce8e12d981039), last edited 2026-03-06. Read 2026-10-15.

**How to read this file.** The page is a flat list of short callouts. Most notes are not about this game. They are about the 1996 Red/Green prototype and what was cut from it, which Minamoto wants to restore. The topic headings here are mine; the Notion page has none. Status means:

- **idea**: one line, no rules or numbers.
- **partly specified**: the intent is clear, but the numbers, rules or scope are missing.
- **specified**: written up with rules elsewhere (usually the GDD; see `mechanics.md`).

Names are kept as spelled in the source (Sylph, GEOvanni, Kotora, Hitodeman...). Anything I inferred is marked "(inferred)".

---

## 1. Battle and capture mechanics

| Idea (translated) | Status |
|---|---|
| **Charisma** stat for the trainer (the player): it lets you catch certain Pokémon, and it can trigger statuses in other trainers. | **specified** in the GDD (Charisma + Trainer States); see `mechanics.md` §5-6. The "lets you catch certain Pokémon" part is still idea-level. |
| **Illusory Monsters**: the equivalent of legendaries. They can choose NOT to join you; you might miss your chance to see them and never be able to catch them. | **partly specified**: the GDD ties them to Charisma; who they are and how they refuse is not defined (open-questions A.12). |
| **Talk** option to speak to trainers during battles. | **specified** (GDD: R button, does not end the turn); see `mechanics.md` §7 and `ui.md`. |
| **Technical Points**: "the ancestor of PP"; they let monsters use elemental attacks or Special attacks. | **specified** (GDD TP formula); see `mechanics.md` §2. The brainstorm framing (TP only for elemental/special moves) differs from the GDD, where every move costs TP. |
| Pokémon had a **gold value** and an **intelligence parameter**. | **specified** as COST (P$) and INT in the GDD stat table; the COST formula is still undefined. |
| **DRAIN for TP** on some moves. Bug type? | **idea**. Probably TP-draining moves, perhaps a Bug-type niche (inferred). Not defined anywhere. |
| Reference to **stackable Toxic** statuses, as in the Gen-1 bug. Also, as a loose line: "Stack/combo degli status" (status stacking/combos). | **idea**. |
| **Break bars?** | **idea**. Paired with the next row. |
| **Fights against legendaries → double boss battle with break bars.** | **idea**: no break-bar rules. Emerald has the double-battle engine (inferred). |
| Bind **L** to a quick switch, PLA-style (*Pokémon Legends: Arceus*). | **idea**; see `ui.md`. |
| **Catch chain**: catch rate rises with each ball thrown, but the chain breaks as soon as you use a move. If the Pokémon uses certain kinds of move, it can lower the chain's catch-rate bonus. | **partly specified**: the rule is clear, but there are no numbers. Needs one battle-time counter (inferred). |
| **Evolution through quests.** | **idea**. The Story doc's Silph research quests are related (inferred). |
| Can wild Pokémon **flee after some time / HP**, Safari Zone style? | **idea** (phrased as a question). |
| **Nidoran nests**: encounter rate reacts to your party lead. E.g. lead with Nidoran F and males are more likely to be attracted. | **partly specified**: the rule is clear, rates missing. Emerald already has lead-based encounter hooks (e.g. abilities); a species-based bias would be new code (inferred). |
| **Terrain hazards** that hit Ground-types especially hard, like quicksand or mud pits that slow or immobilise Pokémon caught in them. The player must navigate the field and use multi-target or long-reach attacks. (Written in English, gray "idea" callout.) | **idea**. It overlaps the later Arena Ecology terrain states (Brock's TERRAIN/MOISTURE); see `roster-and-encounters.md`. |

## 2. Economy and shops

| Idea | Status |
|---|---|
| **(IDEA ORIGINAL)** An economy where you do **not** earn money from every defeated trainer, only from themed ones (e.g. the Gambler who bets money on the win, the bully who bets his pocket money). Money also comes from the Colosseum and from selling items. Possibly an allowance from your mother when you go back to visit her after a while. | **partly specified**: the GDD confirms "money only from specific trainers". The Colosseum, the allowance and the amounts are undefined. |
| **Items on sale**; differentiate the sales in each Market? | **idea**. The Green-run notes saw SALE signs in the original Market (see `research-and-sources.md`). The city framework says Market prices react to events (`region-and-maps.md`). |

## 3. World, exploration and quests

| Idea | Status |
|---|---|
| **Pokémon cries** can be heard while exploring towns and routes. | **idea**. Ambient cries are cheap on Emerald: map scripts or a field effect can play `PlayCry` (inferred). |
| **Ninetales subquest**: a special interaction if you have a Growlithe in the party; Ninetales takes human form and tries to trick you. | **partly specified**: premise only. Ninetales is a Liminal species (`roster-and-encounters.md`). |
| **Pokémon in the wilderness?** (visible overworld Pokémon). The idea came from the old Pokémon GDD images (two images on the page, both named `Untitled.png`). | **idea**. |
| Change the Kanto route so the map's shape stops the player surfing from Fuchsia, and makes them **backtrack through Vermilion** instead. [Proposed after seeing the 1996 map.] | **idea**. Region layout is in `region-and-maps.md`. |
| **Izu Islands** were Tajiri's first inspiration for Cinnabar Island, possibly extended to the Sevii Islands; small islets appear on the prototype maps. Idea: include the Sevii/Orange Islands as the progression route to Cinnabar. | **idea**. |
| The **Pewter → Cerulean progression is flawed because of HMs**: should Flash have been handed out earlier? Parallel with squid ink. Idea to tie Flash to Butterfree or Pikachu, since they are among the first Pokémon in the game. | **idea**. |
| **Zener cards** as a test/experiment on the player to see whether they can face Sabrina: the classic gym riddle. | **idea**. |
| **Danpei** could be the man who originally shows you the Pewter gym and from then on guides you through the gym leaders' gimmicks. | **idea** (character role). |

## 4. Gym leaders, Elite Four and ranks

| Idea | Status |
|---|---|
| **Erika or Sabrina?** Psychics are often shown with floating balls and Erika has the same pose; also the city's background is fuchsia at first, a colour linked to the Psychic type. Still to be looked into. | **idea** (open lore question). Related to the Erika/Ghost vs Grass contradiction in `open-questions.md` B. |
| **Sabrina and the Mind badge**: it resembles the badges of Yuujirou, Shigeru, Umezou and Satoshi. Probably a trainer of the same rank or with the same purpose? Unresolved. | **idea**. |
| **Sabrina defeats the dojo** (formerly the Fighting gym) and gives you her own badge when you beat her, because you are the first to have beaten her. | **partly specified** (story beat). |
| **Bruno** as possible head of the Fighting dojo? | **idea**. |
| **Ranks from Egg to Master**, probably used by an academy to judge a trainer's rank? Probably linked to the badges Sabrina and the protagonists wear. | **idea**. The city framework has "PMC rank progression" (`region-and-maps.md`); a link is (inferred). |
| Gym leaders were originally called **"Judge"**, probably because they judged your skills and maybe raised your rank. | **idea** (lore). |
| Badges were originally meant to be **belts**? Trainers would use these belts to whip their Pokémon, starting from a white belt and reaching a black belt (Saffron gym). | **idea** (lore). |
| The **Elite Four**, called 四天王 (Shitennō), could reference the Four Heavenly Kings, since one is said to be linked to rain and another has to do with dragons. | **idea** (lore). "Four Heavenly Kings" is the GDD's name for the Elite Four. |
| **Elite Four as possible villains**, as in the manga? Their status as simple elite trainers needs revisiting. | **idea**. |
| **Katsura's (Coleman's) gym** was originally a minefield with holes in the ground and damaged flooring. | **idea** (layout). Note the source equates Katsura with Coleman, which bears on the Katsura vs Coleman contradiction in `open-questions.md` B. |

## 5. Factions and trainer classes (lore)

| Idea | Status |
|---|---|
| **ROCKETS** are modelled on **Bōsōzoku**, Japanese biker gangs associated with the Yakuza, and take their clothing from them. (Bookmark: "Bōsōzoku - Wikipedia".) | **partly specified** (art/lore direction). |
| **ROCKETS vs Sylph**: the Rockets are a criminal organisation led by **GEOvanni**, devoted to searching for fossil fuel that they sell around to scrape money together. | **partly specified**. The Story doc has the full faction setup (`story-and-setting.md`). |
| Possibly a gang war between **Black Emperor** and **Skinhead** gangs originally. | **idea**. |
| The **Biker/Bōsōzoku** is called **Emperor** in the beta, after Black Emperor, the most famous Bōsōzoku group in Japan, which operated nationally. | lore; already reflected in the trainer-class list (`roster-and-encounters.md`). |
| **Youngster** and **Lass** were called **Shorts Boy** and **Miniskirt Girl** after their clothes. Shorts and miniskirts can only be worn as kids; once grown up they must face the "adult" world, where society dictates how you dress. | lore; class names already in the trainer list. |
| The **Old Electrician** is called **National** in the beta, after a Panasonic sub-brand that distributed home appliances. | lore. |
| **Shinjuku Jack** might reference the Shinjuku Robot Restaurant ("[to verify the dates match]"). More likely a reference to Virtua Fighter's Jacky, specifically the famous VF player Shinjuku Jacky, co-author of the *Virtua Fighter Maniax* guidebook. He could have had **Porygon** because of its shape, probably a robot created by the Sylph. Shinjuku probably refers to the technology district (to verify; Cydonia points to Akihabara). | **idea** (a possible NPC/trainer). |
| **Rocker / Denki Group** references Denki Groove, a Japanese band from '89. | lore; "ELECTRIC GROOVE" class exists. |
| **Gypsy** is the Juggler's original name: foreigners bringing their dances and folk entertainment. | lore; class exists. |
| **Erika is very probably an Itako** (a blind Japanese spirit medium); she was originally called **Shizuka**. (Bookmark: "Itako - Wikipedia".) | **partly specified**. It fits the arena doc's Ghost-type Erika ("KODAMA"). |
| **Red had a father** and was a non-challengeable character. | **idea** (lore). |

## 6. Species and region lore (prototype research)

| Idea | Status |
|---|---|
| **Golem was NOT Graveler's evolution**: their index numbers are too close, from a period when direct evolutions weren't being planned. | lore. Compare the Nishinomori quote about Machamp and Golem not evolving by growth (`research-and-sources.md`). |
| **Starter attempts**: Goldeen, Kotora and Ponyta first; Pikachu, Charmander and Squirtle second; three-stage Grass lines were created to act as starters, then dropped in favour of Bulbasaur. | lore. The repo starters are Bulbasaur/Charmander/Squirtle (`src/starter_choose.c`). |
| **Dragonite** very likely separate from Dratini and Dragonair; those two come from a period of designing two-stage lines tied to dragons/sea creatures. | lore. It affects whether the Dratini line is one line or two in the PKMN INDEX (inferred). |
| **Hitodeman → Hitodama** (Japanese will-o'-the-wisp). Possible link to souls of the dead reincarnating as starfish? | **idea**. |
| **C Town?** C could stand for CAVE, and T for TOWER; a personal idea is T = TOKYO, while C still needs an interpretation. The truck warp in Vermilion (Aranciopoli) links to an unknown ghost town: possibly town C and a Team Rocket hideout? Another take: T = Tokyo and C = **Chiyoda**, the Tokyo district of the Imperial castle, isolated and surrounded by water; maybe some vehicle was needed to move around inside it? Maybe Sabrina was originally linked to some imperial family? Image: a map showing the towns against their real Japanese counterparts (`Untitled.png`). | **idea**. |
| **Mt. Akagi = Mt. Moon.** | lore (real-place mapping). |

---

## Notes for implementers

- The most engine-ready ideas: Pokémon cries while exploring, the money-only-from-themed-trainers economy, and per-Market sale items. All three fit existing pokeemerald systems: map scripts, `trainers.h` money, and Mart item lists (inferred).
- The catch chain and the Nidoran-nest lead bias are short, clear rules but need numbers from Omar.
- The economy row conflicts with Emerald's default prize money: vanilla pays every trainer. Decide this before trainer data work (inferred).
