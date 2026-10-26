# pokeminamoto

A Gen 1-only fork of the [pokeemerald](https://github.com/pret/pokeemerald) GBA decompilation: a playable
Pokémon Emerald ROM with the original 151 Pokémon, heading towards the Minamoto design (`docs/design/`).

Build: `tools/setup-cloud.sh` on a fresh Debian/Ubuntu box (or WSL2), then `make -j$(nproc)` for
`pokeemerald_modern.gba`. There is no original-ROM checksum to match any more. See [CLAUDE.md](CLAUDE.md)
for the layout, build variants, debugging and the regression harness (`tools/regress`).

Upstream credit: [pret](https://pret.github.io/) and everyone behind pokeemerald.
