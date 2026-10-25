
CableClub_OnWarp:
	map_script_2 VAR_CABLE_CLUB_STATE, USING_SINGLE_BATTLE, CableClub_EventScript_CheckTurnAttendant
	map_script_2 VAR_CABLE_CLUB_STATE, USING_TRADE_CENTER, CableClub_EventScript_CheckTurnAttendant
	.2byte 0

CableClub_EventScript_CheckTurnAttendant::
	goto_if_eq VAR_0x8007, 0, CableClub_EventScript_DontTurnAttendant
	turnobject VAR_0x8007, DIR_WEST
CableClub_EventScript_DontTurnAttendant::
	end

CableClub_OnLoad:
	goto_if_eq VAR_CABLE_CLUB_STATE, USING_SINGLE_BATTLE, CableClub_EventScript_OnLoadFromColosseum
	goto_if_eq VAR_CABLE_CLUB_STATE, USING_TRADE_CENTER, CableClub_EventScript_OnLoadFromTradeCenter
	end

CableClub_EventScript_OnLoadFromColosseum::
	call CableClub_EventScript_OpenColosseumBarrier
	end

CableClub_EventScript_OnLoadFromTradeCenter::
	call CableClub_EventScript_OpenColosseumBarrier
	end

CableClub_OnFrame:
	map_script_2 VAR_CABLE_CLUB_STATE, USING_SINGLE_BATTLE, CableClub_EventScript_ExitLinkRoom
	map_script_2 VAR_CABLE_CLUB_STATE, USING_TRADE_CENTER, CableClub_EventScript_ExitTradeCenter
	.2byte 0

CableClub_EventScript_ExitLinkRoom::
	lockall
	call CableClub_EventScript_CloseLinkAndExitLinkRoom
	call CableClub_EventScript_CloseColosseumBarrier
	special DrawWholeMapView
	playse SE_CLICK
	erasebox 0, 0, 29, 19
	releaseall
	end

CableClub_EventScript_CloseLinkAndExitLinkRoom::
	special CloseLink
	setvar VAR_CABLE_CLUB_STATE, 0
	goto_if_eq VAR_0x8007, 0, CableClub_EventScript_PlayerExitLinkRoom
	applymovement VAR_0x8007, Movement_AttendantFaceLeft
	waitmovement 0
	applymovement LOCALID_PLAYER, Movement_PlayerExitLinkRoom
	waitmovement 0
	applymovement VAR_0x8007, Movement_AttendantFaceDown
	waitmovement 0
	return

CableClub_EventScript_ExitTradeCenter::
	lockall
	call CableClub_EventScript_PlayerExitTradeCenter
	call CableClub_EventScript_CloseColosseumBarrier
	special DrawWholeMapView
	playse SE_CLICK
	erasebox 0, 0, 29, 19
	releaseall
	end

CableClub_EventScript_PlayerExitTradeCenter::
	special CloseLink
	setvar VAR_CABLE_CLUB_STATE, 0
	goto_if_eq VAR_0x8007, 0, CableClub_EventScript_PlayerExitLinkRoom
	applymovement LOCALID_PLAYER, Movement_PlayerFaceAttendantRight
	waitmovement 0
	applymovement VAR_0x8007, Movement_AttendantFaceLeft
	waitmovement 0
	call CableClub_EventScript_TrainerCardDataOverwritten
	return

CableClub_EventScript_TrainerCardDataOverwritten::
	message CableClub_Text_TrainerCardDataOverwritten
	waitmessage
	playse SE_PIN
	delay 60
	message CableClub_Text_HopeToSeeYouAgain
	waitmessage
	applymovement LOCALID_PLAYER, Movement_PlayerExitLinkRoom
	waitmovement 0
	applymovement VAR_0x8007, Movement_AttendantFaceDown
	waitmovement 0
	return

CableClub_EventScript_PlayerExitLinkRoom::
	applymovement LOCALID_PLAYER, Movement_PlayerExitLinkRoom
	waitmovement 0
	return

CableClub_EventScript_WelcomeToCableClub::
	message CableClub_Text_WelcomeWhichCableClubService
	waitmessage
	delay 28
	goto CableClub_EventScript_SelectCableClubRoom
	end

CableClub_EventScript_UnusedWelcomeToCableClub::
	msgbox CableClub_Text_WhichService, MSGBOX_DEFAULT
	goto CableClub_EventScript_SelectCableClubRoom
	end

