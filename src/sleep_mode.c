#include "global.h"
#include "gba/io_reg.h"
#include "gba/syscall.h"
#include "link.h"
#include "m4a.h"
#include "main.h"
#include "sleep_mode.h"
#include "constants/rgb.h"

// Sleep mode: the BIOS Stop call (SWI 3) powers the GBA down to almost
// nothing until a keypad interrupt. The sequence is
//   1. wait until every key is released (a keypad IRQ only fires when the
//      selected condition becomes true, so a held key would never wake us)
//   2. show a black frame, mask all interrupts except the keypad one, mute
//   3. Stop
//   4. restore everything, wait until the waking key is released again

#define KEYS_ALL 0x3FF
#define SLEEP_DEBOUNCE_FRAMES 20

static void SleepCall(void)
{
    // Thumb SWI; the BIOS reads the number from the instruction.
    asm volatile("svc 3" ::: "memory");
}

static void WaitFrames(u32 frames)
{
    while (frames-- != 0)
        VBlankIntrWait();
}

// Waits for every key to be up for SLEEP_DEBOUNCE_FRAMES in a row.
static void WaitForKeysReleased(void)
{
    u32 quiet = 0;

    while (quiet < SLEEP_DEBOUNCE_FRAMES)
    {
        VBlankIntrWait();
        if ((REG_KEYINPUT & KEYS_ALL) == KEYS_ALL)
            quiet++;
        else
            quiet = 0;
    }
}

bool8 SleepMode_ComboPressed(void)
{
    // Raw keys, so the L=A option doesn't matter. SELECT has to come last so
    // the combo can't be hit by accident while holding L+R to run.
    return (gMain.heldKeysRaw & SLEEP_MODE_COMBO) == SLEEP_MODE_COMBO
        && (gMain.newKeysRaw & SELECT_BUTTON);
}

void SleepMode_Enter(void)
{
    IntrCallback vblankCallback;
    u16 dispcnt, backdrop, ie;

    // Never while linked: the other side would time out.
    if (gReceivedRemoteLinkPlayers || gLinkTransferringData)
        return;

    WaitForKeysReleased();

    // One black frame, so the screen is black and not frozen on the map.
    // The backdrop is palette entry 0; with every layer off nothing else shows.
    // The field's VBlank callback would copy the palette buffer over it.
    vblankCallback = gMain.vblankCallback;
    SetVBlankCallback(NULL);
    VBlankIntrWait();
    dispcnt = REG_DISPCNT;
    REG_DISPCNT = DISPCNT_FORCED_BLANK;
    backdrop = *(vu16 *)BG_PLTT;
    *(vu16 *)BG_PLTT = RGB_BLACK;
    REG_DISPCNT = 0;
    WaitFrames(2);

    // Only the keypad may wake us: any button (OR mode), IRQ enabled. IME
    // stays on so the wake-up runs IntrMain, whose keypad handler is empty.
    ie = REG_IE;
    REG_IE = INTR_FLAG_KEYPAD;
    REG_KEYCNT = KEYS_ALL | KEY_INTR_ENABLE;
    REG_IF = INTR_FLAG_KEYPAD;
    m4aSoundVSyncOff();

    SleepCall();

    // Awake. Put everything back in reverse order.
    REG_KEYCNT = 0;
    REG_IF = INTR_FLAG_KEYPAD;
    m4aSoundVSyncOn();
    REG_IE = ie;

    *(vu16 *)BG_PLTT = backdrop;
    REG_DISPCNT = dispcnt;
    SetVBlankCallback(vblankCallback);

    WaitForKeysReleased();
}
