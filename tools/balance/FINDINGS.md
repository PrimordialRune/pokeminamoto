# Early-game balance: first findings (2026-10-22)

**Update, same day:** Omar restored the flat +2 (PR #9) and set the TP direction: TP is a resource
that runs out across battles and only comes back with rest, with a PP-like progression. The model's
defaults now have the +2 on and no badge-3 speed boost. Sections 1-6 were run before that, without
the +2; section 7 is the new TP direction.

Model: `tools/balance` (see README.md). Rules = develop + PR #9: TP 100, regen 5 + INT/8 with INT =
base Sp. Def, placeholder `tpCost` (status 10, hits power²/320 rounded to 5), crit stages
16/8/4/2/1, no flat +2, type badges x1.25, Gym Leader Focus Sash (rule 0.1, not in the ROM yet).
Species, moves and learnsets are the ROM's (Emerald data on Gen 1 species). Trainer teams in
`scenarios/early_game.yaml` are placeholders from Green/RB except Brock and Misty, which follow
Omar's Green notes. Numbers come from 300-2000 simulated battles or playthroughs each.

## 1. TP doesn't matter yet in the early game

- Regen is 9-15 TP a turn for early species (Pidgey 9, Charmander 11, Squirtle/Bulbasaur 13,
  Starmie 15). Every early damaging move costs 5 (Tackle, Ember, Water Gun, Vine Whip, Gust, Peck)
  and status moves cost 10. A cost at or below regen can be used forever.
- In every simulated battle from Masara to Misty, no Pokémon on either side was ever unable to
  afford its preferred move. The lowest TP reached in a gym fight averages 90-95.
- TP starts to bite only around "status 15, hits power²/80, minimum 10" (Tackle 15, Ember 20,
  Rock Throw 30, Razor Leaf 40). With that, Bulbasaur L21 vs Misty runs out of TP in 88% of fights
  and Struggles in 40%. Its win rate goes **up** (31% to 52%) because Starmie runs dry too.
  So TP pressure currently hurts whoever has fewer cheap moves, and bosses need a TP-aware AI.
- Design question: should TP be a factor before Erika (gym 5, "TP resource exhaustion")? If yes,
  costs need to roughly double or triple, or regen needs to drop (e.g. base 5 to 2).

## 2. Removing the flat +2 doubles the length of low-level fights

- At Lv 5, Charmander's Scratch does 1-2 damage to Squirtle (10-20 hits to KO). With the +2 it
  does 3-4 (5-7 hits). Damage of 1 is common until about Lv 10 once Growl or Tail Whip lands.
- First rival battle, win rate and length without +2 / with +2:
  Charmander 7% / 48%, Squirtle 74% / 44%, Bulbasaur 29% / 20%; 11-19 turns / 6-7 turns.
- Without the +2 the starter triangle decides the opening fight far more than with it. The +2 also
  helps super-effective hits more than neutral ones (it is added before the type multiplier), which
  is why turning it back on doesn't simply help the player.

## 3. Gym 1 (Jiro) is the wall, with the placeholder team

Placeholder: Spearow 8, Pidgey 9, Farfetch'd 11 holding the Leek (+2 stages = 25% crits), with
the sash. Player arrives at about Lv 7.5-9.5.

| Starter | Lv for a 50% first-try win | First-try win in the playthrough |
|---|---|---|
| Charmander | 13 | 5% |
| Squirtle | 13-14 | 0% |
| Bulbasaur | not reached by 14 (Flying beats Grass) | 0% (gives up after 10 tries) |

Farfetch'd alone at Lv 11 beats a Lv 12 Squirtle 98% of the time with the Leek and 89-93%
without it, so the level gap and the sash do most of the work, not the crits. A first gym at the
Tokiwa stage needs leaders around Lv 6-8, or a deliberate "come back later" design.

## 4. Brock and Misty with Green's levels

- Brock (Geodude 11, Onix 14): Squirtle jumps from 16% at Lv 12 to 97% at Lv 13 (Water Gun), Bulbasaur passes 50% at Lv 12; Charmander only
  25% even at Lv 20. Two causes: in the ROM's (Emerald) learnsets both Geodude and Onix know Rock
  Throw at 11, which Green's Brock didn't, and Charmander has only Ember (40) until Flamethrower at
  Lv 31. Onix's Gen 3 Sp. Def (45) is also higher than its Gen 1 Special (30).
- Misty (Staryu 18, Starmie 21): Squirtle and Bulbasaur win at the levels they arrive with;
  Charmander/Charmeleon never wins solo (Ember does 5-6 to Starmie, 10-12 hits).
- Crits are 5-12% of the player's damage across these fights.

## 5. Pacing against Omar's Green notes

- With the scenario's battle counts (Tokiwa Forest 9, Route 3 14, Mt. Moon 16 wild battles, from
  the LP table's "Battles" column), the solo starter reaches Brock at Lv 13.5-16.5 (Green: about
  12-14) and Misty at Lv 24.5-25.7 (Omar's team: 18-21).
- Part of that overshoot is retries: every lost gym attempt still pays EXP for what was knocked out.
  The rest says either fewer wild battles or less EXP per battle than Emerald's yield x level / 7.
- What the LP table's "Battles" and "Trainers" columns count is still a guess.

## 6. Move ranks with a tiered TP cost (asked by the design thread)

Proposal tested: one slot = one move family, the player picks the rank each turn, cost by rank,
regen stays 5 + INT/8. Rank = Notion's stars for NORMAL/ELEMENTAL family moves, otherwise a power
tier (<=45 one star, <=75 two, above three) as a stand-in. Enemy AI uses the highest affordable rank.

- **No real rank choice before Misty.** In the ROM learnsets no starter has two ranks of one family
  by Lv 21. Charmander is all one-star until Flamethrower (Lv 31), Squirtle gets Bite (two-star) at
  18 and Bulbasaur Razor Leaf (two-star) at 20. Only Nidoran M (Double Kick 12, Horn Attack 20) and
  Rattata (Hyper Fang 13) carry several ranks, and those are different families.
- Even with a full claws ladder (Scratch/Cut/Slash), type matchups and "cheapest move that
  finishes" decide the pick. TP forces a lower rank in 0% of single-mon gym fights and 3-13% of a
  four-mon rival fight, where one TP pool covers the whole battle.
- A three-star spammer runs dry after 4-5 uses at cost 30 and after 3 at cost 40. A two-star at 15
  lasts 15-43 turns (forever for Starmie). Early fights last 3-6 turns per mon.
- **The Starmie problem goes away.** One star at 5 is below every early regen (9-15), so no one
  Struggles under any ladder tried. Starmie with Hydro Pump bottoms out at 42 TP (5/15/30) or
  17 (5/15/40) but always has Water Gun.
- Suggested ladder: 5 / 15 / 40, status 10 / 20 / 30. The one-star cost must stay at or under the
  lowest regen (9) or bosses can starve. Raising the three-star cost works better than cutting regen
  (base 2 barely changes anything); TP max 60 drains Misty to 4 TP.
- Caveat: Notion ranks Tail Whip as a two-star damaging TAIL move, but the ROM's Tail Whip is status.

In the tool: `--set rank_costs=5,15,40 --set rank_status_costs=10,20,30` and `--foe-ai rank`.

## 7. TP that only refills when you rest (Omar's direction)

Model: `tp_persist=true` (TP carries over between battles and refills at a Pokémon Center or
a blackout), costs from the move's PP tier so a full 100 TP buys roughly what a modified PP of
20/15/10/5 would: PP 30+ costs 5, PP 20-25 costs 7, PP 15 costs 10, PP 10 or less costs 20
(Tackle, Scratch, Gust, Peck 5; Ember, Water Gun, Bite, Razor Leaf 7; Rock Throw, Flamethrower 10;
Vine Whip, Hydro Pump 20). Solo starter, 100 playthroughs each; "TP-short" = a battle where the lead
couldn't afford the move it wanted. The long stretches are Tokiwa to Nibi (Route 2 + the forest,
about 27 battles), Nibi to the Mt. Moon Center (Route 3, about 24) and Mt. Moon to Hanada (about 28).