CableClub_EventScript_SelectCableClubRoom::
	setvar VAR_0x8004, 0
	multichoice 0, 0, MULTI_CABLE_CLUB_NO_RECORD_MIX, FALSE
	switch VAR_RESULT
	case 0, CableClub_EventScript_TradeCenter
	case 1, CableClub_EventScript_Colosseum
	case 2, CableClub_EventScript_AbortLink
	case MULTI_B_PRESSED, CableClub_EventScript_AbortLink
	end

CableClub_EventScript_Colosseum::
	copyvar VAR_0x8007, VAR_LAST_TALKED
	goto CableClub_EventScript_SingleBattleMode
	end

CableClub_EventScript_SingleBattleMode::
	setvar VAR_0x8004, USING_SINGLE_BATTLE
	goto CableClub_EventScript_TryEnterColosseum
	end

CableClub_EventScript_TryEnterColosseum::
	call Common_EventScript_SaveGame
	goto_if_eq VAR_RESULT, 0, CableClub_EventScript_AbortLink
	message gText_PleaseWaitForLink
	waitmessage
	special TryBattleLinkup
	waitstate
	goto_if_eq VAR_RESULT, LINKUP_SUCCESS, CableClub_EventScript_EnterColosseum
	goto_if_eq VAR_RESULT, LINKUP_SOMEONE_NOT_READY, CableClub_EventScript_AbortLinkSomeoneNotReady
	goto_if_eq VAR_RESULT, LINKUP_DIFF_SELECTIONS, CableClub_EventScript_AbortLinkDifferentSelections
	goto_if_eq VAR_RESULT, LINKUP_WRONG_NUM_PLAYERS, CableClub_EventScript_AbortLinkWrongNumberForSingleBattle
	goto_if_eq VAR_RESULT, LINKUP_FAILED, CableClub_EventScript_AbortLink
	goto_if_eq VAR_RESULT, LINKUP_CONNECTION_ERROR, CableClub_EventScript_AbortLinkConnectionError
	end

CableClub_EventScript_EnterColosseum::
	special HealPlayerParty
	special SavePlayerParty
	copyvar VAR_CABLE_CLUB_STATE, VAR_0x8004
	messageautoscroll CableClub_Text_PleaseEnter
	waitmessage
	call CableClub_EventScript_OpenColosseumBarrier
	special DrawWholeMapView
	playse SE_CLICK
	delay 60
	applymovement VAR_LAST_TALKED, Movement_AttendantFaceLeft
	waitmovement 0
	closemessage
	applymovement LOCALID_PLAYER, Movement_PlayerApproachLinkRoomLeft
	waitmovement 0
	opendoor 9, 1
	waitdooranim
	applymovement LOCALID_PLAYER, Movement_PlayerEnterLinkRoom
	waitmovement 0
	hideplayer
	closedoor 9, 1
	waitdooranim
	release
	special SetCableClubWarp
	warp MAP_BATTLE_COLOSSEUM_2P, 6, 8
	special DoCableClubWarp
	waitstate
	end


CableClub_EventScript_AbortLinkWrongNumberForSingleBattle::
	special CloseLink
	msgbox CableClub_Text_CantSingleBattleWithXPlayers, MSGBOX_DEFAULT
	goto CableClub_EventScript_ConfirmNumberAndRestart
	end

CableClub_EventScript_ConfirmNumberAndRestart::
	special CloseLink  @ Redundant
	msgbox CableClub_Text_PleaseConfirmNumberAndRestart, MSGBOX_DEFAULT
	release
	end

CableClub_EventScript_TradeCenter::
	copyvar VAR_0x8007, VAR_LAST_TALKED
	call CableClub_EventScript_CheckPartyTradeRequirements
	goto_if_eq VAR_RESULT, 0, CableClub_EventScript_AbortLink
	call Common_EventScript_SaveGame
	goto_if_eq VAR_RESULT, 0, CableClub_EventScript_AbortLink
	message gText_PleaseWaitForLink
	waitmessage
	special TryTradeLinkup
	waitstate
	goto_if_eq VAR_RESULT, LINKUP_SUCCESS, CableClub_EventScript_EnterTradeCenter
	goto_if_eq VAR_RESULT, LINKUP_SOMEONE_NOT_READY, CableClub_EventScript_AbortLinkSomeoneNotReady
	goto_if_eq VAR_RESULT, LINKUP_DIFF_SELECTIONS, CableClub_EventScript_AbortLinkDifferentSelections
	goto_if_eq VAR_RESULT, LINKUP_WRONG_NUM_PLAYERS, CableClub_EventScript_AbortLinkIncorrectNumberOfParticipants
	goto_if_eq VAR_RESULT, LINKUP_FAILED, CableClub_EventScript_AbortLink
	goto_if_eq VAR_RESULT, LINKUP_CONNECTION_ERROR, CableClub_EventScript_AbortLinkConnectionError
	goto_if_eq VAR_RESULT, LINKUP_PLAYER_NOT_READY, CableClub_EventScript_AbortLinkPlayerNotReady
	goto_if_eq VAR_RESULT, LINKUP_PARTNER_NOT_READY, CableClub_EventScript_AbortLinkOtherTrainerNotReady
	end

