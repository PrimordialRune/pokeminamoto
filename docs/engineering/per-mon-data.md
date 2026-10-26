# Per-mon data in BoxPokemon

`struct BoxPokemon` stays **80 bytes** and `struct Pokemon` **100 bytes**
(`STATIC_ASSERT`s in `include/pokemon.h`). The PC boxes, the daycare, the party in
SaveBlock1 and link trades all copy these structs whole, so Minamoto's per-mon data
goes into space Emerald used for systems we dropped, never into new fields.

Everything below is in the 48-byte `secure` part: encrypted with
`personality ^ otId` and covered by `checksum`. Always go through
`GetBoxMonData`/`SetBoxMonData` (or the `Mon` versions); they decrypt, check the
checksum, write and re-encrypt. Writing the struct directly corrupts the mon
(it turns into a Bad Egg).

## Layout of the four substructs (12 bytes each)

| substruct | bytes | contents |
|---|---|---|
| 0 | 0-1 | species |
| 0 | 2-3 | held item |
| 0 | 4-7 | experience |
| 0 | 8 | **free** (`freedPpBonuses`, was PP Up bonuses) |
| 0 | 9 | friendship |
| 0 | 10-11 | **free** (`filler`) |
| 1 | 0-7 | moves[4] |
| 1 | 8-11 | **Minamoto bit field** (was PP[4]), see below |
| 2 | 0-5 | EVs |
| 2 | 6 | `specialProgress` (was cool) |
| 2 | 7-11 | **free** (`freedContest[5]`, was beauty, cute, smart, tough, sheen) |
| 3 | 0-11 | pokérus, met data, ball, OT gender, IVs, isEgg, abilityNum, ribbons, fateful encounter (unchanged) |

### Substruct 1, bytes 8-11 (u32 bit field)

| bits | field | MON_DATA | meaning |
|---|---|---|---|
| 0-2 | `moveRank0` | `MON_DATA_MOVE_RANK1` | rank of move slot 1, 0 = `MOVE_RANK_NONE`, max 7 |
| 3-5 | `moveRank1` | `MON_DATA_MOVE_RANK2` | slot 2 |
| 6-8 | `moveRank2` | `MON_DATA_MOVE_RANK3` | slot 3 |
| 9-11 | `moveRank3` | `MON_DATA_MOVE_RANK4` | slot 4 |
| 0-11 | (all four) | `MON_DATA_MOVE_RANKS` | packed, slot 1 in bits 0-2 (u16 in/out) |
| 12 | `specialAwakened` | `MON_DATA_SPECIAL_AWAKENED` | the species' Special (signature move) is unsealed |
| 13-19 | `savedTp` | `MON_DATA_SAVED_TP` | TP kept between battles, 0-`TP_MAX` (writes clamp) |
| 20-31 | `reserved` | none | free |

### Other accessors

| MON_DATA | storage | meaning |
|---|---|---|
| `MON_DATA_SPECIAL_PROGRESS` | substruct 2 byte 6 | progress towards awakening the Special, 0-255 |
| `MON_DATA_DVS` | none (view over the IVs) | Gen 1 DVs, u16: Attack bits 15-12, Defense 11-8, Speed 7-4, Special 3-0 |
| `MON_DATA_COOL` .. `MON_DATA_SHEEN` | none | contest stats are gone: reads return 0, writes are dropped |

## Design status of each field

- **Move ranks**: Omar dropped move ranks on 2026-10-22 (one Special per species
  instead). The 12 bits are allocated as asked so the data has a home if ranks come
  back; nothing reads or writes them. They can be handed to something else.
- **Special**: per the design sessions (2026-10-22) the Special is innate but
  sealed, a fifth move outside the 4 slots, unlocked per mon (Deed / Place / Witness,
  Bond as fallback) and kept through evolution. The move itself comes from the
  species (`gSpecialMoves[]` in `src/data/pokemon/special_moves.h`), so a mon only
  needs the awakened flag and a progress counter. There is no "signature slot" field
  because the Special never takes one of the 4 slots. Wired since v1 (2026-10-26,
  `src/special_move.c`): Bond and Deed conditions, the awakening message after a won
  battle, and the Special page on SELECT in the FIGHT menu.
- **savedTp**: TP resets to `TP_MAX` at the start of each battle (GDD) and lives in
  `gBattleStruct->tp`. Omar wants TP to run out and need rest to recover, which may
  mean TP that persists between battles; `savedTp` is reserved for that. Unused.
- **DVs**: Gen 1 DVs reuse the IV bits instead of new storage. A DV is the top 4
  bits of the 5-bit IV (`DV = IV >> 1`); Special reads the Sp. Atk IV. Setting DVs
  writes each IV as `DV * 2 + (DV & 1)` (DV 15 is IV 31), copies Special into Sp. Atk
  and Sp. Def, and rebuilds the HP IV from the low bits of the four DVs as Gen 1
  does. The stat formulas still read IVs, so nothing changes until the Gen 1 stat
  formula (GDD: DV/4) is wired in.

## Still free

| where | bits | notes |
|---|---:|---|
| substruct 0 byte 8 | 8 | |
| substruct 0 bytes 10-11 | 16 | |
| substruct 1 bits 20-31 | 12 | |
| substruct 2 bytes 7-11 | 40 | |
| **total, ready to use** | **76** | |

Reclaimable later (still read or written today, retire the accessors first):

| where | bits | blocker |
|---|---:|---|
| substruct 3 cool..tough ribbons | 15 | ribbon count / summary ribbon page |
| substruct 3 winning..world + unused ribbons | 15 | same (keep `championRibbon`) |
| substruct 3 `abilityNum` | 1 | abilities still exist in the battle engine |
| substruct 3 `modernFatefulEncounter` | 1 | Mew obedience check |
| substruct 0 species bits 9-15 | 7 | species ids stay below 512, but every species read would need masking |
| `BoxPokemon.unknown` | 16 | outside encryption and checksum (readable by `MON_DATA_ENCRYPT_SEPARATOR`) |

## Adding a field

1. Take bits from the "still free" table and rename them in `include/pokemon.h`
   (update the comment block above `struct PokemonSubstruct0`).
2. Append a `MON_DATA_*` at the end of the enum (after the existing Minamoto ones)
   so the encryption check (`field > MON_DATA_ENCRYPT_SEPARATOR`) covers it.
3. Add the get and set cases in `GetBoxMonData3` / `SetBoxMonData` in `src/pokemon.c`.
4. Update this file. Old saves read the new bits as whatever the old field held,
   so either clear them on load or accept that a new save is needed.
