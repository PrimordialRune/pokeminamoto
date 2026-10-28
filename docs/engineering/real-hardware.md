# Running the slice on a real GBA (flash cart checklist)

For Omar, working alone. Target: ChisFlash V1.1 (128 KB Flash save, Seiko RTC) written with the GBFlash flasher, played on an original GBA / GBA SP, and later on the Analogue Pocket. Written 2026-10-28 by the hardware workstream. The numbers come from headless mGBA runs of the integrated ROM (`tools/regress/slice_*.py`); nothing here has touched a real cart yet, and the places where that matters are marked **unverified**.

The short version:

1. `tools/flash/flash.sh --dry-run` (works with nothing plugged in) to see what it will do.
2. Plug in the flasher and the cart, attach the flasher to WSL, run `tools/flash/flash.sh --info`.
3. `tools/flash/flash.sh` builds, flashes, verifies.
4. Play the "test first" list below, in order, and write down anything that differs.

## 0. What the ROM needs from the cart

| Need | What the ROM has | Cart |
|---|---|---|
| Save chip | `FLASH1M_V103` tag in the ROM: 128 KB Flash, 13 sectors of 3,968 B per slot, copy-on-write (a power cut keeps the last good save) | ChisFlash V1.1 has 128 KB Flash. Do **not** use a cart that is SRAM-only or FRAM-only (the 1.2 and 1.2V models): the game would not save. |
| Clock | `SIIRTC_V001` tag: Seiko S-3511A on the cart's GPIO port | ChisFlash V1.1 has it, on a CR1616 battery. |
| ROM size | 16 MiB padded (content about 9 MB), limit 32 MiB | 32 MB cart: fine. |
| Header | game code `BPEE`, checksum fixed by `gbafix` | `flash.sh` checks all of this before it writes. |

The ROM to flash is `pokeemerald_modern_release.gba` (`make release`): no debug menus, no test hooks. Use `--kind modern` for the debug-menu build when you want to force the time of day (see section 4). `flash.sh` refuses the `*_regress` and `*_test` ROMs.

## 1. One-time setup (WSL)