CableClub_EventScript_EnterTradeCenter::
	setvar VAR_0x8004, USING_TRADE_CENTER
	copyvar VAR_CABLE_CLUB_STATE, VAR_0x8004
	messageautoscroll CableClub_Text_PleaseEnter
	waitmessage
	call CableClub_EventScript_OpenColosseumBarrier
	special DrawWholeMapView
	playse SE_CLICK
	delay 60
	applymovement VAR_LAST_TALKED, Movement_AttendantFaceLeft
	waitmovement 0
	closemessage
	applymovement LOCALID_PLAYER, Movement_PlayerApproachLinkRoomLeft
	waitmovement 0
	opendoor 9, 1
	waitdooranim
	applymovement LOCALID_PLAYER, Movement_PlayerEnterLinkRoom
	waitmovement 0
	hideplayer
	closedoor 9, 1
	waitdooranim
	release
	special SetCableClubWarp
	setwarp MAP_TRADE_CENTER, 5, 8
	special DoCableClubWarp
	waitstate
	end

CableClub_EventScript_CheckPartyTradeRequirements::
	specialvar VAR_RESULT, CalculatePlayerPartyCount
	goto_if_lt VAR_RESULT, 2, CableClub_EventScript_NeedTwoMonsToTrade
	setvar VAR_RESULT, 1
	return

CableClub_EventScript_NeedTwoMonsToTrade::
	msgbox CableClub_Text_NeedTwoMonsToTrade, MSGBOX_DEFAULT
	setvar VAR_RESULT, 0
	return

CableClub_EventScript_AbortLinkPlayerNotReady::
	special CloseLink
	msgbox CableClub_Text_NotSetUpForFarAwayRegion, MSGBOX_DEFAULT
	release
	end

CableClub_EventScript_AbortLinkOtherTrainerNotReady::
	special CloseLink
	msgbox CableClub_Text_OtherTrainerNotReady, MSGBOX_DEFAULT
	release
	end

CableClub_EventScript_AbortLinkConnectionError::
	special CloseLink
	msgbox Text_LinkErrorPleaseReset, MSGBOX_DEFAULT
	release
	end

CableClub_EventScript_AbortLinkSomeoneNotReady::
	special CloseLink
	msgbox Text_SomeoneIsNotReadyToLink, MSGBOX_DEFAULT
	release
	end

CableClub_EventScript_AbortLinkDifferentSelections::
	special CloseLink
	msgbox Text_PlayersMadeDifferentSelections, MSGBOX_DEFAULT
	release
	end

CableClub_EventScript_AbortLink::
	special CloseLink
	msgbox CableClub_Text_PleaseVisitAgain, MSGBOX_DEFAULT
	release
	end

@ Unused
CableClub_EventScript_CableClubWarp::
	special SetCableClubWarp
	special DoCableClubWarp
	waitstate
	end

CableClub_EventScript_AbortLinkIncorrectNumberOfParticipants::
	special CloseLink
	msgbox CableClub_Text_IncorrectNumberOfParticipants, MSGBOX_DEFAULT
	release
	end

CableClub_EventScript_AbortLinkPlayerHasBadEgg::
	special CloseLink
	msgbox CableClub_Text_YouHaveAMonThatCantBeTaken, MSGBOX_DEFAULT
	release
	end

CableClub_EventScript_AbortLinkForeignGame::
	special CloseLink
	msgbox CableClub_Text_CantMixWithJapaneseGame, MSGBOX_DEFAULT
	release
	end

Movement_AttendantFaceDown:
	face_down
	step_end

@ Unused
Movement_AttendantFaceRight:
	face_right
	step_end

Movement_AttendantFaceLeft:
	face_left
	step_end

