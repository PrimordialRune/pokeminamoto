#ifndef GUARD_SCRIPT_MENU_H
#define GUARD_SCRIPT_MENU_H

#include "list_menu.h"
#include "constants/script_menu.h"

extern const u8 *const gStdStrings[];

// Dynamic multichoice: dynmultipush adds options to this stack (heap-allocated,
// grows as needed) and dynmultistack shows them.
#define MULTICHOICE_DYNAMIC_STACK_SIZE 5
#define MULTICHOICE_DYNAMIC_STACK_INC  5

struct DynamicMultichoiceStack
{
    s32 top;
    u32 capacity;
    struct ListMenuItem *elements;
};

void MultichoiceDynamic_PushElement(struct ListMenuItem item);
u32 MultichoiceDynamic_StackSize(void);
struct ListMenuItem *MultichoiceDynamic_PeekElementAt(u32 index);
void MultichoiceDynamic_DestroyStack(void);
bool8 ScriptMenu_MultichoiceDynamic(u8 left, u8 top, u8 argc, struct ListMenuItem *items, bool8 ignoreBPress, u8 maxBeforeScroll, u32 initialRow, u32 callbackSet);
bool8 ScriptMenu_Multichoice(u8 left, u8 top, u8 multichoiceId, bool8 ignoreBPress);
bool8 ScriptMenu_MultichoiceWithDefault(u8 left, u8 top, u8 multichoiceId, bool8 ignoreBPress, u8 defaultChoice);
bool8 ScriptMenu_YesNo(u8 left, u8 top);
bool8 ScriptMenu_MultichoiceGrid(u8 left, u8 top, u8 multichoiceId, bool8 ignoreBPress, u8 columnCount);
bool8 ScriptMenu_ShowPokemonPic(u16 species, u8 x, u8 y);
bool8 (*ScriptMenu_HidePokemonPic(void))(void);
int ConvertPixelWidthToTileWidth(int width);
u8 CreateWindowFromRect(u8 x, u8 y, u8 width, u8 height);
void ClearToTransparentAndRemoveWindow(u8 windowId);
int DisplayTextAndGetWidth(const u8 *str, int prevWidth);
int ScriptMenu_AdjustLeftCoordFromWidth(int left, int width);
bool16 ScriptMenu_CreatePCMultichoice(void);
void ScriptMenu_DisplayPCStartupPrompt(void);

#endif //GUARD_SCRIPT_MENU_H
