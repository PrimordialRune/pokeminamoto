#include "global.h"
#include "battle.h"
#include "battle_anim.h"
#include "battle_arena.h"
#include "battle_controllers.h"
#include "battle_dome.h"
#include "battle_interface.h"
#include "battle_message.h"
#include "battle_setup.h"
#include "battle_talk.h"
#include "battle_foe_info.h"
#include "bg.h"
#include "data.h"
#include "graphics.h"
#include "item.h"
#include "item_menu.h"
#include "menu.h"
#include "link.h"
#include "main.h"
#include "m4a.h"
#include "palette.h"
#include "party_menu.h"
#include "pokeball.h"
#include "pokemon.h"
#include "random.h"
#include "reshow_battle_screen.h"
#include "sound.h"
#include "special_move.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "util.h"
#include "window.h"
#include "debug_menu.h"
#include "constants/battle_anim.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/party_menu.h"
#include "constants/songs.h"
#include "constants/trainers.h"
#include "constants/rgb.h"

static void PlayerHandleGetMonData(void);
static void PlayerHandleSetMonData(void);
static void PlayerHandleSetRawMonData(void);
static void PlayerHandleLoadMonSprite(void);
static void PlayerHandleSwitchInAnim(void);
static void PlayerHandleReturnMonToBall(void);
static void PlayerHandleDrawTrainerPic(void);
static void PlayerHandleTrainerSlide(void);
static void PlayerHandleTrainerSlideBack(void);
static void PlayerHandleFaintAnimation(void);
static void PlayerHandlePaletteFade(void);
static void PlayerHandleSuccessBallThrowAnim(void);
static void PlayerHandleBallThrowAnim(void);
static void PlayerHandlePause(void);
static void PlayerHandleMoveAnimation(void);
static void PlayerHandlePrintString(void);
static void PlayerHandlePrintSelectionString(void);
static void PlayerHandleChooseAction(void);
static void PlayerHandleYesNoBox(void);
static void PlayerHandleChooseMove(void);
static void PlayerHandleChooseItem(void);
static void PlayerHandleChoosePokemon(void);
static void PlayerHandleCmd23(void);
static void PlayerHandleHealthBarUpdate(void);
static void PlayerHandleExpUpdate(void);
static void PlayerHandleStatusIconUpdate(void);
static void PlayerHandleStatusAnimation(void);
static void PlayerHandleStatusXor(void);
static void PlayerHandleDataTransfer(void);
static void PlayerHandleDMA3Transfer(void);
static void PlayerHandlePlayBGM(void);
static void PlayerHandleCmd32(void);
static void PlayerHandleTwoReturnValues(void);
static void PlayerHandleChosenMonReturnValue(void);
static void PlayerHandleOneReturnValue(void);
static void PlayerHandleOneReturnValue_Duplicate(void);
static void PlayerHandleClearUnkVar(void);
static void PlayerHandleSetUnkVar(void);
static void PlayerHandleClearUnkFlag(void);
static void PlayerHandleToggleUnkFlag(void);
static void PlayerHandleHitAnimation(void);
static void PlayerHandleCantSwitch(void);
static void PlayerHandlePlaySE(void);
static void PlayerHandlePlayFanfareOrBGM(void);
static void PlayerHandleFaintingCry(void);
static void PlayerHandleIntroSlide(void);
static void PlayerHandleIntroTrainerBallThrow(void);
static void PlayerHandleDrawPartyStatusSummary(void);
static void PlayerHandleHidePartyStatusSummary(void);
static void PlayerHandleEndBounceEffect(void);
static void PlayerHandleSpriteInvisibility(void);
static void PlayerHandleBattleAnimation(void);
static void PlayerHandleLinkStandbyMsg(void);
static void PlayerHandleResetActionMoveSelection(void);
static void PlayerHandleEndLinkBattle(void);
static void PlayerCmdEnd(void);

static void PlayerBufferRunCommand(void);
static void HandleInputChooseTarget(void);
static void HandleInputChooseMove(void);
static void MoveSelectionCreateCursorAt(u8, u8);
static void MoveSelectionDestroyCursorAt(u8);
static void MoveSelectionDisplayTpCost(void);
static void MoveSelectionDisplayMoveType(void);
static void MoveSelectionDisplayMoveNames(void);
static void MoveSelectionDisplayEffectiveness(void);
static void MoveSelectionDrawMoveNames(void);
static void MoveSelectionDrawSpecialPage(void);
static void RecolorWindowPixels(u8 windowId, u8 from, u8 to, u16 left, u16 top, u16 width, u16 height);
static void ActionSelectionPrintPrompt(const u8 *text);
static void HandleInputChooseTalk(void);
static void Talk_WaitForSlideIn(void);
static void Talk_WaitForAnswerRead(void);
static void Talk_WaitForSlideOut(void);
static void OpenFoeInfoScreen(void);
static void ReturnToChooseAction(void);
static void SwitchIn_HandleSoundAndEnd(void);
static void WaitForMonSelection(void);
static void CompleteWhenChoseItem(void);
#ifdef DEBUG_MENUS
static void OpenBattleDebugMenu(void);
#endif
static void Task_LaunchLvlUpAnim(u8);
static void Task_PrepareToGiveExpWithExpBar(u8);
static void DestroyExpTaskAndCompleteOnInactiveTextPrinter(u8);
static void Task_GiveExpWithExpBar(u8);
static void Task_UpdateLvlInHealthbox(u8);
static void PrintLinkStandbyMsg(void);
static u32 CopyPlayerMonData(u8, u8 *);
static void SetPlayerMonData(u8);
static void StartSendOutAnim(u8, bool8);
static void DoSwitchOutAnimation(void);
static void PlayerDoMoveAnimation(void);
static void Task_StartSendOutAnim(u8);
static void EndDrawPartyStatusSummary(void);

// Colors of the battle textbox palette (BG palette 0, tools/battle_ui/gen_textbox.py)
#define TEXTBOX_PAL_NAVY   1
#define TEXTBOX_PAL_WHITE  5
#define TEXTBOX_PAL_ORANGE 9
#define TEXTBOX_PAL_GOLD  10
#define TEXTBOX_PAL_CYAN  11
#define TEXTBOX_PAL_GREY  13
#define TEXTBOX_PAL_LCD   15

#define MOVE_CELL_WIDTH      56
#define MOVE_NAME_MAX_WIDTH  52
#define TYPE_NAME_MAX_WIDTH  36

enum {
    MOVE_NAME_NORMAL,
    MOVE_NAME_SELECTED,
};

// SELECT inside FIGHT flips the move grid to the Special page, which has one entry: the mon's
// Special, or "???" while it is sealed. The page is up while gMoveSelectionCursor == SPECIAL_MOVE_POS.
#define IsOnSpecialPage() (gMoveSelectionCursor[gActiveBattler] == SPECIAL_MOVE_POS)

// How each cell of the move grid is drawn; the grid is one window, redrawn whole
static EWRAM_DATA u8 sMoveNameStyles[MAX_MON_MOVES] = {0};

static void (*const sPlayerBufferCommands[CONTROLLER_CMDS_COUNT])(void) =
{
    [CONTROLLER_GETMONDATA]               = PlayerHandleGetMonData,
    [CONTROLLER_GETRAWMONDATA]            = PlayerHandleGetRawMonData,
    [CONTROLLER_SETMONDATA]               = PlayerHandleSetMonData,
    [CONTROLLER_SETRAWMONDATA]            = PlayerHandleSetRawMonData,
    [CONTROLLER_LOADMONSPRITE]            = PlayerHandleLoadMonSprite,
    [CONTROLLER_SWITCHINANIM]             = PlayerHandleSwitchInAnim,
    [CONTROLLER_RETURNMONTOBALL]          = PlayerHandleReturnMonToBall,
    [CONTROLLER_DRAWTRAINERPIC]           = PlayerHandleDrawTrainerPic,
    [CONTROLLER_TRAINERSLIDE]             = PlayerHandleTrainerSlide,
    [CONTROLLER_TRAINERSLIDEBACK]         = PlayerHandleTrainerSlideBack,
    [CONTROLLER_FAINTANIMATION]           = PlayerHandleFaintAnimation,
    [CONTROLLER_PALETTEFADE]              = PlayerHandlePaletteFade,
    [CONTROLLER_SUCCESSBALLTHROWANIM]     = PlayerHandleSuccessBallThrowAnim,
    [CONTROLLER_BALLTHROWANIM]            = PlayerHandleBallThrowAnim,
    [CONTROLLER_PAUSE]                    = PlayerHandlePause,
    [CONTROLLER_MOVEANIMATION]            = PlayerHandleMoveAnimation,
    [CONTROLLER_PRINTSTRING]              = PlayerHandlePrintString,
    [CONTROLLER_PRINTSTRINGPLAYERONLY]    = PlayerHandlePrintSelectionString,
    [CONTROLLER_CHOOSEACTION]             = PlayerHandleChooseAction,
    [CONTROLLER_YESNOBOX]                 = PlayerHandleYesNoBox,
    [CONTROLLER_CHOOSEMOVE]               = PlayerHandleChooseMove,
    [CONTROLLER_OPENBAG]                  = PlayerHandleChooseItem,
    [CONTROLLER_CHOOSEPOKEMON]            = PlayerHandleChoosePokemon,
    [CONTROLLER_23]                       = PlayerHandleCmd23,
    [CONTROLLER_HEALTHBARUPDATE]          = PlayerHandleHealthBarUpdate,
    [CONTROLLER_EXPUPDATE]                = PlayerHandleExpUpdate,
    [CONTROLLER_STATUSICONUPDATE]         = PlayerHandleStatusIconUpdate,
    [CONTROLLER_STATUSANIMATION]          = PlayerHandleStatusAnimation,
    [CONTROLLER_STATUSXOR]                = PlayerHandleStatusXor,
    [CONTROLLER_DATATRANSFER]             = PlayerHandleDataTransfer,
    [CONTROLLER_DMA3TRANSFER]             = PlayerHandleDMA3Transfer,
    [CONTROLLER_PLAYBGM]                  = PlayerHandlePlayBGM,
    [CONTROLLER_32]                       = PlayerHandleCmd32,
    [CONTROLLER_TWORETURNVALUES]          = PlayerHandleTwoReturnValues,
    [CONTROLLER_CHOSENMONRETURNVALUE]     = PlayerHandleChosenMonReturnValue,
    [CONTROLLER_ONERETURNVALUE]           = PlayerHandleOneReturnValue,
    [CONTROLLER_ONERETURNVALUE_DUPLICATE] = PlayerHandleOneReturnValue_Duplicate,
    [CONTROLLER_CLEARUNKVAR]              = PlayerHandleClearUnkVar,
    [CONTROLLER_SETUNKVAR]                = PlayerHandleSetUnkVar,
    [CONTROLLER_CLEARUNKFLAG]             = PlayerHandleClearUnkFlag,
    [CONTROLLER_TOGGLEUNKFLAG]            = PlayerHandleToggleUnkFlag,
    [CONTROLLER_HITANIMATION]             = PlayerHandleHitAnimation,
    [CONTROLLER_CANTSWITCH]               = PlayerHandleCantSwitch,
    [CONTROLLER_PLAYSE]                   = PlayerHandlePlaySE,
    [CONTROLLER_PLAYFANFAREORBGM]         = PlayerHandlePlayFanfareOrBGM,
    [CONTROLLER_FAINTINGCRY]              = PlayerHandleFaintingCry,
    [CONTROLLER_INTROSLIDE]               = PlayerHandleIntroSlide,
    [CONTROLLER_INTROTRAINERBALLTHROW]    = PlayerHandleIntroTrainerBallThrow,
    [CONTROLLER_DRAWPARTYSTATUSSUMMARY]   = PlayerHandleDrawPartyStatusSummary,
    [CONTROLLER_HIDEPARTYSTATUSSUMMARY]   = PlayerHandleHidePartyStatusSummary,
    [CONTROLLER_ENDBOUNCE]                = PlayerHandleEndBounceEffect,
    [CONTROLLER_SPRITEINVISIBILITY]       = PlayerHandleSpriteInvisibility,
    [CONTROLLER_BATTLEANIMATION]          = PlayerHandleBattleAnimation,
    [CONTROLLER_LINKSTANDBYMSG]           = PlayerHandleLinkStandbyMsg,
    [CONTROLLER_RESETACTIONMOVESELECTION] = PlayerHandleResetActionMoveSelection,
    [CONTROLLER_ENDLINKBATTLE]            = PlayerHandleEndLinkBattle,
    [CONTROLLER_TERMINATOR_NOP]           = PlayerCmdEnd
};

static const u8 sTargetIdentities[MAX_BATTLERS_COUNT] = {B_POSITION_PLAYER_LEFT, B_POSITION_PLAYER_RIGHT, B_POSITION_OPPONENT_RIGHT, B_POSITION_OPPONENT_LEFT};

// unknown unused data
static const u8 sUnused[] = {0x48, 0x48, 0x20, 0x5a, 0x50, 0x50, 0x50, 0x58};

void BattleControllerDummy(void)
{
}

void SetControllerToPlayer(void)
{
    gBattlerControllerFuncs[gActiveBattler] = PlayerBufferRunCommand;
    gDoingBattleAnim = FALSE;
    gPlayerDpadHoldFrames = 0;
}

static void PlayerBufferExecCompleted(void)
{
    SetTpBarPreview(gActiveBattler, 0);
    gBattlerControllerFuncs[gActiveBattler] = PlayerBufferRunCommand;
    if (gBattleTypeFlags & BATTLE_TYPE_LINK)
    {
        u8 playerId = GetMultiplayerId();

        PrepareBufferDataTransferLink(B_COMM_CONTROLLER_IS_DONE, 4, &playerId);
        gBattleBufferA[gActiveBattler][0] = CONTROLLER_TERMINATOR_NOP;
    }
    else
    {
        gBattleControllerExecFlags &= ~gBitTable[gActiveBattler];
    }
}

static void PlayerBufferRunCommand(void)
{
    if (gBattleControllerExecFlags & gBitTable[gActiveBattler])
    {
        if (gBattleBufferA[gActiveBattler][0] < ARRAY_COUNT(sPlayerBufferCommands))
            sPlayerBufferCommands[gBattleBufferA[gActiveBattler][0]]();
        else
            PlayerBufferExecCompleted();
    }
}

static void CompleteOnBankSpritePosX_0(void)
{
    if (gSprites[gBattlerSpriteIds[gActiveBattler]].x2 == 0)
        PlayerBufferExecCompleted();
}