Movement_PlayerExitLinkRoom:
	walk_down
	walk_down
	step_end

@ Functionally unused
Movement_PlayerApproachLinkRoomRight:
	walk_right
	walk_up
	walk_up
	step_end

Movement_PlayerApproachLinkRoomLeft:
	walk_left
	walk_up
	walk_up
	step_end

Movement_PlayerEnterLinkRoom:
	walk_up
	step_end

@ Unused
Movement_PlayerFaceAttendantLeft:
	face_left
	step_end

Movement_PlayerFaceAttendantRight:
	face_right
	step_end

EventScript_CableBoxResults::
	lockall
	setvar VAR_0x8004, 0
	special ShowLinkBattleRecords
	waitbuttonpress
	special RemoveRecordsWindow
	releaseall
	end

EventScript_BattleColosseum_2P_PlayerSpot0::
	setvar VAR_0x8005, 0
	special ColosseumPlayerSpotTriggered
	waitstate
	end

EventScript_BattleColosseum_2P_PlayerSpot1::
	setvar VAR_0x8005, 1
	special ColosseumPlayerSpotTriggered
	waitstate
	end

EventScript_TradeCenter_Chair0::
	setvar VAR_0x8005, 0
	special PlayerEnteredTradeSeat
	waitstate
	end

EventScript_TradeCenter_Chair1::
	setvar VAR_0x8005, 1
	special PlayerEnteredTradeSeat
	waitstate
	end

/* Never used */
EventScript_TradeCenter_Chair2::
	setvar VAR_0x8005, 2
	special PlayerEnteredTradeSeat
	waitstate
	end

/* Never used */
EventScript_TradeCenter_Chair3::
	setvar VAR_0x8005, 3
	special PlayerEnteredTradeSeat
	waitstate
	end

CableClub_EventScript_ReadTrainerCard::
	msgbox CableClub_Text_GotToLookAtTrainerCard, MSGBOX_DEFAULT
	fadescreen FADE_TO_BLACK
	special Script_ShowLinkTrainerCard
	waitstate
	end

CableClub_EventScript_ReadTrainerCardColored::
	msgbox CableClub_Text_GotToLookAtColoredTrainerCard, MSGBOX_DEFAULT
	fadescreen FADE_TO_BLACK
	special Script_ShowLinkTrainerCard
	waitstate
	end

CableClub_EventScript_TooBusyToNotice::
	msgbox CableClub_Text_TooBusyToNotice, MSGBOX_DEFAULT
	closemessage
	end

BattleColosseum_2P_EventScript_Attendant::
	special Script_FacePlayer
	msgbox BattleColosseum_2P_Text_TakePlaceStartBattle, MSGBOX_DEFAULT
	special Script_ClearHeldMovement
	closemessage
	end

TradeCenter_EventScript_Attendant::
	special Script_FacePlayer
	msgbox TradeCenter_Text_TakeSeatStartTrade, MSGBOX_DEFAULT
	special Script_ClearHeldMovement
	closemessage
	end

EventScript_ConfirmLeaveCableClubRoom::
	msgbox Text_TerminateLinkConfirmation, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, EventScript_TerminateLink
	erasebox 0, 0, 29, 19
	releaseall
	end

EventScript_TerminateLink::
	messageautoscroll Text_TerminateLinkPleaseWait
	waitmessage
	special ExitLinkRoom
	end

EventScript_DoLinkRoomExit::
	special CleanupLinkRoomState
	special ReturnFromLinkRoom
	waitstate
	end

CableClub_EventScript_Attendant::
	lock
	faceplayer
	setvar VAR_FRONTIER_FACILITY, FACILITY_MULTI_OR_EREADER
	specialvar VAR_RESULT, IsBadEggInParty
	goto_if_eq VAR_RESULT, TRUE, CableClub_EventScript_AbortLinkPlayerHasBadEgg
	goto CableClub_EventScript_WelcomeToCableClub
	end

CableClub_EventScript_OpenColosseumBarrier::
	setmetatile 9, 2, METATILE_PokemonCenter_Floor_ShadowTop_Alt, FALSE
	setmetatile 9, 3, METATILE_PokemonCenter_Floor_Plain_Alt, FALSE
	return

CableClub_EventScript_CloseColosseumBarrier::
	setmetatile 9, 2, METATILE_PokemonCenter_Floor_ShadowTop, TRUE
	setmetatile 9, 3, METATILE_PokemonCenter_CounterBarrier, TRUE
	return
