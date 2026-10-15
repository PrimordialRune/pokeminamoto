# Design Principles and How Omar Works

The rules every design or code proposal for Minamoto is judged against, and how Omar wants collaboration to run. Read this before proposing any design change.

Source: notes carried over from the original claude.ai project (imported memory, 2026-10-11), cross-checked against `sources/city-design-framework-v2.1.md` and `sources/arena-ecology-v2.1.txt`.

## Creative goals

- A deliberate **reimagining, not a replacement**, of Pokémon Red/Green: understand the canonical games first, then build on them. "Less faithful to the letter of the originals, more faithful to their underlying inspiration" (city framework §1).
- Setting: 1980s-90s Japan with a post-war industrial narrative.
- **Moral ambiguity** over clean heroism.
- Cities and battles that feel **alive and systemic**, not static.
- **Physical / scientific plausibility is the primary design authority.** Taxonomies and arena rules must survive physical scrutiny first; flavour comes second.
- **Dual audience**: reward players who explore; engineer a gradual "ludic shock" for players who ignore the world.

## Design principles

- **Arenas are state machines**: at most two state variables and four states per arena. Complexity comes from move-tag interactions, not per-move rules. Arena rules are written as tag-conditional logic, never as references to individual moves.
- **Type advantage is preserved, not replaced.** The arena adds new liabilities for bringing the "correct" counter; it never overrides the type chart.
- **Each gym teaches one discrete mechanic** (turn order, speed control, momentum, risk management, resource management, attrition, spread attacks).
- **Gym Leaders use the environment to cover their type weakness.** Canonical model: Lt. Surge's metal container disables Ground moves.
- **City identity uses three layers**: original games, then beta/leak material, then real-world city research. Document what was preserved and what changed.
- **Not every NPC needs narrative weight.** Overloading everything with significance undermines the design (city framework ratio: 7 pure flavour / 2 informative / at most 1 narrative per 10 interactables).
- **Charisma is a soft difficulty scaler**: it rewards engagement with the world without hard-gating.
- **Prototype archaeology needs strict sourcing**: never conflate human-readable translated names with raw internal labels from source code; prefer a false negative to a false positive when unsure.
- **Frameworks enable design, they don't cage it.** Overly prescriptive tier structures get rejected ("Ume/Take/Matsu is the language, not the cage").
- Early design docs describe NPCs by **conversation themes**, not final dialogue.

## How Omar wants to work

- He asks for **harsh critique** and expects it throughout. Corrections should be immediate and precise.
- Proposals need **physical or evidential grounding**; ungrounded ones get pushed back (example: a proposed SUPPRESSION tag was dropped after he asked what grounded it).
- Decisions are argued in terms of **plausibility and systemic coherence**, not flavour alone.
- Work moves through **versioned documents** (.docx, Excel workbooks) with numbered revisions. Keep version numbers in filenames and headers.
- He prefers **co-design as a professional collaboration with targeted questions**, not passive document generation.
- Research briefs are self-contained and rigorous: they state the source hierarchy, normalisation rules and output format up front.
- Repo rules (from project memory): ask before committing, pushing, creating branches or deleting; flag anything that adds RAM or save data (EWRAM ~95%, IWRAM ~93-94%); Sonnet is the default subagent model.

## Research sources in use

Helix Chamber, TCRF, Bulbapedia, pret/pokered disassembly, hakuda2 / Legendary Star Blob 2, Glitch City Laboratories. Primary materials: the Spaceworld 1997 demo ROM, the 2020 Game Freak source leak, and a confirmed move CSV of 261 moves (not in the repo; see `open-questions.md` §D).