static void HandleInputChooseAction(void)
{
    u16 itemId = gBattleBufferA[gActiveBattler][2] | (gBattleBufferA[gActiveBattler][3] << 8);

    DoBounceEffect(gActiveBattler, BOUNCE_HEALTHBOX, 7, 1);
    DoBounceEffect(gActiveBattler, BOUNCE_MON, 7, 1);

    if (JOY_REPEAT(DPAD_ANY) && gSaveBlock2Ptr->optionsButtonMode == OPTIONS_BUTTON_MODE_L_EQUALS_A)
        gPlayerDpadHoldFrames++;
    else
        gPlayerDpadHoldFrames = 0;

    if (JOY_NEW(A_BUTTON))
    {
        PlaySE(SE_SELECT);

        switch (gActionSelectionCursor[gActiveBattler])
        {
        case 0: // Top left
            BtlController_EmitTwoReturnValues(B_COMM_TO_ENGINE, B_ACTION_USE_MOVE, 0);
            break;
        case 1: // Top right, PKMN
            BtlController_EmitTwoReturnValues(B_COMM_TO_ENGINE, B_ACTION_SWITCH, 0);
            break;
        case 2: // Bottom left, ITEM
            BtlController_EmitTwoReturnValues(B_COMM_TO_ENGINE, B_ACTION_USE_ITEM, 0);
            break;
        case 3: // Bottom right
            BtlController_EmitTwoReturnValues(B_COMM_TO_ENGINE, B_ACTION_RUN, 0);
            break;
        }
        PlayerBufferExecCompleted();
    }
    else if (JOY_NEW(DPAD_LEFT))
    {
        if (gActionSelectionCursor[gActiveBattler] & 1) // if is PKMN or RUN
        {
            PlaySE(SE_SELECT);
            ActionSelectionDestroyCursorAt(gActionSelectionCursor[gActiveBattler]);
            gActionSelectionCursor[gActiveBattler] ^= 1;
            ActionSelectionCreateCursorAt(gActionSelectionCursor[gActiveBattler], 0);
        }
    }
    else if (JOY_NEW(DPAD_RIGHT))
    {
        if (!(gActionSelectionCursor[gActiveBattler] & 1)) // if is FIGHT or ITEM
        {
            PlaySE(SE_SELECT);
            ActionSelectionDestroyCursorAt(gActionSelectionCursor[gActiveBattler]);
            gActionSelectionCursor[gActiveBattler] ^= 1;
            ActionSelectionCreateCursorAt(gActionSelectionCursor[gActiveBattler], 0);
        }
    }
    else if (JOY_NEW(DPAD_UP))
    {
        if (gActionSelectionCursor[gActiveBattler] & 2) // if is ITEM or RUN
        {
            PlaySE(SE_SELECT);
            ActionSelectionDestroyCursorAt(gActionSelectionCursor[gActiveBattler]);
            gActionSelectionCursor[gActiveBattler] ^= 2;
            ActionSelectionCreateCursorAt(gActionSelectionCursor[gActiveBattler], 0);
        }
    }
    else if (JOY_NEW(DPAD_DOWN))
    {
        if (!(gActionSelectionCursor[gActiveBattler] & 2)) // if is FIGHT or PKMN
        {
            PlaySE(SE_SELECT);
            ActionSelectionDestroyCursorAt(gActionSelectionCursor[gActiveBattler]);
            gActionSelectionCursor[gActiveBattler] ^= 2;
            ActionSelectionCreateCursorAt(gActionSelectionCursor[gActiveBattler], 0);
        }
    }
    else if (JOY_NEW(B_BUTTON) || gPlayerDpadHoldFrames > 59)
    {
        if ((gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
         && GetBattlerPosition(gActiveBattler) == B_POSITION_PLAYER_RIGHT
         && !(gAbsentBattlerFlags & gBitTable[GetBattlerAtPosition(B_POSITION_PLAYER_LEFT)])
         && !(gBattleTypeFlags & BATTLE_TYPE_MULTI))
        {
            if (gBattleBufferA[gActiveBattler][1] == B_ACTION_USE_ITEM)
            {
                // Add item to bag if it is a ball
                if (itemId <= LAST_BALL)
                    AddBagItem(itemId, 1);
                else
                    return;
            }
            PlaySE(SE_SELECT);
            BtlController_EmitTwoReturnValues(B_COMM_TO_ENGINE, B_ACTION_CANCEL_PARTNER, 0);
            PlayerBufferExecCompleted();
        }
    }
#ifdef DEBUG_MENUS
    else if (JOY_NEW(SELECT_BUTTON) && JOY_HELD(L_BUTTON))
    {
        PlaySE(SE_SELECT);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        gBattlerControllerFuncs[gActiveBattler] = OpenBattleDebugMenu;
    }
#endif
    else if (JOY_NEW(START_BUTTON))
    {
        SwapHpBarsWithHpText();
    }
    else if (JOY_NEW(R_BUTTON))
    {
        // Talk (src/battle_talk.c). It doesn't use up the turn.
        u8 availability = GetTalkAvailability();

        PlaySE(SE_SELECT);
        if (availability == TALK_AVAILABLE)
        {
            gMultiUsePlayerCursor = TALK_PROVOKE;
            ActionSelectionPrintPrompt(GetTalkMenuText(gMultiUsePlayerCursor));
            gBattlerControllerFuncs[gActiveBattler] = HandleInputChooseTalk;
        }
        else
        {
            ActionSelectionPrintPrompt(GetTalkUnavailableText(availability));
        }
    }
    else if (JOY_NEW(SELECT_BUTTON))
    {
        // SEL.: the foe info screen (src/battle_foe_info.c)
        PlaySE(SE_SELECT);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        gBattlerControllerFuncs[gActiveBattler] = OpenFoeInfoScreen;
    }
}

// Prints into the action menu's LCD, expanding battle placeholders
static void ActionSelectionPrintPrompt(const u8 *text)
{
    BattleStringExpandPlaceholdersToDisplayedString(text);
    BattlePutTextOnWindow(gDisplayedStringBattle, B_WIN_ACTION_PROMPT);
}

// The Talk lines are listed in the LCD while the action buttons stay up.
// A says the line, B or R goes back.
static void HandleInputChooseTalk(void)
{
    if (JOY_NEW(A_BUTTON))
    {
        // The trainer slides in to answer, then back out
        PlaySE(SE_SELECT);
        ActionSelectionPrintPrompt(gText_EmptyString3);
        TalkTrainerSlideIn();
        gBattlerControllerFuncs[gActiveBattler] = Talk_WaitForSlideIn;
    }
    else if (JOY_NEW(B_BUTTON | R_BUTTON))
    {
        PlaySE(SE_SELECT);
        ActionSelectionPrintPrompt(gText_WhatWillPkmnDo);
        gBattlerControllerFuncs[gActiveBattler] = HandleInputChooseAction;
    }
    else if (JOY_NEW(DPAD_UP | DPAD_DOWN))
    {
        PlaySE(SE_SELECT);
        gMultiUsePlayerCursor ^= 1;
        ActionSelectionPrintPrompt(GetTalkMenuText(gMultiUsePlayerCursor));
    }
}

static void Talk_WaitForSlideIn(void)
{
    if (IsTalkTrainerSlideDone())
    {
        ActionSelectionPrintPrompt(DoBattleTalk(gMultiUsePlayerCursor));
        gBattlerControllerFuncs[gActiveBattler] = Talk_WaitForAnswerRead;
    }
}

// The answer stays up with the trainer until A or B
static void Talk_WaitForAnswerRead(void)
{
    if (JOY_NEW(A_BUTTON | B_BUTTON))
    {
        PlaySE(SE_SELECT);
        TalkTrainerSlideOut();
        gBattlerControllerFuncs[gActiveBattler] = Talk_WaitForSlideOut;
    }
}

static void Talk_WaitForSlideOut(void)
{
    if (IsTalkTrainerSlideDone())
    {
        TalkTrainerRemove();
        gBattlerControllerFuncs[gActiveBattler] = HandleInputChooseAction;
    }
}

static void UNUSED UnusedEndBounceEffect(void)
{
    EndBounceEffect(gActiveBattler, BOUNCE_HEALTHBOX);
    EndBounceEffect(gActiveBattler, BOUNCE_MON);
    gBattlerControllerFuncs[gActiveBattler] = HandleInputChooseTarget;
}

static void HandleInputChooseTarget(void)
{
    s32 i;
    u8 identities[MAX_BATTLERS_COUNT];
    memcpy(identities, sTargetIdentities, ARRAY_COUNT(sTargetIdentities));

    DoBounceEffect(gMultiUsePlayerCursor, BOUNCE_HEALTHBOX, 15, 1);

    // what a weird loop
    i = 0;
    if (gBattlersCount != 0)
    {
        do
        {
            if (i != gMultiUsePlayerCursor)
                EndBounceEffect(i, BOUNCE_HEALTHBOX);
            i++;
        } while (i < gBattlersCount);
    }

    if (JOY_HELD(DPAD_ANY) && gSaveBlock2Ptr->optionsButtonMode == OPTIONS_BUTTON_MODE_L_EQUALS_A)
        gPlayerDpadHoldFrames++;
    else
        gPlayerDpadHoldFrames = 0;

    if (JOY_NEW(A_BUTTON))
    {
        PlaySE(SE_SELECT);
        gSprites[gBattlerSpriteIds[gMultiUsePlayerCursor]].callback = SpriteCB_HideAsMoveTarget;
        BtlController_EmitTwoReturnValues(B_COMM_TO_ENGINE, 10, gMoveSelectionCursor[gActiveBattler] | (gMultiUsePlayerCursor << 8));
        EndBounceEffect(gMultiUsePlayerCursor, BOUNCE_HEALTHBOX);
        PlayerBufferExecCompleted();
    }
    else if (JOY_NEW(B_BUTTON) || gPlayerDpadHoldFrames > 59)
    {
        PlaySE(SE_SELECT);
        gSprites[gBattlerSpriteIds[gMultiUsePlayerCursor]].callback = SpriteCB_HideAsMoveTarget;
        gBattlerControllerFuncs[gActiveBattler] = HandleInputChooseMove;
        DoBounceEffect(gActiveBattler, BOUNCE_HEALTHBOX, 7, 1);
        DoBounceEffect(gActiveBattler, BOUNCE_MON, 7, 1);
        EndBounceEffect(gMultiUsePlayerCursor, BOUNCE_HEALTHBOX);
    }
    else if (JOY_NEW(DPAD_LEFT | DPAD_UP))
    {
        PlaySE(SE_SELECT);
        gSprites[gBattlerSpriteIds[gMultiUsePlayerCursor]].callback = SpriteCB_HideAsMoveTarget;

        do
        {
            u8 currSelIdentity = GetBattlerPosition(gMultiUsePlayerCursor);

            for (i = 0; i < MAX_BATTLERS_COUNT; i++)
            {
                if (currSelIdentity == identities[i])
                    break;
            }
            do
            {
                if (--i < 0)
#ifdef UBFIX
                    i = MAX_BATTLERS_COUNT - 1;
#else
                    i = MAX_BATTLERS_COUNT; // UB: array out of range
#endif
                gMultiUsePlayerCursor = GetBattlerAtPosition(identities[i]);
            } while (gMultiUsePlayerCursor == gBattlersCount);

            i = 0;
            switch (GetBattlerPosition(gMultiUsePlayerCursor))
            {
            case B_POSITION_PLAYER_LEFT:
            case B_POSITION_PLAYER_RIGHT:
                if (gActiveBattler != gMultiUsePlayerCursor)
                    i++;
                else if (gMovesInfo[GetMonData(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_MOVE1 + gMoveSelectionCursor[gActiveBattler])].target & MOVE_TARGET_USER_OR_SELECTED)
                    i++;
                break;
            case B_POSITION_OPPONENT_LEFT:
            case B_POSITION_OPPONENT_RIGHT:
                i++;
                break;
            }

            if (gAbsentBattlerFlags & gBitTable[gMultiUsePlayerCursor])
                i = 0;
        } while (i == 0);
        gSprites[gBattlerSpriteIds[gMultiUsePlayerCursor]].callback = SpriteCB_ShowAsMoveTarget;
    }
    else if (JOY_NEW(DPAD_RIGHT | DPAD_DOWN))
    {
        PlaySE(SE_SELECT);
        gSprites[gBattlerSpriteIds[gMultiUsePlayerCursor]].callback = SpriteCB_HideAsMoveTarget;

        do
        {
            u8 currSelIdentity = GetBattlerPosition(gMultiUsePlayerCursor);

            for (i = 0; i < MAX_BATTLERS_COUNT; i++)
            {
                if (currSelIdentity == identities[i])
                    break;
            }
            do
            {
                if (++i > 3)
                    i = 0;
                gMultiUsePlayerCursor = GetBattlerAtPosition(identities[i]);
            } while (gMultiUsePlayerCursor == gBattlersCount);

            i = 0;
            switch (GetBattlerPosition(gMultiUsePlayerCursor))
            {
            case B_POSITION_PLAYER_LEFT:
            case B_POSITION_PLAYER_RIGHT:
                if (gActiveBattler != gMultiUsePlayerCursor)
                    i++;
                else if (gMovesInfo[GetMonData(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_MOVE1 + gMoveSelectionCursor[gActiveBattler])].target & MOVE_TARGET_USER_OR_SELECTED)
                    i++;
                break;
            case B_POSITION_OPPONENT_LEFT:
            case B_POSITION_OPPONENT_RIGHT:
                i++;
                break;
            }

            if (gAbsentBattlerFlags & gBitTable[gMultiUsePlayerCursor])
                i = 0;
        } while (i == 0);
        gSprites[gBattlerSpriteIds[gMultiUsePlayerCursor]].callback = SpriteCB_ShowAsMoveTarget;
    }
}

// The move under the cursor, on either page (MOVE_NONE for a sealed Special)
static u16 GetSelectedMove(void)
{
    struct ChooseMoveStruct *moveInfo = (struct ChooseMoveStruct *)(&gBattleBufferA[gActiveBattler][4]);

    if (IsOnSpecialPage())
        return GetBattlerSpecialMove(gActiveBattler);
    return moveInfo->moves[gMoveSelectionCursor[gActiveBattler]];
}

static u8 GetSelectedTpCost(void)
{
    struct ChooseMoveStruct *moveInfo = (struct ChooseMoveStruct *)(&gBattleBufferA[gActiveBattler][4]);

    if (IsOnSpecialPage())
        return GetBattlerMoveTPCost(gActiveBattler, GetBattlerSpecialMove(gActiveBattler));
    return moveInfo->tpCost[gMoveSelectionCursor[gActiveBattler]];
}

static void HandleInputChooseMove(void)
{
    bool32 canSelectTarget = FALSE;
    struct ChooseMoveStruct *moveInfo = (struct ChooseMoveStruct *)(&gBattleBufferA[gActiveBattler][4]);

    if (JOY_HELD(DPAD_ANY) && gSaveBlock2Ptr->optionsButtonMode == OPTIONS_BUTTON_MODE_L_EQUALS_A)
        gPlayerDpadHoldFrames++;
    else
        gPlayerDpadHoldFrames = 0;

    if (JOY_NEW(A_BUTTON))
    {
        u8 moveTarget;

        // a sealed Special can't be chosen
        if (IsOnSpecialPage() && GetSelectedMove() == MOVE_NONE)
        {
            PlaySE(SE_FAILURE);
            return;
        }
        PlaySE(SE_SELECT);
        if (GetSelectedMove() == MOVE_CURSE)
        {
            if (moveInfo->monTypes[0] != TYPE_GHOST && moveInfo->monTypes[1] != TYPE_GHOST)
                moveTarget = MOVE_TARGET_USER;
            else
                moveTarget = MOVE_TARGET_SELECTED;
        }
        else
        {
            moveTarget = gMovesInfo[GetSelectedMove()].target;
        }

        if (moveTarget & MOVE_TARGET_USER)
            gMultiUsePlayerCursor = gActiveBattler;
        else
            gMultiUsePlayerCursor = GetBattlerAtPosition(BATTLE_OPPOSITE(GET_BATTLER_SIDE(gActiveBattler)));

        if (!gBattleBufferA[gActiveBattler][1]) // not a double battle
        {
            if (moveTarget & MOVE_TARGET_USER_OR_SELECTED && !gBattleBufferA[gActiveBattler][2])
                canSelectTarget++;
        }
        else // double battle
        {
            if (!(moveTarget & (MOVE_TARGET_RANDOM | MOVE_TARGET_BOTH | MOVE_TARGET_DEPENDS | MOVE_TARGET_FOES_AND_ALLY | MOVE_TARGET_OPPONENTS_FIELD | MOVE_TARGET_USER)))
                canSelectTarget++; // either selected or user

            if (GetSelectedTpCost() > moveInfo->currentTp)
            {
                canSelectTarget = FALSE;
            }
            else if (!(moveTarget & (MOVE_TARGET_USER | MOVE_TARGET_USER_OR_SELECTED)) && CountAliveMonsInBattle(BATTLE_ALIVE_EXCEPT_ACTIVE) <= 1)
            {
                gMultiUsePlayerCursor = GetDefaultMoveTarget(gActiveBattler);
                canSelectTarget = FALSE;
            }
        }

        if (!canSelectTarget)
        {
            BtlController_EmitTwoReturnValues(B_COMM_TO_ENGINE, 10, gMoveSelectionCursor[gActiveBattler] | (gMultiUsePlayerCursor << 8));
            PlayerBufferExecCompleted();
        }
        else
        {
            gBattlerControllerFuncs[gActiveBattler] = HandleInputChooseTarget;

            if (moveTarget & (MOVE_TARGET_USER | MOVE_TARGET_USER_OR_SELECTED))
                gMultiUsePlayerCursor = gActiveBattler;
            else if (gAbsentBattlerFlags & gBitTable[GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT)])
                gMultiUsePlayerCursor = GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT);
            else
                gMultiUsePlayerCursor = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);

            gSprites[gBattlerSpriteIds[gMultiUsePlayerCursor]].callback = SpriteCB_ShowAsMoveTarget;
        }
    }
    else if (JOY_NEW(B_BUTTON) || gPlayerDpadHoldFrames > 59)
    {
        PlaySE(SE_SELECT);
        if (IsOnSpecialPage())
            gMoveSelectionCursor[gActiveBattler] = 0;
        BtlController_EmitTwoReturnValues(B_COMM_TO_ENGINE, 10, 0xFFFF);
        PlayerBufferExecCompleted();
    }
    else if (JOY_NEW(SELECT_BUTTON))
    {
        // Flip between the move grid and the Special page. Link battles have no Specials.
        // While the Special page is up, gMultiUsePlayerCursor keeps the grid's cursor.
        if (!(gBattleTypeFlags & BATTLE_TYPE_LINK))
        {
            PlaySE(SE_SELECT);
            if (IsOnSpecialPage())
            {
                gMoveSelectionCursor[gActiveBattler] = (gMultiUsePlayerCursor < gNumberOfMovesToChoose) ? gMultiUsePlayerCursor : 0;
                MoveSelectionCreateCursorAt(gMoveSelectionCursor[gActiveBattler], 0);
            }
            else
            {
                gMultiUsePlayerCursor = gMoveSelectionCursor[gActiveBattler];
                gMoveSelectionCursor[gActiveBattler] = SPECIAL_MOVE_POS;
                MoveSelectionDrawSpecialPage();
            }
            MoveSelectionDisplayTpCost();
            MoveSelectionDisplayMoveType();
        }
    }
    else if (IsOnSpecialPage())
    {
        // the Special page has one entry, so the D-pad does nothing
    }
    else if (JOY_NEW(DPAD_LEFT))
    {
        if (gMoveSelectionCursor[gActiveBattler] & 1)
        {
            MoveSelectionDestroyCursorAt(gMoveSelectionCursor[gActiveBattler]);
            gMoveSelectionCursor[gActiveBattler] ^= 1;
            PlaySE(SE_SELECT);
            MoveSelectionCreateCursorAt(gMoveSelectionCursor[gActiveBattler], 0);
            MoveSelectionDisplayTpCost();
            MoveSelectionDisplayMoveType();
        }
    }
    else if (JOY_NEW(DPAD_RIGHT))
    {
        if (!(gMoveSelectionCursor[gActiveBattler] & 1)
         && (gMoveSelectionCursor[gActiveBattler] ^ 1) < gNumberOfMovesToChoose)
        {
            MoveSelectionDestroyCursorAt(gMoveSelectionCursor[gActiveBattler]);
            gMoveSelectionCursor[gActiveBattler] ^= 1;
            PlaySE(SE_SELECT);
            MoveSelectionCreateCursorAt(gMoveSelectionCursor[gActiveBattler], 0);
            MoveSelectionDisplayTpCost();
            MoveSelectionDisplayMoveType();
        }
    }
    else if (JOY_NEW(DPAD_UP))
    {
        if (gMoveSelectionCursor[gActiveBattler] & 2)
        {
            MoveSelectionDestroyCursorAt(gMoveSelectionCursor[gActiveBattler]);
            gMoveSelectionCursor[gActiveBattler] ^= 2;
            PlaySE(SE_SELECT);
            MoveSelectionCreateCursorAt(gMoveSelectionCursor[gActiveBattler], 0);
            MoveSelectionDisplayTpCost();
            MoveSelectionDisplayMoveType();
        }
    }
    else if (JOY_NEW(DPAD_DOWN))
    {
        if (!(gMoveSelectionCursor[gActiveBattler] & 2)
         && (gMoveSelectionCursor[gActiveBattler] ^ 2) < gNumberOfMovesToChoose)
        {
            MoveSelectionDestroyCursorAt(gMoveSelectionCursor[gActiveBattler]);
            gMoveSelectionCursor[gActiveBattler] ^= 2;
            PlaySE(SE_SELECT);
            MoveSelectionCreateCursorAt(gMoveSelectionCursor[gActiveBattler], 0);
            MoveSelectionDisplayTpCost();
            MoveSelectionDisplayMoveType();
        }
    }
}

