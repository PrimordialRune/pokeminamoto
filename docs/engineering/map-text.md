# Map text and translations

Every map keeps its logic and its words in separate files, so a translator never opens a script:

```
data/maps/MasaraTown/
  scripts.pory    logic: msgbox(MasaraTown_Text_Fiora), flags, movement...
  text.en.pory    English: only `text Name { format("...") }` blocks
  text.it.pory    Italian: the same block names (optional; any subset)
```

A ROM holds one language, picked at build time like the original games:

```bash
make -j$(nproc)                    # English
make LANGUAGE=ITALIAN -j$(nproc)   # Italian (codes: ENGLISH en, ITALIAN it, FRENCH fr, GERMAN de, SPANISH es, JAPANESE ja)
```

For each map, `tools/textlang.py merge` takes the blocks of `text.en.pory` in order and swaps in the
`text.<code>.pory` version of each one that has a translation. Untranslated blocks keep the English
text, so a half-done translation still builds and plays. Poryscript compiles the result into
`text.inc` (generated, gitignored). Switching `LANGUAGE` rebuilds every generated script file.

## Rules for a new map

1. Write logic in `scripts.pory` and no `format("...")` there: every line the player reads gets a
   named block in `text.en.pory`, called `<Map>_Text_<What>`.
2. Add `.include "data/maps/<Map>/text.inc"` after the map's `scripts.inc` line in `data/event_scripts.s`.
3. A text file holds `text` blocks and `//` comments only; anything else is a build error.
4. Renaming a block means renaming it in every `text.<code>.pory` too.

`format()` wraps lines for the font (tools/poryscript/font_config.json), so translations need no manual
line breaks either; `\p` (new box) and `\n` (forced break) work as usual. Accented Latin letters
(à è é ì ò ù and the rest of Emerald's European set) are in `charmap.txt`.

## Checks

`make check` runs `map_text_translations` (in `tools/check_data.py`, using `tools/textlang.py check`):

- a translation that lacks a block of `text.en.pory` fails it, as does a block that exists only in a
  translation (usually a rename the translation missed) or a block defined twice.

While a translation is in progress, record its current gaps with
`python3 tools/check_data.py --update-known`: the check then fails only when English gains a block
the translation doesn't have yet. `python3 tools/textlang.py check` prints every gap.

## Not covered yet

Older maps still have inline text (some use `poryswitch(LANGUAGE)`, as in LittlerootTown); move them
to `text.en.pory` when they are next edited. Text in C (`src/strings.c`, menus, battle messages) and in
`data/text/*.inc` is a separate pass.