| In-battle regen | Squirtle: first TP-short battle in a long stretch | TP-short battles per stretch | Struggled | TP left on arrival |
|---|---|---|---|---|
| 5 + INT/8 (now, 9-15 a turn) | almost never | 0 | 0 | 70-90 |
| 2 + INT/32 (2-4 a turn) | battle 15-25 | 2-8 | 2-8 | 35-43 |
| INT/32 (1-2 a turn) | battle 13-23 | 4-9 | 4-9 | 30-46 |
| none | battle 11-21 | 6-10 | 6-10 | 33-50 |

- With today's regen TP never runs out, persistent or not. Regen has to drop to a few TP a turn.
- **Suggested starting point: persistent TP, PP-tier costs, regen 2 + INT/32.** The lead runs short
  around the last third of each long stretch. Rotating the team or turning back to the Center is then
  the decision. Charmander and Bulbasaur behave the same (first short at battle 15-23).
- Bulbasaur shows the risk of copying PP straight across: Vine Whip has 10 PP, so it costs 20 TP and
  Bulbasaur runs short by battle 6 after Nibi. Core early moves need a deliberate tier, not their
  Gen 1 PP.
- Engine cost: TP has to be stored per party mon to survive between battles. One byte per mon fits
  in the four PP bytes of the BoxPokemon attacks substruct, so no new save data or RAM is needed,
  but the Pokémon Center, party menu, summary screen and items (Ethers) all have to learn about it.
- Caveats: the wild-battle counts per area are placeholders and decide when TP runs out; the
  placeholder Jiro inflates blackouts (each blackout refills TP).