static u32 UNUSED HandleMoveInputUnused(void)
{
    u32 var = 0;

    if (JOY_NEW(A_BUTTON))
    {
        PlaySE(SE_SELECT);
        var = 1;
    }
    if (JOY_NEW(B_BUTTON))
    {
        PlaySE(SE_SELECT);
        gBattle_BG0_X = 0;
        gBattle_BG0_Y = DISPLAY_HEIGHT * 2;
        var = 0xFF;
    }
    if (JOY_NEW(DPAD_LEFT) && gMoveSelectionCursor[gActiveBattler] & 1)
    {
        MoveSelectionDestroyCursorAt(gMoveSelectionCursor[gActiveBattler]);
        gMoveSelectionCursor[gActiveBattler] ^= 1;
        PlaySE(SE_SELECT);
        MoveSelectionCreateCursorAt(gMoveSelectionCursor[gActiveBattler], 0);
    }
    if (JOY_NEW(DPAD_RIGHT) && !(gMoveSelectionCursor[gActiveBattler] & 1)
        && (gMoveSelectionCursor[gActiveBattler] ^ 1) < gNumberOfMovesToChoose)
    {
        MoveSelectionDestroyCursorAt(gMoveSelectionCursor[gActiveBattler]);
        gMoveSelectionCursor[gActiveBattler] ^= 1;
        PlaySE(SE_SELECT);
        MoveSelectionCreateCursorAt(gMoveSelectionCursor[gActiveBattler], 0);
    }
    if (JOY_NEW(DPAD_UP) && gMoveSelectionCursor[gActiveBattler] & 2)
    {
        MoveSelectionDestroyCursorAt(gMoveSelectionCursor[gActiveBattler]);
        gMoveSelectionCursor[gActiveBattler] ^= 2;
        PlaySE(SE_SELECT);
        MoveSelectionCreateCursorAt(gMoveSelectionCursor[gActiveBattler], 0);
    }
    if (JOY_NEW(DPAD_DOWN) && !(gMoveSelectionCursor[gActiveBattler] & 2)
        && (gMoveSelectionCursor[gActiveBattler] ^ 2) < gNumberOfMovesToChoose)
    {
        MoveSelectionDestroyCursorAt(gMoveSelectionCursor[gActiveBattler]);
        gMoveSelectionCursor[gActiveBattler] ^= 2;
        PlaySE(SE_SELECT);
        MoveSelectionCreateCursorAt(gMoveSelectionCursor[gActiveBattler], 0);
    }

    return var;
}

static void SetLinkBattleEndCallbacks(void)
{
    if (gReceivedRemoteLinkPlayers == 0)
    {
        m4aSongNumStop(SE_LOW_HEALTH);
        gMain.inBattle = FALSE;
        gMain.callback1 = gPreBattleCallback1;
        SetMainCallback2(CB2_InitEndLinkBattle);
        if (gBattleOutcome == B_OUTCOME_WON)
        FreeAllWindowBuffers();
    }
}

// Despite handling link battles separately, this is only ever used by link battles
void SetBattleEndCallbacks(void)
{
    if (!gPaletteFade.active)
    {
        if (gBattleTypeFlags & BATTLE_TYPE_LINK)
        {
            if (IsLinkTaskFinished())
            {
                SetCloseLinkCallback();

                gBattlerControllerFuncs[gActiveBattler] = SetLinkBattleEndCallbacks;
            }
        }
        else
        {
            m4aSongNumStop(SE_LOW_HEALTH);
            gMain.inBattle = FALSE;
            gMain.callback1 = gPreBattleCallback1;
            SetMainCallback2(gMain.savedCallback);
        }
    }
}

static void CompleteOnBattlerSpriteCallbackDummy(void)
{
    if (gSprites[gBattlerSpriteIds[gActiveBattler]].callback == SpriteCallbackDummy)
        PlayerBufferExecCompleted();
}

static void CompleteOnBankSpriteCallbackDummy2(void)
{
    if (gSprites[gBattlerSpriteIds[gActiveBattler]].callback == SpriteCallbackDummy)
        PlayerBufferExecCompleted();
}

static void FreeTrainerSpriteAfterSlide(void)
{
    if (gSprites[gBattlerSpriteIds[gActiveBattler]].callback == SpriteCallbackDummy)
    {
        BattleGfxSfxDummy3(gSaveBlock2Ptr->playerGender);
        FreeSpriteOamMatrix(&gSprites[gBattlerSpriteIds[gActiveBattler]]);
        DestroySprite(&gSprites[gBattlerSpriteIds[gActiveBattler]]);
        PlayerBufferExecCompleted();
    }
}

static void Intro_DelayAndEnd(void)
{
    if (--gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].introEndDelay == (u8)-1)
    {
        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].introEndDelay = 0;
        PlayerBufferExecCompleted();
    }
}

static void Intro_WaitForShinyAnimAndHealthbox(void)
{
    bool8 healthboxAnimDone = FALSE;

    // Check if healthbox has finished sliding in
    if (!IsDoubleBattle() || (IsDoubleBattle() && (gBattleTypeFlags & BATTLE_TYPE_MULTI)))
    {
        if (gSprites[gHealthboxSpriteIds[gActiveBattler]].callback == SpriteCallbackDummy)
            healthboxAnimDone = TRUE;
    }
    else
    {
        if (gSprites[gHealthboxSpriteIds[gActiveBattler]].callback == SpriteCallbackDummy
         && gSprites[gHealthboxSpriteIds[BATTLE_PARTNER(gActiveBattler)]].callback == SpriteCallbackDummy)
            healthboxAnimDone = TRUE;
    }

    // If healthbox and shiny anim are done
    if (healthboxAnimDone && gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].finishedShinyMonAnim
        && gBattleSpritesDataPtr->healthBoxesData[BATTLE_PARTNER(gActiveBattler)].finishedShinyMonAnim)
    {
        // Reset shiny anim (even if it didn't occur)
        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].triedShinyMonAnim = FALSE;
        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].finishedShinyMonAnim = FALSE;
        gBattleSpritesDataPtr->healthBoxesData[BATTLE_PARTNER(gActiveBattler)].triedShinyMonAnim = FALSE;
        gBattleSpritesDataPtr->healthBoxesData[BATTLE_PARTNER(gActiveBattler)].finishedShinyMonAnim = FALSE;
        FreeSpriteTilesByTag(ANIM_TAG_GOLD_STARS);
        FreeSpritePaletteByTag(ANIM_TAG_GOLD_STARS);

        HandleLowHpMusicChange(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], gActiveBattler);

        if (IsDoubleBattle())
            HandleLowHpMusicChange(&gPlayerParty[gBattlerPartyIndexes[BATTLE_PARTNER(gActiveBattler)]], BATTLE_PARTNER(gActiveBattler));

        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].introEndDelay = 3;
        gBattlerControllerFuncs[gActiveBattler] = Intro_DelayAndEnd;
    }
}

static void Intro_TryShinyAnimShowHealthbox(void)
{
    bool32 bgmRestored = FALSE;
    bool32 battlerAnimsDone = FALSE;

    // Start shiny animation if applicable for 1st Pokémon
    if (!gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].triedShinyMonAnim
     && !gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].ballAnimActive)
        TryShinyAnimation(gActiveBattler, &gPlayerParty[gBattlerPartyIndexes[gActiveBattler]]);

    // Start shiny animation if applicable for 2nd Pokémon
    if (!gBattleSpritesDataPtr->healthBoxesData[BATTLE_PARTNER(gActiveBattler)].triedShinyMonAnim
     && !gBattleSpritesDataPtr->healthBoxesData[BATTLE_PARTNER(gActiveBattler)].ballAnimActive)
        TryShinyAnimation(BATTLE_PARTNER(gActiveBattler), &gPlayerParty[gBattlerPartyIndexes[BATTLE_PARTNER(gActiveBattler)]]);

    // Show healthbox after ball anim
    if (!gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].ballAnimActive
     && !gBattleSpritesDataPtr->healthBoxesData[BATTLE_PARTNER(gActiveBattler)].ballAnimActive)
    {
        if (!gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].healthboxSlideInStarted)
        {
            if (IsDoubleBattle() && !(gBattleTypeFlags & BATTLE_TYPE_MULTI))
            {
                UpdateHealthboxAttribute(gHealthboxSpriteIds[BATTLE_PARTNER(gActiveBattler)], &gPlayerParty[gBattlerPartyIndexes[BATTLE_PARTNER(gActiveBattler)]], HEALTHBOX_ALL);
                StartHealthboxSlideIn(BATTLE_PARTNER(gActiveBattler));
                SetHealthboxSpriteVisible(gHealthboxSpriteIds[BATTLE_PARTNER(gActiveBattler)]);
            }
            UpdateHealthboxAttribute(gHealthboxSpriteIds[gActiveBattler], &gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], HEALTHBOX_ALL);
            StartHealthboxSlideIn(gActiveBattler);
            SetHealthboxSpriteVisible(gHealthboxSpriteIds[gActiveBattler]);
        }
        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].healthboxSlideInStarted = TRUE;
    }

    // Restore bgm after cry has played and healthbox anim is started
    if (!gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].waitForCry
        && gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].healthboxSlideInStarted
        && !gBattleSpritesDataPtr->healthBoxesData[BATTLE_PARTNER(gActiveBattler)].waitForCry
        && !IsCryPlayingOrClearCrySongs())
    {
        if (!gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].bgmRestored)
        {
            if (gBattleTypeFlags & BATTLE_TYPE_MULTI && gBattleTypeFlags & BATTLE_TYPE_LINK)
                m4aMPlayContinue(&gMPlayInfo_BGM);
            else
                m4aMPlayVolumeControl(&gMPlayInfo_BGM, TRACKS_ALL, 0x100);
        }
        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].bgmRestored = TRUE;
        bgmRestored = TRUE;
    }

    // Wait for battler anims
    if (!IsDoubleBattle() || (IsDoubleBattle() && (gBattleTypeFlags & BATTLE_TYPE_MULTI)))
    {
        if (gSprites[gBattleControllerData[gActiveBattler]].callback == SpriteCallbackDummy
            && gSprites[gBattlerSpriteIds[gActiveBattler]].callback == SpriteCallbackDummy)
        {
            battlerAnimsDone = TRUE;
        }
    }
    else
    {
        if (gSprites[gBattleControllerData[gActiveBattler]].callback == SpriteCallbackDummy
            && gSprites[gBattlerSpriteIds[gActiveBattler]].callback == SpriteCallbackDummy
            && gSprites[gBattleControllerData[BATTLE_PARTNER(gActiveBattler)]].callback == SpriteCallbackDummy
            && gSprites[gBattlerSpriteIds[BATTLE_PARTNER(gActiveBattler)]].callback == SpriteCallbackDummy)
        {
            battlerAnimsDone = TRUE;
        }
    }

    // Clean up
    if (bgmRestored && battlerAnimsDone)
    {
        if (IsDoubleBattle() && !(gBattleTypeFlags & BATTLE_TYPE_MULTI))
            DestroySprite(&gSprites[gBattleControllerData[BATTLE_PARTNER(gActiveBattler)]]);
        DestroySprite(&gSprites[gBattleControllerData[gActiveBattler]]);

        gBattleSpritesDataPtr->animationData->introAnimActive = FALSE;
        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].bgmRestored = FALSE;
        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].healthboxSlideInStarted = FALSE;

        gBattlerControllerFuncs[gActiveBattler] = Intro_WaitForShinyAnimAndHealthbox;
    }
}

static void SwitchIn_CleanShinyAnimShowSubstitute(void)
{
    if (gSprites[gHealthboxSpriteIds[gActiveBattler]].callback == SpriteCallbackDummy
     && gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].finishedShinyMonAnim
     && gSprites[gBattlerSpriteIds[gActiveBattler]].callback == SpriteCallbackDummy)
    {
        CopyBattleSpriteInvisibility(gActiveBattler);

        // Reset shiny anim (even if it didn't occur)
        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].triedShinyMonAnim = FALSE;
        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].finishedShinyMonAnim = FALSE;
        FreeSpriteTilesByTag(ANIM_TAG_GOLD_STARS);
        FreeSpritePaletteByTag(ANIM_TAG_GOLD_STARS);

        // Check if Substitute should be shown
        if (gBattleSpritesDataPtr->battlerData[gActiveBattler].behindSubstitute)
            InitAndLaunchSpecialAnimation(gActiveBattler, gActiveBattler, gActiveBattler, B_ANIM_MON_TO_SUBSTITUTE);

        gBattlerControllerFuncs[gActiveBattler] = SwitchIn_HandleSoundAndEnd;
    }
}

static void SwitchIn_HandleSoundAndEnd(void)
{
    if (!gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].specialAnimActive
        && !IsCryPlayingOrClearCrySongs())
    {
        m4aMPlayVolumeControl(&gMPlayInfo_BGM, TRACKS_ALL, 0x100);
        HandleLowHpMusicChange(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], gActiveBattler);
        PlayerBufferExecCompleted();
    }
}

static void SwitchIn_TryShinyAnimShowHealthbox(void)
{
    // Start shiny animation if applicable
    if (!gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].triedShinyMonAnim
        && !gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].ballAnimActive)
        TryShinyAnimation(gActiveBattler, &gPlayerParty[gBattlerPartyIndexes[gActiveBattler]]);

    // Wait for ball anim, then show healthbox
    if (gSprites[gBattleControllerData[gActiveBattler]].callback == SpriteCallbackDummy
     && !gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].ballAnimActive)
    {
        DestroySprite(&gSprites[gBattleControllerData[gActiveBattler]]);
        UpdateHealthboxAttribute(gHealthboxSpriteIds[gActiveBattler], &gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], HEALTHBOX_ALL);
        StartHealthboxSlideIn(gActiveBattler);
        SetHealthboxSpriteVisible(gHealthboxSpriteIds[gActiveBattler]);
        gBattlerControllerFuncs[gActiveBattler] = SwitchIn_CleanShinyAnimShowSubstitute;
    }
}

void Task_PlayerController_RestoreBgmAfterCry(u8 taskId)
{
    if (!IsCryPlayingOrClearCrySongs())
    {
        m4aMPlayVolumeControl(&gMPlayInfo_BGM, TRACKS_ALL, 0x100);
        DestroyTask(taskId);
    }
}

static void CompleteOnHealthbarDone(void)
{
    s16 hpValue = MoveBattleBar(gActiveBattler, gHealthboxSpriteIds[gActiveBattler], HEALTH_BAR, 0);

    SetHealthboxSpriteVisible(gHealthboxSpriteIds[gActiveBattler]);

    if (hpValue != -1)
    {
        UpdateHpTextInHealthbox(gHealthboxSpriteIds[gActiveBattler], hpValue, HP_CURRENT);
    }
    else
    {
        HandleLowHpMusicChange(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], gActiveBattler);
        PlayerBufferExecCompleted();
    }
}

static void CompleteOnInactiveTextPrinter(void)
{
    if (!IsTextPrinterActive(B_WIN_MSG))
        PlayerBufferExecCompleted();
}

#define tExpTask_monId      data[0]
#define tExpTask_gainedExp  data[1]
#define tExpTask_battler    data[2]
#define tExpTask_frames     data[10]

static void Task_GiveExpToMon(u8 taskId)
{
    u32 monId = (u8)(gTasks[taskId].tExpTask_monId);
    u8 battler = gTasks[taskId].tExpTask_battler;
    s16 gainedExp = gTasks[taskId].tExpTask_gainedExp;

    if (IsDoubleBattle() == TRUE || monId != gBattlerPartyIndexes[battler]) // Give exp without moving the expbar.
    {
        struct Pokemon *mon = &gPlayerParty[monId];
        u16 species = GetMonData(mon, MON_DATA_SPECIES);
        u8 level = GetMonData(mon, MON_DATA_LEVEL);
        u32 currExp = GetMonData(mon, MON_DATA_EXP);
        u32 nextLvlExp = gExperienceTables[gSpeciesInfo[species].growthRate][level + 1];

        if (currExp + gainedExp >= nextLvlExp)
        {
            u8 savedActiveBattler;

            SetMonData(mon, MON_DATA_EXP, &nextLvlExp);
            CalculateMonStats(mon);
            gainedExp -= nextLvlExp - currExp;
            savedActiveBattler = gActiveBattler;
            gActiveBattler = battler;
            BtlController_EmitTwoReturnValues(B_COMM_TO_ENGINE, RET_VALUE_LEVELED_UP, gainedExp);
            gActiveBattler = savedActiveBattler;

            if (IsDoubleBattle() == TRUE
             && ((u16)(monId) == gBattlerPartyIndexes[battler] || (u16)(monId) == gBattlerPartyIndexes[BATTLE_PARTNER(battler)]))
                gTasks[taskId].func = Task_LaunchLvlUpAnim;
            else
                gTasks[taskId].func = DestroyExpTaskAndCompleteOnInactiveTextPrinter;
        }
        else
        {
            currExp += gainedExp;
            SetMonData(mon, MON_DATA_EXP, &currExp);
            gBattlerControllerFuncs[battler] = CompleteOnInactiveTextPrinter;
            DestroyTask(taskId);
        }
    }
    else
    {
        gTasks[taskId].func = Task_PrepareToGiveExpWithExpBar;
    }
}

static void Task_PrepareToGiveExpWithExpBar(u8 taskId)
{
    u8 monIndex = gTasks[taskId].tExpTask_monId;
    s32 gainedExp = gTasks[taskId].tExpTask_gainedExp;
    u8 battler = gTasks[taskId].tExpTask_battler;
    struct Pokemon *mon = &gPlayerParty[monIndex];
    u8 level = GetMonData(mon, MON_DATA_LEVEL);
    u16 species = GetMonData(mon, MON_DATA_SPECIES);
    u32 exp = GetMonData(mon, MON_DATA_EXP);
    u32 currLvlExp = gExperienceTables[gSpeciesInfo[species].growthRate][level];
    u32 expToNextLvl;

    exp -= currLvlExp;
    expToNextLvl = gExperienceTables[gSpeciesInfo[species].growthRate][level + 1] - currLvlExp;
    SetBattleBarStruct(battler, gHealthboxSpriteIds[battler], expToNextLvl, exp, -gainedExp);
    PlaySE(SE_EXP);
    gTasks[taskId].func = Task_GiveExpWithExpBar;
}

static void Task_GiveExpWithExpBar(u8 taskId)
{
    if (gTasks[taskId].tExpTask_frames < 13)
    {
        gTasks[taskId].tExpTask_frames++;
    }
    else
    {
        u8 monId = gTasks[taskId].tExpTask_monId;
        s16 gainedExp = gTasks[taskId].tExpTask_gainedExp;
        u8 battler = gTasks[taskId].tExpTask_battler;
        s16 newExpPoints;

        newExpPoints = MoveBattleBar(battler, gHealthboxSpriteIds[battler], EXP_BAR, 0);
        SetHealthboxSpriteVisible(gHealthboxSpriteIds[battler]);
        if (newExpPoints == -1) // The bar has been filled with given exp points.
        {
            u8 level;
            s32 currExp;
            u16 species;
            s32 expOnNextLvl;

            m4aSongNumStop(SE_EXP);
            level = GetMonData(&gPlayerParty[monId], MON_DATA_LEVEL);
            currExp = GetMonData(&gPlayerParty[monId], MON_DATA_EXP);
            species = GetMonData(&gPlayerParty[monId], MON_DATA_SPECIES);
            expOnNextLvl = gExperienceTables[gSpeciesInfo[species].growthRate][level + 1];

            if (currExp + gainedExp >= expOnNextLvl)
            {
                u8 savedActiveBattler;

                SetMonData(&gPlayerParty[monId], MON_DATA_EXP, &expOnNextLvl);
                CalculateMonStats(&gPlayerParty[monId]);
                gainedExp -= expOnNextLvl - currExp;
                savedActiveBattler = gActiveBattler;
                gActiveBattler = battler;
                BtlController_EmitTwoReturnValues(B_COMM_TO_ENGINE, RET_VALUE_LEVELED_UP, gainedExp);
                gActiveBattler = savedActiveBattler;
                gTasks[taskId].func = Task_LaunchLvlUpAnim;
            }
            else
            {
                currExp += gainedExp;
                SetMonData(&gPlayerParty[monId], MON_DATA_EXP, &currExp);
                gBattlerControllerFuncs[battler] = CompleteOnInactiveTextPrinter;
                DestroyTask(taskId);
            }
        }
    }
}

