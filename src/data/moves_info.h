// The move table: everything about a move in one place (name, battle data,
// TP cost, description). It replaces src/data/battle_moves.h,
// src/data/text/move_names.h and src/data/text/move_descriptions.h.
//
// .tpCost is the Technical Points a move spends in battle (see GetMoveTPCost in
// src/battle_util.c). The values come from the balance model's cost curve
// (tools/balance/costcurve.py, FINDINGS section 10): TP = round(1 + 0.13 x value),
// where value is one use's worth in base-power units, so Gen 1 moves cost 4-14.
// They are baked from tools/balance/tp_model.json: edit that, then run
// `python3 tools/balance/costcurve.py --write` (`make check` fails when out of sync).
//
// .rank and .signatureSpecies are placeholders for Minamoto data that no code
// reads yet (see struct MoveInfo in include/pokemon.h). Contest data still lives in
// gContestMoves (src/data/contest_moves.h) while contests are being removed.

static const u8 sNullDescription[] = _(
    "");

static const u8 sPoundDescription[] = _(
    "Pounds the foe with\n"
    "forelegs or tail.");

static const u8 sKarateChopDescription[] = _(
    "A chopping attack with a\n"
    "high critical-hit ratio.");

static const u8 sDoubleSlapDescription[] = _(
    "Repeatedly slaps the foe\n"
    "2 to 5 times.");

static const u8 sCometPunchDescription[] = _(
    "Repeatedly punches the foe\n"
    "2 to 5 times.");

static const u8 sMegaPunchDescription[] = _(
    "A strong punch thrown with\n"
    "incredible power.");

static const u8 sPayDayDescription[] = _(
    "Throws coins at the foe.\n"
    "Money is recovered after.");

static const u8 sFirePunchDescription[] = _(
    "A fiery punch that may burn\n"
    "the foe.");

static const u8 sIcePunchDescription[] = _(
    "An icy punch that may\n"
    "freeze the foe.");

static const u8 sThunderPunchDescription[] = _(
    "An electrified punch that\n"
    "may paralyze the foe.");

static const u8 sScratchDescription[] = _(
    "Scratches the foe with\n"
    "sharp claws.");

static const u8 sViceGripDescription[] = _(
    "Grips the foe with large and\n"
    "powerful pincers.");

static const u8 sGuillotineDescription[] = _(
    "A powerful pincer attack\n"
    "that may cause fainting.");

static const u8 sRazorWindDescription[] = _(
    "A 2-turn move that strikes\n"
    "the foe on the 2nd turn.");

static const u8 sSwordsDanceDescription[] = _(
    "A fighting dance that\n"
    "sharply raises ATTACK.");

static const u8 sCutDescription[] = _(
    "Cuts the foe with sharp\n"
    "scythes, claws, etc.");

static const u8 sGustDescription[] = _(
    "Strikes the foe with a gust\n"
    "of wind whipped up by wings.");

static const u8 sWingAttackDescription[] = _(
    "Strikes the foe with wings\n"
    "spread wide.");

static const u8 sWhirlwindDescription[] = _(
    "Blows away the foe with\n"
    "wind and ends the battle.");

static const u8 sFlyDescription[] = _(
    "Flies up on the first turn,\n"
    "then strikes the next turn.");

static const u8 sBindDescription[] = _(
    "Binds and squeezes the foe\n"
    "for 2 to 5 turns.");

static const u8 sSlamDescription[] = _(
    "Slams the foe with a long\n"
    "tail, vine, etc.");

static const u8 sVineWhipDescription[] = _(
    "Strikes the foe with\n"
    "slender, whiplike vines.");

static const u8 sStompDescription[] = _(
    "Stomps the enemy with a big\n"
    "foot. May cause flinching.");

static const u8 sDoubleKickDescription[] = _(
    "A double-kicking attack\n"
    "that strikes the foe twice.");

static const u8 sMegaKickDescription[] = _(
    "An extremely powerful kick\n"
    "with intense force.");

static const u8 sJumpKickDescription[] = _(
    "A strong jumping kick. May\n"
    "miss and hurt the kicker.");

static const u8 sRollingKickDescription[] = _(
    "A fast kick delivered from\n"
    "a rapid spin.");

static const u8 sSandAttackDescription[] = _(
    "Reduces the foe's accuracy\n"
    "by hurling sand in its face.");

static const u8 sHeadbuttDescription[] = _(
    "A ramming attack that may\n"
    "cause flinching.");

static const u8 sHornAttackDescription[] = _(
    "Jabs the foe with sharp\n"
    "horns.");

static const u8 sFuryAttackDescription[] = _(
    "Jabs the foe 2 to 5 times\n"
    "with sharp horns, etc.");

static const u8 sHornDrillDescription[] = _(
    "A one-hit KO attack that\n"
    "uses a horn like a drill.");

static const u8 sTackleDescription[] = _(
    "Charges the foe with a full-\n"
    "body tackle.");

static const u8 sBodySlamDescription[] = _(
    "A full-body slam that may\n"
    "cause paralysis.");

static const u8 sWrapDescription[] = _(
    "Wraps and squeezes the foe\n"
    "2 to 5 times with vines, etc.");

static const u8 sTakeDownDescription[] = _(
    "A reckless charge attack\n"
    "that also hurts the user.");

static const u8 sThrashDescription[] = _(
    "A rampage of 2 to 3 turns\n"
    "that confuses the user.");

static const u8 sDoubleEdgeDescription[] = _(
    "A life-risking tackle that\n"
    "also hurts the user.");

static const u8 sTailWhipDescription[] = _(
    "Wags the tail to lower the\n"
    "foe's DEFENSE.");

static const u8 sPoisonStingDescription[] = _(
    "A toxic attack with barbs,\n"
    "etc., that may poison.");

static const u8 sTwineedleDescription[] = _(
    "Stingers on the forelegs\n"
    "jab the foe twice.");

static const u8 sPinMissileDescription[] = _(
    "Sharp pins are fired to\n"
    "strike 2 to 5 times.");

static const u8 sLeerDescription[] = _(
    "Frightens the foe with a\n"
    "leer to lower DEFENSE.");

static const u8 sBiteDescription[] = _(
    "Bites with vicious fangs.\n"
    "May cause flinching.");

static const u8 sGrowlDescription[] = _(
    "Growls cutely to reduce the\n"
    "foe's ATTACK.");

static const u8 sRoarDescription[] = _(
    "Makes the foe flee to end\n"
    "the battle.");

static const u8 sSingDescription[] = _(
    "A soothing song lulls the\n"
    "foe into a deep slumber.");

static const u8 sSupersonicDescription[] = _(
    "Emits bizarre sound waves\n"
    "that may confuse the foe.");

static const u8 sSonicBoomDescription[] = _(
    "Launches shock waves that\n"
    "always inflict 20 HP damage.");

static const u8 sDisableDescription[] = _(
    "Psychically disables one of\n"
    "the foe's moves.");

static const u8 sAcidDescription[] = _(
    "Sprays a hide-melting acid.\n"
    "May lower DEFENSE.");

static const u8 sEmberDescription[] = _(
    "A weak fire attack that may\n"
    "inflict a burn.");

static const u8 sFlamethrowerDescription[] = _(
    "A powerful fire attack that\n"
    "may inflict a burn.");

static const u8 sMistDescription[] = _(
    "Creates a mist that stops\n"
    "reduction of abilities.");

static const u8 sWaterGunDescription[] = _(
    "Squirts water to attack\n"
    "the foe.");

static const u8 sHydroPumpDescription[] = _(
    "Blasts water at high power\n"
    "to strike the foe.");

static const u8 sSurfDescription[] = _(
    "Creates a huge wave, then\n"
    "crashes it down on the foe.");

static const u8 sIceBeamDescription[] = _(
    "Blasts the foe with an icy\n"
    "beam that may freeze it.");

static const u8 sBlizzardDescription[] = _(
    "Hits the foe with an icy\n"
    "storm that may freeze it.");

static const u8 sPsybeamDescription[] = _(
    "Fires a peculiar ray that\n"
    "may confuse the foe.");

static const u8 sBubbleBeamDescription[] = _(
    "Forcefully sprays bubbles\n"
    "that may lower SPEED.");

static const u8 sAuroraBeamDescription[] = _(
    "Fires a rainbow-colored\n"
    "beam that may lower ATTACK.");

static const u8 sHyperBeamDescription[] = _(
    "Powerful, but leaves the\n"
    "user immobile the next turn.");

static const u8 sPeckDescription[] = _(
    "Attacks the foe with a\n"
    "jabbing beak, etc.");

static const u8 sDrillPeckDescription[] = _(
    "A corkscrewing attack with\n"
    "the beak acting as a drill.");

static const u8 sSubmissionDescription[] = _(
    "A reckless body slam that\n"
    "also hurts the user.");

static const u8 sLowKickDescription[] = _(
    "A kick that inflicts more\n"
    "damage on heavier foes.");

static const u8 sCounterDescription[] = _(
    "Retaliates any physical hit\n"
    "with double the power.");

static const u8 sSeismicTossDescription[] = _(
    "Inflicts damage identical\n"
    "to the user's level.");

static const u8 sStrengthDescription[] = _(
    "Builds enormous power,\n"
    "then slams the foe.");

static const u8 sAbsorbDescription[] = _(
    "An attack that absorbs\n"
    "half the damage inflicted.");

static const u8 sMegaDrainDescription[] = _(
    "An attack that absorbs\n"
    "half the damage inflicted.");

static const u8 sLeechSeedDescription[] = _(
    "Plants a seed on the foe to\n"
    "steal HP on every turn.");

static const u8 sGrowthDescription[] = _(
    "Forces the body to grow\n"
    "and heightens SP. ATK.");

static const u8 sRazorLeafDescription[] = _(
    "Cuts the enemy with leaves.\n"
    "High critical-hit ratio.");

static const u8 sSolarBeamDescription[] = _(
    "Absorbs light in one turn,\n"
    "then attacks next turn.");

static const u8 sPoisonPowderDescription[] = _(
    "Scatters a toxic powder\n"
    "that may poison the foe.");

static const u8 sStunSporeDescription[] = _(
    "Scatters a powder that may\n"
    "paralyze the foe.");

static const u8 sSleepPowderDescription[] = _(
    "Scatters a powder that may\n"
    "cause the foe to sleep.");

static const u8 sPetalDanceDescription[] = _(
    "A rampage of 2 to 3 turns\n"
    "that confuses the user.");

static const u8 sStringShotDescription[] = _(
    "Binds the foe with string\n"
    "to reduce its SPEED.");

static const u8 sDragonRageDescription[] = _(
    "Launches shock waves that\n"
    "always inflict 40 HP damage.");

static const u8 sFireSpinDescription[] = _(
    "Traps the foe in a ring of\n"
    "fire for 2 to 5 turns.");

static const u8 sThunderShockDescription[] = _(
    "An electrical attack that\n"
    "may paralyze the foe.");

static const u8 sThunderboltDescription[] = _(
    "A strong electrical attack\n"
    "that may paralyze the foe.");

static const u8 sThunderWaveDescription[] = _(
    "A weak jolt of electricity\n"
    "that paralyzes the foe.");

static const u8 sThunderDescription[] = _(
    "A lightning attack that may\n"
    "cause paralysis.");

static const u8 sRockThrowDescription[] = _(
    "Throws small rocks to\n"
    "strike the foe.");

static const u8 sEarthquakeDescription[] = _(
    "A powerful quake, but has\n"
    "no effect on flying foes.");

static const u8 sFissureDescription[] = _(
    "A one-hit KO move that\n"
    "drops the foe in a fissure.");

static const u8 sDigDescription[] = _(
    "Digs underground the first\n"
    "turn and strikes next turn.");

static const u8 sToxicDescription[] = _(
    "Poisons the foe with an\n"
    "intensifying toxin.");

static const u8 sConfusionDescription[] = _(
    "A psychic attack that may\n"
    "cause confusion.");

static const u8 sPsychicDescription[] = _(
    "A powerful psychic attack\n"
    "that may lower SP. DEF.");

static const u8 sHypnosisDescription[] = _(
    "A hypnotizing move that\n"
    "may induce sleep.");

static const u8 sMeditateDescription[] = _(
    "Meditates in a peaceful\n"
    "fashion to raise ATTACK.");

static const u8 sAgilityDescription[] = _(
    "Relaxes the body to sharply\n"
    "boost SPEED.");

static const u8 sQuickAttackDescription[] = _(
    "An extremely fast attack\n"
    "that always strikes first.");

static const u8 sRageDescription[] = _(
    "Raises the user's ATTACK\n"
    "every time it is hit.");

static const u8 sTeleportDescription[] = _(
    "A psychic move for fleeing\n"
    "from battle instantly.");

static const u8 sNightShadeDescription[] = _(
    "Inflicts damage identical\n"
    "to the user's level.");

static const u8 sMimicDescription[] = _(
    "Copies a move used by the\n"
    "foe during one battle.");

static const u8 sScreechDescription[] = _(
    "Emits a screech to sharply\n"
    "reduce the foe's DEFENSE.");

static const u8 sDoubleTeamDescription[] = _(
    "Creates illusory copies to\n"
    "raise evasiveness.");

static const u8 sRecoverDescription[] = _(
    "Recovers up to half the\n"
    "user's maximum HP.");

static const u8 sHardenDescription[] = _(
    "Stiffens the body's \n"
    "muscles to raise DEFENSE.");

static const u8 sMinimizeDescription[] = _(
    "Minimizes the user's size\n"
    "to raise evasiveness.");

static const u8 sSmokescreenDescription[] = _(
    "Lowers the foe's accuracy\n"
    "using smoke, ink, etc.");

static const u8 sConfuseRayDescription[] = _(
    "A sinister ray that\n"
    "confuses the foe.");

static const u8 sWithdrawDescription[] = _(
    "Withdraws the body into its\n"
    "hard shell to raise DEFENSE.");

static const u8 sDefenseCurlDescription[] = _(
    "Curls up to conceal weak\n"
    "spots and raise DEFENSE.");

static const u8 sBarrierDescription[] = _(
    "Creates a barrier that\n"
    "sharply raises DEFENSE.");

static const u8 sLightScreenDescription[] = _(
    "Creates a wall of light that\n"
    "lowers SP. ATK damage.");

static const u8 sHazeDescription[] = _(
    "Creates a black haze that\n"
    "eliminates all stat changes.");

static const u8 sReflectDescription[] = _(
    "Creates a wall of light that\n"
    "weakens physical attacks.");

static const u8 sFocusEnergyDescription[] = _(
    "Focuses power to raise the\n"
    "critical-hit ratio.");

static const u8 sBideDescription[] = _(
    "Endures attack for 2\n"
    "turns to retaliate double.");

static const u8 sMetronomeDescription[] = _(
    "Waggles a finger to use any\n"
    "POKéMON move at random.");

static const u8 sMirrorMoveDescription[] = _(
    "Counters the foe's attack\n"
    "with the same move.");

static const u8 sSelfDestructDescription[] = _(
    "Inflicts severe damage but\n"
    "makes the user faint.");

static const u8 sEggBombDescription[] = _(
    "An egg is forcibly hurled at\n"
    "the foe.");

static const u8 sLickDescription[] = _(
    "Licks with a long tongue to\n"
    "injure. May also paralyze.");

static const u8 sSmogDescription[] = _(
    "An exhaust-gas attack\n"
    "that may also poison.");

static const u8 sSludgeDescription[] = _(
    "Sludge is hurled to inflict\n"
    "damage. May also poison.");

static const u8 sBoneClubDescription[] = _(
    "Clubs the foe with a bone.\n"
    "May cause flinching.");

static const u8 sFireBlastDescription[] = _(
    "Incinerates everything it\n"
    "strikes. May cause a burn.");

static const u8 sWaterfallDescription[] = _(
    "Charges the foe with speed\n"
    "to climb waterfalls.");

static const u8 sClampDescription[] = _(
    "Traps and squeezes the\n"
    "foe for 2 to 5 turns.");

static const u8 sSwiftDescription[] = _(
    "Sprays star-shaped rays\n"
    "that never miss.");

static const u8 sSkullBashDescription[] = _(
    "Tucks in the head, then\n"
    "attacks on the next turn.");

static const u8 sSpikeCannonDescription[] = _(
    "Launches sharp spikes that\n"
    "strike 2 to 5 times.");

static const u8 sConstrictDescription[] = _(
    "Constricts to inflict pain.\n"
    "May lower SPEED.");

static const u8 sAmnesiaDescription[] = _(
    "Forgets about something\n"
    "and sharply raises SP. DEF.");

static const u8 sKinesisDescription[] = _(
    "Distracts the foe.\n"
    "May lower accuracy.");

static const u8 sSoftBoiledDescription[] = _(
    "Recovers up to half the\n"
    "user's maximum HP.");

static const u8 sHiJumpKickDescription[] = _(
    "A jumping knee kick. If it\n"
    "misses, the user is hurt.");

static const u8 sGlareDescription[] = _(
    "Intimidates and frightens\n"
    "the foe into paralysis.");

static const u8 sDreamEaterDescription[] = _(
    "Takes one half the damage\n"
    "inflicted on a sleeping foe.");

static const u8 sPoisonGasDescription[] = _(
    "Envelops the foe in a toxic\n"
    "gas that may poison.");

static const u8 sBarrageDescription[] = _(
    "Hurls round objects at the\n"
    "foe 2 to 5 times.");

static const u8 sLeechLifeDescription[] = _(
    "An attack that steals half\n"
    "the damage inflicted.");

static const u8 sLovelyKissDescription[] = _(
    "Demands a kiss with a scary\n"
    "face that induces sleep.");

static const u8 sSkyAttackDescription[] = _(
    "Searches out weak spots,\n"
    "then strikes the next turn.");