Try it: `python3 sim.py --set tp_persist=true --set "tp_formula=5 if pp>=30 else 7 if pp>=20 else 10 if pp>=15 else 20" --set tp_regen_base=2 --set tp_int_div=32 run scenarios/early_game.yaml --starter SQUIRTLE`

## 8. Omar's MathModels folder (read from his PC, 2026-10-22)

`PKMN_Minamoto\MathModels` holds the 2023 scripts (`Battle.py`, `Pokemon.py`, `pokemon_simulations.py`,
`old/`) and the `newMM/` rewrite (entities, repos, Gen 1 DVs 0-15, stat EXP, data loader). Both data
folders are identical. A copy of `newMM/data` and its main files is in
`/mnt/project-files/balance-model/mathmodels/`. There is no TP or INT in any of it.

- `bosses.yaml` has one boss: Brock's Onix, Lv 14, Gen 1 stats (35/45/160/30/70), Tackle and
  Rock Throw. That matches the scenario's Onix.
- `encounters.yaml` covers Route 1, "Route 13" (Pidgey/Rattata 3-5, Mankey/Nidoran F 2-4, between
  Route 1 and Route 2, probably the LP table's "Route 12-1"), Route 2, Viridian Forest and Route 3.
- `newMM/main.py` draws the number of battles in each area as round(gamma(2, 5)), clamped to 1-64
  (mean 10). The tool now offers it: `run --battles gamma`. With it, the persistent-TP result of
  section 7 holds: Squirtle first runs short at battle 14-24 of the long stretches.

## 9. "C+" move ranks on persistent TP (asked by the design thread)

Proposal: a slot holds a move family; the rank cap rises with evolution (basic form one star, middle
form two, final form three; single-stage species two stars at Lv 20 and three at 35); the player
picks the rank each use. Families are Notion's (NORMAL subcategories, ELEMENTAL types). Ranks the ROM
lacks (Fireball, Hydro Jet, Lunge...) are plain hits with Notion's power. A learned ROM move maps to
its Notion family, or to its type's family (Vine Whip and Razor Leaf share the Grass slot, Bubble and
Water Gun the Water slot). Costs: PP tiers, with two stars counted as PP 15 (10 TP) and three stars as
PP 5 (20 TP). Regen 2 + INT/32, TP persists between battles. Habit A: one star on wild mons, highest
affordable rank on trainers. Habit B: always the strongest affordable rank. 150 playthroughs per row.

| Starter | Rank choice at Brock | Rank choice at Misty | TP-short battles, Nibi to Mt. Moon / Mt. Moon to Hanada, habit A vs B | Misty first-try win, evolves / never evolves |
|---|---|---|---|---|
| Charmander | 83% of turns (Charmeleon by then) | 100% | 10.4 vs 12.5 / 9.9 vs 12.1 | 1% / 0% (Brock 33% vs 1%) |
| Squirtle | 41% (Wartortle in 61% of runs) | 100% | 6.0 vs 9.6 / 5.6 vs 13.0 | 95% / 0% |
| Bulbasaur | 0% (still Bulbasaur) | 100% | 16.7 vs 16.7 / 13.0 vs 15.7 | 100% / 60% |

1. **A one-star vs two-star choice only exists after the first evolution (Lv 16).** By Brock it
   depends on the arrival level. Arrival levels here run high (Charmander about 16.6, Squirtle 14.7,
   Bulbasaur 13.2) because lost Jiro attempts still pay EXP. By Misty every evolved starter has the
   choice on every turn and picks two stars all the time against her.
2. **Route 2 + forest to Pewter: no difference between the habits**, about 5-6 TP-short battles for
   Charmander and Bulbasaur and under 1 for Squirtle, because nobody has evolved yet, so one star is the
   only rank. After evolving, the one-star habit saves 2-7 TP-short battles per long stretch
   (Squirtle 13.0 to 5.6 on Mt. Moon to Hanada). Bulbasaur is TP-short in 13-17 battles per stretch
   either way: its one-star Grass move is Vine Whip (10 PP, so 20 TP), the core-move cost problem from
   section 7.
3. **An unevolved mon falls behind badly.** Squirtle never beats Misty at Lv 24 without evolving
   (Wartortle wins 95%); Charmander's Brock win rate drops from 33% to 1%; Bulbasaur's Misty win rate
   drops from 100% to 60%. Blackouts per playthrough rise by 3-13. The cap ties power to evolution
   much more tightly than stats alone do.

Try it: add `--set cplus=true` (and `--set cplus_wild_rank1=false` for habit B, `run --no-evolve`)
to the section 7 command.

## Open inputs for the model

- Gym 1-3 teams, levels and movesets; where Jiro's gym sits on the route.
- Real TP costs and a real INT stat.
- The arena states (AIRFLOW, TERRAIN, TIDE) are the next thing to add to the engine.
