#!/usr/bin/env python3
"""Build every trainer's party in the game and save the bytes, to show that a
change to the trainer data or to CreateNPCTrainerParty creates the same Pokémon.

    python3 tools/trainerproc/partydump.py TREE OUTDIR
    cmp -s OLD/TRAINER_X.bin NEW/TRAINER_X.bin   # or diff -r OLD NEW

TREE is a checkout with a built pokeemerald_modern.gba/.elf (any build; debug
info is not needed) and its include/constants/opponents.h. The ROM runs in the
headless emulator (tools/gba-emu) to the title screen; then, for each trainer,
gdb-multiarch calls CreateNPCTrainerParty(gEnemyParty, id, TRUE) from the same
savestate with the same RNG seed, interrupts off (VBlank advances the RNG) and
a zeroed stack (Emerald copies uninitialized bytes after a species name's
terminator into the nickname), and OUTDIR/TRAINER_X.bin gets the 600 bytes of
gEnemyParty: the full encrypted Pokémon, stats included.
"""
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'regress'))
import run as R  # noqa: E402

BATTLE_TYPE_TRAINER = 8
REG_IME = 0x04000208
PARTY_BYTES = 6 * 100


class Emu(R.Emu):
    """The regression harness's emulator, without the regression-build symbols."""
    def __init__(self, rom):
        self.p = subprocess.Popen([R.EMU], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                  text=True, bufsize=1, cwd=R.ROOT)
        self.record = False
        self.recordable = False
        self.cmd('load ' + os.path.abspath(rom))


def main():
    tree, out = sys.argv[1:3]
    os.makedirs(out, exist_ok=True)
    out = os.path.abspath(out)
    elf = os.path.join(tree, 'pokeemerald_modern.elf')
    with open(os.path.join(tree, 'include/constants/opponents.h'), encoding='utf-8') as f:
        ids = re.findall(r'^#define (TRAINER_\w+)\s+(\d+)\s*$', f.read(), re.M)
    zeros = os.path.join(out, 'zeros.bin')
    with open(zeros, 'wb') as f:
        f.write(bytes(2048))

    emu = Emu(os.path.join(tree, 'pokeemerald_modern.gba'))
    emu.run(3400)                      # title screen: save block and RNG are set up
    state = os.path.join(out, 'title.ss')
    emu.cmd(f'savestate {state}')
    port = 23000 + os.getpid() % 1000
    emu.cmd(f'gdb {port}')
    made = 0
    with open(os.path.join(out, 'gdb.log'), 'w') as log:
        # One gdb session per trainer: after an inferior call the stub leaves the
        # CPU in a state the next call cannot use, so each starts from the savestate.
        for name, n in ids:
            if name == 'TRAINER_NONE':
                continue
            emu.cmd(f'loadstate {state}')
            cmds = ['set confirm off', 'set pagination off', f'target remote :{port}',
                    'tbreak RunTasks', 'continue',  # call from game code, not from the BIOS
                    f'set var *(unsigned int *)&gBattleTypeFlags = {BATTLE_TYPE_TRAINER}',
                    'set var *(unsigned int *)&gRngValue = 0x12345678',
                    f'set var *(unsigned short *){REG_IME:#x} = 0',
                    'set var $cpsr = $cpsr | 0x80',
                    f'restore {zeros} binary $sp-2048',
                    f'print ((int (*)(void *, int, int))CreateNPCTrainerParty)((void *)&gEnemyParty, {n}, 1)',
                    f'dump binary memory {out}/{name}.bin (char*)&gEnemyParty (char*)&gEnemyParty+{PARTY_BYTES}',
                    'disconnect']
            script = os.path.join(out, 'cmds.gdb')
            with open(script, 'w') as f:
                f.write('\n'.join(cmds) + '\n')
            r = subprocess.run(['gdb-multiarch', '-batch', '-nx', '-x', script, elf],
                               capture_output=True, text=True, timeout=120)
            log.write(r.stdout + r.stderr)
            if re.search(r'^\$\d+ = \d+', r.stdout, re.M):
                made += 1
            else:
                print(f'{name}: no party (see {out}/gdb.log)')
    emu.p.kill()
    for f in (zeros, state, os.path.join(out, 'cmds.gdb')):
        os.remove(f)
    print(f'{made} of {len(ids) - 1} parties written to {out}')
    return 0 if made == len(ids) - 1 else 1


if __name__ == '__main__':
    sys.exit(main())