static void Task_LaunchLvlUpAnim(u8 taskId)
{
    u8 battler = gTasks[taskId].tExpTask_battler;
    u8 monIndex = gTasks[taskId].tExpTask_monId;

    if (IsDoubleBattle() == TRUE && monIndex == gBattlerPartyIndexes[BATTLE_PARTNER(battler)])
        battler ^= BIT_FLANK;

    InitAndLaunchSpecialAnimation(battler, battler, battler, B_ANIM_LVL_UP);
    gTasks[taskId].func = Task_UpdateLvlInHealthbox;
}

static void Task_UpdateLvlInHealthbox(u8 taskId)
{
    u8 battler = gTasks[taskId].tExpTask_battler;

    if (!gBattleSpritesDataPtr->healthBoxesData[battler].specialAnimActive)
    {
        u8 monIndex = gTasks[taskId].tExpTask_monId;

        GetMonData(&gPlayerParty[monIndex], MON_DATA_LEVEL);  // Unused return value.

        if (IsDoubleBattle() == TRUE && monIndex == gBattlerPartyIndexes[BATTLE_PARTNER(battler)])
            UpdateHealthboxAttribute(gHealthboxSpriteIds[BATTLE_PARTNER(battler)], &gPlayerParty[monIndex], HEALTHBOX_ALL);
        else
            UpdateHealthboxAttribute(gHealthboxSpriteIds[battler], &gPlayerParty[monIndex], HEALTHBOX_ALL);

        gTasks[taskId].func = DestroyExpTaskAndCompleteOnInactiveTextPrinter;
    }
}

static void DestroyExpTaskAndCompleteOnInactiveTextPrinter(u8 taskId)
{
    u8 monIndex;
    u8 battler;

    monIndex = gTasks[taskId].tExpTask_monId;
    GetMonData(&gPlayerParty[monIndex], MON_DATA_LEVEL);  // Unused return value.
    battler = gTasks[taskId].tExpTask_battler;
    gBattlerControllerFuncs[battler] = CompleteOnInactiveTextPrinter;
    DestroyTask(taskId);
}

static void FreeMonSpriteAfterFaintAnim(void)
{
    if (gSprites[gBattlerSpriteIds[gActiveBattler]].y + gSprites[gBattlerSpriteIds[gActiveBattler]].y2 > DISPLAY_HEIGHT)
    {
        u16 species = GetMonData(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_SPECIES);

        BattleGfxSfxDummy2(species);
        FreeOamMatrix(gSprites[gBattlerSpriteIds[gActiveBattler]].oam.matrixNum);
        DestroySprite(&gSprites[gBattlerSpriteIds[gActiveBattler]]);
        SetHealthboxSpriteInvisible(gHealthboxSpriteIds[gActiveBattler]);
        PlayerBufferExecCompleted();
    }
}

static void FreeMonSpriteAfterSwitchOutAnim(void)
{
    if (!gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].specialAnimActive)
    {
        FreeSpriteOamMatrix(&gSprites[gBattlerSpriteIds[gActiveBattler]]);
        DestroySprite(&gSprites[gBattlerSpriteIds[gActiveBattler]]);
        SetHealthboxSpriteInvisible(gHealthboxSpriteIds[gActiveBattler]);
        PlayerBufferExecCompleted();
    }
}

static void CompleteOnInactiveTextPrinter2(void)
{
    if (!IsTextPrinterActive(B_WIN_MSG))
        PlayerBufferExecCompleted();
}

static void OpenPartyMenuToChooseMon(void)
{
    if (!gPaletteFade.active)
    {
        u8 caseId;

        gBattlerControllerFuncs[gActiveBattler] = WaitForMonSelection;
        caseId = gTasks[gBattleControllerData[gActiveBattler]].data[0];
        DestroyTask(gBattleControllerData[gActiveBattler]);
        FreeAllWindowBuffers();
        OpenPartyMenuInBattle(caseId);
    }
}

static void WaitForMonSelection(void)
{
    if (gMain.callback2 == BattleMainCB2 && !gPaletteFade.active)
    {
        if (gPartyMenuUseExitCallback == TRUE)
            BtlController_EmitChosenMonReturnValue(B_COMM_TO_ENGINE, gSelectedMonPartyId, gBattlePartyCurrentOrder);
        else
            BtlController_EmitChosenMonReturnValue(B_COMM_TO_ENGINE, PARTY_SIZE, NULL);

        if ((gBattleBufferA[gActiveBattler][1] & 0xF) == 1)
            PrintLinkStandbyMsg();

        PlayerBufferExecCompleted();
    }
}

static void OpenBagAndChooseItem(void)
{
    if (!gPaletteFade.active)
    {
        gBattlerControllerFuncs[gActiveBattler] = CompleteWhenChoseItem;
        ReshowBattleScreenDummy();
        FreeAllWindowBuffers();
        CB2_BagMenuFromBattle();
    }
}

#ifdef DEBUG_MENUS
static void OpenBattleDebugMenu(void)
{
    if (!gPaletteFade.active)
    {
        gBattlerControllerFuncs[gActiveBattler] = ReturnToChooseAction;
        ReshowBattleScreenDummy();
        FreeAllWindowBuffers();
        SetMainCallback2(CB2_BattleDebugMenu);
    }
}
#endif

static void OpenFoeInfoScreen(void)
{
    if (!gPaletteFade.active)
    {
        gBattlerControllerFuncs[gActiveBattler] = ReturnToChooseAction;
        ReshowBattleScreenDummy();
        FreeAllWindowBuffers();
        SetMainCallback2(CB2_FoeInfoScreen);
    }
}

// Back on the battle screen from the debug menu or the foe info screen: show
// the action menu again, still answering the same ChooseAction request.
static void ReturnToChooseAction(void)
{
    if (gMain.callback2 == BattleMainCB2 && !gPaletteFade.active)
        PlayerHandleChooseAction();
}

static void CompleteWhenChoseItem(void)
{
    if (gMain.callback2 == BattleMainCB2 && !gPaletteFade.active)
    {
        BtlController_EmitOneReturnValue(B_COMM_TO_ENGINE, gSpecialVar_ItemId);
        PlayerBufferExecCompleted();
    }
}

static void CompleteOnSpecialAnimDone(void)
{
    if (!gDoingBattleAnim || !gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].specialAnimActive)
        PlayerBufferExecCompleted();
}

static void DoHitAnimBlinkSpriteEffect(void)
{
    u8 spriteId = gBattlerSpriteIds[gActiveBattler];

    if (gSprites[spriteId].data[1] == 32)
    {
        gSprites[spriteId].data[1] = 0;
        gSprites[spriteId].invisible = FALSE;
        gDoingBattleAnim = FALSE;
        PlayerBufferExecCompleted();
    }
    else
    {
        if ((gSprites[spriteId].data[1] % 4) == 0)
            gSprites[spriteId].invisible ^= 1;
        gSprites[spriteId].data[1]++;
    }
}

static void PlayerHandleYesNoInput(void)
{
    if (JOY_NEW(DPAD_UP) && gMultiUsePlayerCursor != 0)
    {
        PlaySE(SE_SELECT);
        BattleDestroyYesNoCursorAt(gMultiUsePlayerCursor);
        gMultiUsePlayerCursor = 0;
        BattleCreateYesNoCursorAt(0);
    }
    if (JOY_NEW(DPAD_DOWN) && gMultiUsePlayerCursor == 0)
    {
        PlaySE(SE_SELECT);
        BattleDestroyYesNoCursorAt(gMultiUsePlayerCursor);
        gMultiUsePlayerCursor = 1;
        BattleCreateYesNoCursorAt(1);
    }
    if (JOY_NEW(A_BUTTON))
    {
        HandleBattleWindow(YESNOBOX_X_Y, WINDOW_CLEAR);
        PlaySE(SE_SELECT);

        if (gMultiUsePlayerCursor != 0)
            BtlController_EmitTwoReturnValues(B_COMM_TO_ENGINE, 0xE, 0);
        else
            BtlController_EmitTwoReturnValues(B_COMM_TO_ENGINE, 0xD, 0);

        PlayerBufferExecCompleted();
    }
    if (JOY_NEW(B_BUTTON))
    {
        HandleBattleWindow(YESNOBOX_X_Y, WINDOW_CLEAR);
        PlaySE(SE_SELECT);
        PlayerBufferExecCompleted();
    }
}

// The move screen's windows sit on art, so each one starts from a copy of the art under it
// (graphics/battle_interface/move_grid.png and move_info.png, made by tools/battle_ui/gen_textbox.py).
static void MoveScreenWindowStart(u8 windowId)
{
    switch (windowId)
    {
    case B_WIN_MOVE_NAME_1:
        CopyToWindowPixelBuffer(windowId, gBattleMoveGridGfx, 15 * 6 * TILE_SIZE_4BPP, 0);
        break;
    case B_WIN_MOVE_TYPE:
        CopyToWindowPixelBuffer(windowId, gBattleMoveInfoGfx, 5 * 2 * TILE_SIZE_4BPP, 0);
        break;
    case B_WIN_TP_COST:
        CopyToWindowPixelBuffer(windowId, gBattleMoveInfoGfx + 10 * TILE_SIZE_4BPP, 3 * 2 * TILE_SIZE_4BPP, 0);
        break;
    case B_WIN_TP_LEFT:
        CopyToWindowPixelBuffer(windowId, gBattleMoveInfoGfx + 16 * TILE_SIZE_4BPP, 3 * 3 * TILE_SIZE_4BPP, 0);
        break;
    }
}

static void MoveScreenWindowFinish(u8 windowId)
{
    PutWindowTilemap(windowId);
    CopyWindowToVram(windowId, COPYWIN_FULL);
}

static void MoveSelectionDisplayMoveNames(void)
{
    s32 i;
    struct ChooseMoveStruct *moveInfo = (struct ChooseMoveStruct *)(&gBattleBufferA[gActiveBattler][4]);
    gNumberOfMovesToChoose = 0;

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        sMoveNameStyles[i] = MOVE_NAME_NORMAL;
        if (moveInfo->moves[i] != MOVE_NONE)
            gNumberOfMovesToChoose++;
    }
    MoveSelectionDrawMoveNames();
}

// One number of the move info box, right-aligned; 0 prints "-"
static void PrintMoveStat(u8 value, u8 y, const u8 *colors)
{
    static const u8 sText_NoValue[] = _("-");
    const u8 *str = sText_NoValue;

    if (value != 0)
    {
        ConvertIntToDecimalStringN(gDisplayedStringBattle, value, STR_CONV_MODE_LEFT_ALIGN, 3);
        str = gDisplayedStringBattle;
    }
    AddTextPrinterParameterized4(B_WIN_TP_LEFT, FONT_SMALL, 22 - GetStringWidth(FONT_SMALL, str, 0), y, 0, 0, colors, TEXT_SKIP_DRAW, str);
}

static void MoveSelectionDisplayTpCost(void)
{
    struct ChooseMoveStruct *moveInfo = (struct ChooseMoveStruct *)(&gBattleBufferA[gActiveBattler][4]);
    u8 colors[3] = {TEXT_COLOR_TRANSPARENT, B_UI_PAL_TYPE, TEXT_COLOR_TRANSPARENT};
    u8 width;

    MoveScreenWindowStart(B_WIN_TP_COST);
    MoveScreenWindowStart(B_WIN_TP_LEFT);
    if (gBattleBufferA[gActiveBattler][2] != TRUE) // check if we didn't want to display the TP numbers
    {
        // The selected move's TP cost, centered in its field. It turns yellow when it can be paid
        // only once more and red when it can't be paid.
        // A sealed Special shows no cost. The TP bar previews the cost too.
        SetTpBarPreview(gActiveBattler, GetSelectedMove() != MOVE_NONE ? GetSelectedTpCost() : 0);
        if (GetSelectedMove() != MOVE_NONE)
        {
            u8 tpCost = GetSelectedTpCost();

            SetTpCostPaletteInMoveSelection(tpCost);
            if (GetTPCostToCurrentTPState(tpCost, moveInfo->currentTp) != 3)
                colors[1] = 12;
            ConvertIntToDecimalStringN(gDisplayedStringBattle, tpCost, STR_CONV_MODE_LEFT_ALIGN, 3);
            width = GetStringWidth(FONT_SMALL, gDisplayedStringBattle, 0);
            AddTextPrinterParameterized4(B_WIN_TP_COST, FONT_SMALL, 13 - width / 2, 2, 0, 0, colors, TEXT_SKIP_DRAW, gDisplayedStringBattle);
        }

        // The selected move's power over its accuracy, right-aligned. "-" when the move has no
        // set power (status moves, and power 1 for fixed or variable damage) or never misses.
        const struct MoveInfo *move = &gMovesInfo[GetSelectedMove()];

        colors[1] = B_UI_PAL_TYPE;
        PrintMoveStat(move->power > 1 ? move->power : 0, 3, colors);
        PrintMoveStat(move->accuracy, 12, colors);
    }
    MoveScreenWindowFinish(B_WIN_TP_COST);
    MoveScreenWindowFinish(B_WIN_TP_LEFT);
}

// Type names are printed in a color for their type, in BG palette 5 slot B_UI_PAL_TYPE
// (and on the foe info screen)
const u16 gTypeTextColors[NUMBER_OF_MON_TYPES] =
{
    [TYPE_NORMAL]   = RGB(24, 24, 27),
    [TYPE_FIGHTING] = RGB(30, 17, 13),
    [TYPE_FLYING]   = RGB(23, 21, 31),
    [TYPE_POISON]   = RGB(26, 17, 27),
    [TYPE_GROUND]   = RGB(29, 25, 15),
    [TYPE_ROCK]     = RGB(26, 23, 12),
    [TYPE_BUG]      = RGB(23, 26, 8),
    [TYPE_GHOST]    = RGB(21, 18, 26),
    [TYPE_STEEL]    = RGB(25, 25, 28),
    [TYPE_MYSTERY]  = RGB(17, 24, 22),
    [TYPE_FIRE]     = RGB(31, 18, 8),
    [TYPE_WATER]    = RGB(13, 21, 31),
    [TYPE_GRASS]    = RGB(20, 24, 13),
    [TYPE_ELECTRIC] = RGB(31, 26, 8),
    [TYPE_PSYCHIC]  = RGB(31, 17, 21),
    [TYPE_ICE]      = RGB(21, 28, 28),
    [TYPE_DRAGON]   = RGB(20, 16, 31),
    [TYPE_DARK]     = RGB(23, 20, 18),
};

static const u8 sText_SpecialSealed[] = _("???");

static void MoveSelectionDisplayMoveType(void)
{
    u16 move = GetSelectedMove();
    u8 type = gMovesInfo[move].type;
    const u8 *typeName = gTypeNames[type];
    u16 color = gTypeTextColors[type];
    u8 colors[3] = {TEXT_COLOR_TRANSPARENT, B_UI_PAL_TYPE, TEXT_COLOR_TRANSPARENT};
    u8 fontId = FONT_SMALL;

    // A sealed Special hides its type: grey "???", the textbox's grey (slot 13)
    if (move == MOVE_NONE)
    {
        typeName = sText_SpecialSealed;
        color = RGB(17, 17, 20);
    }

    // The whole info box (rim, type tab, TP fields, icon) is drawn in the type's color
    gPlttBufferUnfaded[BG_PLTT_ID(5) + B_UI_PAL_TYPE] = color;
    gPlttBufferFaded[BG_PLTT_ID(5) + B_UI_PAL_TYPE] = color;
    if (GetStringWidth(fontId, typeName, 0) > TYPE_NAME_MAX_WIDTH)
        fontId = FONT_SMALL_NARROW;
    MoveScreenWindowStart(B_WIN_MOVE_TYPE);
    AddTextPrinterParameterized4(B_WIN_MOVE_TYPE, fontId, 2, 4, 0, 0, colors, TEXT_SKIP_DRAW, typeName);
    MoveScreenWindowFinish(B_WIN_MOVE_TYPE);
    MoveSelectionDisplayEffectiveness();
}

enum {
    EFFECT_ICON_NEUTRAL,
    EFFECT_ICON_WEAK,
    EFFECT_ICON_STRONG,
    EFFECT_ICON_NONE,
    EFFECT_ICON_HIDDEN,
};

// How the move's type matches up against the defender, read from gTypeEffectiveness.
// Abilities and held items are left out: the icon shows the type chart only.
static u8 GetMoveEffectivenessIcon(u16 move, u8 defender)
{
    s32 i;
    u16 multiplier = TYPE_MUL_NORMAL;
    u8 moveType = gMovesInfo[move].type;
    u8 type1 = gBattleMons[defender].types[0];
    u8 type2 = gBattleMons[defender].types[1];

    if (move == MOVE_NONE || gMovesInfo[move].power == 0)
        return EFFECT_ICON_HIDDEN;

    for (i = 0; TYPE_EFFECT_ATK_TYPE(i) != TYPE_ENDTABLE; i += 3)
    {
        if (TYPE_EFFECT_ATK_TYPE(i) != moveType)
            continue;
        if (TYPE_EFFECT_DEF_TYPE(i) == type1)
            multiplier = multiplier * TYPE_EFFECT_MULTIPLIER(i) / TYPE_MUL_NORMAL;
        if (TYPE_EFFECT_DEF_TYPE(i) == type2 && type1 != type2)
            multiplier = multiplier * TYPE_EFFECT_MULTIPLIER(i) / TYPE_MUL_NORMAL;
    }

    if (multiplier == TYPE_MUL_NO_EFFECT)
        return EFFECT_ICON_NONE;
    if (multiplier < TYPE_MUL_NORMAL)
        return EFFECT_ICON_WEAK;
    if (multiplier > TYPE_MUL_NORMAL)
        return EFFECT_ICON_STRONG;
    return EFFECT_ICON_NEUTRAL;
}