static const u8 sTransformDescription[] = _(
    "Alters the user's cells to\n"
    "become a copy of the foe.");

static const u8 sBubbleDescription[] = _(
    "An attack using bubbles.\n"
    "May lower the foe's SPEED.");

static const u8 sDizzyPunchDescription[] = _(
    "A rhythmic punch that may\n"
    "confuse the foe.");

static const u8 sSporeDescription[] = _(
    "Scatters a cloud of spores\n"
    "that always induce sleep.");

static const u8 sFlashDescription[] = _(
    "Looses a powerful blast of\n"
    "light that cuts accuracy.");

static const u8 sPsywaveDescription[] = _(
    "Attacks with a psychic\n"
    "wave of varying intensity.");

static const u8 sSplashDescription[] = _(
    "It's just a splash...\n"
    "Has no effect whatsoever.");

static const u8 sAcidArmorDescription[] = _(
    "Liquifies the user's body\n"
    "to sharply raise DEFENSE.");

static const u8 sCrabhammerDescription[] = _(
    "Hammers with a pincer. Has a\n"
    "high critical-hit ratio.");

static const u8 sExplosionDescription[] = _(
    "Inflicts severe damage but\n"
    "makes the user faint.");

static const u8 sFurySwipesDescription[] = _(
    "Rakes the foe with sharp\n"
    "claws, etc., 2 to 5 times.");

static const u8 sBonemerangDescription[] = _(
    "Throws a bone boomerang\n"
    "that strikes twice.");

static const u8 sRestDescription[] = _(
    "The user sleeps for 2 turns,\n"
    "restoring HP and status.");

static const u8 sRockSlideDescription[] = _(
    "Large boulders are hurled.\n"
    "May cause flinching.");

static const u8 sHyperFangDescription[] = _(
    "Attacks with sharp fangs.\n"
    "May cause flinching.");

static const u8 sSharpenDescription[] = _(
    "Reduces the polygon count\n"
    "and raises ATTACK.");

static const u8 sConversionDescription[] = _(
    "Changes the user's type\n"
    "into a known move's type.");

static const u8 sTriAttackDescription[] = _(
    "Fires three types of beams\n"
    "at the same time.");

static const u8 sSuperFangDescription[] = _(
    "Attacks with sharp fangs\n"
    "and cuts half the foe's HP.");

static const u8 sSlashDescription[] = _(
    "Slashes with claws, etc. Has\n"
    "a high critical-hit ratio.");

static const u8 sSubstituteDescription[] = _(
    "Creates a decoy using 1/4\n"
    "of the user's maximum HP.");

static const u8 sStruggleDescription[] = _(
    "Used when no move's TP can\n"
    "be paid. Hurts the user.");

static const u8 sSketchDescription[] = _(
    "Copies the foe's last move\n"
    "permanently.");

static const u8 sTripleKickDescription[] = _(
    "Kicks the foe 3 times in a\n"
    "row with rising intensity.");

static const u8 sThiefDescription[] = _(
    "While attacking, it may\n"
    "steal the foe's held item.");

static const u8 sSpiderWebDescription[] = _(
    "Ensnares the foe to stop it\n"
    "from fleeing or switching.");

static const u8 sMindReaderDescription[] = _(
    "Senses the foe's action to\n"
    "ensure the next move's hit.");

static const u8 sNightmareDescription[] = _(
    "Inflicts 1/4 damage on a\n"
    "sleeping foe every turn.");

static const u8 sFlameWheelDescription[] = _(
    "A fiery charge attack that\n"
    "may inflict a burn.");

static const u8 sSnoreDescription[] = _(
    "A loud attack that can be\n"
    "used only while asleep.");

static const u8 sCurseDescription[] = _(
    "A move that functions\n"
    "differently for GHOSTS.");

static const u8 sFlailDescription[] = _(
    "Inflicts more damage when\n"
    "the user's HP is down.");

static const u8 sConversion2Description[] = _(
    "Makes the user resistant\n"
    "to the last attack's type.");

static const u8 sAeroblastDescription[] = _(
    "Launches a vacuumed blast.\n"
    "High critical-hit ratio.");

static const u8 sCottonSporeDescription[] = _(
    "Spores cling to the foe,\n"
    "sharply reducing SPEED.");

static const u8 sReversalDescription[] = _(
    "Inflicts more damage when\n"
    "the user's HP is down.");

static const u8 sSpiteDescription[] = _(
    "Has no effect for now:\n"
    "it cut PP, which is gone.");

static const u8 sPowderSnowDescription[] = _(
    "Blasts the foe with a snowy\n"
    "gust. May cause freezing.");

static const u8 sProtectDescription[] = _(
    "Evades attack, but may fail\n"
    "if used in succession.");

static const u8 sMachPunchDescription[] = _(
    "A punch is thrown at wicked\n"
    "speed to strike first.");

static const u8 sScaryFaceDescription[] = _(
    "Frightens with a scary face\n"
    "to sharply reduce SPEED.");

static const u8 sFaintAttackDescription[] = _(
    "Draws the foe close, then\n"
    "strikes without fail.");

static const u8 sSweetKissDescription[] = _(
    "Demands a kiss with a cute\n"
    "look. May cause confusion.");

static const u8 sBellyDrumDescription[] = _(
    "Maximizes ATTACK while\n"
    "sacrificing HP.");

static const u8 sSludgeBombDescription[] = _(
    "Sludge is hurled to inflict\n"
    "damage. May also poison.");

static const u8 sMudSlapDescription[] = _(
    "Hurls mud in the foe's face\n"
    "to reduce its accuracy.");

static const u8 sOctazookaDescription[] = _(
    "Fires a lump of ink to\n"
    "damage and cut accuracy.");

static const u8 sSpikesDescription[] = _(
    "Sets spikes that hurt a \n"
    "foe switching in.");

static const u8 sZapCannonDescription[] = _(
    "Powerful and sure to cause\n"
    "paralysis, but inaccurate.");

static const u8 sForesightDescription[] = _(
    "Negates the foe's efforts\n"
    "to heighten evasiveness.");

static const u8 sDestinyBondDescription[] = _(
    "If the user faints, the foe\n"
    "is also made to faint.");

static const u8 sPerishSongDescription[] = _(
    "Any POKéMON hearing this\n"
    "song faints in 3 turns.");

static const u8 sIcyWindDescription[] = _(
    "A chilling attack that\n"
    "lowers the foe's SPEED.");

static const u8 sDetectDescription[] = _(
    "Evades attack, but may fail\n"
    "if used in succession.");

static const u8 sBoneRushDescription[] = _(
    "Strikes the foe with a bone\n"
    "in hand 2 to 5 times.");

static const u8 sLockOnDescription[] = _(
    "Locks on to the foe to\n"
    "ensure the next move hits.");

static const u8 sOutrageDescription[] = _(
    "A rampage of 2 to 3 turns\n"
    "that confuses the user.");

static const u8 sSandstormDescription[] = _(
    "Causes a sandstorm that\n"
    "rages for several turns.");

static const u8 sGigaDrainDescription[] = _(
    "An attack that steals half\n"
    "the damage inflicted.");

static const u8 sEndureDescription[] = _(
    "Endures any attack for\n"
    "1 turn, leaving at least 1HP.");

static const u8 sCharmDescription[] = _(
    "Charms the foe and sharply\n"
    "reduces its ATTACK.");

static const u8 sRolloutDescription[] = _(
    "An attack lasting 5 turns\n"
    "with rising intensity.");

static const u8 sFalseSwipeDescription[] = _(
    "An attack that leaves the\n"
    "foe with at least 1 HP.");

static const u8 sSwaggerDescription[] = _(
    "Confuses the foe, but also\n"
    "sharply raises ATTACK.");

static const u8 sMilkDrinkDescription[] = _(
    "Recovers up to half the\n"
    "user's maximum HP.");

static const u8 sSparkDescription[] = _(
    "An electrified tackle that\n"
    "may paralyze the foe.");

static const u8 sFuryCutterDescription[] = _(
    "An attack that intensifies\n"
    "on each successive hit.");

static const u8 sSteelWingDescription[] = _(
    "Strikes the foe with hard\n"
    "wings spread wide.");

static const u8 sMeanLookDescription[] = _(
    "Fixes the foe with a mean\n"
    "look that prevents escape.");

static const u8 sAttractDescription[] = _(
    "Makes the opposite gender\n"
    "less likely to attack.");

static const u8 sSleepTalkDescription[] = _(
    "Uses an available move\n"
    "randomly while asleep.");

static const u8 sHealBellDescription[] = _(
    "Chimes soothingly to heal\n"
    "all status abnormalities.");

static const u8 sReturnDescription[] = _(
    "An attack that increases\n"
    "in power with friendship.");

static const u8 sPresentDescription[] = _(
    "A gift in the form of a\n"
    "bomb. May restore HP.");

static const u8 sFrustrationDescription[] = _(
    "An attack that is stronger\n"
    "if the TRAINER is disliked.");

static const u8 sSafeguardDescription[] = _(
    "A mystical force prevents\n"
    "all status problems.");

static const u8 sPainSplitDescription[] = _(
    "Adds the user and foe's HP,\n"
    "then shares them equally.");

static const u8 sSacredFireDescription[] = _(
    "A mystical fire attack that\n"
    "may inflict a burn.");

static const u8 sMagnitudeDescription[] = _(
    "A ground-shaking attack\n"
    "of random intensity.");

static const u8 sDynamicPunchDescription[] = _(
    "Powerful and sure to cause\n"
    "confusion, but inaccurate.");

static const u8 sMegahornDescription[] = _(
    "A brutal ramming attack\n"
    "using out-thrust horns.");

static const u8 sDragonBreathDescription[] = _(
    "Strikes the foe with an\n"
    "incredible blast of breath.");

static const u8 sBatonPassDescription[] = _(
    "Switches out the user while\n"
    "keeping effects in play.");

static const u8 sEncoreDescription[] = _(
    "Makes the foe repeat its\n"
    "last move over 2 to 6 turns.");

static const u8 sPursuitDescription[] = _(
    "Inflicts bad damage if used\n"
    "on a foe switching out.");

static const u8 sRapidSpinDescription[] = _(
    "Spins the body at high\n"
    "speed to strike the foe.");

static const u8 sSweetScentDescription[] = _(
    "Allures the foe to reduce\n"
    "evasiveness.");

static const u8 sIronTailDescription[] = _(
    "Attacks with a rock-hard\n"
    "tail. May lower DEFENSE.");

static const u8 sMetalClawDescription[] = _(
    "A claw attack that may\n"
    "raise the user's ATTACK.");

static const u8 sVitalThrowDescription[] = _(
    "Makes the user's move last,\n"
    "but it never misses.");

static const u8 sMorningSunDescription[] = _(
    "Restores HP. The amount\n"
    "varies with the weather.");

static const u8 sSynthesisDescription[] = _(
    "Restores HP. The amount\n"
    "varies with the weather.");

static const u8 sMoonlightDescription[] = _(
    "Restores HP. The amount\n"
    "varies with the weather.");

static const u8 sHiddenPowerDescription[] = _(
    "The effectiveness varies\n"
    "with the user.");

static const u8 sCrossChopDescription[] = _(
    "A double-chopping attack.\n"
    "High critical-hit ratio.");

static const u8 sTwisterDescription[] = _(
    "Whips up a vicious twister\n"
    "to tear at the foe.");

static const u8 sRainDanceDescription[] = _(
    "Boosts the power of WATER-\n"
    "type moves for 5 turns.");

static const u8 sSunnyDayDescription[] = _(
    "Boosts the power of FIRE-\n"
    "type moves for 5 turns.");

static const u8 sCrunchDescription[] = _(
    "Crunches with sharp fangs.\n"
    "May lower SP. DEF.");

static const u8 sMirrorCoatDescription[] = _(
    "Counters the foe's special\n"
    "attack at double the power.");

static const u8 sPsychUpDescription[] = _(
    "Copies the foe's effect(s)\n"
    "and gives to the user.");

static const u8 sExtremeSpeedDescription[] = _(
    "An extremely fast and\n"
    "powerful attack.");

static const u8 sAncientPowerDescription[] = _(
    "An attack that may raise\n"
    "all stats.");

static const u8 sShadowBallDescription[] = _(
    "Hurls a black blob that may\n"
    "lower the foe's SP. DEF.");

static const u8 sFutureSightDescription[] = _(
    "Heightens inner power to\n"
    "strike 2 turns later.");

static const u8 sRockSmashDescription[] = _(
    "A rock-crushing attack\n"
    "that may lower DEFENSE.");

static const u8 sWhirlpoolDescription[] = _(
    "Traps and hurts the foe in\n"
    "a whirlpool for 2 to 5 turns.");

static const u8 sBeatUpDescription[] = _(
    "Summons party POKéMON to\n"
    "join in the attack.");

static const u8 sFakeOutDescription[] = _(
    "A 1st-turn, 1st-strike move\n"
    "that causes flinching.");

static const u8 sUproarDescription[] = _(
    "Causes an uproar for 2 to 5\n"
    "turns and prevents sleep.");

static const u8 sStockpileDescription[] = _(
    "Charges up power for up to\n"
    "3 turns.");

static const u8 sSpitUpDescription[] = _(
    "Releases stockpiled power\n"
    "(the more the better).");

static const u8 sSwallowDescription[] = _(
    "Absorbs stockpiled power\n"
    "and restores HP.");

static const u8 sHeatWaveDescription[] = _(
    "Exhales a hot breath on the\n"
    "foe. May inflict a burn.");

static const u8 sHailDescription[] = _(
    "Summons a hailstorm that\n"
    "strikes every turn.");

static const u8 sTormentDescription[] = _(
    "Torments the foe and stops\n"
    "successive use of a move.");

static const u8 sFlatterDescription[] = _(
    "Confuses the foe, but\n"
    "raises its SP. ATK.");

static const u8 sWillOWispDescription[] = _(
    "Inflicts a burn on the foe\n"
    "with intense fire.");

static const u8 sMementoDescription[] = _(
    "The user faints and lowers\n"
    "the foe's abilities.");

static const u8 sFacadeDescription[] = _(
    "Boosts ATTACK when burned,\n"
    "paralyzed, or poisoned.");

static const u8 sFocusPunchDescription[] = _(
    "A powerful loyalty attack.\n"
    "The user flinches if hit.");

static const u8 sSmellingSaltDescription[] = _(
    "Powerful against paralyzed\n"
    "foes, but also heals them.");

static const u8 sFollowMeDescription[] = _(
    "Draws attention to make\n"
    "foes attack only the user.");

static const u8 sNaturePowerDescription[] = _(
    "The type of attack varies\n"
    "depending on the location.");

static const u8 sChargeDescription[] = _(
    "Charges power to boost the\n"
    "electric move used next.");

static const u8 sTauntDescription[] = _(
    "Taunts the foe into only\n"
    "using attack moves.");

static const u8 sHelpingHandDescription[] = _(
    "Boosts the power of the\n"
    "recipient's moves.");

static const u8 sTrickDescription[] = _(
    "Tricks the foe into trading\n"
    "held items.");

static const u8 sRolePlayDescription[] = _(
    "Mimics the target and\n"
    "copies its special ability.");

static const u8 sWishDescription[] = _(
    "A wish that restores HP.\n"
    "It takes time to work.");

static const u8 sAssistDescription[] = _(
    "Attacks randomly with one\n"
    "of the partner's moves.");

static const u8 sIngrainDescription[] = _(
    "Lays roots that restore HP.\n"
    "The user can't switch out.");

static const u8 sSuperpowerDescription[] = _(
    "Boosts strength sharply,\n"
    "but lowers abilities.");

static const u8 sMagicCoatDescription[] = _(
    "Reflects special effects\n"
    "back to the attacker.");

static const u8 sRecycleDescription[] = _(
    "Recycles a used item for\n"
    "one more use.");

static const u8 sRevengeDescription[] = _(
    "An attack that gains power\n"
    "if injured by the foe.");

static const u8 sBrickBreakDescription[] = _(
    "Destroys barriers such as\n"
    "REFLECT and causes damage.");

static const u8 sYawnDescription[] = _(
    "Lulls the foe into yawning,\n"
    "then sleeping next turn.");

static const u8 sKnockOffDescription[] = _(
    "Knocks down the foe's held\n"
    "item to prevent its use.");

static const u8 sEndeavorDescription[] = _(
    "Gains power if the user's HP\n"
    "is lower than the foe's HP.");

static const u8 sEruptionDescription[] = _(
    "The higher the user's HP,\n"
    "the more damage caused.");

static const u8 sSkillSwapDescription[] = _(
    "The user swaps special\n"
    "abilities with the target.");

static const u8 sImprisonDescription[] = _(
    "Prevents foes from using\n"
    "moves known by the user.");

static const u8 sRefreshDescription[] = _(
    "Heals poisoning, paralysis,\n"
    "or a burn.");

static const u8 sGrudgeDescription[] = _(
    "Has no effect for now:\n"
    "it emptied PP, now gone.");

static const u8 sSnatchDescription[] = _(
    "Steals the effects of the\n"
    "move the target uses next.");

static const u8 sSecretPowerDescription[] = _(
    "An attack with effects\n"
    "that vary by location.");

static const u8 sDiveDescription[] = _(
    "Dives underwater the first\n"
    "turn and strikes next turn.");

static const u8 sArmThrustDescription[] = _(
    "Straight-arm punches that\n"
    "strike the foe 2 to 5 times.");

static const u8 sCamouflageDescription[] = _(
    "Alters the POKéMON's type\n"
    "depending on the location.");

static const u8 sTailGlowDescription[] = _(
    "Flashes a light that sharply\n"
    "raises SP. ATK.");

