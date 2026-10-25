#ifndef GUARD_SLEEP_MODE_H
#define GUARD_SLEEP_MODE_H

// Hold L and R, then press SELECT in the overworld to put the GBA to sleep.
// Any button wakes it.
#define SLEEP_MODE_COMBO (L_BUTTON | R_BUTTON | SELECT_BUTTON)

bool8 SleepMode_ComboPressed(void);
void SleepMode_Enter(void);

#endif // GUARD_SLEEP_MODE_H