// Draws the effectiveness icon next to the move type: = neutral, down arrow not very effective,
// up arrow super effective, x no effect. Single battles only, since doubles have two foes.
static void MoveSelectionDisplayEffectiveness(void)
{
    u16 tiles[4];
    u8 icon = EFFECT_ICON_HIDDEN;
    s32 i;

    if (!(gBattleTypeFlags & BATTLE_TYPE_DOUBLE))
        icon = GetMoveEffectivenessIcon(GetSelectedMove(),
                                        GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT));

    for (i = 0; i < 4; i++)
    {
        if (icon == EFFECT_ICON_HIDDEN)
            tiles[i] = B_TEXTBOX_TILE_NAVY;
        else
            tiles[i] = B_TEXTBOX_TILE_EFFECT_ICON + icon * 4 + i;
    }
    CopyToBgTilemapBufferRect_ChangePalette(0, tiles, 27, 57, 2, 2, 5);
    CopyBgTilemapBufferToVram(0);
}

// Prints one move name into its cell of the move grid. A name too long for one line breaks at its
// last space that fits, as in the mockup's "FRAGRANCE / TRAP"; one with no space uses a smaller font.
static void MoveSelectionPrintMoveName(u8 position)
{
    struct ChooseMoveStruct *moveInfo = (struct ChooseMoveStruct *)(&gBattleBufferA[gActiveBattler][4]);
    u8 colors[3] = {TEXT_COLOR_TRANSPARENT, TEXTBOX_PAL_WHITE, TEXT_COLOR_TRANSPARENT};
    u8 x = (position & 1) ? 62 : 5;
    u8 y = (position & 2) ? 26 : 6;
    u8 fontId = FONT_SMALL;
    u8 *line2 = NULL;
    u8 *c;

    StringCopy(gDisplayedStringBattle, gMovesInfo[moveInfo->moves[position]].name);
    if (GetStringWidth(fontId, gDisplayedStringBattle, 0) > MOVE_NAME_MAX_WIDTH)
    {
        for (c = gDisplayedStringBattle; *c != EOS; c++)
        {
            if (*c != CHAR_SPACE)
                continue;
            *c = EOS;
            if (GetStringWidth(fontId, gDisplayedStringBattle, 0) <= MOVE_NAME_MAX_WIDTH)
                line2 = c;
            *c = CHAR_SPACE;
        }
        if (line2 != NULL)
            *line2++ = EOS;
        else
        {
            fontId = FONT_SMALL_NARROW;
        }
    }
    AddTextPrinterParameterized4(B_WIN_MOVE_NAME_1, fontId, x, y, 0, 0, colors, TEXT_SKIP_DRAW, gDisplayedStringBattle);
    if (line2 != NULL)
        AddTextPrinterParameterized4(B_WIN_MOVE_NAME_1, fontId, x, y + 8, 0, 0, colors, TEXT_SKIP_DRAW, line2);

    // The selected name fades from white to LCD green in its bottom two rows
    if (sMoveNameStyles[position] == MOVE_NAME_SELECTED)
    {
        RecolorWindowPixels(B_WIN_MOVE_NAME_1, TEXTBOX_PAL_WHITE, TEXTBOX_PAL_LCD, x, y + 8, MOVE_NAME_MAX_WIDTH + 2, 2);
        if (line2 != NULL)
            RecolorWindowPixels(B_WIN_MOVE_NAME_1, TEXTBOX_PAL_WHITE, TEXTBOX_PAL_LCD, x, y + 16, MOVE_NAME_MAX_WIDTH + 2, 2);
    }
}

static void RecolorWindowPixels(u8 windowId, u8 from, u8 to, u16 left, u16 top, u16 width, u16 height)
{
    u8 *tileData = gWindows[windowId].tileData;
    u16 widthInTiles = gWindows[windowId].window.width;
    u16 x, y;

    for (y = top; y < top + height; y++)
    {
        for (x = left; x < left + width; x++)
        {
            u8 *pixels = &tileData[((y / 8) * widthInTiles + x / 8) * TILE_SIZE_4BPP + (y % 8) * 4 + (x % 8) / 2];
            if (x & 1)
            {
                if ((*pixels >> 4) == from)
                    *pixels = (*pixels & 0xF) | (to << 4);
            }
            else if ((*pixels & 0xF) == from)
            {
                *pixels = (*pixels & 0xF0) | to;
            }
        }
    }
}

// Draws the 2x2 move grid: the scanlined screen, a rounded navy highlight behind the selected
// move, and the four names.
static void MoveSelectionDrawMoveNames(void)
{
    s32 i;

    MoveScreenWindowStart(B_WIN_MOVE_NAME_1);
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        if (sMoveNameStyles[i] != MOVE_NAME_NORMAL)
        {
            u8 fill = PIXEL_FILL(TEXTBOX_PAL_NAVY);
            u8 x = (i & 1) ? 59 : 3;
            u8 y = (i & 2) ? 27 : 7;
            u8 height = (i & 2) ? 18 : 19;

            FillWindowPixelRect(B_WIN_MOVE_NAME_1, fill, x + 1, y, MOVE_CELL_WIDTH - 2, 1);
            FillWindowPixelRect(B_WIN_MOVE_NAME_1, fill, x, y + 1, MOVE_CELL_WIDTH, height - 2);
            FillWindowPixelRect(B_WIN_MOVE_NAME_1, fill, x + 1, y + height - 1, MOVE_CELL_WIDTH - 2, 1);
        }
        MoveSelectionPrintMoveName(i);
    }
    MoveScreenWindowFinish(B_WIN_MOVE_NAME_1);
}

// baseTileNum was Emerald's cursor graphic and is unused
static void MoveSelectionCreateCursorAt(u8 cursorPosition, u8 baseTileNum)
{
    sMoveNameStyles[cursorPosition] = MOVE_NAME_SELECTED;
    MoveSelectionDrawMoveNames();
}

// The Signature page: one full-width row in place of the grid. Awakened, the Signature's name in
// gold inside an amber double frame (textbox palette slots 9 and 10); sealed, a dashed grey
// frame (slot 13) around "???". No new tiles or palettes.
#define SPECIAL_ROW_X  3
#define SPECIAL_ROW_Y 16
#define SPECIAL_ROW_W 112
#define SPECIAL_ROW_H 20

static const u8 sText_SpecialPage[] = _("SIGNATURE");

static void MoveSelectionDrawSpecialPage(void)
{
    u16 move = GetBattlerSpecialMove(gActiveBattler);
    u8 colors[3] = {TEXT_COLOR_TRANSPARENT, TEXTBOX_PAL_GOLD, TEXT_COLOR_TRANSPARENT};
    const u8 *name;
    s32 i;

    MoveScreenWindowStart(B_WIN_MOVE_NAME_1);
    if (move != MOVE_NONE)
    {
        FillWindowPixelRect(B_WIN_MOVE_NAME_1, PIXEL_FILL(TEXTBOX_PAL_ORANGE), SPECIAL_ROW_X + 1, SPECIAL_ROW_Y, SPECIAL_ROW_W - 2, SPECIAL_ROW_H);
        FillWindowPixelRect(B_WIN_MOVE_NAME_1, PIXEL_FILL(TEXTBOX_PAL_ORANGE), SPECIAL_ROW_X, SPECIAL_ROW_Y + 1, SPECIAL_ROW_W, SPECIAL_ROW_H - 2);
        FillWindowPixelRect(B_WIN_MOVE_NAME_1, PIXEL_FILL(TEXTBOX_PAL_GOLD), SPECIAL_ROW_X + 1, SPECIAL_ROW_Y + 1, SPECIAL_ROW_W - 2, SPECIAL_ROW_H - 2);
        FillWindowPixelRect(B_WIN_MOVE_NAME_1, PIXEL_FILL(TEXTBOX_PAL_NAVY), SPECIAL_ROW_X + 2, SPECIAL_ROW_Y + 2, SPECIAL_ROW_W - 4, SPECIAL_ROW_H - 4);
        name = gMovesInfo[move].name;
    }
    else
    {
        for (i = 0; i < SPECIAL_ROW_W; i += 4)
        {
            FillWindowPixelRect(B_WIN_MOVE_NAME_1, PIXEL_FILL(TEXTBOX_PAL_GREY), SPECIAL_ROW_X + i, SPECIAL_ROW_Y, 2, 1);
            FillWindowPixelRect(B_WIN_MOVE_NAME_1, PIXEL_FILL(TEXTBOX_PAL_GREY), SPECIAL_ROW_X + i, SPECIAL_ROW_Y + SPECIAL_ROW_H - 1, 2, 1);
        }
        for (i = 0; i < SPECIAL_ROW_H; i += 4)
        {
            FillWindowPixelRect(B_WIN_MOVE_NAME_1, PIXEL_FILL(TEXTBOX_PAL_GREY), SPECIAL_ROW_X, SPECIAL_ROW_Y + i, 1, 2);
            FillWindowPixelRect(B_WIN_MOVE_NAME_1, PIXEL_FILL(TEXTBOX_PAL_GREY), SPECIAL_ROW_X + SPECIAL_ROW_W - 1, SPECIAL_ROW_Y + i, 1, 2);
        }
        colors[1] = TEXTBOX_PAL_GREY;
        name = sText_SpecialSealed;
    }
    AddTextPrinterParameterized4(B_WIN_MOVE_NAME_1, FONT_SMALL, SPECIAL_ROW_X + (SPECIAL_ROW_W - GetStringWidth(FONT_SMALL, name, 0)) / 2,
                                 SPECIAL_ROW_Y + 3, 0, 0, colors, TEXT_SKIP_DRAW, name);
    colors[1] = TEXTBOX_PAL_LCD;
    AddTextPrinterParameterized4(B_WIN_MOVE_NAME_1, FONT_SMALL, SPECIAL_ROW_X + 2, 3, 0, 0, colors, TEXT_SKIP_DRAW, sText_SpecialPage);
    MoveScreenWindowFinish(B_WIN_MOVE_NAME_1);
}

static void MoveSelectionDestroyCursorAt(u8 cursorPosition)
{
    sMoveNameStyles[cursorPosition] = MOVE_NAME_NORMAL;
    MoveSelectionDrawMoveNames();
}

// Buttons in B_WIN_ACTION_MENU, from the mockup: FIGHT x2-39, PKMN x41-79, rows 6-19 and 21-34
#define ACTION_BUTTON_HEIGHT 14

static const u8 sActionButtonX[] = {2, 41, 2, 41};
static const u8 sActionButtonWidth[] = {38, 39, 38, 39};

static const u8 *const sActionMenuLabels[] =
{
    gText_BattleMenuFight, gText_BattleMenuPkmn,
    gText_BattleMenuItem,  gText_BattleMenuRun,
};

static const u8 *const sSafariMenuLabels[] =
{
    gText_SafariMenuBall, gText_SafariMenuNone,
    gText_SafariMenuNear, gText_BattleMenuRun,
};

// The action menu is four cyan buttons on the art copied into B_WIN_ACTION_MENU.
// Each has a white lower edge that curls up at the group's outer corner; the selected one turns navy.
static void DrawActionButton(u8 position, bool8 selected)
{
    const u8 *label = (gBattleTypeFlags & BATTLE_TYPE_SAFARI) ? sSafariMenuLabels[position] : sActionMenuLabels[position];
    u8 x = sActionButtonX[position];
    u8 y = (position & 2) ? 21 : 6;
    u8 width = sActionButtonWidth[position];
    u8 outerX = (position & 1) ? x + width - 1 : x;
    u8 colors[3] = {TEXT_COLOR_TRANSPARENT, TEXTBOX_PAL_WHITE, TEXT_COLOR_TRANSPARENT};

    FillWindowPixelRect(B_WIN_ACTION_MENU, PIXEL_FILL(TEXTBOX_PAL_NAVY), x, y, width, ACTION_BUTTON_HEIGHT);
    if (!selected)
    {
        FillWindowPixelRect(B_WIN_ACTION_MENU, PIXEL_FILL(TEXTBOX_PAL_CYAN), x, y + 1, width, ACTION_BUTTON_HEIGHT - 2);
        FillWindowPixelRect(B_WIN_ACTION_MENU, PIXEL_FILL(TEXTBOX_PAL_WHITE), x + 1, y + ACTION_BUTTON_HEIGHT - 1, width - 2, 1);
        if (position & 2) // bottom row: square top corner on the inside
        {
            FillWindowPixelRect(B_WIN_ACTION_MENU, PIXEL_FILL(TEXTBOX_PAL_CYAN), x + 1, y, width - 2, 1);
            FillWindowPixelRect(B_WIN_ACTION_MENU, PIXEL_FILL(TEXTBOX_PAL_CYAN), (position & 1) ? x : x + width - 1, y, 1, 1);
        }
        else // top row: square bottom corner on the inside
        {
            FillWindowPixelRect(B_WIN_ACTION_MENU, PIXEL_FILL(TEXTBOX_PAL_CYAN), x + 1, y, width - 2, 1);
            FillWindowPixelRect(B_WIN_ACTION_MENU, PIXEL_FILL(TEXTBOX_PAL_WHITE), (position & 1) ? x : x + width - 1, y + ACTION_BUTTON_HEIGHT - 1, 1, 1);
        }
        FillWindowPixelRect(B_WIN_ACTION_MENU, PIXEL_FILL(TEXTBOX_PAL_WHITE), outerX, y + ACTION_BUTTON_HEIGHT - 2, 1, 1);
    }
    AddTextPrinterParameterized4(B_WIN_ACTION_MENU, FONT_SMALL, x + (width - GetStringWidth(FONT_SMALL, label, 0)) / 2, y,
                                 0, 0, colors, TEXT_SKIP_DRAW, label);
    PutWindowTilemap(B_WIN_ACTION_MENU);
    CopyWindowToVram(B_WIN_ACTION_MENU, COPYWIN_FULL);
}

void ActionSelectionDrawMenu(void)
{
    s32 i;

    CopyToWindowPixelBuffer(B_WIN_ACTION_MENU, gBattleActionMenuGfx, 10 * 5 * TILE_SIZE_4BPP, 0);
    for (i = 0; i < 4; i++)
        DrawActionButton(i, FALSE);
}

void ActionSelectionCreateCursorAt(u8 cursorPosition, u8 baseTileNum)
{
    DrawActionButton(cursorPosition, TRUE);
}

void ActionSelectionDestroyCursorAt(u8 cursorPosition)
{
    DrawActionButton(cursorPosition, FALSE);
}

void CB2_SetUpReshowBattleScreenAfterMenu(void)
{
    SetMainCallback2(ReshowBattleScreenAfterMenu);
}

void CB2_SetUpReshowBattleScreenAfterMenu2(void)
{
    SetMainCallback2(ReshowBattleScreenAfterMenu);
}

static void CompleteOnFinishedStatusAnimation(void)
{
    if (!gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].statusAnimActive)
        PlayerBufferExecCompleted();
}

static void CompleteOnFinishedBattleAnimation(void)
{
    if (!gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].animFromTableActive)
        PlayerBufferExecCompleted();
}

static void PrintLinkStandbyMsg(void)
{
    if (gBattleTypeFlags & BATTLE_TYPE_LINK)
    {
        gBattle_BG0_X = 0;
        gBattle_BG0_Y = 0;
        BattlePutTextOnWindow(gText_LinkStandby, B_WIN_MSG);
    }
}

static void PlayerHandleGetMonData(void)
{
    u8 monData[sizeof(struct Pokemon) * 2 + 56]; // this allows to get full data of two Pokémon, trying to get more will result in overwriting data
    u32 size = 0;
    u8 monToCheck;
    s32 i;

    if (gBattleBufferA[gActiveBattler][2] == 0)
    {
        size += CopyPlayerMonData(gBattlerPartyIndexes[gActiveBattler], monData);
    }
    else
    {
        monToCheck = gBattleBufferA[gActiveBattler][2];
        for (i = 0; i < PARTY_SIZE; i++)
        {
            if (monToCheck & 1)
                size += CopyPlayerMonData(i, monData + size);
            monToCheck >>= 1;
        }
    }
    BtlController_EmitDataTransfer(B_COMM_TO_ENGINE, size, monData);
    PlayerBufferExecCompleted();
}