1. FlashGBX: `pipx install FlashGBX` (or `python3 -m pip install --user FlashGBX`). The script calls it as `FlashGBX --cli --mode agb ...` (checked against FlashGBX 5.1's command line; it asks the flasher for the cart, so a cart must be present for a real run).
2. USB into WSL. WSL cannot see USB devices by itself. In an elevated Windows PowerShell:
   ```
   winget install usbipd          # once
   usbipd list                    # find the GBFlash (CH340 / serial) device and note its BUSID
   usbipd bind --busid <BUSID>    # once
   usbipd attach --wsl --busid <BUSID>    # every time you plug it in
   ```
   Then in WSL `ls /dev/ttyACM* /dev/ttyUSB*` must show a port. If a permission error appears: `sudo usermod -aG dialout $USER`, then close and reopen the WSL window.
3. Cart in the flasher with the flasher unplugged, then plug the flasher in. Never swap a cart with the flasher powered.

## 2. Flash

```
tools/flash/flash.sh --info       # what FlashGBX sees: header, flash chip, save type, "Real Time Clock"
tools/flash/flash.sh              # build -> pick ROM -> back up the save -> flash -> verify
tools/flash/flash.sh --help
```

What the script does, in order: preflight (FlashGBX, port, WSL hints); `make release -j$(nproc)`; picks the ROM and checks it (BPEE, header checksum, Flash and RTC tags, no REGRESS hooks, prints the sha256); cart info; backs up the cart's save to `tools/flash/backups/save_<date>.sav` (git-ignored); asks before writing; `--action flash-rom` with FlashGBX's own write verify; then reads the ROM back and `cmp`s it with the file. Any mismatch stops the script with a message. Keep the sha256 it prints next to your test notes: it identifies the build you tested.

If FlashGBX does not recognise the cart (`--info` shows no flash cart profile): try `--cart-type "<profile name from the FlashGBX list>"`. If it still fails, Playback with the GB Operator writes the ChisFlash too (flashcarts.net lists it); use the ROM file `flash.sh` built.

**Do not** interrupt a write. A half-written ROM just means flashing again; it cannot hurt the cart.

## 3. The test-first list (30 minutes)

Tick these in order. Each says what a pass looks like and what to write down if it fails. Use fresh batteries or the USB-C power cable.

1. **Boots.** Power on: the logo, the title screen, music. *Fail*: black or white screen (reflash, then try another ROM build), garbled title (cart contacts: clean with isopropyl alcohol).
2. **New game to Satoshi's room.** NEW GAME, name, you wake in your room in Masara. Walk downstairs and outside.
3. **Save and power cut.** Start menu, SAVE, wait for "saved". Power off. Power on, CONTINUE: you stand where you saved. *Fail*: "the save file is corrupt" or no CONTINUE: the save chip is the wrong type. Note the cart model and stop.
4. **Save during a power cut (once).** Save again, and switch the GBA off while "Saving, don't turn off" is on screen. Power on: CONTINUE must load the previous save (the emulator test cut the power at every flash write, 44 cuts, all recovered). If this fails on the real cart, it is a bug with the highest priority: write down exactly when you cut.
5. **Lag.** Walk the length of Route 1, then the Viridian ("Tokiwa") streets while running (hold B). The scroll should be even. The emulator result: 0 dropped frames on every walk loop (table below). One hitch when you cross from one map to the next on foot is normal (1 to 2 frames; same in vanilla Emerald).
6. **Inn rest.** Rest at the Tokaido-ya and the Akagi-ya: fade to black, jingle, "good morning". TP is restored. (Each rest drops 1 frame at the room menu, invisible.)
7. **A gym fight.** Yujirou or Takeshi: the battle intro, menus and message turns. Expect small hitches when the intro starts, when the FIGHT menu opens and when you come back to the map: 1 to 6 dropped frames each in the emulator (bug HW-001). Anything that feels like seconds, not a blink, is new: note it.
8. **Sleep.** In the overworld hold L and R and press SELECT: the screen goes black and the GBA stops. Any button wakes it where you were. (Emulator: passes on three maps, with and without a clock.)
9. **Clock and day/night.** See section 4.
10. **Cold boot with the battery out.** Optional, last: take the CR1616 out for a minute, put it back, boot. See "battery" in section 4.

## 4. Clock, day/night and the timed NPCs

How the game reads the time (`src/day_night.c`): the cart clock once a second, never while the sound IRQ is due. If there is no clock, or it reads garbage, it uses **play time**: one game hour per 5 minutes of play, starting at 10:00 on a new game, so a full day takes two hours of play. Both clocks drive the same things: the morning, day and night palettes, and every NPC that shows up only in a window (Hiroshi, the CVS regulars, Kimura at night, the school pupil).

What the emulator proved (`tools/regress/slice_clock.py`): for every hour 0 to 23 and 15 maps with timed content, the flag block (every hide flag the map scripts set) and the day/night phase are identical between a real clock set to that hour and the play-time fallback set to the same hour. The fallback clock advances one hour per 5 play-minutes. So a cart with a dead or missing clock plays the same content, in a different order: nothing is missing, nothing freezes.

What to check on the cart:

- **The clock starts at midnight, not at your wall clock (bug HW-003).** A new game resets the cart clock to 00:00 and the slice has no clock-setting screen, so the game shows the time since you started the game: the first hours are NITE (dark violet tint, lit windows). This is expected until HW-003 is fixed. To see MORN and DAY without waiting, use the debug build (`flash.sh --kind modern`): hold R and press START, open the flags/vars menu and set variable `VAR_TIME_OF_DAY_OVERRIDE` to 1 (MORN), 2 (DAY), 3 (NITE), 0 (follow the clock again).
- **Battery message.** If the main menu says the internal battery has run dry, the cart clock lost power or was never set. Reseat the cart and boot again. The game still works (play-time clock). The ChisFlash can also read **2000-01-01** after a flash or a save write; for this game that is harmless, because it starts its own clock on a new game. If you want the clock to show real time, set it first with the GBA RTC tool for your flasher (the `GBA_RTCRead` ROM, run it once from the cart, set the date and time, then flash Minamoto again) **unverified**: the game ignores the date and uses the hour, and until HW-003 is fixed it resets the clock on a new game anyway.
- The phases by hour: MORN 4 to 10, DAY 10 to 18, NITE 18 to 4.

## 5. Analogue Pocket (later)

Two routes, **neither tried yet**:

1. **Cart in the Pocket with Analogue's GBA adapter.** The same cart you just tested. The Pocket reads the cart's save chip and clock itself, so everything above applies. Check the same list.
2. **openFPGA GBA core with the `.gba` on the SD card.** Copy `pokeemerald_modern_release.gba` to the core's asset folder (the core's own README says where). The core makes the `.sav` and picks the save type from the `FLASH1M_V103` tag in the ROM, which is why no patch is needed. The clock depends on the core; if it has none the game uses the play-time clock. Run the test-first list; for the save test, back out to the Pocket menu after saving and relaunch.

