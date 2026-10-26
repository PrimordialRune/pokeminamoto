OldaleTown_House1_MapScripts::
	.byte 0


OldaleTown_House1_EventScript_Innkeeper::
	call Common_EventScript_Inn
	compare VAR_RESULT, TRUE
	goto_if_eq OldaleTown_House1_EventScript_Innkeeper_2
OldaleTown_House1_EventScript_Innkeeper_1:
	release
	end

OldaleTown_House1_EventScript_Innkeeper_2:
	setrespawn HEAL_LOCATION_OLDALE_TOWN_INN
	goto OldaleTown_House1_EventScript_Innkeeper_1


OldaleTown_House1_EventScript_TakeRoomItem::
	finditem ITEM_SUPER_POTION
	end