static u32 CopyPlayerMonData(u8 monId, u8 *dst)
{
    struct BattlePokemon battleMon;
    u8 nickname[POKEMON_NAME_BUFFER_SIZE];
    u8 *src;
    s16 data16;
    u32 data32;
    s32 size = 0;

    switch (gBattleBufferA[gActiveBattler][1])
    {
    case REQUEST_ALL_BATTLE:
        battleMon.species = GetMonData(&gPlayerParty[monId], MON_DATA_SPECIES);
        battleMon.item = GetMonData(&gPlayerParty[monId], MON_DATA_HELD_ITEM);
        for (size = 0; size < MAX_MON_MOVES; size++)
        {
            battleMon.moves[size] = GetMonData(&gPlayerParty[monId], MON_DATA_MOVE1 + size);
        }
        battleMon.friendship = GetMonData(&gPlayerParty[monId], MON_DATA_FRIENDSHIP);
        battleMon.experience = GetMonData(&gPlayerParty[monId], MON_DATA_EXP);
        battleMon.hpIV = GetMonData(&gPlayerParty[monId], MON_DATA_HP_IV);
        battleMon.attackIV = GetMonData(&gPlayerParty[monId], MON_DATA_ATK_IV);
        battleMon.defenseIV = GetMonData(&gPlayerParty[monId], MON_DATA_DEF_IV);
        battleMon.speedIV = GetMonData(&gPlayerParty[monId], MON_DATA_SPEED_IV);
        battleMon.spAttackIV = GetMonData(&gPlayerParty[monId], MON_DATA_SPATK_IV);
        battleMon.spDefenseIV = GetMonData(&gPlayerParty[monId], MON_DATA_SPDEF_IV);
        battleMon.personality = GetMonData(&gPlayerParty[monId], MON_DATA_PERSONALITY);
        battleMon.status1 = GetMonData(&gPlayerParty[monId], MON_DATA_STATUS);
        battleMon.level = GetMonData(&gPlayerParty[monId], MON_DATA_LEVEL);
        battleMon.hp = GetMonData(&gPlayerParty[monId], MON_DATA_HP);
        battleMon.maxHP = GetMonData(&gPlayerParty[monId], MON_DATA_MAX_HP);
        battleMon.attack = GetMonData(&gPlayerParty[monId], MON_DATA_ATK);
        battleMon.defense = GetMonData(&gPlayerParty[monId], MON_DATA_DEF);
        battleMon.speed = GetMonData(&gPlayerParty[monId], MON_DATA_SPEED);
        battleMon.spAttack = GetMonData(&gPlayerParty[monId], MON_DATA_SPATK);
        battleMon.spDefense = GetMonData(&gPlayerParty[monId], MON_DATA_SPDEF);
        battleMon.isEgg = GetMonData(&gPlayerParty[monId], MON_DATA_IS_EGG);
        battleMon.abilityNum = GetMonData(&gPlayerParty[monId], MON_DATA_ABILITY_NUM);
        battleMon.otId = GetMonData(&gPlayerParty[monId], MON_DATA_OT_ID);
        GetMonData(&gPlayerParty[monId], MON_DATA_NICKNAME, nickname);
        StringCopy_Nickname(battleMon.nickname, nickname);
        GetMonData(&gPlayerParty[monId], MON_DATA_OT_NAME, battleMon.otName);
        src = (u8 *)&battleMon;
        for (size = 0; size < sizeof(battleMon); size++)
            dst[size] = src[size];
        break;
    case REQUEST_SPECIES_BATTLE:
        data16 = GetMonData(&gPlayerParty[monId], MON_DATA_SPECIES);
        dst[0] = data16;
        dst[1] = data16 >> 8;
        size = 2;
        break;
    case REQUEST_HELDITEM_BATTLE:
        data16 = GetMonData(&gPlayerParty[monId], MON_DATA_HELD_ITEM);
        dst[0] = data16;
        dst[1] = data16 >> 8;
        size = 2;
        break;
    case REQUEST_MOVES_BATTLE:
        for (size = 0; size < MAX_MON_MOVES; size++)
        {
            data16 = GetMonData(&gPlayerParty[monId], MON_DATA_MOVE1 + size);
            dst[size * 2] = data16;
            dst[size * 2 + 1] = data16 >> 8;
        }
        size *= 2;
        break;
    case REQUEST_MOVE1_BATTLE:
    case REQUEST_MOVE2_BATTLE:
    case REQUEST_MOVE3_BATTLE:
    case REQUEST_MOVE4_BATTLE:
        data16 = GetMonData(&gPlayerParty[monId], MON_DATA_MOVE1 + gBattleBufferA[gActiveBattler][1] - REQUEST_MOVE1_BATTLE);
        dst[0] = data16;
        dst[1] = data16 >> 8;
        size = 2;
        break;
    case REQUEST_OTID_BATTLE:
        data32 = GetMonData(&gPlayerParty[monId], MON_DATA_OT_ID);
        dst[0] = (data32 & 0x000000FF);
        dst[1] = (data32 & 0x0000FF00) >> 8;
        dst[2] = (data32 & 0x00FF0000) >> 16;
        size = 3;
        break;
    case REQUEST_EXP_BATTLE:
        data32 = GetMonData(&gPlayerParty[monId], MON_DATA_EXP);
        dst[0] = (data32 & 0x000000FF);
        dst[1] = (data32 & 0x0000FF00) >> 8;
        dst[2] = (data32 & 0x00FF0000) >> 16;
        size = 3;
        break;
    case REQUEST_HP_EV_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_HP_EV);
        size = 1;
        break;
    case REQUEST_ATK_EV_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_ATK_EV);
        size = 1;
        break;
    case REQUEST_DEF_EV_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_DEF_EV);
        size = 1;
        break;
    case REQUEST_SPEED_EV_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_SPEED_EV);
        size = 1;
        break;
    case REQUEST_SPATK_EV_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_SPATK_EV);
        size = 1;
        break;
    case REQUEST_SPDEF_EV_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_SPDEF_EV);
        size = 1;
        break;
    case REQUEST_FRIENDSHIP_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_FRIENDSHIP);
        size = 1;
        break;
    case REQUEST_POKERUS_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_POKERUS);
        size = 1;
        break;
    case REQUEST_MET_LOCATION_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_MET_LOCATION);
        size = 1;
        break;
    case REQUEST_MET_LEVEL_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_MET_LEVEL);
        size = 1;
        break;
    case REQUEST_MET_GAME_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_MET_GAME);
        size = 1;
        break;
    case REQUEST_POKEBALL_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_POKEBALL);
        size = 1;
        break;
    case REQUEST_ALL_IVS_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_HP_IV);
        dst[1] = GetMonData(&gPlayerParty[monId], MON_DATA_ATK_IV);
        dst[2] = GetMonData(&gPlayerParty[monId], MON_DATA_DEF_IV);
        dst[3] = GetMonData(&gPlayerParty[monId], MON_DATA_SPEED_IV);
        dst[4] = GetMonData(&gPlayerParty[monId], MON_DATA_SPATK_IV);
        dst[5] = GetMonData(&gPlayerParty[monId], MON_DATA_SPDEF_IV);
        size = 6;
        break;
    case REQUEST_HP_IV_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_HP_IV);
        size = 1;
        break;
    case REQUEST_ATK_IV_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_ATK_IV);
        size = 1;
        break;
    case REQUEST_DEF_IV_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_DEF_IV);
        size = 1;
        break;
    case REQUEST_SPEED_IV_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_SPEED_IV);
        size = 1;
        break;
    case REQUEST_SPATK_IV_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_SPATK_IV);
        size = 1;
        break;
    case REQUEST_SPDEF_IV_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_SPDEF_IV);
        size = 1;
        break;
    case REQUEST_PERSONALITY_BATTLE:
        data32 = GetMonData(&gPlayerParty[monId], MON_DATA_PERSONALITY);
        dst[0] = (data32 & 0x000000FF);
        dst[1] = (data32 & 0x0000FF00) >> 8;
        dst[2] = (data32 & 0x00FF0000) >> 16;
        dst[3] = (data32 & 0xFF000000) >> 24;
        size = 4;
        break;
    case REQUEST_CHECKSUM_BATTLE:
        data16 = GetMonData(&gPlayerParty[monId], MON_DATA_CHECKSUM);
        dst[0] = data16;
        dst[1] = data16 >> 8;
        size = 2;
        break;
    case REQUEST_STATUS_BATTLE:
        data32 = GetMonData(&gPlayerParty[monId], MON_DATA_STATUS);
        dst[0] = (data32 & 0x000000FF);
        dst[1] = (data32 & 0x0000FF00) >> 8;
        dst[2] = (data32 & 0x00FF0000) >> 16;
        dst[3] = (data32 & 0xFF000000) >> 24;
        size = 4;
        break;
    case REQUEST_LEVEL_BATTLE:
        dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_LEVEL);
        size = 1;
        break;
    case REQUEST_HP_BATTLE:
        data16 = GetMonData(&gPlayerParty[monId], MON_DATA_HP);
        dst[0] = data16;
        dst[1] = data16 >> 8;
        size = 2;
        break;
    case REQUEST_MAX_HP_BATTLE:
        data16 = GetMonData(&gPlayerParty[monId], MON_DATA_MAX_HP);
        dst[0] = data16;
        dst[1] = data16 >> 8;
        size = 2;
        break;
    case REQUEST_ATK_BATTLE:
        data16 = GetMonData(&gPlayerParty[monId], MON_DATA_ATK);
        dst[0] = data16;
        dst[1] = data16 >> 8;
        size = 2;
        break;
    case REQUEST_DEF_BATTLE:
        data16 = GetMonData(&gPlayerParty[monId], MON_DATA_DEF);
        dst[0] = data16;
        dst[1] = data16 >> 8;
        size = 2;
        break;
    case REQUEST_SPEED_BATTLE:
        data16 = GetMonData(&gPlayerParty[monId], MON_DATA_SPEED);
        dst[0] = data16;
        dst[1] = data16 >> 8;
        size = 2;
        break;
    case REQUEST_SPATK_BATTLE:
        data16 = GetMonData(&gPlayerParty[monId], MON_DATA_SPATK);
        dst[0] = data16;
        dst[1] = data16 >> 8;
        size = 2;
        break;
    case REQUEST_SPDEF_BATTLE:
        data16 = GetMonData(&gPlayerParty[monId], MON_DATA_SPDEF);
        dst[0] = data16;
        dst[1] = data16 >> 8;
        size = 2;
        break;
    }

    return size;
}

void PlayerHandleGetRawMonData(void)
{
    struct BattlePokemon battleMon;
    u8 *src = (u8 *)&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]] + gBattleBufferA[gActiveBattler][1];
    u8 *dst = (u8 *)&battleMon + gBattleBufferA[gActiveBattler][1];
    u8 i;

    for (i = 0; i < gBattleBufferA[gActiveBattler][2]; i++)
        dst[i] = src[i];

    BtlController_EmitDataTransfer(B_COMM_TO_ENGINE, gBattleBufferA[gActiveBattler][2], dst);
    PlayerBufferExecCompleted();
}

static void PlayerHandleSetMonData(void)
{
    u8 monToCheck;
    u8 i;

    if (gBattleBufferA[gActiveBattler][2] == 0)
    {
        SetPlayerMonData(gBattlerPartyIndexes[gActiveBattler]);
    }
    else
    {
        monToCheck = gBattleBufferA[gActiveBattler][2];
        for (i = 0; i < PARTY_SIZE; i++)
        {
            if (monToCheck & 1)
                SetPlayerMonData(i);
            monToCheck >>= 1;
        }
    }
    PlayerBufferExecCompleted();
}

static void SetPlayerMonData(u8 monId)
{
    struct BattlePokemon *battlePokemon = (struct BattlePokemon *)&gBattleBufferA[gActiveBattler][3];
    s32 i;

    switch (gBattleBufferA[gActiveBattler][1])
    {
    case REQUEST_ALL_BATTLE:
        {
            u8 iv;

            SetMonData(&gPlayerParty[monId], MON_DATA_SPECIES, &battlePokemon->species);
            SetMonData(&gPlayerParty[monId], MON_DATA_HELD_ITEM, &battlePokemon->item);
            for (i = 0; i < MAX_MON_MOVES; i++)
            {
                SetMonData(&gPlayerParty[monId], MON_DATA_MOVE1 + i, &battlePokemon->moves[i]);
            }
            SetMonData(&gPlayerParty[monId], MON_DATA_FRIENDSHIP, &battlePokemon->friendship);
            SetMonData(&gPlayerParty[monId], MON_DATA_EXP, &battlePokemon->experience);
            iv = battlePokemon->hpIV;
            SetMonData(&gPlayerParty[monId], MON_DATA_HP_IV, &iv);
            iv = battlePokemon->attackIV;
            SetMonData(&gPlayerParty[monId], MON_DATA_ATK_IV, &iv);
            iv = battlePokemon->defenseIV;
            SetMonData(&gPlayerParty[monId], MON_DATA_DEF_IV, &iv);
            iv = battlePokemon->speedIV;
            SetMonData(&gPlayerParty[monId], MON_DATA_SPEED_IV, &iv);
            iv = battlePokemon->spAttackIV;
            SetMonData(&gPlayerParty[monId], MON_DATA_SPATK_IV, &iv);
            iv = battlePokemon->spDefenseIV;
            SetMonData(&gPlayerParty[monId], MON_DATA_SPDEF_IV, &iv);
            SetMonData(&gPlayerParty[monId], MON_DATA_PERSONALITY, &battlePokemon->personality);
            SetMonData(&gPlayerParty[monId], MON_DATA_STATUS, &battlePokemon->status1);
            SetMonData(&gPlayerParty[monId], MON_DATA_LEVEL, &battlePokemon->level);
            SetMonData(&gPlayerParty[monId], MON_DATA_HP, &battlePokemon->hp);
            SetMonData(&gPlayerParty[monId], MON_DATA_MAX_HP, &battlePokemon->maxHP);
            SetMonData(&gPlayerParty[monId], MON_DATA_ATK, &battlePokemon->attack);
            SetMonData(&gPlayerParty[monId], MON_DATA_DEF, &battlePokemon->defense);
            SetMonData(&gPlayerParty[monId], MON_DATA_SPEED, &battlePokemon->speed);
            SetMonData(&gPlayerParty[monId], MON_DATA_SPATK, &battlePokemon->spAttack);
            SetMonData(&gPlayerParty[monId], MON_DATA_SPDEF, &battlePokemon->spDefense);
        }
        break;
    case REQUEST_SPECIES_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_SPECIES, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_HELDITEM_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_HELD_ITEM, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_MOVES_BATTLE:
        for (i = 0; i < MAX_MON_MOVES; i++)
            SetMonData(&gPlayerParty[monId], MON_DATA_MOVE1 + i, &gBattleBufferA[gActiveBattler][3 + i * 2]);
        break;
    case REQUEST_MOVE1_BATTLE:
    case REQUEST_MOVE2_BATTLE:
    case REQUEST_MOVE3_BATTLE:
    case REQUEST_MOVE4_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_MOVE1 + gBattleBufferA[gActiveBattler][1] - REQUEST_MOVE1_BATTLE, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_OTID_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_OT_ID, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_EXP_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_EXP, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_HP_EV_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_HP_EV, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_ATK_EV_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_ATK_EV, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_DEF_EV_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_DEF_EV, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_SPEED_EV_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_SPEED_EV, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_SPATK_EV_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_SPATK_EV, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_SPDEF_EV_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_SPDEF_EV, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_FRIENDSHIP_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_FRIENDSHIP, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_POKERUS_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_POKERUS, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_MET_LOCATION_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_MET_LOCATION, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_MET_LEVEL_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_MET_LEVEL, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_MET_GAME_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_MET_GAME, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_POKEBALL_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_POKEBALL, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_ALL_IVS_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_HP_IV, &gBattleBufferA[gActiveBattler][3]);
        SetMonData(&gPlayerParty[monId], MON_DATA_ATK_IV, &gBattleBufferA[gActiveBattler][4]);
        SetMonData(&gPlayerParty[monId], MON_DATA_DEF_IV, &gBattleBufferA[gActiveBattler][5]);
        SetMonData(&gPlayerParty[monId], MON_DATA_SPEED_IV, &gBattleBufferA[gActiveBattler][6]);
        SetMonData(&gPlayerParty[monId], MON_DATA_SPATK_IV, &gBattleBufferA[gActiveBattler][7]);
        SetMonData(&gPlayerParty[monId], MON_DATA_SPDEF_IV, &gBattleBufferA[gActiveBattler][8]);
        break;
    case REQUEST_HP_IV_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_HP_IV, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_ATK_IV_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_ATK_IV, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_DEF_IV_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_DEF_IV, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_SPEED_IV_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_SPEED_IV, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_SPATK_IV_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_SPATK_IV, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_SPDEF_IV_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_SPDEF_IV, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_PERSONALITY_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_PERSONALITY, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_CHECKSUM_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_CHECKSUM, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_STATUS_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_STATUS, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_LEVEL_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_LEVEL, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_HP_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_HP, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_MAX_HP_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_MAX_HP, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_ATK_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_ATK, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_DEF_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_DEF, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_SPEED_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_SPEED, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_SPATK_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_SPATK, &gBattleBufferA[gActiveBattler][3]);
        break;
    case REQUEST_SPDEF_BATTLE:
        SetMonData(&gPlayerParty[monId], MON_DATA_SPDEF, &gBattleBufferA[gActiveBattler][3]);
        break;
    }

    HandleLowHpMusicChange(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], gActiveBattler);
}

static void PlayerHandleSetRawMonData(void)
{
    u8 *dst = (u8 *)&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]] + gBattleBufferA[gActiveBattler][1];
    u8 i;

    for (i = 0; i < gBattleBufferA[gActiveBattler][2]; i++)
        dst[i] = gBattleBufferA[gActiveBattler][3 + i];

    PlayerBufferExecCompleted();
}

static void PlayerHandleLoadMonSprite(void)
{
    BattleLoadPlayerMonSpriteGfx(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], gActiveBattler);
    gSprites[gBattlerSpriteIds[gActiveBattler]].oam.paletteNum = gActiveBattler;
    gBattlerControllerFuncs[gActiveBattler] = CompleteOnBankSpritePosX_0;
}

static void PlayerHandleSwitchInAnim(void)
{
    ClearTemporarySpeciesSpriteData(gActiveBattler, gBattleBufferA[gActiveBattler][2]);
    gBattlerPartyIndexes[gActiveBattler] = gBattleBufferA[gActiveBattler][1];
    BattleLoadPlayerMonSpriteGfx(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], gActiveBattler);
    gActionSelectionCursor[gActiveBattler] = 0;
    gMoveSelectionCursor[gActiveBattler] = 0;
    StartSendOutAnim(gActiveBattler, gBattleBufferA[gActiveBattler][2]);
    gBattlerControllerFuncs[gActiveBattler] = SwitchIn_TryShinyAnimShowHealthbox;
}

static void StartSendOutAnim(u8 battler, bool8 dontClearSubstituteBit)
{
    u16 species;

    ClearTemporarySpeciesSpriteData(battler, dontClearSubstituteBit);
    gBattlerPartyIndexes[battler] = gBattleBufferA[battler][1];
    species = GetMonData(&gPlayerParty[gBattlerPartyIndexes[battler]], MON_DATA_SPECIES);
    gBattleControllerData[battler] = CreateInvisibleSpriteWithCallback(SpriteCB_WaitForBattlerBallReleaseAnim);
    SetMultiuseSpriteTemplateToPokemon(species, GetBattlerPosition(battler));

    gBattlerSpriteIds[battler] = CreateSprite(
      &gMultiuseSpriteTemplate,
      GetBattlerSpriteCoord(battler, BATTLER_COORD_X_2),
      GetBattlerSpriteDefault_Y(battler),
      GetBattlerSpriteSubpriority(battler));

    gSprites[gBattleControllerData[battler]].data[1] = gBattlerSpriteIds[battler];
    gSprites[gBattleControllerData[battler]].data[2] = battler;

    gSprites[gBattlerSpriteIds[battler]].data[0] = battler;
    gSprites[gBattlerSpriteIds[battler]].data[2] = species;
    gSprites[gBattlerSpriteIds[battler]].oam.paletteNum = battler;

    StartSpriteAnim(&gSprites[gBattlerSpriteIds[battler]], gBattleMonForms[battler]);

    gSprites[gBattlerSpriteIds[battler]].invisible = TRUE;
    gSprites[gBattlerSpriteIds[battler]].callback = SpriteCallbackDummy;

    gSprites[gBattleControllerData[battler]].data[0] = DoPokeballSendOutAnimation(0, POKEBALL_PLAYER_SENDOUT);
}

static void PlayerHandleReturnMonToBall(void)
{
    if (!gBattleBufferA[gActiveBattler][1])
    {
        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].animationState = 0;
        gBattlerControllerFuncs[gActiveBattler] = DoSwitchOutAnimation;
    }
    else
    {
        // Skip animation, just remove battler
        FreeSpriteOamMatrix(&gSprites[gBattlerSpriteIds[gActiveBattler]]);
        DestroySprite(&gSprites[gBattlerSpriteIds[gActiveBattler]]);
        SetHealthboxSpriteInvisible(gHealthboxSpriteIds[gActiveBattler]);
        PlayerBufferExecCompleted();
    }
}