static const u8 sLusterPurgeDescription[] = _(
    "Attacks with a burst of\n"
    "light. May lower SP. DEF.");

static const u8 sMistBallDescription[] = _(
    "Attacks with a flurry of\n"
    "down. May lower SP. ATK.");

static const u8 sFeatherDanceDescription[] = _(
    "Envelops the foe with down\n"
    "to sharply reduce ATTACK.");

static const u8 sTeeterDanceDescription[] = _(
    "Confuses all POKéMON on\n"
    "the scene.");

static const u8 sBlazeKickDescription[] = _(
    "A kick with a high critical-\n"
    "hit ratio. May cause a burn.");

static const u8 sMudSportDescription[] = _(
    "Covers the user in mud to\n"
    "raise electrical resistance.");

static const u8 sIceBallDescription[] = _(
    "A 5-turn attack that gains\n"
    "power on successive hits.");

static const u8 sNeedleArmDescription[] = _(
    "Attacks with thorny arms.\n"
    "May cause flinching.");

static const u8 sSlackOffDescription[] = _(
    "Slacks off and restores\n"
    "half the maximum HP.");

static const u8 sHyperVoiceDescription[] = _(
    "A loud attack that uses\n"
    "sound waves to injure.");

static const u8 sPoisonFangDescription[] = _(
    "A sharp-fanged attack.\n"
    "May badly poison the foe.");

static const u8 sCrushClawDescription[] = _(
    "Tears at the foe with sharp\n"
    "claws. May lower DEFENSE.");

static const u8 sBlastBurnDescription[] = _(
    "Powerful, but leaves the\n"
    "user immobile the next turn.");

static const u8 sHydroCannonDescription[] = _(
    "Powerful, but leaves the\n"
    "user immobile the next turn.");

static const u8 sMeteorMashDescription[] = _(
    "Fires a meteor-like punch.\n"
    "May raise ATTACK.");

static const u8 sAstonishDescription[] = _(
    "An attack that may shock\n"
    "the foe into flinching.");

static const u8 sWeatherBallDescription[] = _(
    "The move's type and power\n"
    "change with the weather.");

static const u8 sAromatherapyDescription[] = _(
    "Heals all status problems\n"
    "with a soothing scent.");

static const u8 sFakeTearsDescription[] = _(
    "Feigns crying to sharply\n"
    "lower the foe's SP. DEF.");

static const u8 sAirCutterDescription[] = _(
    "Hacks with razorlike wind.\n"
    "High critical-hit ratio.");

static const u8 sOverheatDescription[] = _(
    "Allows a full-power attack,\n"
    "but sharply lowers SP. ATK.");

static const u8 sFlameClawsDescription[] = _(
    "Rakes the foe with blazing\n"
    "claws. May cause a burn.");

#if STARTER_SIGNATURES_PROPOSAL
static const u8 sGreedyPlantDescription[] = _(
    "Roots drink the foe dry.\n"
    "Drains half the damage.");

static const u8 sSkullBashSignatureDescription[] = _(
    "A shell-first charge that\n"
    "hits after a turn to hide.");
#endif

static const u8 sLeekSlapDescription[] = _(
    "Slaps the foe with a leek.\n"
    "High critical-hit ratio.");

static const u8 sOdorSleuthDescription[] = _(
    "Negates the foe's efforts\n"
    "to heighten evasiveness.");

static const u8 sRockTombDescription[] = _(
    "Stops the foe from moving\n"
    "with rocks and cuts SPEED.");

static const u8 sSilverWindDescription[] = _(
    "A powdery attack that may\n"
    "raise abilities.");

static const u8 sMetalSoundDescription[] = _(
    "Emits a horrible screech\n"
    "that sharply lowers SP. DEF.");

static const u8 sGrassWhistleDescription[] = _(
    "Lulls the foe into sleep\n"
    "with a pleasant melody.");

static const u8 sTickleDescription[] = _(
    "Makes the foe laugh to\n"
    "lower ATTACK and DEFENSE.");

static const u8 sCosmicPowerDescription[] = _(
    "Raises DEFENSE and SP. DEF\n"
    "with a mystic power.");

static const u8 sWaterSpoutDescription[] = _(
    "Inflicts more damage if the\n"
    "user's HP is high.");

static const u8 sSignalBeamDescription[] = _(
    "A strange beam attack that\n"
    "may confuse the foe.");

static const u8 sShadowPunchDescription[] = _(
    "An unavoidable punch that\n"
    "is thrown from shadows.");

static const u8 sExtrasensoryDescription[] = _(
    "Attacks with a peculiar\n"
    "power. May cause flinching.");

static const u8 sSkyUppercutDescription[] = _(
    "An uppercut thrown as if\n"
    "leaping into the sky.");

static const u8 sSandTombDescription[] = _(
    "Traps and hurts the foe in\n"
    "quicksand for 2 to 5 turns.");

static const u8 sSheerColdDescription[] = _(
    "A chilling attack that\n"
    "causes fainting if it hits.");

static const u8 sMuddyWaterDescription[] = _(
    "Attacks with muddy water.\n"
    "May lower accuracy.");

static const u8 sBulletSeedDescription[] = _(
    "Shoots 2 to 5 seeds in a row\n"
    "to strike the foe.");

static const u8 sAerialAceDescription[] = _(
    "An extremely speedy and\n"
    "unavoidable attack.");

static const u8 sIcicleSpearDescription[] = _(
    "Attacks the foe by firing\n"
    "2 to 5 icicles in a row.");

static const u8 sIronDefenseDescription[] = _(
    "Hardens the body's surface\n"
    "to sharply raise DEFENSE.");

static const u8 sBlockDescription[] = _(
    "Blocks the foe's way to\n"
    "prevent escape.");

static const u8 sHowlDescription[] = _(
    "Howls to raise the spirit\n"
    "and boosts ATTACK.");

static const u8 sDragonClawDescription[] = _(
    "Slashes the foe with sharp\n"
    "claws.");

static const u8 sFrenzyPlantDescription[] = _(
    "Powerful, but leaves the\n"
    "user immobile the next turn.");

static const u8 sBulkUpDescription[] = _(
    "Bulks up the body to boost\n"
    "both ATTACK and DEFENSE.");

static const u8 sBounceDescription[] = _(
    "Bounces up, then down the\n"
    "next turn. May paralyze.");

static const u8 sMudShotDescription[] = _(
    "Hurls mud at the foe and\n"
    "reduces SPEED.");

static const u8 sPoisonTailDescription[] = _(
    "Has a high critical-hit\n"
    "ratio. May also poison.");

static const u8 sCovetDescription[] = _(
    "Cutely begs to obtain an\n"
    "item held by the foe.");

static const u8 sVoltTackleDescription[] = _(
    "A life-risking tackle that\n"
    "slightly hurts the user.");

static const u8 sMagicalLeafDescription[] = _(
    "Attacks with a strange leaf\n"
    "that cannot be evaded.");

static const u8 sWaterSportDescription[] = _(
    "The user becomes soaked to\n"
    "raise resistance to fire.");

static const u8 sCalmMindDescription[] = _(
    "Raises SP. ATK and SP. DEF\n"
    "by focusing the mind.");

static const u8 sLeafBladeDescription[] = _(
    "Slashes with a sharp leaf.\n"
    "High critical-hit ratio.");

static const u8 sDragonDanceDescription[] = _(
    "A mystical dance that ups\n"
    "ATTACK and SPEED.");

static const u8 sRockBlastDescription[] = _(
    "Hurls boulders at the foe\n"
    "2 to 5 times in a row.");

static const u8 sShockWaveDescription[] = _(
    "A fast and unavoidable\n"
    "electric attack.");

static const u8 sWaterPulseDescription[] = _(
    "Attacks with ultrasonic\n"
    "waves. May confuse the foe.");

static const u8 sDoomDesireDescription[] = _(
    "Summons strong sunlight to\n"
    "attack 2 turns later.");

static const u8 sPsychoBoostDescription[] = _(
    "Allows a full-power attack,\n"
    "but sharply lowers SP. ATK.");

