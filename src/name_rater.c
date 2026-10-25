#include "global.h"
#include "event_data.h"
#include "field_message_box.h"
#include "international_string_util.h"
#include "field_specials.h"
#include "name_rater.h"
#include "naming_screen.h"
#include "overworld.h"
#include "pokemon_storage_system.h"
#include "string_util.h"
#include "strings.h"
#include "data.h"

static void ChangePokemonNickname_CB(void);
static void ChangeBoxPokemonNickname_CB(void);

static bool8 IsPartyMonNicknamedOrNotEnglish(u8 monIdx)
{
    struct Pokemon *pokemon;
    u8 language;

    pokemon = &gPlayerParty[monIdx];
    GetMonData(pokemon, MON_DATA_NICKNAME, gStringVar1);
    language = GetMonData(pokemon, MON_DATA_LANGUAGE, &language);
    if (language == GAME_LANGUAGE && !StringCompare(gSpeciesNames[GetMonData(pokemon, MON_DATA_SPECIES, NULL)], gStringVar1))
        return FALSE;

    return TRUE;
}

bool8 IsLeadMonNicknamedOrNotEnglish(void)
{
    return IsPartyMonNicknamedOrNotEnglish(GetLeadMonIndex());
}


void ChangePokemonNickname(void)
{
    GetMonData(&gPlayerParty[gSpecialVar_0x8004], MON_DATA_NICKNAME, gStringVar3);
    GetMonData(&gPlayerParty[gSpecialVar_0x8004], MON_DATA_NICKNAME, gStringVar2);
    DoNamingScreen(NAMING_SCREEN_NICKNAME, gStringVar2, GetMonData(&gPlayerParty[gSpecialVar_0x8004], MON_DATA_SPECIES, NULL), GetMonGender(&gPlayerParty[gSpecialVar_0x8004]), GetMonData(&gPlayerParty[gSpecialVar_0x8004], MON_DATA_PERSONALITY, NULL), ChangePokemonNickname_CB);
}

static void ChangePokemonNickname_CB(void)
{
    SetMonData(&gPlayerParty[gSpecialVar_0x8004], MON_DATA_NICKNAME, gStringVar2);
    CB2_ReturnToFieldContinueScriptPlayMapMusic();
}

void ChangeBoxPokemonNickname(void)
{
    struct BoxPokemon *boxMon;

    boxMon = GetBoxedMonPtr(gSpecialVar_MonBoxId, gSpecialVar_MonBoxPos);
    GetBoxMonData(boxMon, MON_DATA_NICKNAME, gStringVar3);
    GetBoxMonData(boxMon, MON_DATA_NICKNAME, gStringVar2);
    DoNamingScreen(NAMING_SCREEN_NICKNAME, gStringVar2, GetBoxMonData(boxMon, MON_DATA_SPECIES, NULL), GetBoxMonGender(boxMon), GetBoxMonData(boxMon, MON_DATA_PERSONALITY, NULL), ChangeBoxPokemonNickname_CB);
}

static void ChangeBoxPokemonNickname_CB(void)
{
    SetBoxMonNickAt(gSpecialVar_MonBoxId, gSpecialVar_MonBoxPos, gStringVar2);
    CB2_ReturnToFieldContinueScriptPlayMapMusic();
}

void BufferMonNickname(void)
{
    GetMonData(&gPlayerParty[gSpecialVar_0x8004], MON_DATA_NICKNAME, gStringVar1);
    StringGet_Nickname(gStringVar1);
}

void IsMonOTIDNotPlayers(void)
{
    if (GetPlayerIDAsU32() == GetMonData(&gPlayerParty[gSpecialVar_0x8004], MON_DATA_OT_ID, NULL))
        gSpecialVar_Result = FALSE;
    else
        gSpecialVar_Result = TRUE;
}


u32 GetPlayerIDAsU32(void)
{
    return (gSaveBlock2Ptr->playerTrainerId[3] << 24) | (gSaveBlock2Ptr->playerTrainerId[2] << 16) | (gSaveBlock2Ptr->playerTrainerId[1] << 8) | gSaveBlock2Ptr->playerTrainerId[0];
}