static void DoSwitchOutAnimation(void)
{
    switch (gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].animationState)
    {
    case 0:
        if (gBattleSpritesDataPtr->battlerData[gActiveBattler].behindSubstitute)
            InitAndLaunchSpecialAnimation(gActiveBattler, gActiveBattler, gActiveBattler, B_ANIM_SUBSTITUTE_TO_MON);

        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].animationState = 1;
        break;
    case 1:
        if (!gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].specialAnimActive)
        {
            gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].animationState = 0;
            InitAndLaunchSpecialAnimation(gActiveBattler, gActiveBattler, gActiveBattler, B_ANIM_SWITCH_OUT_PLAYER_MON);
            gBattlerControllerFuncs[gActiveBattler] = FreeMonSpriteAfterSwitchOutAnim;
        }
        break;
    }
}

#define sSpeedX data[0]

// In emerald it's possible to have a tag battle in the battle frontier facilities with AI
// which use the front sprite for both the player and the partner as opposed to any other battles (including the one with Steven)
// that use an animated back pic.
static void PlayerHandleDrawTrainerPic(void)
{
    s16 xPos, yPos;
    u32 trainerPicId;

    if (gBattleTypeFlags & BATTLE_TYPE_LINK)
    {
        if ((gLinkPlayers[GetMultiplayerId()].version & 0xFF) == VERSION_FIRE_RED
            || (gLinkPlayers[GetMultiplayerId()].version & 0xFF) == VERSION_LEAF_GREEN)
        {
            trainerPicId = gLinkPlayers[GetMultiplayerId()].gender + TRAINER_BACK_PIC_RED;
        }
        else if ((gLinkPlayers[GetMultiplayerId()].version & 0xFF) == VERSION_RUBY
                 || (gLinkPlayers[GetMultiplayerId()].version & 0xFF) == VERSION_SAPPHIRE)
        {
            trainerPicId = gLinkPlayers[GetMultiplayerId()].gender + TRAINER_BACK_PIC_RUBY_SAPPHIRE_BRENDAN;
        }
        else
        {
            trainerPicId = gLinkPlayers[GetMultiplayerId()].gender + TRAINER_BACK_PIC_BRENDAN;
        }
    }
    else
    {
        trainerPicId = gSaveBlock2Ptr->playerGender;
    }

    if (gBattleTypeFlags & BATTLE_TYPE_MULTI)
    {
        if ((GetBattlerPosition(gActiveBattler) & BIT_FLANK) != B_FLANK_LEFT) // Second mon, on the right.
            xPos = 90;
        else // First mon, on the left.
            xPos = 32;

        if (gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER && gPartnerTrainerId != TRAINER_STEVEN_PARTNER)
        {
            xPos = 90;
            yPos = (8 - gTrainerFrontPicCoords[trainerPicId].size) * 4 + 80;
        }
        else
        {
            yPos = (8 - gTrainerBackPicCoords[trainerPicId].size) * 4 + 80;
        }

    }
    else
    {
        xPos = 80;
        yPos = (8 - gTrainerBackPicCoords[trainerPicId].size) * 4 + 80;
    }

    // Use front pic table for any tag battles unless your partner is Steven.
    if (gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER && gPartnerTrainerId != TRAINER_STEVEN_PARTNER)
    {
        trainerPicId = PlayerGenderToFrontTrainerPicId(gSaveBlock2Ptr->playerGender);
        DecompressTrainerFrontPic(trainerPicId, gActiveBattler);
        SetMultiuseSpriteTemplateToTrainerFront(trainerPicId, GetBattlerPosition(gActiveBattler));
        gBattlerSpriteIds[gActiveBattler] = CreateSprite(&gMultiuseSpriteTemplate, xPos, yPos, GetBattlerSpriteSubpriority(gActiveBattler));

        gSprites[gBattlerSpriteIds[gActiveBattler]].oam.paletteNum = IndexOfSpritePaletteTag(gTrainerFrontPicPaletteTable[trainerPicId].tag);
        gSprites[gBattlerSpriteIds[gActiveBattler]].x2 = DISPLAY_WIDTH;
        gSprites[gBattlerSpriteIds[gActiveBattler]].y2 = 48;
        gSprites[gBattlerSpriteIds[gActiveBattler]].sSpeedX = -2;
        gSprites[gBattlerSpriteIds[gActiveBattler]].callback = SpriteCB_TrainerSlideIn;
        gSprites[gBattlerSpriteIds[gActiveBattler]].oam.affineMode = ST_OAM_AFFINE_OFF;
        gSprites[gBattlerSpriteIds[gActiveBattler]].hFlip = 1;
    }
    // Use the back pic in any other scenario.
    else
    {
        DecompressTrainerBackPic(trainerPicId, gActiveBattler);
        SetMultiuseSpriteTemplateToTrainerBack(trainerPicId, GetBattlerPosition(gActiveBattler));
        gBattlerSpriteIds[gActiveBattler] = CreateSprite(&gMultiuseSpriteTemplate, xPos, yPos, GetBattlerSpriteSubpriority(gActiveBattler));

        gSprites[gBattlerSpriteIds[gActiveBattler]].oam.paletteNum = gActiveBattler;
        gSprites[gBattlerSpriteIds[gActiveBattler]].x2 = DISPLAY_WIDTH;
        gSprites[gBattlerSpriteIds[gActiveBattler]].sSpeedX = -2;
        gSprites[gBattlerSpriteIds[gActiveBattler]].callback = SpriteCB_TrainerSlideIn;
    }

    gBattlerControllerFuncs[gActiveBattler] = CompleteOnBattlerSpriteCallbackDummy;
}

static void PlayerHandleTrainerSlide(void)
{
    u32 trainerPicId;

    if (gBattleTypeFlags & BATTLE_TYPE_LINK)
    {
        if ((gLinkPlayers[GetMultiplayerId()].version & 0xFF) == VERSION_FIRE_RED
            || (gLinkPlayers[GetMultiplayerId()].version & 0xFF) == VERSION_LEAF_GREEN)
        {
            trainerPicId = gLinkPlayers[GetMultiplayerId()].gender + TRAINER_BACK_PIC_RED;
        }
        else if ((gLinkPlayers[GetMultiplayerId()].version & 0xFF) == VERSION_RUBY
                 || (gLinkPlayers[GetMultiplayerId()].version & 0xFF) == VERSION_SAPPHIRE)
        {
            trainerPicId = gLinkPlayers[GetMultiplayerId()].gender + TRAINER_BACK_PIC_RUBY_SAPPHIRE_BRENDAN;
        }
        else
        {
            trainerPicId = gLinkPlayers[GetMultiplayerId()].gender + TRAINER_BACK_PIC_BRENDAN;
        }
    }
    else
    {
        trainerPicId = gSaveBlock2Ptr->playerGender + TRAINER_BACK_PIC_BRENDAN;
    }

    DecompressTrainerBackPic(trainerPicId, gActiveBattler);
    SetMultiuseSpriteTemplateToTrainerBack(trainerPicId, GetBattlerPosition(gActiveBattler));
    gBattlerSpriteIds[gActiveBattler] = CreateSprite(&gMultiuseSpriteTemplate, 80, (8 - gTrainerBackPicCoords[trainerPicId].size) * 4 + 80, 30);

    gSprites[gBattlerSpriteIds[gActiveBattler]].oam.paletteNum = gActiveBattler;
    gSprites[gBattlerSpriteIds[gActiveBattler]].x2 = -96;
    gSprites[gBattlerSpriteIds[gActiveBattler]].sSpeedX = 2;
    gSprites[gBattlerSpriteIds[gActiveBattler]].callback = SpriteCB_TrainerSlideIn;

    gBattlerControllerFuncs[gActiveBattler] = CompleteOnBankSpriteCallbackDummy2;
}

#undef sSpeedX

static void PlayerHandleTrainerSlideBack(void)
{
    SetSpritePrimaryCoordsFromSecondaryCoords(&gSprites[gBattlerSpriteIds[gActiveBattler]]);
    gSprites[gBattlerSpriteIds[gActiveBattler]].data[0] = 50;
    gSprites[gBattlerSpriteIds[gActiveBattler]].data[2] = -40;
    gSprites[gBattlerSpriteIds[gActiveBattler]].data[4] = gSprites[gBattlerSpriteIds[gActiveBattler]].y;
    gSprites[gBattlerSpriteIds[gActiveBattler]].callback = StartAnimLinearTranslation;
    StoreSpriteCallbackInData6(&gSprites[gBattlerSpriteIds[gActiveBattler]], SpriteCallbackDummy);
    StartSpriteAnim(&gSprites[gBattlerSpriteIds[gActiveBattler]], 1);
    gBattlerControllerFuncs[gActiveBattler] = FreeTrainerSpriteAfterSlide;
}

#define sSpeedX data[1]
#define sSpeedY data[2]

static void PlayerHandleFaintAnimation(void)
{
    if (gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].animationState == 0)
    {
        if (gBattleSpritesDataPtr->battlerData[gActiveBattler].behindSubstitute)
            InitAndLaunchSpecialAnimation(gActiveBattler, gActiveBattler, gActiveBattler, B_ANIM_SUBSTITUTE_TO_MON);
        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].animationState++;
    }
    else
    {
        if (!gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].specialAnimActive)
        {
            gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].animationState = 0;
            HandleLowHpMusicChange(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], gActiveBattler);
            PlaySE12WithPanning(SE_FAINT, SOUND_PAN_ATTACKER);
            gSprites[gBattlerSpriteIds[gActiveBattler]].sSpeedX = 0;
            gSprites[gBattlerSpriteIds[gActiveBattler]].sSpeedY = 5;
            gSprites[gBattlerSpriteIds[gActiveBattler]].callback = SpriteCB_FaintSlideAnim;
            gBattlerControllerFuncs[gActiveBattler] = FreeMonSpriteAfterFaintAnim;
        }
    }
}

#undef sSpeedX
#undef sSpeedY

static void PlayerHandlePaletteFade(void)
{
    BeginNormalPaletteFade(PALETTES_ALL, 2, 0, 16, RGB_BLACK);
    PlayerBufferExecCompleted();
}

static void PlayerHandleSuccessBallThrowAnim(void)
{
    gBattleSpritesDataPtr->animationData->ballThrowCaseId = BALL_3_SHAKES_SUCCESS;
    gDoingBattleAnim = TRUE;
    InitAndLaunchSpecialAnimation(gActiveBattler, gActiveBattler, GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT), B_ANIM_BALL_THROW);
    gBattlerControllerFuncs[gActiveBattler] = CompleteOnSpecialAnimDone;
}

static void PlayerHandleBallThrowAnim(void)
{
    u8 ballThrowCaseId = gBattleBufferA[gActiveBattler][1];

    gBattleSpritesDataPtr->animationData->ballThrowCaseId = ballThrowCaseId;
    gDoingBattleAnim = TRUE;
    InitAndLaunchSpecialAnimation(gActiveBattler, gActiveBattler, GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT), B_ANIM_BALL_THROW);
    gBattlerControllerFuncs[gActiveBattler] = CompleteOnSpecialAnimDone;
}

static void PlayerHandlePause(void)
{
    u8 timer = gBattleBufferA[gActiveBattler][1];

    while (timer != 0)
        timer--;

    PlayerBufferExecCompleted();
}

static void PlayerHandleMoveAnimation(void)
{
    if (!IsBattleSEPlaying(gActiveBattler))
    {
        u16 move = gBattleBufferA[gActiveBattler][1] | (gBattleBufferA[gActiveBattler][2] << 8);

        gAnimMoveTurn = gBattleBufferA[gActiveBattler][3];
        gAnimMovePower = gBattleBufferA[gActiveBattler][4] | (gBattleBufferA[gActiveBattler][5] << 8);
        gAnimMoveDmg = gBattleBufferA[gActiveBattler][6] | (gBattleBufferA[gActiveBattler][7] << 8) | (gBattleBufferA[gActiveBattler][8] << 16) | (gBattleBufferA[gActiveBattler][9] << 24);
        gAnimFriendship = gBattleBufferA[gActiveBattler][10];
        gWeatherMoveAnim = gBattleBufferA[gActiveBattler][12] | (gBattleBufferA[gActiveBattler][13] << 8);
        gAnimDisableStructPtr = (struct DisableStruct *)&gBattleBufferA[gActiveBattler][16];
        gTransformedPersonalities[gActiveBattler] = gAnimDisableStructPtr->transformedMonPersonality;
        if (IsMoveWithoutAnimation(move, gAnimMoveTurn)) // Always returns FALSE.
        {
            PlayerBufferExecCompleted();
        }
        else
        {
            gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].animationState = 0;
            gBattlerControllerFuncs[gActiveBattler] = PlayerDoMoveAnimation;
        }
    }
}

static void PlayerDoMoveAnimation(void)
{
    u16 move = gBattleBufferA[gActiveBattler][1] | (gBattleBufferA[gActiveBattler][2] << 8);
    u8 multihit = gBattleBufferA[gActiveBattler][11];

    switch (gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].animationState)
    {
    case 0:
        if (gBattleSpritesDataPtr->battlerData[gActiveBattler].behindSubstitute
            && !gBattleSpritesDataPtr->battlerData[gActiveBattler].flag_x8)
        {
            gBattleSpritesDataPtr->battlerData[gActiveBattler].flag_x8 = 1;
            InitAndLaunchSpecialAnimation(gActiveBattler, gActiveBattler, gActiveBattler, B_ANIM_SUBSTITUTE_TO_MON);
        }
        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].animationState = 1;
        break;
    case 1:
        if (!gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].specialAnimActive)
        {
            SetBattlerSpriteAffineMode(ST_OAM_AFFINE_OFF);
            DoMoveAnim(move);
            gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].animationState = 2;
        }
        break;
    case 2:
        gAnimScriptCallback();
        if (!gAnimScriptActive)
        {
            SetBattlerSpriteAffineMode(ST_OAM_AFFINE_NORMAL);
            if (gBattleSpritesDataPtr->battlerData[gActiveBattler].behindSubstitute && multihit < 2)
            {
                InitAndLaunchSpecialAnimation(gActiveBattler, gActiveBattler, gActiveBattler, B_ANIM_MON_TO_SUBSTITUTE);
                gBattleSpritesDataPtr->battlerData[gActiveBattler].flag_x8 = 0;
            }
            gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].animationState = 3;
        }
        break;
    case 3:
        if (!gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].specialAnimActive)
        {
            CopyAllBattleSpritesInvisibilities();
            TrySetBehindSubstituteSpriteBit(gActiveBattler, gBattleBufferA[gActiveBattler][1] | (gBattleBufferA[gActiveBattler][2] << 8));
            gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].animationState = 0;
            PlayerBufferExecCompleted();
        }
        break;
    }
}

static void PlayerHandlePrintString(void)
{
    u16 *stringId;

    gBattle_BG0_X = 0;
    gBattle_BG0_Y = 0;
    stringId = (u16 *)(&gBattleBufferA[gActiveBattler][2]);
    BufferStringBattle(*stringId);
    BattlePutTextOnWindow(gDisplayedStringBattle, B_WIN_MSG);
    gBattlerControllerFuncs[gActiveBattler] = CompleteOnInactiveTextPrinter2;
    BattleArena_DeductSkillPoints(gActiveBattler, *stringId);
}

static void PlayerHandlePrintSelectionString(void)
{
    if (GetBattlerSide(gActiveBattler) == B_SIDE_PLAYER)
        PlayerHandlePrintString();
    else
        PlayerBufferExecCompleted();
}

static void HandleChooseActionAfterDma3(void)
{
    if (!IsDma3ManagerBusyWithBgCopy())
    {
        gBattle_BG0_X = 0;
        gBattle_BG0_Y = DISPLAY_HEIGHT;
        gBattlerControllerFuncs[gActiveBattler] = HandleInputChooseAction;
    }
}

static void PlayerHandleChooseAction(void)
{
    gBattlerControllerFuncs[gActiveBattler] = HandleChooseActionAfterDma3;
    ActionSelectionDrawMenu();
    ActionSelectionCreateCursorAt(gActionSelectionCursor[gActiveBattler], 0);
    BattleStringExpandPlaceholdersToDisplayedString(gText_WhatWillPkmnDo);
    BattlePutTextOnWindow(gDisplayedStringBattle, B_WIN_ACTION_PROMPT);
}

static void PlayerHandleYesNoBox(void)
{
    if (GetBattlerSide(gActiveBattler) == B_SIDE_PLAYER)
    {
        HandleBattleWindow(YESNOBOX_X_Y, 0);
        BattlePutTextOnWindow(gText_BattleYesNoChoice, B_WIN_YESNO);
        gMultiUsePlayerCursor = 1;
        BattleCreateYesNoCursorAt(1);
        gBattlerControllerFuncs[gActiveBattler] = PlayerHandleYesNoInput;
    }
    else
    {
        PlayerBufferExecCompleted();
    }
}

static void HandleChooseMoveAfterDma3(void)
{
    if (!IsDma3ManagerBusyWithBgCopy())
    {
        gBattle_BG0_X = 0;
        gBattle_BG0_Y = DISPLAY_HEIGHT * 2;
        gBattlerControllerFuncs[gActiveBattler] = HandleInputChooseMove;
    }
}

// arenaMindPoints is used here as a placeholder for a timer.

static void PlayerChooseMoveInBattlePalace(void)
{
    if (--*(gBattleStruct->arenaMindPoints + gActiveBattler) == 0)
    {
        gBattlePalaceMoveSelectionRngValue = gRngValue;
        BtlController_EmitTwoReturnValues(B_COMM_TO_ENGINE, 10, ChooseMoveAndTargetInBattlePalace());
        PlayerBufferExecCompleted();
    }
}

static void PlayerHandleChooseMove(void)
{
    if (gBattleTypeFlags & BATTLE_TYPE_PALACE)
    {
        *(gBattleStruct->arenaMindPoints + gActiveBattler) = 8;
        gBattlerControllerFuncs[gActiveBattler] = PlayerChooseMoveInBattlePalace;
    }
    else
    {
        InitMoveSelectionsVarsAndStrings();
        gBattlerControllerFuncs[gActiveBattler] = HandleChooseMoveAfterDma3;
    }
}

void InitMoveSelectionsVarsAndStrings(void)
{
    // FIGHT always opens on the move grid
    if (IsOnSpecialPage())
        gMoveSelectionCursor[gActiveBattler] = 0;
    MoveSelectionDisplayMoveNames();
    gMultiUsePlayerCursor = 0xFF;
    MoveSelectionCreateCursorAt(gMoveSelectionCursor[gActiveBattler], 0);
    MoveSelectionDisplayTpCost();
    MoveSelectionDisplayMoveType();
}

static void PlayerHandleChooseItem(void)
{
    s32 i;

    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_BLACK);
    gBattlerControllerFuncs[gActiveBattler] = OpenBagAndChooseItem;
    gBattlerInMenuId = gActiveBattler;

    for (i = 0; i < (int)ARRAY_COUNT(gBattlePartyCurrentOrder); i++)
        gBattlePartyCurrentOrder[i] = gBattleBufferA[gActiveBattler][1 + i];
}

