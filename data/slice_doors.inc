@ Slice doors and the Masara-Route 1 seam (studio world M2; owner: world, then frozen).
@
@ Masara's Porytiles tilesets give every metatile MB_NORMAL, and a warp_event only fires on a
@ door, ladder or arrow-mat behaviour, so Masara's warp_events can't fire on their own. Each
@ Masara door cell (made walkable) and the two north-edge cells carry a coord event that runs one
@ of these scripts; the warp_events in MasaraTown/map.json are the arrival points in front of the
@ doors, where the interiors' exits land. Route 1's south end is the same (FRLG grass, no warp
@ behaviour), and a connection can't be used because the two maps have different tilesets
@ (tech brief section 1).
@ PLACEHOLDER mechanism (refine in thread "Overworld art pass"): once art gives Masara's door
@ metatiles MB_NON_ANIMATED_DOOR, the door coord events can go and the warp_events move onto the
@ door cells (studio/requests.md, world M2).
@ The coord events use VAR_TEMP_F == 0: don't set VAR_TEMP_F in MasaraTown or Route1 scripts.

Slice_EventScript_MasaraToRoute1West::
	lockall
	warp MAP_ROUTE1, 12, 38
	waitstate
	releaseall
	end

Slice_EventScript_MasaraToRoute1East::
	lockall
	warp MAP_ROUTE1, 13, 38
	waitstate
	releaseall
	end

Slice_EventScript_Route1ToMasaraWest::
	lockall
	warp MAP_MASARA_TOWN, 15, 1
	waitstate
	releaseall
	end

Slice_EventScript_Route1ToMasaraEast::
	lockall
	warp MAP_MASARA_TOWN, 16, 1
	waitstate
	releaseall
	end

@ Masara doors: Satoshi's house, the Okido house, the Okido institute (lab), the Okada shop.
Slice_EventScript_MasaraDoorHome::
	lockall
	warp MAP_PALLET_TOWN_PLAYERS_HOUSE_1F, 1
	waitstate
	releaseall
	end

Slice_EventScript_MasaraDoorOkidoHouse::
	lockall
	warp MAP_PALLET_TOWN_RIVALS_HOUSE, 0
	waitstate
	releaseall
	end

Slice_EventScript_MasaraDoorLab::
	lockall
	warp MAP_PALLET_TOWN_PROFESSOR_OAKS_LAB, 0
	waitstate
	releaseall
	end

Slice_EventScript_MasaraDoorOkadaShop::
	lockall
	warp MAP_MASARA_TOWN_OKADA_SHOP, 1
	waitstate
	releaseall
	end

@ Locked fronts (slice-plan 2.3): Masara's two spare houses and Hamada's house in Tokiwa.
@ Area owners point their bg event at their own script when the door needs states
@ (Hamada after B1: tokiwa-town).
Slice_EventScript_LockedDoor::
	msgbox Slice_Text_NobodyAnswers, MSGBOX_SIGN
	end

Slice_Text_NobodyAnswers:
	.string "Nobody answers.$"

@ Tokiwa PMC (ViridianCity_Gym): the barrier box and its sign both read "Closed." until the
@ arena opens. Text from slice-plan TOK-004. PLACEHOLDER script: no gating yet; tokiwa-town replaces
@ it with its own (barrier lifts on FLAG_Q_YUJI_SLEPT, TOK-009; thread "Design arena ecology for gym battles").
Slice_EventScript_TokiwaPMCClosed::
	msgbox ViridianCity_Gym_Text_Closed, MSGBOX_SIGN
	end

@ Route 3 wall hiker in front of the rockfall at x=31 (omar-questions Q112).
@ PLACEHOLDER line (refine in thread "Define Minamoto narrative direction"; Route 3 owner later).
Slice_EventScript_Route3WallHiker::
	msgbox Route3_Text_WallHiker, MSGBOX_NPC
	end