## 6. What the emulator results say (hardware workstream, 2026-10-28)

Integrated ROM at commit `5500a9860` (+ hardware worktree), `make release REGRESS=1 DINFO=1`, mGBA headless. The budget is 228 scanlines per frame; "busy" is how many the main loop used.

| Run | Result |
|---|---|
| Walk loops, 8 outdoor maps x MORN, DAY, NITE x walking and running, about 1,800 frames each (about 87,000 frames in all) | **0 lag frames.** Busy 37 avg (Masara) to 71 avg (Route 1); worst frame 169 of 228 (Viridian Forest). |
| Timed-NPC slots: 13 maps x every hour 0 to 23 | 0 lag frames; most object events alive in a slot: 7 (Akagi-ya common room), sprites 11. |
| Map seams on foot (6 seams, both ways) | 1 to 2 lag frames per crossing, worst busy 170. |
| Doors (every outdoor door, in and out) | 4 to 7 lag frames, during the fade, worst busy 195. |
| Inn rests (2 inns x MORN, DAY, NITE x UME, TAKE) | 1 lag frame each (room menu), 12 of 12 rests counted. |
| Yujirou, Takeshi (real fights, forced win after 3,600 frames) | 25 to 27 lag frames each; busy avg 64, worst 225. Same as a plain trainer battle (29) and a wild battle (12). |
| Save, power cuts | 44 cuts (every flash write, not started and half done), blank cart and loaded save: all recovered, saving again works. Partial saves (link paths): pass. Slice flags and all 256 vars compared. |
| Save layout | Format 3, layout hash 0xD93A, unchanged from the base (the literal is in the ROM). Zero RAM change. |
| No clock | 24 of 24 hours match the real-clock runs on 15 maps; fallback advances with play time; sleep passes with and without a clock. |

The per-scene lag numbers and the JSON are in `/mnt/project-files/studio/ws/hardware/`.

## 7. Known issues the cart will show (all filed in `studio/qa/bugs.md`)

- HW-001 battle intro, FIGHT menu and return-to-map drop 1 to 6 frames each (all battles, not only arenas).
- HW-002 on a cart with no clock chip the game still polls the port every second (harmless, tidy-up).
- HW-003 the game cannot set the clock; day/night follows time since new game, starting at midnight.

## 8. If something goes wrong

| Symptom | Likely cause | Do |
|---|---|---|
| `flash.sh`: no serial port | flasher not attached to WSL | `usbipd attach` (section 1) |
| FlashGBX cannot find the cart | contacts, cart not seated, wrong profile | reseat, clean, `--cart-type` |
| READ-BACK MISMATCH | bad contact or a failing flash chip | flash again; if it repeats, try the GB Operator |
| Game boots, will not save | save chip is not 128 KB Flash | check `--info`; wrong cart model |
| Boots to "battery has run dry" every time | clock lost power | reseat; the game still plays |
| Frozen after the title screen on a cart | very rarely: a bad cart ROM bank | reflash; note the sha256 |

Everything about the build side (RAM budget, save hash, `make check`) is in `CLAUDE.md` and `/mnt/project-files/studio/base-ram.md`.