static void PlayerHandleChoosePokemon(void)
{
    s32 i;

    for (i = 0; i < (int)ARRAY_COUNT(gBattlePartyCurrentOrder); i++)
        gBattlePartyCurrentOrder[i] = gBattleBufferA[gActiveBattler][4 + i];

    if (gBattleTypeFlags & BATTLE_TYPE_ARENA && (gBattleBufferA[gActiveBattler][1] & 0xF) != PARTY_ACTION_CANT_SWITCH)
    {
        BtlController_EmitChosenMonReturnValue(B_COMM_TO_ENGINE, gBattlerPartyIndexes[gActiveBattler] + 1, gBattlePartyCurrentOrder);
        PlayerBufferExecCompleted();
    }
    else
    {
        gBattleControllerData[gActiveBattler] = CreateTask(TaskDummy, 0xFF);
        gTasks[gBattleControllerData[gActiveBattler]].data[0] = gBattleBufferA[gActiveBattler][1] & 0xF;
        *(&gBattleStruct->battlerPreventingSwitchout) = gBattleBufferA[gActiveBattler][1] >> 4;
        *(&gBattleStruct->prevSelectedPartySlot) = gBattleBufferA[gActiveBattler][2];
        *(&gBattleStruct->abilityPreventingSwitchout) = gBattleBufferA[gActiveBattler][3];
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_BLACK);
        gBattlerControllerFuncs[gActiveBattler] = OpenPartyMenuToChooseMon;
        gBattlerInMenuId = gActiveBattler;
    }
}

static void PlayerHandleCmd23(void)
{
    BattleStopLowHpSound();
    BeginNormalPaletteFade(PALETTES_ALL, 2, 0, 16, RGB_BLACK);
    PlayerBufferExecCompleted();
}

static void PlayerHandleHealthBarUpdate(void)
{
    s16 hpVal;

    LoadBattleBarGfx(0);
    hpVal = gBattleBufferA[gActiveBattler][2] | (gBattleBufferA[gActiveBattler][3] << 8);

    // gPlayerPartyLostHP used by Battle Dome, but never read
    if (hpVal > 0)
        gPlayerPartyLostHP += hpVal;

    if (hpVal != INSTANT_HP_BAR_DROP)
    {
        u32 maxHP = GetMonData(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_MAX_HP);
        u32 curHP = GetMonData(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_HP);

        SetBattleBarStruct(gActiveBattler, gHealthboxSpriteIds[gActiveBattler], maxHP, curHP, hpVal);
    }
    else
    {
        u32 maxHP = GetMonData(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_MAX_HP);

        SetBattleBarStruct(gActiveBattler, gHealthboxSpriteIds[gActiveBattler], maxHP, 0, hpVal);
        UpdateHpTextInHealthbox(gHealthboxSpriteIds[gActiveBattler], 0, HP_CURRENT);
    }

    gBattlerControllerFuncs[gActiveBattler] = CompleteOnHealthbarDone;
}

static void PlayerHandleExpUpdate(void)
{
    u8 monId = gBattleBufferA[gActiveBattler][1];

    if (GetMonData(&gPlayerParty[monId], MON_DATA_LEVEL) >= MAX_LEVEL)
    {
        PlayerBufferExecCompleted();
    }
    else
    {
        s16 expPointsToGive;
        u8 taskId;

        LoadBattleBarGfx(1);
        GetMonData(&gPlayerParty[monId], MON_DATA_SPECIES);  // Unused return value.
        expPointsToGive = T1_READ_16(&gBattleBufferA[gActiveBattler][2]);
        taskId = CreateTask(Task_GiveExpToMon, 10);
        gTasks[taskId].tExpTask_monId = monId;
        gTasks[taskId].tExpTask_gainedExp = expPointsToGive;
        gTasks[taskId].tExpTask_battler = gActiveBattler;
        gBattlerControllerFuncs[gActiveBattler] = BattleControllerDummy;
    }
}

#undef tExpTask_monId
#undef tExpTask_gainedExp
#undef tExpTask_battler
#undef tExpTask_frames

static void PlayerHandleStatusIconUpdate(void)
{
    if (!IsBattleSEPlaying(gActiveBattler))
    {
        u8 battler;

        UpdateHealthboxAttribute(gHealthboxSpriteIds[gActiveBattler], &gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], HEALTHBOX_STATUS_ICON);
        battler = gActiveBattler;
        gBattleSpritesDataPtr->healthBoxesData[battler].statusAnimActive = 0;
        gBattlerControllerFuncs[gActiveBattler] = CompleteOnFinishedStatusAnimation;
    }
}

static void PlayerHandleStatusAnimation(void)
{
    if (!IsBattleSEPlaying(gActiveBattler))
    {
        InitAndLaunchChosenStatusAnimation(gBattleBufferA[gActiveBattler][1],
                        gBattleBufferA[gActiveBattler][2] | (gBattleBufferA[gActiveBattler][3] << 8) | (gBattleBufferA[gActiveBattler][4] << 16) | (gBattleBufferA[gActiveBattler][5] << 24));
        gBattlerControllerFuncs[gActiveBattler] = CompleteOnFinishedStatusAnimation;
    }
}

static void PlayerHandleStatusXor(void)
{
    u8 val = GetMonData(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_STATUS) ^ gBattleBufferA[gActiveBattler][1];

    SetMonData(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_STATUS, &val);
    PlayerBufferExecCompleted();
}

static void PlayerHandleDataTransfer(void)
{
    PlayerBufferExecCompleted();
}

static void PlayerHandleDMA3Transfer(void)
{
    u32 dstArg = gBattleBufferA[gActiveBattler][1]
            | (gBattleBufferA[gActiveBattler][2] << 8)
            | (gBattleBufferA[gActiveBattler][3] << 16)
            | (gBattleBufferA[gActiveBattler][4] << 24);
    u16 sizeArg = gBattleBufferA[gActiveBattler][5] | (gBattleBufferA[gActiveBattler][6] << 8);

    const u8 *src = &gBattleBufferA[gActiveBattler][7];
    u8 *dst = (u8 *)(dstArg);
    u32 size = sizeArg;

    while (1)
    {
        if (size <= 0x1000)
        {
            DmaCopy16(3, src, dst, size);
            break;
        }
        DmaCopy16(3, src, dst, 0x1000);
        src += 0x1000;
        dst += 0x1000;
        size -= 0x1000;
    }
    PlayerBufferExecCompleted();
}

static void PlayerHandlePlayBGM(void)
{
    PlayBGM(gBattleBufferA[gActiveBattler][1] | (gBattleBufferA[gActiveBattler][2] << 8));
    PlayerBufferExecCompleted();
}

static void PlayerHandleCmd32(void)
{
    PlayerBufferExecCompleted();
}

static void PlayerHandleTwoReturnValues(void)
{
    BtlController_EmitTwoReturnValues(B_COMM_TO_ENGINE, 0, 0);
    PlayerBufferExecCompleted();
}

static void PlayerHandleChosenMonReturnValue(void)
{
    BtlController_EmitChosenMonReturnValue(B_COMM_TO_ENGINE, 0, NULL);
    PlayerBufferExecCompleted();
}

static void PlayerHandleOneReturnValue(void)
{
    BtlController_EmitOneReturnValue(B_COMM_TO_ENGINE, 0);
    PlayerBufferExecCompleted();
}

static void PlayerHandleOneReturnValue_Duplicate(void)
{
    BtlController_EmitOneReturnValue_Duplicate(B_COMM_TO_ENGINE, 0);
    PlayerBufferExecCompleted();
}

static void PlayerHandleClearUnkVar(void)
{
    gUnusedControllerStruct.unk = 0;
    PlayerBufferExecCompleted();
}

static void PlayerHandleSetUnkVar(void)
{
    gUnusedControllerStruct.unk = gBattleBufferA[gActiveBattler][1];
    PlayerBufferExecCompleted();
}

static void PlayerHandleClearUnkFlag(void)
{
    gUnusedControllerStruct.flag = 0;
    PlayerBufferExecCompleted();
}

static void PlayerHandleToggleUnkFlag(void)
{
    gUnusedControllerStruct.flag ^= 1;
    PlayerBufferExecCompleted();
}

static void PlayerHandleHitAnimation(void)
{
    if (gSprites[gBattlerSpriteIds[gActiveBattler]].invisible == TRUE)
    {
        PlayerBufferExecCompleted();
    }
    else
    {
        gDoingBattleAnim = TRUE;
        gSprites[gBattlerSpriteIds[gActiveBattler]].data[1] = 0;
        DoHitAnimHealthboxEffect(gActiveBattler);
        gBattlerControllerFuncs[gActiveBattler] = DoHitAnimBlinkSpriteEffect;
    }
}

static void PlayerHandleCantSwitch(void)
{
    PlayerBufferExecCompleted();
}

static void PlayerHandlePlaySE(void)
{
    s8 pan;

    if (GetBattlerSide(gActiveBattler) == B_SIDE_PLAYER)
        pan = SOUND_PAN_ATTACKER;
    else
        pan = SOUND_PAN_TARGET;

    PlaySE12WithPanning(gBattleBufferA[gActiveBattler][1] | (gBattleBufferA[gActiveBattler][2] << 8), pan);
    PlayerBufferExecCompleted();
}

static void PlayerHandlePlayFanfareOrBGM(void)
{
    if (gBattleBufferA[gActiveBattler][3])
    {
        BattleStopLowHpSound();
        PlayBGM(gBattleBufferA[gActiveBattler][1] | (gBattleBufferA[gActiveBattler][2] << 8));
    }
    else
    {
        PlayFanfare(gBattleBufferA[gActiveBattler][1] | (gBattleBufferA[gActiveBattler][2] << 8));
    }

    PlayerBufferExecCompleted();
}

static void PlayerHandleFaintingCry(void)
{
    u16 species = GetMonData(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_SPECIES);

    PlayCry_ByMode(species, -25, CRY_MODE_FAINT);
    PlayerBufferExecCompleted();
}

static void PlayerHandleIntroSlide(void)
{
    HandleIntroSlide(gBattleBufferA[gActiveBattler][1]);
    gIntroSlideFlags |= 1;
    PlayerBufferExecCompleted();
}

// Task data for Task_StartSendOutAnim
#define tBattlerId  data[0]
#define tStartTimer data[1]

#define sBattlerId data[5]

static void PlayerHandleIntroTrainerBallThrow(void)
{
    u8 paletteNum;
    u8 taskId;

    SetSpritePrimaryCoordsFromSecondaryCoords(&gSprites[gBattlerSpriteIds[gActiveBattler]]);

    gSprites[gBattlerSpriteIds[gActiveBattler]].data[0] = 50;
    gSprites[gBattlerSpriteIds[gActiveBattler]].data[2] = -40;
    gSprites[gBattlerSpriteIds[gActiveBattler]].data[4] = gSprites[gBattlerSpriteIds[gActiveBattler]].y;
    gSprites[gBattlerSpriteIds[gActiveBattler]].callback = StartAnimLinearTranslation;
    gSprites[gBattlerSpriteIds[gActiveBattler]].sBattlerId = gActiveBattler;

    StoreSpriteCallbackInData6(&gSprites[gBattlerSpriteIds[gActiveBattler]], SpriteCB_FreePlayerSpriteLoadMonSprite);
    StartSpriteAnim(&gSprites[gBattlerSpriteIds[gActiveBattler]], 1);

    paletteNum = AllocSpritePalette(0xD6F8);
    LoadCompressedPalette(gTrainerBackPicPaletteTable[gSaveBlock2Ptr->playerGender].data, OBJ_PLTT_ID(paletteNum), PLTT_SIZE_4BPP);
    gSprites[gBattlerSpriteIds[gActiveBattler]].oam.paletteNum = paletteNum;

    taskId = CreateTask(Task_StartSendOutAnim, 5);
    gTasks[taskId].tBattlerId = gActiveBattler;

    if (gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].partyStatusSummaryShown)
        gTasks[gBattlerStatusSummaryTaskId[gActiveBattler]].func = Task_HidePartyStatusSummary;

    gBattleSpritesDataPtr->animationData->introAnimActive = TRUE;
    gBattlerControllerFuncs[gActiveBattler] = BattleControllerDummy;
}

void SpriteCB_FreePlayerSpriteLoadMonSprite(struct Sprite *sprite)
{
    u8 battler = sprite->sBattlerId;

    // Free player trainer sprite
    FreeSpriteOamMatrix(sprite);
    FreeSpritePaletteByTag(GetSpritePaletteTagByPaletteNum(sprite->oam.paletteNum));
    DestroySprite(sprite);

    // Load mon sprite
    BattleLoadPlayerMonSpriteGfx(&gPlayerParty[gBattlerPartyIndexes[battler]], battler);
    StartSpriteAnim(&gSprites[gBattlerSpriteIds[battler]], 0);
}

#undef sBattlerId

// Send out at start of battle
static void Task_StartSendOutAnim(u8 taskId)
{
    if (gTasks[taskId].tStartTimer < 31)
    {
        gTasks[taskId].tStartTimer++;
    }
    else
    {
        u8 savedActiveBattler = gActiveBattler;

        gActiveBattler = gTasks[taskId].tBattlerId;
        if (!IsDoubleBattle() || (gBattleTypeFlags & BATTLE_TYPE_MULTI))
        {
            gBattleBufferA[gActiveBattler][1] = gBattlerPartyIndexes[gActiveBattler];
            StartSendOutAnim(gActiveBattler, FALSE);
        }
        else
        {
            gBattleBufferA[gActiveBattler][1] = gBattlerPartyIndexes[gActiveBattler];
            StartSendOutAnim(gActiveBattler, FALSE);
            gActiveBattler ^= BIT_FLANK;
            gBattleBufferA[gActiveBattler][1] = gBattlerPartyIndexes[gActiveBattler];
            BattleLoadPlayerMonSpriteGfx(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], gActiveBattler);
            StartSendOutAnim(gActiveBattler, FALSE);
            gActiveBattler ^= BIT_FLANK;
        }
        gBattlerControllerFuncs[gActiveBattler] = Intro_TryShinyAnimShowHealthbox;
        gActiveBattler = savedActiveBattler;
        DestroyTask(taskId);
    }
}

#undef tBattlerId
#undef tStartTimer

static void PlayerHandleDrawPartyStatusSummary(void)
{
    if (gBattleBufferA[gActiveBattler][1] != 0 && GetBattlerSide(gActiveBattler) == B_SIDE_PLAYER)
    {
        PlayerBufferExecCompleted();
    }
    else
    {
        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].partyStatusSummaryShown = 1;
        gBattlerStatusSummaryTaskId[gActiveBattler] = CreatePartyStatusSummarySprites(gActiveBattler, (struct HpAndStatus *)&gBattleBufferA[gActiveBattler][4], gBattleBufferA[gActiveBattler][1], gBattleBufferA[gActiveBattler][2]);
        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].partyStatusDelayTimer = 0;

        // If intro, skip the delay after drawing
        if (gBattleBufferA[gActiveBattler][2] != 0)
            gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].partyStatusDelayTimer = 93;

        gBattlerControllerFuncs[gActiveBattler] = EndDrawPartyStatusSummary;
    }
}

static void EndDrawPartyStatusSummary(void)
{
    if (gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].partyStatusDelayTimer++ > 92)
    {
        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].partyStatusDelayTimer = 0;
        PlayerBufferExecCompleted();
    }
}

static void PlayerHandleHidePartyStatusSummary(void)
{
    if (gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].partyStatusSummaryShown)
        gTasks[gBattlerStatusSummaryTaskId[gActiveBattler]].func = Task_HidePartyStatusSummary;
    PlayerBufferExecCompleted();
}

static void PlayerHandleEndBounceEffect(void)
{
    EndBounceEffect(gActiveBattler, BOUNCE_HEALTHBOX);
    EndBounceEffect(gActiveBattler, BOUNCE_MON);
    PlayerBufferExecCompleted();
}

static void PlayerHandleSpriteInvisibility(void)
{
    if (IsBattlerSpritePresent(gActiveBattler))
    {
        gSprites[gBattlerSpriteIds[gActiveBattler]].invisible = gBattleBufferA[gActiveBattler][1];
        CopyBattleSpriteInvisibility(gActiveBattler);
    }
    PlayerBufferExecCompleted();
}

static void PlayerHandleBattleAnimation(void)
{
    if (!IsBattleSEPlaying(gActiveBattler))
    {
        u8 animationId = gBattleBufferA[gActiveBattler][1];
        u16 argument = gBattleBufferA[gActiveBattler][2] | (gBattleBufferA[gActiveBattler][3] << 8);

        if (TryHandleLaunchBattleTableAnimation(gActiveBattler, gActiveBattler, gActiveBattler, animationId, argument))
            PlayerBufferExecCompleted();
        else
            gBattlerControllerFuncs[gActiveBattler] = CompleteOnFinishedBattleAnimation;

    }
}

static void PlayerHandleLinkStandbyMsg(void)
{
    switch (gBattleBufferA[gActiveBattler][1])
    {
    case LINK_STANDBY_MSG_STOP_BOUNCE:
        PrintLinkStandbyMsg();
        // fall through
    case LINK_STANDBY_STOP_BOUNCE_ONLY:
        EndBounceEffect(gActiveBattler, BOUNCE_HEALTHBOX);
        EndBounceEffect(gActiveBattler, BOUNCE_MON);
        break;
    case LINK_STANDBY_MSG_ONLY:
        PrintLinkStandbyMsg();
        break;
    }
    PlayerBufferExecCompleted();
}

static void PlayerHandleResetActionMoveSelection(void)
{
    switch (gBattleBufferA[gActiveBattler][1])
    {
    case RESET_ACTION_MOVE_SELECTION:
        gActionSelectionCursor[gActiveBattler] = 0;
        gMoveSelectionCursor[gActiveBattler] = 0;
        break;
    case RESET_ACTION_SELECTION:
        gActionSelectionCursor[gActiveBattler] = 0;
        break;
    case RESET_MOVE_SELECTION:
        gMoveSelectionCursor[gActiveBattler] = 0;
        break;
    }
    PlayerBufferExecCompleted();
}

static void PlayerHandleEndLinkBattle(void)
{
    gBattleOutcome = gBattleBufferA[gActiveBattler][1];
    gSaveBlock2Ptr->frontier.disableRecordBattle = gBattleBufferA[gActiveBattler][2];
    FadeOutMapMusic(5);
    BeginFastPaletteFade(3);
    PlayerBufferExecCompleted();
    gBattlerControllerFuncs[gActiveBattler] = SetBattleEndCallbacks;
}

static void PlayerCmdEnd(void)
{
}