const struct MoveInfo gMovesInfo[MOVES_COUNT] =
{
    [MOVE_NONE] =
    {
        .name = _("-"),
        .effect = EFFECT_HIT,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 0,
        .tpCost = 0,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = 0,
        .description = sNullDescription,
    },

    [MOVE_POUND] =
    {
        .name = _("POUND"),
        .effect = EFFECT_HIT,
        .power = 40,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 6,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sPoundDescription,
    },

    [MOVE_KARATE_CHOP] =
    {
        .name = _("KARATE CHOP"),
        .effect = EFFECT_HIGH_CRITICAL,
        .power = 50,
        .type = TYPE_FIGHTING,
        .accuracy = 100,
        .tpCost = 8,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sKarateChopDescription,
    },

    [MOVE_DOUBLE_SLAP] =
    {
        .name = _("DOUBLESLAP"),
        .effect = EFFECT_MULTI_HIT,
        .power = 15,
        .type = TYPE_NORMAL,
        .accuracy = 85,
        .tpCost = 6,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sDoubleSlapDescription,
    },

    [MOVE_COMET_PUNCH] =
    {
        .name = _("COMET PUNCH"),
        .effect = EFFECT_MULTI_HIT,
        .power = 18,
        .type = TYPE_NORMAL,
        .accuracy = 85,
        .tpCost = 7,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sCometPunchDescription,
    },

    [MOVE_MEGA_PUNCH] =
    {
        .name = _("MEGA PUNCH"),
        .effect = EFFECT_HIT,
        .power = 80,
        .type = TYPE_NORMAL,
        .accuracy = 85,
        .tpCost = 10,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sMegaPunchDescription,
    },

    [MOVE_PAY_DAY] =
    {
        .name = _("PAY DAY"),
        .effect = EFFECT_PAY_DAY,
        .power = 40,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 6,
        .secondaryEffectChance = 100,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sPayDayDescription,
    },

    [MOVE_FIRE_PUNCH] =
    {
        .name = _("FIRE PUNCH"),
        .effect = EFFECT_BURN_HIT,
        .power = 75,
        .type = TYPE_FIRE,
        .accuracy = 100,
        .tpCost = 11,
        .secondaryEffectChance = 10,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sFirePunchDescription,
    },

    [MOVE_ICE_PUNCH] =
    {
        .name = _("ICE PUNCH"),
        .effect = EFFECT_FREEZE_HIT,
        .power = 75,
        .type = TYPE_ICE,
        .accuracy = 100,
        .tpCost = 12,
        .secondaryEffectChance = 10,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sIcePunchDescription,
    },

    [MOVE_THUNDER_PUNCH] =
    {
        .name = _("THUNDERPUNCH"),
        .effect = EFFECT_PARALYZE_HIT,
        .power = 75,
        .type = TYPE_ELECTRIC,
        .accuracy = 100,
        .tpCost = 11,
        .secondaryEffectChance = 10,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sThunderPunchDescription,
    },

    [MOVE_SCRATCH] =
    {
        .name = _("SCRATCH"),
        .effect = EFFECT_HIT,
        .power = 40,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 6,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sScratchDescription,
    },

    [MOVE_VICE_GRIP] =
    {
        .name = _("VICEGRIP"),
        .effect = EFFECT_HIT,
        .power = 55,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 8,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sViceGripDescription,
    },

    [MOVE_GUILLOTINE] =
    {
        .name = _("GUILLOTINE"),
        .effect = EFFECT_OHKO,
        .power = 1,
        .type = TYPE_NORMAL,
        .accuracy = 30,
        .tpCost = 7,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sGuillotineDescription,
    },

    [MOVE_RAZOR_WIND] =
    {
        .name = _("RAZOR WIND"),
        .effect = EFFECT_RAZOR_WIND,
        .power = 80,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 7,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_BOTH,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sRazorWindDescription,
    },

    [MOVE_SWORDS_DANCE] =
    {
        .name = _("SWORDS DANCE"),
        .effect = EFFECT_ATTACK_UP_2,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 0,
        .tpCost = 7,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sSwordsDanceDescription,
    },

    [MOVE_CUT] =
    {
        .name = _("CUT"),
        .effect = EFFECT_HIT,
        .power = 50,
        .type = TYPE_NORMAL,
        .accuracy = 95,
        .tpCost = 7,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sCutDescription,
    },

    [MOVE_GUST] =
    {
        .name = _("GUST"),
        .effect = EFFECT_GUST,
        .power = 40,
        .type = TYPE_FLYING,
        .accuracy = 100,
        .tpCost = 6,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sGustDescription,
    },

    [MOVE_WING_ATTACK] =
    {
        .name = _("WING ATTACK"),
        .effect = EFFECT_HIT,
        .power = 60,
        .type = TYPE_FLYING,
        .accuracy = 100,
        .tpCost = 9,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sWingAttackDescription,
    },

    [MOVE_WHIRLWIND] =
    {
        .name = _("WHIRLWIND"),
        .effect = EFFECT_ROAR,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 4,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = -6,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sWhirlwindDescription,
    },

    [MOVE_FLY] =
    {
        .name = _("FLY"),
        .effect = EFFECT_SEMI_INVULNERABLE,
        .power = 70,
        .type = TYPE_FLYING,
        .accuracy = 95,
        .tpCost = 6,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sFlyDescription,
    },

    [MOVE_BIND] =
    {
        .name = _("BIND"),
        .effect = EFFECT_TRAP,
        .power = 15,
        .type = TYPE_NORMAL,
        .accuracy = 75,
        .tpCost = 4,
        .secondaryEffectChance = 100,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sBindDescription,
    },

    [MOVE_SLAM] =
    {
        .name = _("SLAM"),
        .effect = EFFECT_HIT,
        .power = 80,
        .type = TYPE_NORMAL,
        .accuracy = 75,
        .tpCost = 9,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sSlamDescription,
    },

    [MOVE_VINE_WHIP] =
    {
        .name = _("VINE WHIP"),
        .effect = EFFECT_HIT,
        .power = 35,
        .type = TYPE_GRASS,
        .accuracy = 100,
        .tpCost = 6,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sVineWhipDescription,
    },

    [MOVE_STOMP] =
    {
        .name = _("STOMP"),
        .effect = EFFECT_FLINCH_MINIMIZE_HIT,
        .power = 65,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 9,
        .secondaryEffectChance = 30,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sStompDescription,
    },

    [MOVE_DOUBLE_KICK] =
    {
        .name = _("DOUBLE KICK"),
        .effect = EFFECT_DOUBLE_HIT,
        .power = 30,
        .type = TYPE_FIGHTING,
        .accuracy = 100,
        .tpCost = 9,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sDoubleKickDescription,
    },

    [MOVE_MEGA_KICK] =
    {
        .name = _("MEGA KICK"),
        .effect = EFFECT_HIT,
        .power = 120,
        .type = TYPE_NORMAL,
        .accuracy = 75,
        .tpCost = 13,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sMegaKickDescription,
    },

    [MOVE_JUMP_KICK] =
    {
        .name = _("JUMP KICK"),
        .effect = EFFECT_RECOIL_IF_MISS,
        .power = 70,
        .type = TYPE_FIGHTING,
        .accuracy = 95,
        .tpCost = 10,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sJumpKickDescription,
    },

    [MOVE_ROLLING_KICK] =
    {
        .name = _("ROLLING KICK"),
        .effect = EFFECT_FLINCH_HIT,
        .power = 60,
        .type = TYPE_FIGHTING,
        .accuracy = 85,
        .tpCost = 9,
        .secondaryEffectChance = 30,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sRollingKickDescription,
    },

    [MOVE_SAND_ATTACK] =
    {
        .name = _("SAND-ATTACK"),
        .effect = EFFECT_ACCURACY_DOWN,
        .power = 0,
        .type = TYPE_GROUND,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sSandAttackDescription,
    },

    [MOVE_HEADBUTT] =
    {
        .name = _("HEADBUTT"),
        .effect = EFFECT_FLINCH_HIT,
        .power = 70,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 11,
        .secondaryEffectChance = 30,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sHeadbuttDescription,
    },

    [MOVE_HORN_ATTACK] =
    {
        .name = _("HORN ATTACK"),
        .effect = EFFECT_HIT,
        .power = 65,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 9,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sHornAttackDescription,
    },

    [MOVE_FURY_ATTACK] =
    {
        .name = _("FURY ATTACK"),
        .effect = EFFECT_MULTI_HIT,
        .power = 15,
        .type = TYPE_NORMAL,
        .accuracy = 85,
        .tpCost = 6,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sFuryAttackDescription,
    },

    [MOVE_HORN_DRILL] =
    {
        .name = _("HORN DRILL"),
        .effect = EFFECT_OHKO,
        .power = 1,
        .type = TYPE_NORMAL,
        .accuracy = 30,
        .tpCost = 7,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sHornDrillDescription,
    },

    [MOVE_TACKLE] =
    {
        .name = _("TACKLE"),
        .effect = EFFECT_HIT,
        .power = 35,
        .type = TYPE_NORMAL,
        .accuracy = 95,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sTackleDescription,
    },

    [MOVE_BODY_SLAM] =
    {
        .name = _("BODY SLAM"),
        .effect = EFFECT_PARALYZE_HIT,
        .power = 85,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 14,
        .secondaryEffectChance = 30,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sBodySlamDescription,
    },

    [MOVE_WRAP] =
    {
        .name = _("WRAP"),
        .effect = EFFECT_TRAP,
        .power = 15,
        .type = TYPE_NORMAL,
        .accuracy = 85,
        .tpCost = 5,
        .secondaryEffectChance = 100,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sWrapDescription,
    },

    [MOVE_TAKE_DOWN] =
    {
        .name = _("TAKE DOWN"),
        .effect = EFFECT_RECOIL,
        .power = 90,
        .type = TYPE_NORMAL,
        .accuracy = 85,
        .tpCost = 9,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sTakeDownDescription,
    },

    [MOVE_THRASH] =
    {
        .name = _("THRASH"),
        .effect = EFFECT_RAMPAGE,
        .power = 90,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 13,
        .secondaryEffectChance = 100,
        .target = MOVE_TARGET_RANDOM,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sThrashDescription,
    },

    [MOVE_DOUBLE_EDGE] =
    {
        .name = _("DOUBLE-EDGE"),
        .effect = EFFECT_DOUBLE_EDGE,
        .power = 120,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 14,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sDoubleEdgeDescription,
    },

    [MOVE_TAIL_WHIP] =
    {
        .name = _("TAIL WHIP"),
        .effect = EFFECT_DEFENSE_DOWN,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 4,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_BOTH,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sTailWhipDescription,
    },

    [MOVE_POISON_STING] =
    {
        .name = _("POISON STING"),
        .effect = EFFECT_POISON_HIT,
        .power = 15,
        .type = TYPE_POISON,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 30,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sPoisonStingDescription,
    },

    [MOVE_TWINEEDLE] =
    {
        .name = _("TWINEEDLE"),
        .effect = EFFECT_TWINEEDLE,
        .power = 25,
        .type = TYPE_BUG,
        .accuracy = 100,
        .tpCost = 9,
        .secondaryEffectChance = 20,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sTwineedleDescription,
    },

    [MOVE_PIN_MISSILE] =
    {
        .name = _("PIN MISSILE"),
        .effect = EFFECT_MULTI_HIT,
        .power = 14,
        .type = TYPE_BUG,
        .accuracy = 85,
        .tpCost = 6,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sPinMissileDescription,
    },

    [MOVE_LEER] =
    {
        .name = _("LEER"),
        .effect = EFFECT_DEFENSE_DOWN,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 4,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_BOTH,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sLeerDescription,
    },

    [MOVE_BITE] =
    {
        .name = _("BITE"),
        .effect = EFFECT_FLINCH_HIT,
        .power = 60,
        .type = TYPE_DARK,
        .accuracy = 100,
        .tpCost = 10,
        .secondaryEffectChance = 30,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sBiteDescription,
    },

    [MOVE_GROWL] =
    {
        .name = _("GROWL"),
        .effect = EFFECT_ATTACK_DOWN,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 4,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_BOTH,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sGrowlDescription,
    },

    [MOVE_ROAR] =
    {
        .name = _("ROAR"),
        .effect = EFFECT_ROAR,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 4,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = -6,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sRoarDescription,
    },

    [MOVE_SING] =
    {
        .name = _("SING"),
        .effect = EFFECT_SLEEP,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 55,
        .tpCost = 7,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sSingDescription,
    },

    [MOVE_SUPERSONIC] =
    {
        .name = _("SUPERSONIC"),
        .effect = EFFECT_CONFUSE,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 55,
        .tpCost = 4,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sSupersonicDescription,
    },

    [MOVE_SONIC_BOOM] =
    {
        .name = _("SONICBOOM"),
        .effect = EFFECT_SONICBOOM,
        .power = 1,
        .type = TYPE_NORMAL,
        .accuracy = 90,
        .tpCost = 6,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sSonicBoomDescription,
    },

    [MOVE_DISABLE] =
    {
        .name = _("DISABLE"),
        .effect = EFFECT_DISABLE,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 55,
        .tpCost = 4,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sDisableDescription,
    },

    [MOVE_ACID] =
    {
        .name = _("ACID"),
        .effect = EFFECT_DEFENSE_DOWN_HIT,
        .power = 40,
        .type = TYPE_POISON,
        .accuracy = 100,
        .tpCost = 7,
        .secondaryEffectChance = 10,
        .target = MOVE_TARGET_BOTH,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sAcidDescription,
    },

    [MOVE_EMBER] =
    {
        .name = _("EMBER"),
        .effect = EFFECT_BURN_HIT,
        .power = 40,
        .type = TYPE_FIRE,
        .accuracy = 100,
        .tpCost = 7,
        .secondaryEffectChance = 10,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sEmberDescription,
    },

    [MOVE_FLAMETHROWER] =
    {
        .name = _("FLAMETHROWER"),
        .effect = EFFECT_BURN_HIT,
        .power = 95,
        .type = TYPE_FIRE,
        .accuracy = 100,
        .tpCost = 14,
        .secondaryEffectChance = 10,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sFlamethrowerDescription,
    },

    [MOVE_MIST] =
    {
        .name = _("MIST"),
        .effect = EFFECT_MIST,
        .power = 0,
        .type = TYPE_ICE,
        .accuracy = 0,
        .tpCost = 4,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sMistDescription,
    },

    [MOVE_WATER_GUN] =
    {
        .name = _("WATER GUN"),
        .effect = EFFECT_HIT,
        .power = 40,
        .type = TYPE_WATER,
        .accuracy = 100,
        .tpCost = 6,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sWaterGunDescription,
    },

    [MOVE_HYDRO_PUMP] =
    {
        .name = _("HYDRO PUMP"),
        .effect = EFFECT_HIT,
        .power = 120,
        .type = TYPE_WATER,
        .accuracy = 80,
        .tpCost = 13,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sHydroPumpDescription,
    },

    [MOVE_SURF] =
    {
        .name = _("SURF"),
        .effect = EFFECT_HIT,
        .power = 95,
        .type = TYPE_WATER,
        .accuracy = 100,
        .tpCost = 13,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_BOTH,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sSurfDescription,
    },

    [MOVE_ICE_BEAM] =
    {
        .name = _("ICE BEAM"),
        .effect = EFFECT_FREEZE_HIT,
        .power = 95,
        .type = TYPE_ICE,
        .accuracy = 100,
        .tpCost = 14,
        .secondaryEffectChance = 10,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sIceBeamDescription,
    },

    [MOVE_BLIZZARD] =
    {
        .name = _("BLIZZARD"),
        .effect = EFFECT_FREEZE_HIT,
        .power = 120,
        .type = TYPE_ICE,
        .accuracy = 70,
        .tpCost = 13,
        .secondaryEffectChance = 10,
        .target = MOVE_TARGET_BOTH,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sBlizzardDescription,
    },

    [MOVE_PSYBEAM] =
    {
        .name = _("PSYBEAM"),
        .effect = EFFECT_CONFUSE_HIT,
        .power = 65,
        .type = TYPE_PSYCHIC,
        .accuracy = 100,
        .tpCost = 10,
        .secondaryEffectChance = 10,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sPsybeamDescription,
    },

    [MOVE_BUBBLE_BEAM] =
    {
        .name = _("BUBBLEBEAM"),
        .effect = EFFECT_SPEED_DOWN_HIT,
        .power = 65,
        .type = TYPE_WATER,
        .accuracy = 100,
        .tpCost = 10,
        .secondaryEffectChance = 10,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sBubbleBeamDescription,
    },

    [MOVE_AURORA_BEAM] =
    {
        .name = _("AURORA BEAM"),
        .effect = EFFECT_ATTACK_DOWN_HIT,
        .power = 65,
        .type = TYPE_ICE,
        .accuracy = 100,
        .tpCost = 10,
        .secondaryEffectChance = 10,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sAuroraBeamDescription,
    },

    [MOVE_HYPER_BEAM] =
    {
        .name = _("HYPER BEAM"),
        .effect = EFFECT_RECHARGE,
        .power = 150,
        .type = TYPE_NORMAL,
        .accuracy = 90,
        .tpCost = 12,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sHyperBeamDescription,
    },

    [MOVE_PECK] =
    {
        .name = _("PECK"),
        .effect = EFFECT_HIT,
        .power = 35,
        .type = TYPE_FLYING,
        .accuracy = 100,
        .tpCost = 6,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sPeckDescription,
    },

    [MOVE_DRILL_PECK] =
    {
        .name = _("DRILL PECK"),
        .effect = EFFECT_HIT,
        .power = 80,
        .type = TYPE_FLYING,
        .accuracy = 100,
        .tpCost = 11,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sDrillPeckDescription,
    },

    [MOVE_SUBMISSION] =
    {
        .name = _("SUBMISSION"),
        .effect = EFFECT_RECOIL,
        .power = 80,
        .type = TYPE_FIGHTING,
        .accuracy = 80,
        .tpCost = 8,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sSubmissionDescription,
    },

    [MOVE_LOW_KICK] =
    {
        .name = _("LOW KICK"),
        .effect = EFFECT_LOW_KICK,
        .power = 1,
        .type = TYPE_FIGHTING,
        .accuracy = 100,
        .tpCost = 8,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sLowKickDescription,
    },

    [MOVE_COUNTER] =
    {
        .name = _("COUNTER"),
        .effect = EFFECT_COUNTER,
        .power = 1,
        .type = TYPE_FIGHTING,
        .accuracy = 100,
        .tpCost = 9,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_DEPENDS,
        .priority = -5,
        .flags = FLAG_MAKES_CONTACT | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sCounterDescription,
    },

    [MOVE_SEISMIC_TOSS] =
    {
        .name = _("SEISMIC TOSS"),
        .effect = EFFECT_LEVEL_DAMAGE,
        .power = 1,
        .type = TYPE_FIGHTING,
        .accuracy = 100,
        .tpCost = 9,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sSeismicTossDescription,
    },

    [MOVE_STRENGTH] =
    {
        .name = _("STRENGTH"),
        .effect = EFFECT_HIT,
        .power = 80,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 11,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sStrengthDescription,
    },

    [MOVE_ABSORB] =
    {
        .name = _("ABSORB"),
        .effect = EFFECT_ABSORB,
        .power = 20,
        .type = TYPE_GRASS,
        .accuracy = 100,
        .tpCost = 4,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sAbsorbDescription,
    },

    [MOVE_MEGA_DRAIN] =
    {
        .name = _("MEGA DRAIN"),
        .effect = EFFECT_ABSORB,
        .power = 40,
        .type = TYPE_GRASS,
        .accuracy = 100,
        .tpCost = 8,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sMegaDrainDescription,
    },

    [MOVE_LEECH_SEED] =
    {
        .name = _("LEECH SEED"),
        .effect = EFFECT_LEECH_SEED,
        .power = 0,
        .type = TYPE_GRASS,
        .accuracy = 90,
        .tpCost = 7,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sLeechSeedDescription,
    },

    [MOVE_GROWTH] =
    {
        .name = _("GROWTH"),
        .effect = EFFECT_SPECIAL_ATTACK_UP,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 0,
        .tpCost = 4,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sGrowthDescription,
    },

    [MOVE_RAZOR_LEAF] =
    {
        .name = _("RAZOR LEAF"),
        .effect = EFFECT_HIGH_CRITICAL,
        .power = 55,
        .type = TYPE_GRASS,
        .accuracy = 95,
        .tpCost = 8,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_BOTH,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sRazorLeafDescription,
    },

    [MOVE_SOLAR_BEAM] =
    {
        .name = _("SOLARBEAM"),
        .effect = EFFECT_SOLAR_BEAM,
        .power = 120,
        .type = TYPE_GRASS,
        .accuracy = 100,
        .tpCost = 10,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sSolarBeamDescription,
    },

    [MOVE_POISON_POWDER] =
    {
        .name = _("POISONPOWDER"),
        .effect = EFFECT_POISON,
        .power = 0,
        .type = TYPE_POISON,
        .accuracy = 75,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sPoisonPowderDescription,
    },

    [MOVE_STUN_SPORE] =
    {
        .name = _("STUN SPORE"),
        .effect = EFFECT_PARALYZE,
        .power = 0,
        .type = TYPE_GRASS,
        .accuracy = 75,
        .tpCost = 6,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sStunSporeDescription,
    },

    [MOVE_SLEEP_POWDER] =
    {
        .name = _("SLEEP POWDER"),
        .effect = EFFECT_SLEEP,
        .power = 0,
        .type = TYPE_GRASS,
        .accuracy = 75,
        .tpCost = 9,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sSleepPowderDescription,
    },

    [MOVE_PETAL_DANCE] =
    {
        .name = _("PETAL DANCE"),
        .effect = EFFECT_RAMPAGE,
        .power = 70,
        .type = TYPE_GRASS,
        .accuracy = 100,
        .tpCost = 10,
        .secondaryEffectChance = 100,
        .target = MOVE_TARGET_RANDOM,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sPetalDanceDescription,
    },

    [MOVE_STRING_SHOT] =
    {
        .name = _("STRING SHOT"),
        .effect = EFFECT_SPEED_DOWN,
        .power = 0,
        .type = TYPE_BUG,
        .accuracy = 95,
        .tpCost = 3,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_BOTH,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sStringShotDescription,
    },

    [MOVE_DRAGON_RAGE] =
    {
        .name = _("DRAGON RAGE"),
        .effect = EFFECT_DRAGON_RAGE,
        .power = 1,
        .type = TYPE_DRAGON,
        .accuracy = 100,
        .tpCost = 9,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sDragonRageDescription,
    },

    [MOVE_FIRE_SPIN] =
    {
        .name = _("FIRE SPIN"),
        .effect = EFFECT_TRAP,
        .power = 15,
        .type = TYPE_FIRE,
        .accuracy = 70,
        .tpCost = 4,
        .secondaryEffectChance = 100,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sFireSpinDescription,
    },

    [MOVE_THUNDER_SHOCK] =
    {
        .name = _("THUNDERSHOCK"),
        .effect = EFFECT_PARALYZE_HIT,
        .power = 40,
        .type = TYPE_ELECTRIC,
        .accuracy = 100,
        .tpCost = 7,
        .secondaryEffectChance = 10,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sThunderShockDescription,
    },

    [MOVE_THUNDERBOLT] =
    {
        .name = _("THUNDERBOLT"),
        .effect = EFFECT_PARALYZE_HIT,
        .power = 95,
        .type = TYPE_ELECTRIC,
        .accuracy = 100,
        .tpCost = 14,
        .secondaryEffectChance = 10,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sThunderboltDescription,
    },

    [MOVE_THUNDER_WAVE] =
    {
        .name = _("THUNDER WAVE"),
        .effect = EFFECT_PARALYZE,
        .power = 0,
        .type = TYPE_ELECTRIC,
        .accuracy = 100,
        .tpCost = 8,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sThunderWaveDescription,
    },

    [MOVE_THUNDER] =
    {
        .name = _("THUNDER"),
        .effect = EFFECT_THUNDER,
        .power = 120,
        .type = TYPE_ELECTRIC,
        .accuracy = 70,
        .tpCost = 14,
        .secondaryEffectChance = 30,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sThunderDescription,
    },

    [MOVE_ROCK_THROW] =
    {
        .name = _("ROCK THROW"),
        .effect = EFFECT_HIT,
        .power = 50,
        .type = TYPE_ROCK,
        .accuracy = 90,
        .tpCost = 7,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sRockThrowDescription,
    },

    [MOVE_EARTHQUAKE] =
    {
        .name = _("EARTHQUAKE"),
        .effect = EFFECT_EARTHQUAKE,
        .power = 100,
        .type = TYPE_GROUND,
        .accuracy = 100,
        .tpCost = 14,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_FOES_AND_ALLY,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sEarthquakeDescription,
    },

    [MOVE_FISSURE] =
    {
        .name = _("FISSURE"),
        .effect = EFFECT_OHKO,
        .power = 1,
        .type = TYPE_GROUND,
        .accuracy = 30,
        .tpCost = 7,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sFissureDescription,
    },

    [MOVE_DIG] =
    {
        .name = _("DIG"),
        .effect = EFFECT_SEMI_INVULNERABLE,
        .power = 60,
        .type = TYPE_GROUND,
        .accuracy = 100,
        .tpCost = 6,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sDigDescription,
    },

    [MOVE_TOXIC] =
    {
        .name = _("TOXIC"),
        .effect = EFFECT_TOXIC,
        .power = 0,
        .type = TYPE_POISON,
        .accuracy = 85,
        .tpCost = 8,
        .secondaryEffectChance = 100,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sToxicDescription,
    },

    [MOVE_CONFUSION] =
    {
        .name = _("CONFUSION"),
        .effect = EFFECT_CONFUSE_HIT,
        .power = 50,
        .type = TYPE_PSYCHIC,
        .accuracy = 100,
        .tpCost = 8,
        .secondaryEffectChance = 10,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sConfusionDescription,
    },

    [MOVE_PSYCHIC] =
    {
        .name = _("PSYCHIC"),
        .effect = EFFECT_SPECIAL_DEFENSE_DOWN_HIT,
        .power = 90,
        .type = TYPE_PSYCHIC,
        .accuracy = 100,
        .tpCost = 13,
        .secondaryEffectChance = 10,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sPsychicDescription,
    },

    [MOVE_HYPNOSIS] =
    {
        .name = _("HYPNOSIS"),
        .effect = EFFECT_SLEEP,
        .power = 0,
        .type = TYPE_PSYCHIC,
        .accuracy = 60,
        .tpCost = 7,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sHypnosisDescription,
    },

    [MOVE_MEDITATE] =
    {
        .name = _("MEDITATE"),
        .effect = EFFECT_ATTACK_UP,
        .power = 0,
        .type = TYPE_PSYCHIC,
        .accuracy = 0,
        .tpCost = 4,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sMeditateDescription,
    },

    [MOVE_AGILITY] =
    {
        .name = _("AGILITY"),
        .effect = EFFECT_SPEED_UP_2,
        .power = 0,
        .type = TYPE_PSYCHIC,
        .accuracy = 0,
        .tpCost = 6,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sAgilityDescription,
    },

    [MOVE_QUICK_ATTACK] =
    {
        .name = _("QUICK ATTACK"),
        .effect = EFFECT_QUICK_ATTACK,
        .power = 40,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 7,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 1,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sQuickAttackDescription,
    },

    [MOVE_RAGE] =
    {
        .name = _("RAGE"),
        .effect = EFFECT_RAGE,
        .power = 20,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 4,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sRageDescription,
    },

    [MOVE_TELEPORT] =
    {
        .name = _("TELEPORT"),
        .effect = EFFECT_TELEPORT,
        .power = 0,
        .type = TYPE_PSYCHIC,
        .accuracy = 0,
        .tpCost = 2,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = 0,
        .description = sTeleportDescription,
    },

    [MOVE_NIGHT_SHADE] =
    {
        .name = _("NIGHT SHADE"),
        .effect = EFFECT_LEVEL_DAMAGE,
        .power = 1,
        .type = TYPE_GHOST,
        .accuracy = 100,
        .tpCost = 9,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sNightShadeDescription,
    },

    [MOVE_MIMIC] =
    {
        .name = _("MIMIC"),
        .effect = EFFECT_MIMIC,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED,
        .description = sMimicDescription,
    },

    [MOVE_SCREECH] =
    {
        .name = _("SCREECH"),
        .effect = EFFECT_DEFENSE_DOWN_2,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 85,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sScreechDescription,
    },

    [MOVE_DOUBLE_TEAM] =
    {
        .name = _("DOUBLE TEAM"),
        .effect = EFFECT_EVASION_UP,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 0,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sDoubleTeamDescription,
    },

    [MOVE_RECOVER] =
    {
        .name = _("RECOVER"),
        .effect = EFFECT_RESTORE_HP,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 0,
        .tpCost = 9,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sRecoverDescription,
    },

    [MOVE_HARDEN] =
    {
        .name = _("HARDEN"),
        .effect = EFFECT_DEFENSE_UP,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 0,
        .tpCost = 4,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sHardenDescription,
    },

    [MOVE_MINIMIZE] =
    {
        .name = _("MINIMIZE"),
        .effect = EFFECT_MINIMIZE,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 0,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sMinimizeDescription,
    },

    [MOVE_SMOKESCREEN] =
    {
        .name = _("SMOKESCREEN"),
        .effect = EFFECT_ACCURACY_DOWN,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sSmokescreenDescription,
    },

    [MOVE_CONFUSE_RAY] =
    {
        .name = _("CONFUSE RAY"),
        .effect = EFFECT_CONFUSE,
        .power = 0,
        .type = TYPE_GHOST,
        .accuracy = 100,
        .tpCost = 7,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sConfuseRayDescription,
    },

    [MOVE_WITHDRAW] =
    {
        .name = _("WITHDRAW"),
        .effect = EFFECT_DEFENSE_UP,
        .power = 0,
        .type = TYPE_WATER,
        .accuracy = 0,
        .tpCost = 4,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sWithdrawDescription,
    },

    [MOVE_DEFENSE_CURL] =
    {
        .name = _("DEFENSE CURL"),
        .effect = EFFECT_DEFENSE_CURL,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 0,
        .tpCost = 4,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sDefenseCurlDescription,
    },

    [MOVE_BARRIER] =
    {
        .name = _("BARRIER"),
        .effect = EFFECT_DEFENSE_UP_2,
        .power = 0,
        .type = TYPE_PSYCHIC,
        .accuracy = 0,
        .tpCost = 6,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sBarrierDescription,
    },

    [MOVE_LIGHT_SCREEN] =
    {
        .name = _("LIGHT SCREEN"),
        .effect = EFFECT_LIGHT_SCREEN,
        .power = 0,
        .type = TYPE_PSYCHIC,
        .accuracy = 0,
        .tpCost = 6,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sLightScreenDescription,
    },

    [MOVE_HAZE] =
    {
        .name = _("HAZE"),
        .effect = EFFECT_HAZE,
        .power = 0,
        .type = TYPE_ICE,
        .accuracy = 0,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED,
        .description = sHazeDescription,
    },

    [MOVE_REFLECT] =
    {
        .name = _("REFLECT"),
        .effect = EFFECT_REFLECT,
        .power = 0,
        .type = TYPE_PSYCHIC,
        .accuracy = 0,
        .tpCost = 6,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sReflectDescription,
    },

    [MOVE_FOCUS_ENERGY] =
    {
        .name = _("FOCUS ENERGY"),
        .effect = EFFECT_FOCUS_ENERGY,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 0,
        .tpCost = 4,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sFocusEnergyDescription,
    },

    [MOVE_BIDE] =
    {
        .name = _("BIDE"),
        .effect = EFFECT_BIDE,
        .power = 1,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 9,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sBideDescription,
    },

    [MOVE_METRONOME] =
    {
        .name = _("METRONOME"),
        .effect = EFFECT_METRONOME,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 0,
        .tpCost = 7,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_DEPENDS,
        .priority = 0,
        .flags = 0,
        .description = sMetronomeDescription,
    },

    [MOVE_MIRROR_MOVE] =
    {
        .name = _("MIRROR MOVE"),
        .effect = EFFECT_MIRROR_MOVE,
        .power = 0,
        .type = TYPE_FLYING,
        .accuracy = 0,
        .tpCost = 6,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_DEPENDS,
        .priority = 0,
        .flags = 0,
        .description = sMirrorMoveDescription,
    },

    [MOVE_SELF_DESTRUCT] =
    {
        .name = _("SELFDESTRUCT"),
        .effect = EFFECT_EXPLOSION,
        .power = 200,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 14,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_FOES_AND_ALLY,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sSelfDestructDescription,
    },

    [MOVE_EGG_BOMB] =
    {
        .name = _("EGG BOMB"),
        .effect = EFFECT_HIT,
        .power = 100,
        .type = TYPE_NORMAL,
        .accuracy = 75,
        .tpCost = 11,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sEggBombDescription,
    },

    [MOVE_LICK] =
    {
        .name = _("LICK"),
        .effect = EFFECT_PARALYZE_HIT,
        .power = 20,
        .type = TYPE_GHOST,
        .accuracy = 100,
        .tpCost = 6,
        .secondaryEffectChance = 30,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sLickDescription,
    },

    [MOVE_SMOG] =
    {
        .name = _("SMOG"),
        .effect = EFFECT_POISON_HIT,
        .power = 20,
        .type = TYPE_POISON,
        .accuracy = 70,
        .tpCost = 5,
        .secondaryEffectChance = 40,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sSmogDescription,
    },

    [MOVE_SLUDGE] =
    {
        .name = _("SLUDGE"),
        .effect = EFFECT_POISON_HIT,
        .power = 65,
        .type = TYPE_POISON,
        .accuracy = 100,
        .tpCost = 11,
        .secondaryEffectChance = 30,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sSludgeDescription,
    },

    [MOVE_BONE_CLUB] =
    {
        .name = _("BONE CLUB"),
        .effect = EFFECT_FLINCH_HIT,
        .power = 65,
        .type = TYPE_GROUND,
        .accuracy = 85,
        .tpCost = 9,
        .secondaryEffectChance = 10,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sBoneClubDescription,
    },

    [MOVE_FIRE_BLAST] =
    {
        .name = _("FIRE BLAST"),
        .effect = EFFECT_BURN_HIT,
        .power = 120,
        .type = TYPE_FIRE,
        .accuracy = 85,
        .tpCost = 15,
        .secondaryEffectChance = 10,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sFireBlastDescription,
    },

    [MOVE_WATERFALL] =
    {
        .name = _("WATERFALL"),
        .effect = EFFECT_HIT,
        .power = 80,
        .type = TYPE_WATER,
        .accuracy = 100,
        .tpCost = 11,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sWaterfallDescription,
    },

    [MOVE_CLAMP] =
    {
        .name = _("CLAMP"),
        .effect = EFFECT_TRAP,
        .power = 35,
        .type = TYPE_WATER,
        .accuracy = 75,
        .tpCost = 6,
        .secondaryEffectChance = 100,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sClampDescription,
    },

    [MOVE_SWIFT] =
    {
        .name = _("SWIFT"),
        .effect = EFFECT_ALWAYS_HIT,
        .power = 60,
        .type = TYPE_NORMAL,
        .accuracy = 0,
        .tpCost = 9,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_BOTH,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sSwiftDescription,
    },

    [MOVE_SKULL_BASH] =
    {
        .name = _("SKULL BASH"),
        .effect = EFFECT_SKULL_BASH,
        .power = 100,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 9,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sSkullBashDescription,
    },

    [MOVE_SPIKE_CANNON] =
    {
        .name = _("SPIKE CANNON"),
        .effect = EFFECT_MULTI_HIT,
        .power = 20,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 9,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sSpikeCannonDescription,
    },

    [MOVE_CONSTRICT] =
    {
        .name = _("CONSTRICT"),
        .effect = EFFECT_SPEED_DOWN_HIT,
        .power = 10,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 3,
        .secondaryEffectChance = 10,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sConstrictDescription,
    },

    [MOVE_AMNESIA] =
    {
        .name = _("AMNESIA"),
        .effect = EFFECT_SPECIAL_DEFENSE_UP_2,
        .power = 0,
        .type = TYPE_PSYCHIC,
        .accuracy = 0,
        .tpCost = 6,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sAmnesiaDescription,
    },

    [MOVE_KINESIS] =
    {
        .name = _("KINESIS"),
        .effect = EFFECT_ACCURACY_DOWN,
        .power = 0,
        .type = TYPE_PSYCHIC,
        .accuracy = 80,
        .tpCost = 4,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sKinesisDescription,
    },

    [MOVE_SOFT_BOILED] =
    {
        .name = _("SOFTBOILED"),
        .effect = EFFECT_SOFTBOILED,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 9,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sSoftBoiledDescription,
    },

    [MOVE_HI_JUMP_KICK] =
    {
        .name = _("HI JUMP KICK"),
        .effect = EFFECT_RECOIL_IF_MISS,
        .power = 85,
        .type = TYPE_FIGHTING,
        .accuracy = 90,
        .tpCost = 11,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sHiJumpKickDescription,
    },

    [MOVE_GLARE] =
    {
        .name = _("GLARE"),
        .effect = EFFECT_PARALYZE,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 75,
        .tpCost = 6,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sGlareDescription,
    },

    [MOVE_DREAM_EATER] =
    {
        .name = _("DREAM EATER"),
        .effect = EFFECT_DREAM_EATER,
        .power = 100,
        .type = TYPE_PSYCHIC,
        .accuracy = 100,
        .tpCost = 17,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sDreamEaterDescription,
    },

    [MOVE_POISON_GAS] =
    {
        .name = _("POISON GAS"),
        .effect = EFFECT_POISON,
        .power = 0,
        .type = TYPE_POISON,
        .accuracy = 55,
        .tpCost = 4,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sPoisonGasDescription,
    },

    [MOVE_BARRAGE] =
    {
        .name = _("BARRAGE"),
        .effect = EFFECT_MULTI_HIT,
        .power = 15,
        .type = TYPE_NORMAL,
        .accuracy = 85,
        .tpCost = 6,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sBarrageDescription,
    },

    [MOVE_LEECH_LIFE] =
    {
        .name = _("LEECH LIFE"),
        .effect = EFFECT_ABSORB,
        .power = 20,
        .type = TYPE_BUG,
        .accuracy = 100,
        .tpCost = 4,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sLeechLifeDescription,
    },

    [MOVE_LOVELY_KISS] =
    {
        .name = _("LOVELY KISS"),
        .effect = EFFECT_SLEEP,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 75,
        .tpCost = 9,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sLovelyKissDescription,
    },

    [MOVE_SKY_ATTACK] =
    {
        .name = _("SKY ATTACK"),
        .effect = EFFECT_SKY_ATTACK,
        .power = 140,
        .type = TYPE_FLYING,
        .accuracy = 90,
        .tpCost = 12,
        .secondaryEffectChance = 30,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sSkyAttackDescription,
    },

    [MOVE_TRANSFORM] =
    {
        .name = _("TRANSFORM"),
        .effect = EFFECT_TRANSFORM,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 0,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = 0,
        .description = sTransformDescription,
    },

    [MOVE_BUBBLE] =
    {
        .name = _("BUBBLE"),
        .effect = EFFECT_SPEED_DOWN_HIT,
        .power = 20,
        .type = TYPE_WATER,
        .accuracy = 100,
        .tpCost = 4,
        .secondaryEffectChance = 10,
        .target = MOVE_TARGET_BOTH,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sBubbleDescription,
    },

    [MOVE_DIZZY_PUNCH] =
    {
        .name = _("DIZZY PUNCH"),
        .effect = EFFECT_CONFUSE_HIT,
        .power = 70,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 11,
        .secondaryEffectChance = 20,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sDizzyPunchDescription,
    },

    [MOVE_SPORE] =
    {
        .name = _("SPORE"),
        .effect = EFFECT_SLEEP,
        .power = 0,
        .type = TYPE_GRASS,
        .accuracy = 100,
        .tpCost = 11,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sSporeDescription,
    },

    [MOVE_FLASH] =
    {
        .name = _("FLASH"),
        .effect = EFFECT_ACCURACY_DOWN,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 70,
        .tpCost = 4,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sFlashDescription,
    },

    [MOVE_PSYWAVE] =
    {
        .name = _("PSYWAVE"),
        .effect = EFFECT_PSYWAVE,
        .power = 1,
        .type = TYPE_PSYCHIC,
        .accuracy = 80,
        .tpCost = 6,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sPsywaveDescription,
    },

    [MOVE_SPLASH] =
    {
        .name = _("SPLASH"),
        .effect = EFFECT_SPLASH,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 0,
        .tpCost = 1,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = 0,
        .description = sSplashDescription,
    },

    [MOVE_ACID_ARMOR] =
    {
        .name = _("ACID ARMOR"),
        .effect = EFFECT_DEFENSE_UP_2,
        .power = 0,
        .type = TYPE_POISON,
        .accuracy = 0,
        .tpCost = 6,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sAcidArmorDescription,
    },

    [MOVE_CRABHAMMER] =
    {
        .name = _("CRABHAMMER"),
        .effect = EFFECT_HIGH_CRITICAL,
        .power = 90,
        .type = TYPE_WATER,
        .accuracy = 85,
        .tpCost = 12,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sCrabhammerDescription,
    },

    [MOVE_EXPLOSION] =
    {
        .name = _("EXPLOSION"),
        .effect = EFFECT_EXPLOSION,
        .power = 250,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 17,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_FOES_AND_ALLY,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sExplosionDescription,
    },

    [MOVE_FURY_SWIPES] =
    {
        .name = _("FURY SWIPES"),
        .effect = EFFECT_MULTI_HIT,
        .power = 18,
        .type = TYPE_NORMAL,
        .accuracy = 80,
        .tpCost = 7,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sFurySwipesDescription,
    },

    [MOVE_BONEMERANG] =
    {
        .name = _("BONEMERANG"),
        .effect = EFFECT_DOUBLE_HIT,
        .power = 50,
        .type = TYPE_GROUND,
        .accuracy = 90,
        .tpCost = 13,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sBonemerangDescription,
    },

    [MOVE_REST] =
    {
        .name = _("REST"),
        .effect = EFFECT_REST,
        .power = 0,
        .type = TYPE_PSYCHIC,
        .accuracy = 0,
        .tpCost = 9,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sRestDescription,
    },

    [MOVE_ROCK_SLIDE] =
    {
        .name = _("ROCK SLIDE"),
        .effect = EFFECT_FLINCH_HIT,
        .power = 75,
        .type = TYPE_ROCK,
        .accuracy = 90,
        .tpCost = 11,
        .secondaryEffectChance = 30,
        .target = MOVE_TARGET_BOTH,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sRockSlideDescription,
    },

    [MOVE_HYPER_FANG] =
    {
        .name = _("HYPER FANG"),
        .effect = EFFECT_FLINCH_HIT,
        .power = 80,
        .type = TYPE_NORMAL,
        .accuracy = 90,
        .tpCost = 11,
        .secondaryEffectChance = 10,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sHyperFangDescription,
    },

    [MOVE_SHARPEN] =
    {
        .name = _("SHARPEN"),
        .effect = EFFECT_ATTACK_UP,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 0,
        .tpCost = 4,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sSharpenDescription,
    },

    [MOVE_CONVERSION] =
    {
        .name = _("CONVERSION"),
        .effect = EFFECT_CONVERSION,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 0,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = 0,
        .description = sConversionDescription,
    },

    [MOVE_TRI_ATTACK] =
    {
        .name = _("TRI ATTACK"),
        .effect = EFFECT_TRI_ATTACK,
        .power = 80,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 11,
        .secondaryEffectChance = 20,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sTriAttackDescription,
    },

    [MOVE_SUPER_FANG] =
    {
        .name = _("SUPER FANG"),
        .effect = EFFECT_SUPER_FANG,
        .power = 1,
        .type = TYPE_NORMAL,
        .accuracy = 90,
        .tpCost = 9,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sSuperFangDescription,
    },

    [MOVE_SLASH] =
    {
        .name = _("SLASH"),
        .effect = EFFECT_HIGH_CRITICAL,
        .power = 70,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 11,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sSlashDescription,
    },

    [MOVE_SUBSTITUTE] =
    {
        .name = _("SUBSTITUTE"),
        .effect = EFFECT_SUBSTITUTE,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 0,
        .tpCost = 6,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sSubstituteDescription,
    },

    [MOVE_STRUGGLE] =
    {
        .name = _("STRUGGLE"),
        .effect = EFFECT_RECOIL,
        .power = 50,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 0,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sStruggleDescription,
    },

    [MOVE_SKETCH] =
    {
        .name = _("SKETCH"),
        .effect = EFFECT_SKETCH,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 0,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = 0,
        .description = sSketchDescription,
    },

    [MOVE_TRIPLE_KICK] =
    {
        .name = _("TRIPLE KICK"),
        .effect = EFFECT_TRIPLE_KICK,
        .power = 10,
        .type = TYPE_FIGHTING,
        .accuracy = 90,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sTripleKickDescription,
    },

    [MOVE_THIEF] =
    {
        .name = _("THIEF"),
        .effect = EFFECT_THIEF,
        .power = 40,
        .type = TYPE_DARK,
        .accuracy = 100,
        .tpCost = 6,
        .secondaryEffectChance = 100,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sThiefDescription,
    },

    [MOVE_SPIDER_WEB] =
    {
        .name = _("SPIDER WEB"),
        .effect = EFFECT_MEAN_LOOK,
        .power = 0,
        .type = TYPE_BUG,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sSpiderWebDescription,
    },

    [MOVE_MIND_READER] =
    {
        .name = _("MIND READER"),
        .effect = EFFECT_LOCK_ON,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sMindReaderDescription,
    },

    [MOVE_NIGHTMARE] =
    {
        .name = _("NIGHTMARE"),
        .effect = EFFECT_NIGHTMARE,
        .power = 0,
        .type = TYPE_GHOST,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sNightmareDescription,
    },

    [MOVE_FLAME_WHEEL] =
    {
        .name = _("FLAME WHEEL"),
        .effect = EFFECT_THAW_HIT,
        .power = 60,
        .type = TYPE_FIRE,
        .accuracy = 100,
        .tpCost = 9,
        .secondaryEffectChance = 10,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sFlameWheelDescription,
    },

    [MOVE_SNORE] =
    {
        .name = _("SNORE"),
        .effect = EFFECT_SNORE,
        .power = 40,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 6,
        .secondaryEffectChance = 30,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sSnoreDescription,
    },

    [MOVE_CURSE] =
    {
        .name = _("CURSE"),
        .effect = EFFECT_CURSE,
        .power = 0,
        .type = TYPE_MYSTERY,
        .accuracy = 0,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = 0,
        .description = sCurseDescription,
    },

    [MOVE_FLAIL] =
    {
        .name = _("FLAIL"),
        .effect = EFFECT_FLAIL,
        .power = 1,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 9,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sFlailDescription,
    },

    [MOVE_CONVERSION_2] =
    {
        .name = _("CONVERSION 2"),
        .effect = EFFECT_CONVERSION_2,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = 0,
        .description = sConversion2Description,
    },

    [MOVE_AEROBLAST] =
    {
        .name = _("AEROBLAST"),
        .effect = EFFECT_HIGH_CRITICAL,
        .power = 100,
        .type = TYPE_FLYING,
        .accuracy = 95,
        .tpCost = 15,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sAeroblastDescription,
    },

    [MOVE_COTTON_SPORE] =
    {
        .name = _("COTTON SPORE"),
        .effect = EFFECT_SPEED_DOWN_2,
        .power = 0,
        .type = TYPE_GRASS,
        .accuracy = 85,
        .tpCost = 4,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sCottonSporeDescription,
    },

    [MOVE_REVERSAL] =
    {
        .name = _("REVERSAL"),
        .effect = EFFECT_FLAIL,
        .power = 1,
        .type = TYPE_FIGHTING,
        .accuracy = 100,
        .tpCost = 9,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sReversalDescription,
    },

    [MOVE_SPITE] =
    {
        .name = _("SPITE"),
        .effect = EFFECT_SPITE,
        .power = 0,
        .type = TYPE_GHOST,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sSpiteDescription,
    },

    [MOVE_POWDER_SNOW] =
    {
        .name = _("POWDER SNOW"),
        .effect = EFFECT_FREEZE_HIT,
        .power = 40,
        .type = TYPE_ICE,
        .accuracy = 100,
        .tpCost = 7,
        .secondaryEffectChance = 10,
        .target = MOVE_TARGET_BOTH,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sPowderSnowDescription,
    },

    [MOVE_PROTECT] =
    {
        .name = _("PROTECT"),
        .effect = EFFECT_PROTECT,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 0,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 3,
        .flags = 0,
        .description = sProtectDescription,
    },

    [MOVE_MACH_PUNCH] =
    {
        .name = _("MACH PUNCH"),
        .effect = EFFECT_QUICK_ATTACK,
        .power = 40,
        .type = TYPE_FIGHTING,
        .accuracy = 100,
        .tpCost = 7,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 1,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sMachPunchDescription,
    },

    [MOVE_SCARY_FACE] =
    {
        .name = _("SCARY FACE"),
        .effect = EFFECT_SPEED_DOWN_2,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 90,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sScaryFaceDescription,
    },

    [MOVE_FAINT_ATTACK] =
    {
        .name = _("FAINT ATTACK"),
        .effect = EFFECT_ALWAYS_HIT,
        .power = 60,
        .type = TYPE_DARK,
        .accuracy = 0,
        .tpCost = 9,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sFaintAttackDescription,
    },

    [MOVE_SWEET_KISS] =
    {
        .name = _("SWEET KISS"),
        .effect = EFFECT_CONFUSE,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 75,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sSweetKissDescription,
    },

    [MOVE_BELLY_DRUM] =
    {
        .name = _("BELLY DRUM"),
        .effect = EFFECT_BELLY_DRUM,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 0,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sBellyDrumDescription,
    },

    [MOVE_SLUDGE_BOMB] =
    {
        .name = _("SLUDGE BOMB"),
        .effect = EFFECT_POISON_HIT,
        .power = 90,
        .type = TYPE_POISON,
        .accuracy = 100,
        .tpCost = 14,
        .secondaryEffectChance = 30,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sSludgeBombDescription,
    },

    [MOVE_MUD_SLAP] =
    {
        .name = _("MUD-SLAP"),
        .effect = EFFECT_ACCURACY_DOWN_HIT,
        .power = 20,
        .type = TYPE_GROUND,
        .accuracy = 100,
        .tpCost = 8,
        .secondaryEffectChance = 100,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sMudSlapDescription,
    },

    [MOVE_OCTAZOOKA] =
    {
        .name = _("OCTAZOOKA"),
        .effect = EFFECT_ACCURACY_DOWN_HIT,
        .power = 65,
        .type = TYPE_WATER,
        .accuracy = 85,
        .tpCost = 10,
        .secondaryEffectChance = 50,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sOctazookaDescription,
    },

    [MOVE_SPIKES] =
    {
        .name = _("SPIKES"),
        .effect = EFFECT_SPIKES,
        .power = 0,
        .type = TYPE_GROUND,
        .accuracy = 0,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_OPPONENTS_FIELD,
        .priority = 0,
        .flags = 0,
        .description = sSpikesDescription,
    },

    [MOVE_ZAP_CANNON] =
    {
        .name = _("ZAP CANNON"),
        .effect = EFFECT_PARALYZE_HIT,
        .power = 100,
        .type = TYPE_ELECTRIC,
        .accuracy = 50,
        .tpCost = 14,
        .secondaryEffectChance = 100,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sZapCannonDescription,
    },

    [MOVE_FORESIGHT] =
    {
        .name = _("FORESIGHT"),
        .effect = EFFECT_FORESIGHT,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sForesightDescription,
    },

    [MOVE_DESTINY_BOND] =
    {
        .name = _("DESTINY BOND"),
        .effect = EFFECT_DESTINY_BOND,
        .power = 0,
        .type = TYPE_GHOST,
        .accuracy = 0,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = 0,
        .description = sDestinyBondDescription,
    },

    [MOVE_PERISH_SONG] =
    {
        .name = _("PERISH SONG"),
        .effect = EFFECT_PERISH_SONG,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 0,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = 0,
        .description = sPerishSongDescription,
    },

    [MOVE_ICY_WIND] =
    {
        .name = _("ICY WIND"),
        .effect = EFFECT_SPEED_DOWN_HIT,
        .power = 55,
        .type = TYPE_ICE,
        .accuracy = 95,
        .tpCost = 10,
        .secondaryEffectChance = 100,
        .target = MOVE_TARGET_BOTH,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sIcyWindDescription,
    },

    [MOVE_DETECT] =
    {
        .name = _("DETECT"),
        .effect = EFFECT_PROTECT,
        .power = 0,
        .type = TYPE_FIGHTING,
        .accuracy = 0,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 3,
        .flags = 0,
        .description = sDetectDescription,
    },

    [MOVE_BONE_RUSH] =
    {
        .name = _("BONE RUSH"),
        .effect = EFFECT_MULTI_HIT,
        .power = 25,
        .type = TYPE_GROUND,
        .accuracy = 80,
        .tpCost = 9,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sBoneRushDescription,
    },

    [MOVE_LOCK_ON] =
    {
        .name = _("LOCK-ON"),
        .effect = EFFECT_LOCK_ON,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sLockOnDescription,
    },

    [MOVE_OUTRAGE] =
    {
        .name = _("OUTRAGE"),
        .effect = EFFECT_RAMPAGE,
        .power = 90,
        .type = TYPE_DRAGON,
        .accuracy = 100,
        .tpCost = 13,
        .secondaryEffectChance = 100,
        .target = MOVE_TARGET_RANDOM,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sOutrageDescription,
    },

    [MOVE_SANDSTORM] =
    {
        .name = _("SANDSTORM"),
        .effect = EFFECT_SANDSTORM,
        .power = 0,
        .type = TYPE_ROCK,
        .accuracy = 0,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = 0,
        .description = sSandstormDescription,
    },

    [MOVE_GIGA_DRAIN] =
    {
        .name = _("GIGA DRAIN"),
        .effect = EFFECT_ABSORB,
        .power = 60,
        .type = TYPE_GRASS,
        .accuracy = 100,
        .tpCost = 11,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sGigaDrainDescription,
    },

    [MOVE_ENDURE] =
    {
        .name = _("ENDURE"),
        .effect = EFFECT_ENDURE,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 0,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 3,
        .flags = 0,
        .description = sEndureDescription,
    },

    [MOVE_CHARM] =
    {
        .name = _("CHARM"),
        .effect = EFFECT_ATTACK_DOWN_2,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 6,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sCharmDescription,
    },

    [MOVE_ROLLOUT] =
    {
        .name = _("ROLLOUT"),
        .effect = EFFECT_ROLLOUT,
        .power = 30,
        .type = TYPE_ROCK,
        .accuracy = 90,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sRolloutDescription,
    },

    [MOVE_FALSE_SWIPE] =
    {
        .name = _("FALSE SWIPE"),
        .effect = EFFECT_FALSE_SWIPE,
        .power = 40,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 6,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sFalseSwipeDescription,
    },

    [MOVE_SWAGGER] =
    {
        .name = _("SWAGGER"),
        .effect = EFFECT_SWAGGER,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 90,
        .tpCost = 5,
        .secondaryEffectChance = 100,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sSwaggerDescription,
    },

    [MOVE_MILK_DRINK] =
    {
        .name = _("MILK DRINK"),
        .effect = EFFECT_SOFTBOILED,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 0,
        .tpCost = 9,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_SNATCH_AFFECTED,
        .description = sMilkDrinkDescription,
    },

    [MOVE_SPARK] =
    {
        .name = _("SPARK"),
        .effect = EFFECT_PARALYZE_HIT,
        .power = 65,
        .type = TYPE_ELECTRIC,
        .accuracy = 100,
        .tpCost = 11,
        .secondaryEffectChance = 30,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sSparkDescription,
    },

    [MOVE_FURY_CUTTER] =
    {
        .name = _("FURY CUTTER"),
        .effect = EFFECT_FURY_CUTTER,
        .power = 10,
        .type = TYPE_BUG,
        .accuracy = 95,
        .tpCost = 2,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sFuryCutterDescription,
    },

    [MOVE_STEEL_WING] =
    {
        .name = _("STEEL WING"),
        .effect = EFFECT_DEFENSE_UP_HIT,
        .power = 70,
        .type = TYPE_STEEL,
        .accuracy = 90,
        .tpCost = 9,
        .secondaryEffectChance = 10,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sSteelWingDescription,
    },

    [MOVE_MEAN_LOOK] =
    {
        .name = _("MEAN LOOK"),
        .effect = EFFECT_MEAN_LOOK,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sMeanLookDescription,
    },

    [MOVE_ATTRACT] =
    {
        .name = _("ATTRACT"),
        .effect = EFFECT_ATTRACT,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sAttractDescription,
    },

    [MOVE_SLEEP_TALK] =
    {
        .name = _("SLEEP TALK"),
        .effect = EFFECT_SLEEP_TALK,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 0,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_DEPENDS,
        .priority = 0,
        .flags = 0,
        .description = sSleepTalkDescription,
    },

    [MOVE_HEAL_BELL] =
    {
        .name = _("HEAL BELL"),
        .effect = EFFECT_HEAL_BELL,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 0,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sHealBellDescription,
    },

    [MOVE_RETURN] =
    {
        .name = _("RETURN"),
        .effect = EFFECT_RETURN,
        .power = 1,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 9,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sReturnDescription,
    },

    [MOVE_PRESENT] =
    {
        .name = _("PRESENT"),
        .effect = EFFECT_PRESENT,
        .power = 1,
        .type = TYPE_NORMAL,
        .accuracy = 90,
        .tpCost = 8,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sPresentDescription,
    },

    [MOVE_FRUSTRATION] =
    {
        .name = _("FRUSTRATION"),
        .effect = EFFECT_FRUSTRATION,
        .power = 1,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 9,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sFrustrationDescription,
    },

    [MOVE_SAFEGUARD] =
    {
        .name = _("SAFEGUARD"),
        .effect = EFFECT_SAFEGUARD,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 0,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sSafeguardDescription,
    },

    [MOVE_PAIN_SPLIT] =
    {
        .name = _("PAIN SPLIT"),
        .effect = EFFECT_PAIN_SPLIT,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sPainSplitDescription,
    },

    [MOVE_SACRED_FIRE] =
    {
        .name = _("SACRED FIRE"),
        .effect = EFFECT_THAW_HIT,
        .power = 100,
        .type = TYPE_FIRE,
        .accuracy = 95,
        .tpCost = 13,
        .secondaryEffectChance = 50,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sSacredFireDescription,
    },

    [MOVE_MAGNITUDE] =
    {
        .name = _("MAGNITUDE"),
        .effect = EFFECT_MAGNITUDE,
        .power = 1,
        .type = TYPE_GROUND,
        .accuracy = 100,
        .tpCost = 9,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_FOES_AND_ALLY,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sMagnitudeDescription,
    },

    [MOVE_DYNAMIC_PUNCH] =
    {
        .name = _("DYNAMICPUNCH"),
        .effect = EFFECT_CONFUSE_HIT,
        .power = 100,
        .type = TYPE_FIGHTING,
        .accuracy = 50,
        .tpCost = 13,
        .secondaryEffectChance = 100,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sDynamicPunchDescription,
    },

    [MOVE_MEGAHORN] =
    {
        .name = _("MEGAHORN"),
        .effect = EFFECT_HIT,
        .power = 120,
        .type = TYPE_BUG,
        .accuracy = 85,
        .tpCost = 14,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sMegahornDescription,
    },

    [MOVE_DRAGON_BREATH] =
    {
        .name = _("DRAGONBREATH"),
        .effect = EFFECT_PARALYZE_HIT,
        .power = 60,
        .type = TYPE_DRAGON,
        .accuracy = 100,
        .tpCost = 11,
        .secondaryEffectChance = 30,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sDragonBreathDescription,
    },

    [MOVE_BATON_PASS] =
    {
        .name = _("BATON PASS"),
        .effect = EFFECT_BATON_PASS,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 0,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = 0,
        .description = sBatonPassDescription,
    },

    [MOVE_ENCORE] =
    {
        .name = _("ENCORE"),
        .effect = EFFECT_ENCORE,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sEncoreDescription,
    },

    [MOVE_PURSUIT] =
    {
        .name = _("PURSUIT"),
        .effect = EFFECT_PURSUIT,
        .power = 40,
        .type = TYPE_DARK,
        .accuracy = 100,
        .tpCost = 6,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sPursuitDescription,
    },

    [MOVE_RAPID_SPIN] =
    {
        .name = _("RAPID SPIN"),
        .effect = EFFECT_RAPID_SPIN,
        .power = 20,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 4,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sRapidSpinDescription,
    },

    [MOVE_SWEET_SCENT] =
    {
        .name = _("SWEET SCENT"),
        .effect = EFFECT_EVASION_DOWN,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 3,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_BOTH,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sSweetScentDescription,
    },

    [MOVE_IRON_TAIL] =
    {
        .name = _("IRON TAIL"),
        .effect = EFFECT_DEFENSE_DOWN_HIT,
        .power = 100,
        .type = TYPE_STEEL,
        .accuracy = 75,
        .tpCost = 12,
        .secondaryEffectChance = 30,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sIronTailDescription,
    },

    [MOVE_METAL_CLAW] =
    {
        .name = _("METAL CLAW"),
        .effect = EFFECT_ATTACK_UP_HIT,
        .power = 50,
        .type = TYPE_STEEL,
        .accuracy = 95,
        .tpCost = 7,
        .secondaryEffectChance = 10,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sMetalClawDescription,
    },

    [MOVE_VITAL_THROW] =
    {
        .name = _("VITAL THROW"),
        .effect = EFFECT_VITAL_THROW,
        .power = 70,
        .type = TYPE_FIGHTING,
        .accuracy = 100,
        .tpCost = 10,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = -1,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sVitalThrowDescription,
    },

    [MOVE_MORNING_SUN] =
    {
        .name = _("MORNING SUN"),
        .effect = EFFECT_MORNING_SUN,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 0,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sMorningSunDescription,
    },

    [MOVE_SYNTHESIS] =
    {
        .name = _("SYNTHESIS"),
        .effect = EFFECT_SYNTHESIS,
        .power = 0,
        .type = TYPE_GRASS,
        .accuracy = 0,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sSynthesisDescription,
    },

    [MOVE_MOONLIGHT] =
    {
        .name = _("MOONLIGHT"),
        .effect = EFFECT_MOONLIGHT,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 0,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sMoonlightDescription,
    },

    [MOVE_HIDDEN_POWER] =
    {
        .name = _("HIDDEN POWER"),
        .effect = EFFECT_HIDDEN_POWER,
        .power = 1,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 9,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sHiddenPowerDescription,
    },

    [MOVE_CROSS_CHOP] =
    {
        .name = _("CROSS CHOP"),
        .effect = EFFECT_HIGH_CRITICAL,
        .power = 100,
        .type = TYPE_FIGHTING,
        .accuracy = 80,
        .tpCost = 12,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sCrossChopDescription,
    },

    [MOVE_TWISTER] =
    {
        .name = _("TWISTER"),
        .effect = EFFECT_TWISTER,
        .power = 40,
        .type = TYPE_DRAGON,
        .accuracy = 100,
        .tpCost = 6,
        .secondaryEffectChance = 20,
        .target = MOVE_TARGET_BOTH,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sTwisterDescription,
    },

    [MOVE_RAIN_DANCE] =
    {
        .name = _("RAIN DANCE"),
        .effect = EFFECT_RAIN_DANCE,
        .power = 0,
        .type = TYPE_WATER,
        .accuracy = 0,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = 0,
        .description = sRainDanceDescription,
    },

    [MOVE_SUNNY_DAY] =
    {
        .name = _("SUNNY DAY"),
        .effect = EFFECT_SUNNY_DAY,
        .power = 0,
        .type = TYPE_FIRE,
        .accuracy = 0,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = 0,
        .description = sSunnyDayDescription,
    },

    [MOVE_CRUNCH] =
    {
        .name = _("CRUNCH"),
        .effect = EFFECT_SPECIAL_DEFENSE_DOWN_HIT,
        .power = 80,
        .type = TYPE_DARK,
        .accuracy = 100,
        .tpCost = 12,
        .secondaryEffectChance = 20,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sCrunchDescription,
    },

    [MOVE_MIRROR_COAT] =
    {
        .name = _("MIRROR COAT"),
        .effect = EFFECT_MIRROR_COAT,
        .power = 1,
        .type = TYPE_PSYCHIC,
        .accuracy = 100,
        .tpCost = 9,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_DEPENDS,
        .priority = -5,
        .flags = FLAG_MIRROR_MOVE_AFFECTED,
        .description = sMirrorCoatDescription,
    },

    [MOVE_PSYCH_UP] =
    {
        .name = _("PSYCH UP"),
        .effect = EFFECT_PSYCH_UP,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 0,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sPsychUpDescription,
    },

    [MOVE_EXTREME_SPEED] =
    {
        .name = _("EXTREMESPEED"),
        .effect = EFFECT_QUICK_ATTACK,
        .power = 80,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 12,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 1,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sExtremeSpeedDescription,
    },

    [MOVE_ANCIENT_POWER] =
    {
        .name = _("ANCIENTPOWER"),
        .effect = EFFECT_ALL_STATS_UP_HIT,
        .power = 60,
        .type = TYPE_ROCK,
        .accuracy = 100,
        .tpCost = 9,
        .secondaryEffectChance = 10,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sAncientPowerDescription,
    },

    [MOVE_SHADOW_BALL] =
    {
        .name = _("SHADOW BALL"),
        .effect = EFFECT_SPECIAL_DEFENSE_DOWN_HIT,
        .power = 80,
        .type = TYPE_GHOST,
        .accuracy = 100,
        .tpCost = 12,
        .secondaryEffectChance = 20,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sShadowBallDescription,
    },

    [MOVE_FUTURE_SIGHT] =
    {
        .name = _("FUTURE SIGHT"),
        .effect = EFFECT_FUTURE_SIGHT,
        .power = 80,
        .type = TYPE_PSYCHIC,
        .accuracy = 90,
        .tpCost = 10,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = 0,
        .description = sFutureSightDescription,
    },

    [MOVE_ROCK_SMASH] =
    {
        .name = _("ROCK SMASH"),
        .effect = EFFECT_DEFENSE_DOWN_HIT,
        .power = 20,
        .type = TYPE_FIGHTING,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 50,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sRockSmashDescription,
    },

    [MOVE_WHIRLPOOL] =
    {
        .name = _("WHIRLPOOL"),
        .effect = EFFECT_TRAP,
        .power = 15,
        .type = TYPE_WATER,
        .accuracy = 70,
        .tpCost = 4,
        .secondaryEffectChance = 100,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sWhirlpoolDescription,
    },

    [MOVE_BEAT_UP] =
    {
        .name = _("BEAT UP"),
        .effect = EFFECT_BEAT_UP,
        .power = 10,
        .type = TYPE_DARK,
        .accuracy = 100,
        .tpCost = 2,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sBeatUpDescription,
    },

    [MOVE_FAKE_OUT] =
    {
        .name = _("FAKE OUT"),
        .effect = EFFECT_FAKE_OUT,
        .power = 40,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 7,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 1,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sFakeOutDescription,
    },

    [MOVE_UPROAR] =
    {
        .name = _("UPROAR"),
        .effect = EFFECT_UPROAR,
        .power = 50,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 8,
        .secondaryEffectChance = 100,
        .target = MOVE_TARGET_RANDOM,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sUproarDescription,
    },

    [MOVE_STOCKPILE] =
    {
        .name = _("STOCKPILE"),
        .effect = EFFECT_STOCKPILE,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 0,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sStockpileDescription,
    },

    [MOVE_SPIT_UP] =
    {
        .name = _("SPIT UP"),
        .effect = EFFECT_SPIT_UP,
        .power = 100,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 14,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sSpitUpDescription,
    },

    [MOVE_SWALLOW] =
    {
        .name = _("SWALLOW"),
        .effect = EFFECT_SWALLOW,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 0,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sSwallowDescription,
    },

    [MOVE_HEAT_WAVE] =
    {
        .name = _("HEAT WAVE"),
        .effect = EFFECT_BURN_HIT,
        .power = 100,
        .type = TYPE_FIRE,
        .accuracy = 90,
        .tpCost = 13,
        .secondaryEffectChance = 10,
        .target = MOVE_TARGET_BOTH,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sHeatWaveDescription,
    },

    [MOVE_HAIL] =
    {
        .name = _("HAIL"),
        .effect = EFFECT_HAIL,
        .power = 0,
        .type = TYPE_ICE,
        .accuracy = 0,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED,
        .description = sHailDescription,
    },

    [MOVE_TORMENT] =
    {
        .name = _("TORMENT"),
        .effect = EFFECT_TORMENT,
        .power = 0,
        .type = TYPE_DARK,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sTormentDescription,
    },

    [MOVE_FLATTER] =
    {
        .name = _("FLATTER"),
        .effect = EFFECT_FLATTER,
        .power = 0,
        .type = TYPE_DARK,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sFlatterDescription,
    },

    [MOVE_WILL_O_WISP] =
    {
        .name = _("WILL-O-WISP"),
        .effect = EFFECT_WILL_O_WISP,
        .power = 0,
        .type = TYPE_FIRE,
        .accuracy = 75,
        .tpCost = 6,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sWillOWispDescription,
    },

    [MOVE_MEMENTO] =
    {
        .name = _("MEMENTO"),
        .effect = EFFECT_MEMENTO,
        .power = 0,
        .type = TYPE_DARK,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sMementoDescription,
    },

    [MOVE_FACADE] =
    {
        .name = _("FACADE"),
        .effect = EFFECT_FACADE,
        .power = 70,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 10,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sFacadeDescription,
    },

    [MOVE_FOCUS_PUNCH] =
    {
        .name = _("FOCUS PUNCH"),
        .effect = EFFECT_FOCUS_PUNCH,
        .power = 150,
        .type = TYPE_FIGHTING,
        .accuracy = 100,
        .tpCost = 20,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = -3,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED,
        .description = sFocusPunchDescription,
    },

    [MOVE_SMELLING_SALT] =
    {
        .name = _("SMELLINGSALT"),
        .effect = EFFECT_SMELLINGSALT,
        .power = 60,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 9,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sSmellingSaltDescription,
    },

    [MOVE_FOLLOW_ME] =
    {
        .name = _("FOLLOW ME"),
        .effect = EFFECT_FOLLOW_ME,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 3,
        .flags = 0,
        .description = sFollowMeDescription,
    },

    [MOVE_NATURE_POWER] =
    {
        .name = _("NATURE POWER"),
        .effect = EFFECT_NATURE_POWER,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 95,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_DEPENDS,
        .priority = 0,
        .flags = 0,
        .description = sNaturePowerDescription,
    },

    [MOVE_CHARGE] =
    {
        .name = _("CHARGE"),
        .effect = EFFECT_CHARGE,
        .power = 0,
        .type = TYPE_ELECTRIC,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sChargeDescription,
    },

    [MOVE_TAUNT] =
    {
        .name = _("TAUNT"),
        .effect = EFFECT_TAUNT,
        .power = 0,
        .type = TYPE_DARK,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED,
        .description = sTauntDescription,
    },

    [MOVE_HELPING_HAND] =
    {
        .name = _("HELPING HAND"),
        .effect = EFFECT_HELPING_HAND,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 5,
        .flags = 0,
        .description = sHelpingHandDescription,
    },

    [MOVE_TRICK] =
    {
        .name = _("TRICK"),
        .effect = EFFECT_TRICK,
        .power = 0,
        .type = TYPE_PSYCHIC,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sTrickDescription,
    },

    [MOVE_ROLE_PLAY] =
    {
        .name = _("ROLE PLAY"),
        .effect = EFFECT_ROLE_PLAY,
        .power = 0,
        .type = TYPE_PSYCHIC,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = 0,
        .description = sRolePlayDescription,
    },

    [MOVE_WISH] =
    {
        .name = _("WISH"),
        .effect = EFFECT_WISH,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED,
        .description = sWishDescription,
    },

    [MOVE_ASSIST] =
    {
        .name = _("ASSIST"),
        .effect = EFFECT_ASSIST,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_DEPENDS,
        .priority = 0,
        .flags = 0,
        .description = sAssistDescription,
    },

    [MOVE_INGRAIN] =
    {
        .name = _("INGRAIN"),
        .effect = EFFECT_INGRAIN,
        .power = 0,
        .type = TYPE_GRASS,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sIngrainDescription,
    },

    [MOVE_SUPERPOWER] =
    {
        .name = _("SUPERPOWER"),
        .effect = EFFECT_SUPERPOWER,
        .power = 120,
        .type = TYPE_FIGHTING,
        .accuracy = 100,
        .tpCost = 17,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sSuperpowerDescription,
    },

    [MOVE_MAGIC_COAT] =
    {
        .name = _("MAGIC COAT"),
        .effect = EFFECT_MAGIC_COAT,
        .power = 0,
        .type = TYPE_PSYCHIC,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_DEPENDS,
        .priority = 4,
        .flags = 0,
        .description = sMagicCoatDescription,
    },

    [MOVE_RECYCLE] =
    {
        .name = _("RECYCLE"),
        .effect = EFFECT_RECYCLE,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = 0,
        .description = sRecycleDescription,
    },

    [MOVE_REVENGE] =
    {
        .name = _("REVENGE"),
        .effect = EFFECT_REVENGE,
        .power = 60,
        .type = TYPE_FIGHTING,
        .accuracy = 100,
        .tpCost = 9,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = -4,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sRevengeDescription,
    },

    [MOVE_BRICK_BREAK] =
    {
        .name = _("BRICK BREAK"),
        .effect = EFFECT_BRICK_BREAK,
        .power = 75,
        .type = TYPE_FIGHTING,
        .accuracy = 100,
        .tpCost = 11,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sBrickBreakDescription,
    },

    [MOVE_YAWN] =
    {
        .name = _("YAWN"),
        .effect = EFFECT_YAWN,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sYawnDescription,
    },

    [MOVE_KNOCK_OFF] =
    {
        .name = _("KNOCK OFF"),
        .effect = EFFECT_KNOCK_OFF,
        .power = 20,
        .type = TYPE_DARK,
        .accuracy = 100,
        .tpCost = 4,
        .secondaryEffectChance = 100,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sKnockOffDescription,
    },

    [MOVE_ENDEAVOR] =
    {
        .name = _("ENDEAVOR"),
        .effect = EFFECT_ENDEAVOR,
        .power = 1,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 9,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sEndeavorDescription,
    },

    [MOVE_ERUPTION] =
    {
        .name = _("ERUPTION"),
        .effect = EFFECT_ERUPTION,
        .power = 150,
        .type = TYPE_FIRE,
        .accuracy = 100,
        .tpCost = 20,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_BOTH,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sEruptionDescription,
    },

    [MOVE_SKILL_SWAP] =
    {
        .name = _("SKILL SWAP"),
        .effect = EFFECT_SKILL_SWAP,
        .power = 0,
        .type = TYPE_PSYCHIC,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sSkillSwapDescription,
    },

    [MOVE_IMPRISON] =
    {
        .name = _("IMPRISON"),
        .effect = EFFECT_IMPRISON,
        .power = 0,
        .type = TYPE_PSYCHIC,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED,
        .description = sImprisonDescription,
    },

    [MOVE_REFRESH] =
    {
        .name = _("REFRESH"),
        .effect = EFFECT_REFRESH,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sRefreshDescription,
    },

    [MOVE_GRUDGE] =
    {
        .name = _("GRUDGE"),
        .effect = EFFECT_GRUDGE,
        .power = 0,
        .type = TYPE_GHOST,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sGrudgeDescription,
    },

    [MOVE_SNATCH] =
    {
        .name = _("SNATCH"),
        .effect = EFFECT_SNATCH,
        .power = 0,
        .type = TYPE_DARK,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_DEPENDS,
        .priority = 4,
        .flags = FLAG_MIRROR_MOVE_AFFECTED,
        .description = sSnatchDescription,
    },

    [MOVE_SECRET_POWER] =
    {
        .name = _("SECRET POWER"),
        .effect = EFFECT_SECRET_POWER,
        .power = 70,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 10,
        .secondaryEffectChance = 30,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sSecretPowerDescription,
    },

    [MOVE_DIVE] =
    {
        .name = _("DIVE"),
        .effect = EFFECT_SEMI_INVULNERABLE,
        .power = 60,
        .type = TYPE_WATER,
        .accuracy = 100,
        .tpCost = 6,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sDiveDescription,
    },

    [MOVE_ARM_THRUST] =
    {
        .name = _("ARM THRUST"),
        .effect = EFFECT_MULTI_HIT,
        .power = 15,
        .type = TYPE_FIGHTING,
        .accuracy = 100,
        .tpCost = 7,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sArmThrustDescription,
    },

    [MOVE_CAMOUFLAGE] =
    {
        .name = _("CAMOUFLAGE"),
        .effect = EFFECT_CAMOUFLAGE,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sCamouflageDescription,
    },

    [MOVE_TAIL_GLOW] =
    {
        .name = _("TAIL GLOW"),
        .effect = EFFECT_SPECIAL_ATTACK_UP_2,
        .power = 0,
        .type = TYPE_BUG,
        .accuracy = 100,
        .tpCost = 7,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sTailGlowDescription,
    },

    [MOVE_LUSTER_PURGE] =
    {
        .name = _("LUSTER PURGE"),
        .effect = EFFECT_SPECIAL_DEFENSE_DOWN_HIT,
        .power = 70,
        .type = TYPE_PSYCHIC,
        .accuracy = 100,
        .tpCost = 12,
        .secondaryEffectChance = 50,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sLusterPurgeDescription,
    },

    [MOVE_MIST_BALL] =
    {
        .name = _("MIST BALL"),
        .effect = EFFECT_SPECIAL_ATTACK_DOWN_HIT,
        .power = 70,
        .type = TYPE_PSYCHIC,
        .accuracy = 100,
        .tpCost = 12,
        .secondaryEffectChance = 50,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sMistBallDescription,
    },

    [MOVE_FEATHER_DANCE] =
    {
        .name = _("FEATHERDANCE"),
        .effect = EFFECT_ATTACK_DOWN_2,
        .power = 0,
        .type = TYPE_FLYING,
        .accuracy = 100,
        .tpCost = 6,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sFeatherDanceDescription,
    },

    [MOVE_TEETER_DANCE] =
    {
        .name = _("TEETER DANCE"),
        .effect = EFFECT_TEETER_DANCE,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_FOES_AND_ALLY,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED,
        .description = sTeeterDanceDescription,
    },

    [MOVE_BLAZE_KICK] =
    {
        .name = _("BLAZE KICK"),
        .effect = EFFECT_BLAZE_KICK,
        .power = 85,
        .type = TYPE_FIRE,
        .accuracy = 90,
        .tpCost = 12,
        .secondaryEffectChance = 10,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sBlazeKickDescription,
    },

    [MOVE_MUD_SPORT] =
    {
        .name = _("MUD SPORT"),
        .effect = EFFECT_MUD_SPORT,
        .power = 0,
        .type = TYPE_GROUND,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = 0,
        .description = sMudSportDescription,
    },

    [MOVE_ICE_BALL] =
    {
        .name = _("ICE BALL"),
        .effect = EFFECT_ROLLOUT,
        .power = 30,
        .type = TYPE_ICE,
        .accuracy = 90,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sIceBallDescription,
    },

    [MOVE_NEEDLE_ARM] =
    {
        .name = _("NEEDLE ARM"),
        .effect = EFFECT_FLINCH_MINIMIZE_HIT,
        .power = 60,
        .type = TYPE_GRASS,
        .accuracy = 100,
        .tpCost = 9,
        .secondaryEffectChance = 30,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sNeedleArmDescription,
    },

    [MOVE_SLACK_OFF] =
    {
        .name = _("SLACK OFF"),
        .effect = EFFECT_RESTORE_HP,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 9,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sSlackOffDescription,
    },

    [MOVE_HYPER_VOICE] =
    {
        .name = _("HYPER VOICE"),
        .effect = EFFECT_HIT,
        .power = 90,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 13,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_BOTH,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sHyperVoiceDescription,
    },

    [MOVE_POISON_FANG] =
    {
        .name = _("POISON FANG"),
        .effect = EFFECT_POISON_FANG,
        .power = 50,
        .type = TYPE_POISON,
        .accuracy = 100,
        .tpCost = 8,
        .secondaryEffectChance = 30,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sPoisonFangDescription,
    },

    [MOVE_CRUSH_CLAW] =
    {
        .name = _("CRUSH CLAW"),
        .effect = EFFECT_DEFENSE_DOWN_HIT,
        .power = 75,
        .type = TYPE_NORMAL,
        .accuracy = 95,
        .tpCost = 12,
        .secondaryEffectChance = 50,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sCrushClawDescription,
    },

    [MOVE_BLAST_BURN] =
    {
        .name = _("BLAST BURN"),
        .effect = EFFECT_RECHARGE,
        .power = 150,
        .type = TYPE_FIRE,
        .accuracy = 90,
        .tpCost = 12,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sBlastBurnDescription,
    },

    [MOVE_HYDRO_CANNON] =
    {
        .name = _("HYDRO CANNON"),
        .effect = EFFECT_RECHARGE,
        .power = 150,
        .type = TYPE_WATER,
        .accuracy = 90,
        .tpCost = 12,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sHydroCannonDescription,
    },

    [MOVE_METEOR_MASH] =
    {
        .name = _("METEOR MASH"),
        .effect = EFFECT_ATTACK_UP_HIT,
        .power = 100,
        .type = TYPE_STEEL,
        .accuracy = 85,
        .tpCost = 12,
        .secondaryEffectChance = 20,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sMeteorMashDescription,
    },

    [MOVE_ASTONISH] =
    {
        .name = _("ASTONISH"),
        .effect = EFFECT_FLINCH_MINIMIZE_HIT,
        .power = 30,
        .type = TYPE_GHOST,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 30,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sAstonishDescription,
    },

    [MOVE_WEATHER_BALL] =
    {
        .name = _("WEATHER BALL"),
        .effect = EFFECT_WEATHER_BALL,
        .power = 50,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 8,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sWeatherBallDescription,
    },

    [MOVE_AROMATHERAPY] =
    {
        .name = _("AROMATHERAPY"),
        .effect = EFFECT_HEAL_BELL,
        .power = 0,
        .type = TYPE_GRASS,
        .accuracy = 0,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sAromatherapyDescription,
    },

    [MOVE_FAKE_TEARS] =
    {
        .name = _("FAKE TEARS"),
        .effect = EFFECT_SPECIAL_DEFENSE_DOWN_2,
        .power = 0,
        .type = TYPE_DARK,
        .accuracy = 100,
        .tpCost = 6,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sFakeTearsDescription,
    },

    [MOVE_AIR_CUTTER] =
    {
        .name = _("AIR CUTTER"),
        .effect = EFFECT_HIGH_CRITICAL,
        .power = 55,
        .type = TYPE_FLYING,
        .accuracy = 95,
        .tpCost = 8,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_BOTH,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sAirCutterDescription,
    },

    [MOVE_OVERHEAT] =
    {
        .name = _("OVERHEAT"),
        .effect = EFFECT_OVERHEAT,
        .power = 140,
        .type = TYPE_FIRE,
        .accuracy = 90,
        .tpCost = 17,
        .secondaryEffectChance = 100,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sOverheatDescription,
    },

    [MOVE_ODOR_SLEUTH] =
    {
        .name = _("ODOR SLEUTH"),
        .effect = EFFECT_FORESIGHT,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sOdorSleuthDescription,
    },

    [MOVE_ROCK_TOMB] =
    {
        .name = _("ROCK TOMB"),
        .effect = EFFECT_SPEED_DOWN_HIT,
        .power = 50,
        .type = TYPE_ROCK,
        .accuracy = 80,
        .tpCost = 9,
        .secondaryEffectChance = 100,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sRockTombDescription,
    },

    [MOVE_SILVER_WIND] =
    {
        .name = _("SILVER WIND"),
        .effect = EFFECT_ALL_STATS_UP_HIT,
        .power = 60,
        .type = TYPE_BUG,
        .accuracy = 100,
        .tpCost = 9,
        .secondaryEffectChance = 10,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sSilverWindDescription,
    },

    [MOVE_METAL_SOUND] =
    {
        .name = _("METAL SOUND"),
        .effect = EFFECT_SPECIAL_DEFENSE_DOWN_2,
        .power = 0,
        .type = TYPE_STEEL,
        .accuracy = 85,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sMetalSoundDescription,
    },

    [MOVE_GRASS_WHISTLE] =
    {
        .name = _("GRASSWHISTLE"),
        .effect = EFFECT_SLEEP,
        .power = 0,
        .type = TYPE_GRASS,
        .accuracy = 55,
        .tpCost = 7,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sGrassWhistleDescription,
    },

    [MOVE_TICKLE] =
    {
        .name = _("TICKLE"),
        .effect = EFFECT_TICKLE,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sTickleDescription,
    },

    [MOVE_COSMIC_POWER] =
    {
        .name = _("COSMIC POWER"),
        .effect = EFFECT_COSMIC_POWER,
        .power = 0,
        .type = TYPE_PSYCHIC,
        .accuracy = 0,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sCosmicPowerDescription,
    },

    [MOVE_WATER_SPOUT] =
    {
        .name = _("WATER SPOUT"),
        .effect = EFFECT_ERUPTION,
        .power = 150,
        .type = TYPE_WATER,
        .accuracy = 100,
        .tpCost = 20,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_BOTH,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sWaterSpoutDescription,
    },

    [MOVE_SIGNAL_BEAM] =
    {
        .name = _("SIGNAL BEAM"),
        .effect = EFFECT_CONFUSE_HIT,
        .power = 75,
        .type = TYPE_BUG,
        .accuracy = 100,
        .tpCost = 11,
        .secondaryEffectChance = 10,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sSignalBeamDescription,
    },

    [MOVE_SHADOW_PUNCH] =
    {
        .name = _("SHADOW PUNCH"),
        .effect = EFFECT_ALWAYS_HIT,
        .power = 60,
        .type = TYPE_GHOST,
        .accuracy = 0,
        .tpCost = 9,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sShadowPunchDescription,
    },

    [MOVE_EXTRASENSORY] =
    {
        .name = _("EXTRASENSORY"),
        .effect = EFFECT_FLINCH_MINIMIZE_HIT,
        .power = 80,
        .type = TYPE_PSYCHIC,
        .accuracy = 100,
        .tpCost = 11,
        .secondaryEffectChance = 10,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sExtrasensoryDescription,
    },

    [MOVE_SKY_UPPERCUT] =
    {
        .name = _("SKY UPPERCUT"),
        .effect = EFFECT_SKY_UPPERCUT,
        .power = 85,
        .type = TYPE_FIGHTING,
        .accuracy = 90,
        .tpCost = 11,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sSkyUppercutDescription,
    },

    [MOVE_SAND_TOMB] =
    {
        .name = _("SAND TOMB"),
        .effect = EFFECT_TRAP,
        .power = 15,
        .type = TYPE_GROUND,
        .accuracy = 70,
        .tpCost = 4,
        .secondaryEffectChance = 100,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sSandTombDescription,
    },

    [MOVE_SHEER_COLD] =
    {
        .name = _("SHEER COLD"),
        .effect = EFFECT_OHKO,
        .power = 1,
        .type = TYPE_ICE,
        .accuracy = 30,
        .tpCost = 7,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sSheerColdDescription,
    },

    [MOVE_MUDDY_WATER] =
    {
        .name = _("MUDDY WATER"),
        .effect = EFFECT_ACCURACY_DOWN_HIT,
        .power = 95,
        .type = TYPE_WATER,
        .accuracy = 85,
        .tpCost = 13,
        .secondaryEffectChance = 30,
        .target = MOVE_TARGET_BOTH,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sMuddyWaterDescription,
    },

    [MOVE_BULLET_SEED] =
    {
        .name = _("BULLET SEED"),
        .effect = EFFECT_MULTI_HIT,
        .power = 10,
        .type = TYPE_GRASS,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sBulletSeedDescription,
    },

    [MOVE_AERIAL_ACE] =
    {
        .name = _("AERIAL ACE"),
        .effect = EFFECT_ALWAYS_HIT,
        .power = 60,
        .type = TYPE_FLYING,
        .accuracy = 0,
        .tpCost = 9,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sAerialAceDescription,
    },

    [MOVE_ICICLE_SPEAR] =
    {
        .name = _("ICICLE SPEAR"),
        .effect = EFFECT_MULTI_HIT,
        .power = 10,
        .type = TYPE_ICE,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sIcicleSpearDescription,
    },

    [MOVE_IRON_DEFENSE] =
    {
        .name = _("IRON DEFENSE"),
        .effect = EFFECT_DEFENSE_UP_2,
        .power = 0,
        .type = TYPE_STEEL,
        .accuracy = 0,
        .tpCost = 6,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sIronDefenseDescription,
    },

    [MOVE_BLOCK] =
    {
        .name = _("BLOCK"),
        .effect = EFFECT_MEAN_LOOK,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MAGIC_COAT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sBlockDescription,
    },

    [MOVE_HOWL] =
    {
        .name = _("HOWL"),
        .effect = EFFECT_ATTACK_UP,
        .power = 0,
        .type = TYPE_NORMAL,
        .accuracy = 0,
        .tpCost = 4,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sHowlDescription,
    },

    [MOVE_DRAGON_CLAW] =
    {
        .name = _("DRAGON CLAW"),
        .effect = EFFECT_HIT,
        .power = 80,
        .type = TYPE_DRAGON,
        .accuracy = 100,
        .tpCost = 11,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sDragonClawDescription,
    },

    [MOVE_FRENZY_PLANT] =
    {
        .name = _("FRENZY PLANT"),
        .effect = EFFECT_RECHARGE,
        .power = 150,
        .type = TYPE_GRASS,
        .accuracy = 90,
        .tpCost = 12,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sFrenzyPlantDescription,
    },

    [MOVE_BULK_UP] =
    {
        .name = _("BULK UP"),
        .effect = EFFECT_BULK_UP,
        .power = 0,
        .type = TYPE_FIGHTING,
        .accuracy = 0,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sBulkUpDescription,
    },

    [MOVE_BOUNCE] =
    {
        .name = _("BOUNCE"),
        .effect = EFFECT_SEMI_INVULNERABLE,
        .power = 85,
        .type = TYPE_FLYING,
        .accuracy = 85,
        .tpCost = 7,
        .secondaryEffectChance = 30,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sBounceDescription,
    },

    [MOVE_MUD_SHOT] =
    {
        .name = _("MUD SHOT"),
        .effect = EFFECT_SPEED_DOWN_HIT,
        .power = 55,
        .type = TYPE_GROUND,
        .accuracy = 95,
        .tpCost = 10,
        .secondaryEffectChance = 100,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sMudShotDescription,
    },

    [MOVE_POISON_TAIL] =
    {
        .name = _("POISON TAIL"),
        .effect = EFFECT_POISON_TAIL,
        .power = 50,
        .type = TYPE_POISON,
        .accuracy = 100,
        .tpCost = 8,
        .secondaryEffectChance = 10,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sPoisonTailDescription,
    },

    [MOVE_COVET] =
    {
        .name = _("COVET"),
        .effect = EFFECT_THIEF,
        .power = 40,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 6,
        .secondaryEffectChance = 100,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED,
        .description = sCovetDescription,
    },

    [MOVE_VOLT_TACKLE] =
    {
        .name = _("VOLT TACKLE"),
        .effect = EFFECT_DOUBLE_EDGE,
        .power = 120,
        .type = TYPE_ELECTRIC,
        .accuracy = 100,
        .tpCost = 14,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sVoltTackleDescription,
    },

    [MOVE_MAGICAL_LEAF] =
    {
        .name = _("MAGICAL LEAF"),
        .effect = EFFECT_ALWAYS_HIT,
        .power = 60,
        .type = TYPE_GRASS,
        .accuracy = 0,
        .tpCost = 9,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sMagicalLeafDescription,
    },

    [MOVE_WATER_SPORT] =
    {
        .name = _("WATER SPORT"),
        .effect = EFFECT_WATER_SPORT,
        .power = 0,
        .type = TYPE_WATER,
        .accuracy = 100,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = 0,
        .description = sWaterSportDescription,
    },

    [MOVE_CALM_MIND] =
    {
        .name = _("CALM MIND"),
        .effect = EFFECT_CALM_MIND,
        .power = 0,
        .type = TYPE_PSYCHIC,
        .accuracy = 0,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sCalmMindDescription,
    },

    [MOVE_LEAF_BLADE] =
    {
        .name = _("LEAF BLADE"),
        .effect = EFFECT_HIGH_CRITICAL,
        .power = 70,
        .type = TYPE_GRASS,
        .accuracy = 100,
        .tpCost = 11,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sLeafBladeDescription,
    },

    [MOVE_DRAGON_DANCE] =
    {
        .name = _("DRAGON DANCE"),
        .effect = EFFECT_DRAGON_DANCE,
        .power = 0,
        .type = TYPE_DRAGON,
        .accuracy = 0,
        .tpCost = 5,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_USER,
        .priority = 0,
        .flags = FLAG_SNATCH_AFFECTED,
        .description = sDragonDanceDescription,
    },

    [MOVE_ROCK_BLAST] =
    {
        .name = _("ROCK BLAST"),
        .effect = EFFECT_MULTI_HIT,
        .power = 25,
        .type = TYPE_ROCK,
        .accuracy = 80,
        .tpCost = 9,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sRockBlastDescription,
    },

    [MOVE_SHOCK_WAVE] =
    {
        .name = _("SHOCK WAVE"),
        .effect = EFFECT_ALWAYS_HIT,
        .power = 60,
        .type = TYPE_ELECTRIC,
        .accuracy = 0,
        .tpCost = 9,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sShockWaveDescription,
    },

    [MOVE_WATER_PULSE] =
    {
        .name = _("WATER PULSE"),
        .effect = EFFECT_CONFUSE_HIT,
        .power = 60,
        .type = TYPE_WATER,
        .accuracy = 100,
        .tpCost = 10,
        .secondaryEffectChance = 20,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sWaterPulseDescription,
    },

    [MOVE_DOOM_DESIRE] =
    {
        .name = _("DOOM DESIRE"),
        .effect = EFFECT_FUTURE_SIGHT,
        .power = 120,
        .type = TYPE_STEEL,
        .accuracy = 85,
        .tpCost = 14,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = 0,
        .description = sDoomDesireDescription,
    },

    [MOVE_PSYCHO_BOOST] =
    {
        .name = _("PSYCHO BOOST"),
        .effect = EFFECT_OVERHEAT,
        .power = 140,
        .type = TYPE_PSYCHIC,
        .accuracy = 90,
        .tpCost = 17,
        .secondaryEffectChance = 100,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .description = sPsychoBoostDescription,
    },

    // Specials: one per species line, used from the Special page of the FIGHT menu
    // (gSpecialMoves in src/data/pokemon/special_moves.h). Placeholder numbers:
    // 40-50 TP per the balance model (FINDINGS section 10).
    [MOVE_FLAME_CLAWS] =
    {
        .name = _("FLAME CLAWS"),
        .effect = EFFECT_BURN_HIT,
        .power = 90,
        .type = TYPE_FIRE,
        .accuracy = 100,
        .tpCost = 45,
        .secondaryEffectChance = 30,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .signatureSpecies = SPECIES_CHARMANDER,
        .description = sFlameClawsDescription,
    },

    // Farfetch'd (Notion Moves DB: SIGNATURE, NORMAL, 60, 100, high crit)
    [MOVE_LEEK_SLAP] =
    {
        .name = _("LEEK SLAP"),
        .effect = EFFECT_HIGH_CRITICAL,
        .power = 60,
        .type = TYPE_NORMAL,
        .accuracy = 100,
        .tpCost = 40,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_MIRROR_MOVE_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .signatureSpecies = SPECIES_FARFETCHD,
        .description = sLeekSlapDescription,
    },

#if STARTER_SIGNATURES_PROPOSAL
    // PLACEHOLDER numbers (Q97; thread "Research TCG attacks as lost moves"). Omar's names, GREEDY PLANT and
    // SKULL BASH; the Special Factor is per move id, so the Squirtle line's Skull Bash has its own id.
    [MOVE_GREEDY_PLANT] =
    {
        .name = _("GREEDY PLANT"),
        .effect = EFFECT_ABSORB,
        .power = 90,
        .type = TYPE_GRASS,
        .accuracy = 100,
        .tpCost = 45,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_PROTECT_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .signatureSpecies = SPECIES_BULBASAUR,
        .description = sGreedyPlantDescription,
    },

    [MOVE_SKULL_BASH_SIGNATURE] =
    {
        .name = _("SKULL BASH"),
        .effect = EFFECT_SKULL_BASH,
        .power = 100,
        .type = TYPE_WATER,
        .accuracy = 100,
        .tpCost = 45,
        .secondaryEffectChance = 0,
        .target = MOVE_TARGET_SELECTED,
        .priority = 0,
        .flags = FLAG_MAKES_CONTACT | FLAG_PROTECT_AFFECTED | FLAG_KINGS_ROCK_AFFECTED,
        .signatureSpecies = SPECIES_SQUIRTLE,
        .description = sSkullBashSignatureDescription,
    },
#endif
};
