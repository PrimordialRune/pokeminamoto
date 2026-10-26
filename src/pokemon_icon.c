#include "global.h"
#include "graphics.h"
#include "mail.h"
#include "palette.h"
#include "pokemon_icon.h"
#include "sprite.h"
#include "constants/pokemon_icon.h"

#define INVALID_ICON_SPECIES SPECIES_OLD_UNOWN_J // Oddly specific, used when an icon should be a ?. Any of the 'old unown' would work

struct MonIconSpriteTemplate
{
    const struct OamData *oam;
    const u8 *image;
    const union AnimCmd *const *anims;
    const union AffineAnimCmd *const *affineAnims;
    void (*callback)(struct Sprite *);
    u16 paletteTag;
};

static u8 CreateMonIconSprite(struct MonIconSpriteTemplate *, s16, s16, u8);
static void FreeAndDestroyMonIconSprite_(struct Sprite *sprite);

const u8 *const gMonIconTable[] =
{
    [SPECIES_NONE] = gMonIcon_Bulbasaur,
    [SPECIES_BULBASAUR] = gMonIcon_Bulbasaur,
    [SPECIES_IVYSAUR] = gMonIcon_Ivysaur,
    [SPECIES_VENUSAUR] = gMonIcon_Venusaur,
    [SPECIES_CHARMANDER] = gMonIcon_Charmander,
    [SPECIES_CHARMELEON] = gMonIcon_Charmeleon,
    [SPECIES_CHARIZARD] = gMonIcon_Charizard,
    [SPECIES_SQUIRTLE] = gMonIcon_Squirtle,
    [SPECIES_WARTORTLE] = gMonIcon_Wartortle,
    [SPECIES_BLASTOISE] = gMonIcon_Blastoise,
    [SPECIES_CATERPIE] = gMonIcon_Caterpie,
    [SPECIES_METAPOD] = gMonIcon_Metapod,
    [SPECIES_BUTTERFREE] = gMonIcon_Butterfree,
    [SPECIES_WEEDLE] = gMonIcon_Weedle,
    [SPECIES_KAKUNA] = gMonIcon_Kakuna,
    [SPECIES_BEEDRILL] = gMonIcon_Beedrill,
    [SPECIES_PIDGEY] = gMonIcon_Pidgey,
    [SPECIES_PIDGEOTTO] = gMonIcon_Pidgeotto,
    [SPECIES_PIDGEOT] = gMonIcon_Pidgeot,
    [SPECIES_RATTATA] = gMonIcon_Rattata,
    [SPECIES_RATICATE] = gMonIcon_Raticate,
    [SPECIES_SPEAROW] = gMonIcon_Spearow,
    [SPECIES_FEAROW] = gMonIcon_Fearow,
    [SPECIES_EKANS] = gMonIcon_Ekans,
    [SPECIES_ARBOK] = gMonIcon_Arbok,
    [SPECIES_PIKACHU] = gMonIcon_Pikachu,
    [SPECIES_RAICHU] = gMonIcon_Raichu,
    [SPECIES_SANDSHREW] = gMonIcon_Sandshrew,
    [SPECIES_SANDSLASH] = gMonIcon_Sandslash,
    [SPECIES_NIDORAN_F] = gMonIcon_NidoranF,
    [SPECIES_NIDORINA] = gMonIcon_Nidorina,
    [SPECIES_NIDOQUEEN] = gMonIcon_Nidoqueen,
    [SPECIES_NIDORAN_M] = gMonIcon_NidoranM,
    [SPECIES_NIDORINO] = gMonIcon_Nidorino,
    [SPECIES_NIDOKING] = gMonIcon_Nidoking,
    [SPECIES_CLEFAIRY] = gMonIcon_Clefairy,
    [SPECIES_CLEFABLE] = gMonIcon_Clefable,
    [SPECIES_VULPIX] = gMonIcon_Vulpix,
    [SPECIES_NINETALES] = gMonIcon_Ninetales,
    [SPECIES_JIGGLYPUFF] = gMonIcon_Jigglypuff,
    [SPECIES_WIGGLYTUFF] = gMonIcon_Wigglytuff,
    [SPECIES_ZUBAT] = gMonIcon_Zubat,
    [SPECIES_GOLBAT] = gMonIcon_Golbat,
    [SPECIES_ODDISH] = gMonIcon_Oddish,
    [SPECIES_GLOOM] = gMonIcon_Gloom,
    [SPECIES_VILEPLUME] = gMonIcon_Vileplume,
    [SPECIES_PARAS] = gMonIcon_Paras,
    [SPECIES_PARASECT] = gMonIcon_Parasect,
    [SPECIES_VENONAT] = gMonIcon_Venonat,
    [SPECIES_VENOMOTH] = gMonIcon_Venomoth,
    [SPECIES_DIGLETT] = gMonIcon_Diglett,
    [SPECIES_DUGTRIO] = gMonIcon_Dugtrio,
    [SPECIES_MEOWTH] = gMonIcon_Meowth,
    [SPECIES_PERSIAN] = gMonIcon_Persian,
    [SPECIES_PSYDUCK] = gMonIcon_Psyduck,
    [SPECIES_GOLDUCK] = gMonIcon_Golduck,
    [SPECIES_MANKEY] = gMonIcon_Mankey,
    [SPECIES_PRIMEAPE] = gMonIcon_Primeape,
    [SPECIES_GROWLITHE] = gMonIcon_Growlithe,
    [SPECIES_ARCANINE] = gMonIcon_Arcanine,
    [SPECIES_POLIWAG] = gMonIcon_Poliwag,
    [SPECIES_POLIWHIRL] = gMonIcon_Poliwhirl,
    [SPECIES_POLIWRATH] = gMonIcon_Poliwrath,
    [SPECIES_ABRA] = gMonIcon_Abra,
    [SPECIES_KADABRA] = gMonIcon_Kadabra,
    [SPECIES_ALAKAZAM] = gMonIcon_Alakazam,
    [SPECIES_MACHOP] = gMonIcon_Machop,
    [SPECIES_MACHOKE] = gMonIcon_Machoke,
    [SPECIES_MACHAMP] = gMonIcon_Machamp,
    [SPECIES_BELLSPROUT] = gMonIcon_Bellsprout,
    [SPECIES_WEEPINBELL] = gMonIcon_Weepinbell,
    [SPECIES_VICTREEBEL] = gMonIcon_Victreebel,
    [SPECIES_TENTACOOL] = gMonIcon_Tentacool,
    [SPECIES_TENTACRUEL] = gMonIcon_Tentacruel,
    [SPECIES_GEODUDE] = gMonIcon_Geodude,
    [SPECIES_GRAVELER] = gMonIcon_Graveler,
    [SPECIES_GOLEM] = gMonIcon_Golem,
    [SPECIES_PONYTA] = gMonIcon_Ponyta,
    [SPECIES_RAPIDASH] = gMonIcon_Rapidash,
    [SPECIES_SLOWPOKE] = gMonIcon_Slowpoke,
    [SPECIES_SLOWBRO] = gMonIcon_Slowbro,
    [SPECIES_MAGNEMITE] = gMonIcon_Magnemite,
    [SPECIES_MAGNETON] = gMonIcon_Magneton,
    [SPECIES_FARFETCHD] = gMonIcon_Farfetchd,
    [SPECIES_DODUO] = gMonIcon_Doduo,
    [SPECIES_DODRIO] = gMonIcon_Dodrio,
    [SPECIES_SEEL] = gMonIcon_Seel,
    [SPECIES_DEWGONG] = gMonIcon_Dewgong,
    [SPECIES_GRIMER] = gMonIcon_Grimer,
    [SPECIES_MUK] = gMonIcon_Muk,
    [SPECIES_SHELLDER] = gMonIcon_Shellder,
    [SPECIES_CLOYSTER] = gMonIcon_Cloyster,
    [SPECIES_GASTLY] = gMonIcon_Gastly,
    [SPECIES_HAUNTER] = gMonIcon_Haunter,
    [SPECIES_GENGAR] = gMonIcon_Gengar,
    [SPECIES_ONIX] = gMonIcon_Onix,
    [SPECIES_DROWZEE] = gMonIcon_Drowzee,
    [SPECIES_HYPNO] = gMonIcon_Hypno,
    [SPECIES_KRABBY] = gMonIcon_Krabby,
    [SPECIES_KINGLER] = gMonIcon_Kingler,
    [SPECIES_VOLTORB] = gMonIcon_Voltorb,
    [SPECIES_ELECTRODE] = gMonIcon_Electrode,
    [SPECIES_EXEGGCUTE] = gMonIcon_Exeggcute,
    [SPECIES_EXEGGUTOR] = gMonIcon_Exeggutor,
    [SPECIES_CUBONE] = gMonIcon_Cubone,
    [SPECIES_MAROWAK] = gMonIcon_Marowak,
    [SPECIES_HITMONLEE] = gMonIcon_Hitmonlee,
    [SPECIES_HITMONCHAN] = gMonIcon_Hitmonchan,
    [SPECIES_LICKITUNG] = gMonIcon_Lickitung,
    [SPECIES_KOFFING] = gMonIcon_Koffing,
    [SPECIES_WEEZING] = gMonIcon_Weezing,
    [SPECIES_RHYHORN] = gMonIcon_Rhyhorn,
    [SPECIES_RHYDON] = gMonIcon_Rhydon,
    [SPECIES_CHANSEY] = gMonIcon_Chansey,
    [SPECIES_TANGELA] = gMonIcon_Tangela,
    [SPECIES_KANGASKHAN] = gMonIcon_Kangaskhan,
    [SPECIES_HORSEA] = gMonIcon_Horsea,
    [SPECIES_SEADRA] = gMonIcon_Seadra,
    [SPECIES_GOLDEEN] = gMonIcon_Goldeen,
    [SPECIES_SEAKING] = gMonIcon_Seaking,
    [SPECIES_STARYU] = gMonIcon_Staryu,
    [SPECIES_STARMIE] = gMonIcon_Starmie,
    [SPECIES_MR_MIME] = gMonIcon_MrMime,
    [SPECIES_SCYTHER] = gMonIcon_Scyther,
    [SPECIES_JYNX] = gMonIcon_Jynx,
    [SPECIES_ELECTABUZZ] = gMonIcon_Electabuzz,
    [SPECIES_MAGMAR] = gMonIcon_Magmar,
    [SPECIES_PINSIR] = gMonIcon_Pinsir,
    [SPECIES_TAUROS] = gMonIcon_Tauros,
    [SPECIES_MAGIKARP] = gMonIcon_Magikarp,
    [SPECIES_GYARADOS] = gMonIcon_Gyarados,
    [SPECIES_LAPRAS] = gMonIcon_Lapras,
    [SPECIES_DITTO] = gMonIcon_Ditto,
    [SPECIES_EEVEE] = gMonIcon_Eevee,
    [SPECIES_VAPOREON] = gMonIcon_Vaporeon,
    [SPECIES_JOLTEON] = gMonIcon_Jolteon,
    [SPECIES_FLAREON] = gMonIcon_Flareon,
    [SPECIES_PORYGON] = gMonIcon_Porygon,
    [SPECIES_OMANYTE] = gMonIcon_Omanyte,
    [SPECIES_OMASTAR] = gMonIcon_Omastar,
    [SPECIES_KABUTO] = gMonIcon_Kabuto,
    [SPECIES_KABUTOPS] = gMonIcon_Kabutops,
    [SPECIES_AERODACTYL] = gMonIcon_Aerodactyl,
    [SPECIES_SNORLAX] = gMonIcon_Snorlax,
    [SPECIES_ARTICUNO] = gMonIcon_Articuno,
    [SPECIES_ZAPDOS] = gMonIcon_Zapdos,
    [SPECIES_MOLTRES] = gMonIcon_Moltres,
    [SPECIES_DRATINI] = gMonIcon_Dratini,
    [SPECIES_DRAGONAIR] = gMonIcon_Dragonair,
    [SPECIES_DRAGONITE] = gMonIcon_Dragonite,
    [SPECIES_MEWTWO] = gMonIcon_Mewtwo,
    [SPECIES_MEW] = gMonIcon_Mew,
    [SPECIES_CHIKORITA] = gMonIcon_QuestionMark,
    [SPECIES_BAYLEEF] = gMonIcon_QuestionMark,
    [SPECIES_MEGANIUM] = gMonIcon_QuestionMark,
    [SPECIES_CYNDAQUIL] = gMonIcon_QuestionMark,
    [SPECIES_QUILAVA] = gMonIcon_QuestionMark,
    [SPECIES_TYPHLOSION] = gMonIcon_QuestionMark,
    [SPECIES_TOTODILE] = gMonIcon_QuestionMark,
    [SPECIES_CROCONAW] = gMonIcon_QuestionMark,
    [SPECIES_FERALIGATR] = gMonIcon_QuestionMark,
    [SPECIES_SENTRET] = gMonIcon_QuestionMark,
    [SPECIES_FURRET] = gMonIcon_QuestionMark,
    [SPECIES_HOOTHOOT] = gMonIcon_QuestionMark,
    [SPECIES_NOCTOWL] = gMonIcon_QuestionMark,
    [SPECIES_LEDYBA] = gMonIcon_QuestionMark,
    [SPECIES_LEDIAN] = gMonIcon_QuestionMark,
    [SPECIES_SPINARAK] = gMonIcon_QuestionMark,
    [SPECIES_ARIADOS] = gMonIcon_QuestionMark,
    [SPECIES_CROBAT] = gMonIcon_QuestionMark,
    [SPECIES_CHINCHOU] = gMonIcon_QuestionMark,
    [SPECIES_LANTURN] = gMonIcon_QuestionMark,
    [SPECIES_PICHU] = gMonIcon_QuestionMark,
    [SPECIES_CLEFFA] = gMonIcon_QuestionMark,
    [SPECIES_IGGLYBUFF] = gMonIcon_QuestionMark,
    [SPECIES_TOGEPI] = gMonIcon_QuestionMark,
    [SPECIES_TOGETIC] = gMonIcon_QuestionMark,
    [SPECIES_NATU] = gMonIcon_QuestionMark,
    [SPECIES_XATU] = gMonIcon_QuestionMark,
    [SPECIES_MAREEP] = gMonIcon_QuestionMark,
    [SPECIES_FLAAFFY] = gMonIcon_QuestionMark,
    [SPECIES_AMPHAROS] = gMonIcon_QuestionMark,
    [SPECIES_BELLOSSOM] = gMonIcon_QuestionMark,
    [SPECIES_MARILL] = gMonIcon_QuestionMark,
    [SPECIES_AZUMARILL] = gMonIcon_QuestionMark,
    [SPECIES_SUDOWOODO] = gMonIcon_QuestionMark,
    [SPECIES_POLITOED] = gMonIcon_QuestionMark,
    [SPECIES_HOPPIP] = gMonIcon_QuestionMark,
    [SPECIES_SKIPLOOM] = gMonIcon_QuestionMark,
    [SPECIES_JUMPLUFF] = gMonIcon_QuestionMark,
    [SPECIES_AIPOM] = gMonIcon_QuestionMark,
    [SPECIES_SUNKERN] = gMonIcon_QuestionMark,
    [SPECIES_SUNFLORA] = gMonIcon_QuestionMark,
    [SPECIES_YANMA] = gMonIcon_QuestionMark,
    [SPECIES_WOOPER] = gMonIcon_QuestionMark,
    [SPECIES_QUAGSIRE] = gMonIcon_QuestionMark,
    [SPECIES_ESPEON] = gMonIcon_QuestionMark,
    [SPECIES_UMBREON] = gMonIcon_QuestionMark,
    [SPECIES_MURKROW] = gMonIcon_QuestionMark,
    [SPECIES_SLOWKING] = gMonIcon_QuestionMark,
    [SPECIES_MISDREAVUS] = gMonIcon_QuestionMark,
    [SPECIES_UNOWN] = gMonIcon_QuestionMark,
    [SPECIES_WOBBUFFET] = gMonIcon_QuestionMark,
    [SPECIES_GIRAFARIG] = gMonIcon_QuestionMark,
    [SPECIES_PINECO] = gMonIcon_QuestionMark,
    [SPECIES_FORRETRESS] = gMonIcon_QuestionMark,
    [SPECIES_DUNSPARCE] = gMonIcon_QuestionMark,
    [SPECIES_GLIGAR] = gMonIcon_QuestionMark,
    [SPECIES_STEELIX] = gMonIcon_QuestionMark,
    [SPECIES_SNUBBULL] = gMonIcon_QuestionMark,
    [SPECIES_GRANBULL] = gMonIcon_QuestionMark,
    [SPECIES_QWILFISH] = gMonIcon_QuestionMark,
    [SPECIES_SCIZOR] = gMonIcon_QuestionMark,
    [SPECIES_SHUCKLE] = gMonIcon_QuestionMark,
    [SPECIES_HERACROSS] = gMonIcon_QuestionMark,
    [SPECIES_SNEASEL] = gMonIcon_QuestionMark,
    [SPECIES_TEDDIURSA] = gMonIcon_QuestionMark,
    [SPECIES_URSARING] = gMonIcon_QuestionMark,
    [SPECIES_SLUGMA] = gMonIcon_QuestionMark,
    [SPECIES_MAGCARGO] = gMonIcon_QuestionMark,
    [SPECIES_SWINUB] = gMonIcon_QuestionMark,
    [SPECIES_PILOSWINE] = gMonIcon_QuestionMark,
    [SPECIES_CORSOLA] = gMonIcon_QuestionMark,
    [SPECIES_REMORAID] = gMonIcon_QuestionMark,
    [SPECIES_OCTILLERY] = gMonIcon_QuestionMark,
    [SPECIES_DELIBIRD] = gMonIcon_QuestionMark,
    [SPECIES_MANTINE] = gMonIcon_QuestionMark,
    [SPECIES_SKARMORY] = gMonIcon_QuestionMark,
    [SPECIES_HOUNDOUR] = gMonIcon_QuestionMark,
    [SPECIES_HOUNDOOM] = gMonIcon_QuestionMark,
    [SPECIES_KINGDRA] = gMonIcon_QuestionMark,
    [SPECIES_PHANPY] = gMonIcon_QuestionMark,
    [SPECIES_DONPHAN] = gMonIcon_QuestionMark,
    [SPECIES_PORYGON2] = gMonIcon_QuestionMark,
    [SPECIES_STANTLER] = gMonIcon_QuestionMark,
    [SPECIES_SMEARGLE] = gMonIcon_QuestionMark,
    [SPECIES_TYROGUE] = gMonIcon_QuestionMark,
    [SPECIES_HITMONTOP] = gMonIcon_QuestionMark,
    [SPECIES_SMOOCHUM] = gMonIcon_QuestionMark,
    [SPECIES_ELEKID] = gMonIcon_QuestionMark,
    [SPECIES_MAGBY] = gMonIcon_QuestionMark,
    [SPECIES_MILTANK] = gMonIcon_QuestionMark,
    [SPECIES_BLISSEY] = gMonIcon_QuestionMark,
    [SPECIES_RAIKOU] = gMonIcon_QuestionMark,
    [SPECIES_ENTEI] = gMonIcon_QuestionMark,
    [SPECIES_SUICUNE] = gMonIcon_QuestionMark,
    [SPECIES_LARVITAR] = gMonIcon_QuestionMark,
    [SPECIES_PUPITAR] = gMonIcon_QuestionMark,
    [SPECIES_TYRANITAR] = gMonIcon_QuestionMark,
    [SPECIES_LUGIA] = gMonIcon_QuestionMark,
    [SPECIES_HO_OH] = gMonIcon_QuestionMark,
    [SPECIES_CELEBI] = gMonIcon_QuestionMark,
    [SPECIES_OLD_UNOWN_B] = gMonIcon_QuestionMark,
    [SPECIES_OLD_UNOWN_C] = gMonIcon_QuestionMark,
    [SPECIES_OLD_UNOWN_D] = gMonIcon_QuestionMark,
    [SPECIES_OLD_UNOWN_E] = gMonIcon_QuestionMark,
    [SPECIES_OLD_UNOWN_F] = gMonIcon_QuestionMark,
    [SPECIES_OLD_UNOWN_G] = gMonIcon_QuestionMark,
    [SPECIES_OLD_UNOWN_H] = gMonIcon_QuestionMark,
    [SPECIES_OLD_UNOWN_I] = gMonIcon_QuestionMark,
    [SPECIES_OLD_UNOWN_J] = gMonIcon_QuestionMark,
    [SPECIES_OLD_UNOWN_K] = gMonIcon_QuestionMark,
    [SPECIES_OLD_UNOWN_L] = gMonIcon_QuestionMark,
    [SPECIES_OLD_UNOWN_M] = gMonIcon_QuestionMark,
    [SPECIES_OLD_UNOWN_N] = gMonIcon_QuestionMark,
    [SPECIES_OLD_UNOWN_O] = gMonIcon_QuestionMark,
    [SPECIES_OLD_UNOWN_P] = gMonIcon_QuestionMark,
    [SPECIES_OLD_UNOWN_Q] = gMonIcon_QuestionMark,
    [SPECIES_OLD_UNOWN_R] = gMonIcon_QuestionMark,
    [SPECIES_OLD_UNOWN_S] = gMonIcon_QuestionMark,
    [SPECIES_OLD_UNOWN_T] = gMonIcon_QuestionMark,
    [SPECIES_OLD_UNOWN_U] = gMonIcon_QuestionMark,
    [SPECIES_OLD_UNOWN_V] = gMonIcon_QuestionMark,
    [SPECIES_OLD_UNOWN_W] = gMonIcon_QuestionMark,
    [SPECIES_OLD_UNOWN_X] = gMonIcon_QuestionMark,
    [SPECIES_OLD_UNOWN_Y] = gMonIcon_QuestionMark,
    [SPECIES_OLD_UNOWN_Z] = gMonIcon_QuestionMark,
    [SPECIES_TREECKO] = gMonIcon_QuestionMark,
    [SPECIES_GROVYLE] = gMonIcon_QuestionMark,
    [SPECIES_SCEPTILE] = gMonIcon_QuestionMark,
    [SPECIES_TORCHIC] = gMonIcon_QuestionMark,
    [SPECIES_COMBUSKEN] = gMonIcon_QuestionMark,
    [SPECIES_BLAZIKEN] = gMonIcon_QuestionMark,
    [SPECIES_MUDKIP] = gMonIcon_QuestionMark,
    [SPECIES_MARSHTOMP] = gMonIcon_QuestionMark,
    [SPECIES_SWAMPERT] = gMonIcon_QuestionMark,
    [SPECIES_POOCHYENA] = gMonIcon_QuestionMark,
    [SPECIES_MIGHTYENA] = gMonIcon_QuestionMark,
    [SPECIES_ZIGZAGOON] = gMonIcon_QuestionMark,
    [SPECIES_LINOONE] = gMonIcon_QuestionMark,
    [SPECIES_WURMPLE] = gMonIcon_QuestionMark,
    [SPECIES_SILCOON] = gMonIcon_QuestionMark,
    [SPECIES_BEAUTIFLY] = gMonIcon_QuestionMark,
    [SPECIES_CASCOON] = gMonIcon_QuestionMark,
    [SPECIES_DUSTOX] = gMonIcon_QuestionMark,
    [SPECIES_LOTAD] = gMonIcon_QuestionMark,
    [SPECIES_LOMBRE] = gMonIcon_QuestionMark,
    [SPECIES_LUDICOLO] = gMonIcon_QuestionMark,
    [SPECIES_SEEDOT] = gMonIcon_QuestionMark,
    [SPECIES_NUZLEAF] = gMonIcon_QuestionMark,
    [SPECIES_SHIFTRY] = gMonIcon_QuestionMark,
    [SPECIES_NINCADA] = gMonIcon_QuestionMark,
    [SPECIES_NINJASK] = gMonIcon_QuestionMark,
    [SPECIES_SHEDINJA] = gMonIcon_QuestionMark,
    [SPECIES_TAILLOW] = gMonIcon_QuestionMark,
    [SPECIES_SWELLOW] = gMonIcon_QuestionMark,
    [SPECIES_SHROOMISH] = gMonIcon_QuestionMark,
    [SPECIES_BRELOOM] = gMonIcon_QuestionMark,
    [SPECIES_SPINDA] = gMonIcon_QuestionMark,
    [SPECIES_WINGULL] = gMonIcon_QuestionMark,
    [SPECIES_PELIPPER] = gMonIcon_QuestionMark,
    [SPECIES_SURSKIT] = gMonIcon_QuestionMark,
    [SPECIES_MASQUERAIN] = gMonIcon_QuestionMark,
    [SPECIES_WAILMER] = gMonIcon_QuestionMark,
    [SPECIES_WAILORD] = gMonIcon_QuestionMark,
    [SPECIES_SKITTY] = gMonIcon_QuestionMark,
    [SPECIES_DELCATTY] = gMonIcon_QuestionMark,
    [SPECIES_KECLEON] = gMonIcon_QuestionMark,
    [SPECIES_BALTOY] = gMonIcon_QuestionMark,
    [SPECIES_CLAYDOL] = gMonIcon_QuestionMark,
    [SPECIES_NOSEPASS] = gMonIcon_QuestionMark,
    [SPECIES_TORKOAL] = gMonIcon_QuestionMark,
    [SPECIES_SABLEYE] = gMonIcon_QuestionMark,
    [SPECIES_BARBOACH] = gMonIcon_QuestionMark,
    [SPECIES_WHISCASH] = gMonIcon_QuestionMark,
    [SPECIES_LUVDISC] = gMonIcon_QuestionMark,
    [SPECIES_CORPHISH] = gMonIcon_QuestionMark,
    [SPECIES_CRAWDAUNT] = gMonIcon_QuestionMark,
    [SPECIES_FEEBAS] = gMonIcon_QuestionMark,
    [SPECIES_MILOTIC] = gMonIcon_QuestionMark,
    [SPECIES_CARVANHA] = gMonIcon_QuestionMark,
    [SPECIES_SHARPEDO] = gMonIcon_QuestionMark,
    [SPECIES_TRAPINCH] = gMonIcon_QuestionMark,
    [SPECIES_VIBRAVA] = gMonIcon_QuestionMark,
    [SPECIES_FLYGON] = gMonIcon_QuestionMark,
    [SPECIES_MAKUHITA] = gMonIcon_QuestionMark,
    [SPECIES_HARIYAMA] = gMonIcon_QuestionMark,
    [SPECIES_ELECTRIKE] = gMonIcon_QuestionMark,
    [SPECIES_MANECTRIC] = gMonIcon_QuestionMark,
    [SPECIES_NUMEL] = gMonIcon_QuestionMark,
    [SPECIES_CAMERUPT] = gMonIcon_QuestionMark,
    [SPECIES_SPHEAL] = gMonIcon_QuestionMark,
    [SPECIES_SEALEO] = gMonIcon_QuestionMark,
    [SPECIES_WALREIN] = gMonIcon_QuestionMark,
    [SPECIES_CACNEA] = gMonIcon_QuestionMark,
    [SPECIES_CACTURNE] = gMonIcon_QuestionMark,
    [SPECIES_SNORUNT] = gMonIcon_QuestionMark,
    [SPECIES_GLALIE] = gMonIcon_QuestionMark,
    [SPECIES_LUNATONE] = gMonIcon_QuestionMark,
    [SPECIES_SOLROCK] = gMonIcon_QuestionMark,
    [SPECIES_AZURILL] = gMonIcon_QuestionMark,
    [SPECIES_SPOINK] = gMonIcon_QuestionMark,
    [SPECIES_GRUMPIG] = gMonIcon_QuestionMark,
    [SPECIES_PLUSLE] = gMonIcon_QuestionMark,
    [SPECIES_MINUN] = gMonIcon_QuestionMark,
    [SPECIES_MAWILE] = gMonIcon_QuestionMark,
    [SPECIES_MEDITITE] = gMonIcon_QuestionMark,
    [SPECIES_MEDICHAM] = gMonIcon_QuestionMark,
    [SPECIES_SWABLU] = gMonIcon_QuestionMark,
    [SPECIES_ALTARIA] = gMonIcon_QuestionMark,
    [SPECIES_WYNAUT] = gMonIcon_QuestionMark,
    [SPECIES_DUSKULL] = gMonIcon_QuestionMark,
    [SPECIES_DUSCLOPS] = gMonIcon_QuestionMark,
    [SPECIES_ROSELIA] = gMonIcon_QuestionMark,
    [SPECIES_SLAKOTH] = gMonIcon_QuestionMark,
    [SPECIES_VIGOROTH] = gMonIcon_QuestionMark,
    [SPECIES_SLAKING] = gMonIcon_QuestionMark,
    [SPECIES_GULPIN] = gMonIcon_QuestionMark,
    [SPECIES_SWALOT] = gMonIcon_QuestionMark,
    [SPECIES_TROPIUS] = gMonIcon_QuestionMark,
    [SPECIES_WHISMUR] = gMonIcon_QuestionMark,
    [SPECIES_LOUDRED] = gMonIcon_QuestionMark,
    [SPECIES_EXPLOUD] = gMonIcon_QuestionMark,
    [SPECIES_CLAMPERL] = gMonIcon_QuestionMark,
    [SPECIES_HUNTAIL] = gMonIcon_QuestionMark,
    [SPECIES_GOREBYSS] = gMonIcon_QuestionMark,
    [SPECIES_ABSOL] = gMonIcon_QuestionMark,
    [SPECIES_SHUPPET] = gMonIcon_QuestionMark,
    [SPECIES_BANETTE] = gMonIcon_QuestionMark,
    [SPECIES_SEVIPER] = gMonIcon_QuestionMark,
    [SPECIES_ZANGOOSE] = gMonIcon_QuestionMark,
    [SPECIES_RELICANTH] = gMonIcon_QuestionMark,
    [SPECIES_ARON] = gMonIcon_QuestionMark,
    [SPECIES_LAIRON] = gMonIcon_QuestionMark,
    [SPECIES_AGGRON] = gMonIcon_QuestionMark,
    [SPECIES_CASTFORM] = gMonIcon_QuestionMark,
    [SPECIES_VOLBEAT] = gMonIcon_QuestionMark,
    [SPECIES_ILLUMISE] = gMonIcon_QuestionMark,
    [SPECIES_LILEEP] = gMonIcon_QuestionMark,
    [SPECIES_CRADILY] = gMonIcon_QuestionMark,
    [SPECIES_ANORITH] = gMonIcon_QuestionMark,
    [SPECIES_ARMALDO] = gMonIcon_QuestionMark,
    [SPECIES_RALTS] = gMonIcon_QuestionMark,
    [SPECIES_KIRLIA] = gMonIcon_QuestionMark,
    [SPECIES_GARDEVOIR] = gMonIcon_QuestionMark,
    [SPECIES_BAGON] = gMonIcon_QuestionMark,
    [SPECIES_SHELGON] = gMonIcon_QuestionMark,
    [SPECIES_SALAMENCE] = gMonIcon_QuestionMark,
    [SPECIES_BELDUM] = gMonIcon_QuestionMark,
    [SPECIES_METANG] = gMonIcon_QuestionMark,
    [SPECIES_METAGROSS] = gMonIcon_QuestionMark,
    [SPECIES_REGIROCK] = gMonIcon_QuestionMark,
    [SPECIES_REGICE] = gMonIcon_QuestionMark,
    [SPECIES_REGISTEEL] = gMonIcon_QuestionMark,
    [SPECIES_KYOGRE] = gMonIcon_QuestionMark,
    [SPECIES_GROUDON] = gMonIcon_QuestionMark,
    [SPECIES_RAYQUAZA] = gMonIcon_QuestionMark,
    [SPECIES_LATIAS] = gMonIcon_QuestionMark,
    [SPECIES_LATIOS] = gMonIcon_QuestionMark,
    [SPECIES_JIRACHI] = gMonIcon_QuestionMark,
    [SPECIES_DEOXYS] = gMonIcon_QuestionMark,
    [SPECIES_CHIMECHO] = gMonIcon_QuestionMark,
    [SPECIES_EGG] = gMonIcon_Egg,
    [SPECIES_UNOWN_B] = gMonIcon_QuestionMark,
    [SPECIES_UNOWN_C] = gMonIcon_QuestionMark,
    [SPECIES_UNOWN_D] = gMonIcon_QuestionMark,
    [SPECIES_UNOWN_E] = gMonIcon_QuestionMark,
    [SPECIES_UNOWN_F] = gMonIcon_QuestionMark,
    [SPECIES_UNOWN_G] = gMonIcon_QuestionMark,
    [SPECIES_UNOWN_H] = gMonIcon_QuestionMark,
    [SPECIES_UNOWN_I] = gMonIcon_QuestionMark,
    [SPECIES_UNOWN_J] = gMonIcon_QuestionMark,
    [SPECIES_UNOWN_K] = gMonIcon_QuestionMark,
    [SPECIES_UNOWN_L] = gMonIcon_QuestionMark,
    [SPECIES_UNOWN_M] = gMonIcon_QuestionMark,
    [SPECIES_UNOWN_N] = gMonIcon_QuestionMark,
    [SPECIES_UNOWN_O] = gMonIcon_QuestionMark,
    [SPECIES_UNOWN_P] = gMonIcon_QuestionMark,
    [SPECIES_UNOWN_Q] = gMonIcon_QuestionMark,
    [SPECIES_UNOWN_R] = gMonIcon_QuestionMark,
    [SPECIES_UNOWN_S] = gMonIcon_QuestionMark,
    [SPECIES_UNOWN_T] = gMonIcon_QuestionMark,
    [SPECIES_UNOWN_U] = gMonIcon_QuestionMark,
    [SPECIES_UNOWN_V] = gMonIcon_QuestionMark,
    [SPECIES_UNOWN_W] = gMonIcon_QuestionMark,
    [SPECIES_UNOWN_X] = gMonIcon_QuestionMark,
    [SPECIES_UNOWN_Y] = gMonIcon_QuestionMark,
    [SPECIES_UNOWN_Z] = gMonIcon_QuestionMark,
    [SPECIES_UNOWN_EMARK] = gMonIcon_QuestionMark,
    [SPECIES_UNOWN_QMARK] = gMonIcon_QuestionMark,
};

const u8 gMonIconPaletteIndices[] =
{
    [SPECIES_NONE] = 0,
    [SPECIES_BULBASAUR] = 1,
    [SPECIES_IVYSAUR] = 1,
    [SPECIES_VENUSAUR] = 1,
    [SPECIES_CHARMANDER] = 0,
    [SPECIES_CHARMELEON] = 0,
    [SPECIES_CHARIZARD] = 0,
    [SPECIES_SQUIRTLE] = 0,
    [SPECIES_WARTORTLE] = 2,
    [SPECIES_BLASTOISE] = 2,
    [SPECIES_CATERPIE] = 1,
    [SPECIES_METAPOD] = 1,
    [SPECIES_BUTTERFREE] = 0,
    [SPECIES_WEEDLE] = 1,
    [SPECIES_KAKUNA] = 2,
    [SPECIES_BEEDRILL] = 2,
    [SPECIES_PIDGEY] = 0,
    [SPECIES_PIDGEOTTO] = 0,
    [SPECIES_PIDGEOT] = 0,
    [SPECIES_RATTATA] = 2,
    [SPECIES_RATICATE] = 1,
    [SPECIES_SPEAROW] = 0,
    [SPECIES_FEAROW] = 0,
    [SPECIES_EKANS] = 2,
    [SPECIES_ARBOK] = 2,
    [SPECIES_PIKACHU] = 2,
    [SPECIES_RAICHU] = 0,
    [SPECIES_SANDSHREW] = 2,
    [SPECIES_SANDSLASH] = 2,
    [SPECIES_NIDORAN_F] = 2,
    [SPECIES_NIDORINA] = 2,
    [SPECIES_NIDOQUEEN] = 2,
    [SPECIES_NIDORAN_M] = 2,
    [SPECIES_NIDORINO] = 2,
    [SPECIES_NIDOKING] = 2,
    [SPECIES_CLEFAIRY] = 0,
    [SPECIES_CLEFABLE] = 0,
    [SPECIES_VULPIX] = 2,
    [SPECIES_NINETALES] = 1,
    [SPECIES_JIGGLYPUFF] = 0,
    [SPECIES_WIGGLYTUFF] = 0,
    [SPECIES_ZUBAT] = 2,
    [SPECIES_GOLBAT] = 2,
    [SPECIES_ODDISH] = 1,
    [SPECIES_GLOOM] = 0,
    [SPECIES_VILEPLUME] = 0,
    [SPECIES_PARAS] = 0,
    [SPECIES_PARASECT] = 0,
    [SPECIES_VENONAT] = 0,
    [SPECIES_VENOMOTH] = 2,
    [SPECIES_DIGLETT] = 2,
    [SPECIES_DUGTRIO] = 2,
    [SPECIES_MEOWTH] = 1,
    [SPECIES_PERSIAN] = 1,
    [SPECIES_PSYDUCK] = 1,
    [SPECIES_GOLDUCK] = 2,
    [SPECIES_MANKEY] = 1,
    [SPECIES_PRIMEAPE] = 2,
    [SPECIES_GROWLITHE] = 0,
    [SPECIES_ARCANINE] = 0,
    [SPECIES_POLIWAG] = 0,
    [SPECIES_POLIWHIRL] = 0,
    [SPECIES_POLIWRATH] = 0,
    [SPECIES_ABRA] = 2,
    [SPECIES_KADABRA] = 2,
    [SPECIES_ALAKAZAM] = 2,
    [SPECIES_MACHOP] = 0,
    [SPECIES_MACHOKE] = 2,
    [SPECIES_MACHAMP] = 0,
    [SPECIES_BELLSPROUT] = 1,
    [SPECIES_WEEPINBELL] = 1,
    [SPECIES_VICTREEBEL] = 1,
    [SPECIES_TENTACOOL] = 2,
    [SPECIES_TENTACRUEL] = 2,
    [SPECIES_GEODUDE] = 1,
    [SPECIES_GRAVELER] = 1,
    [SPECIES_GOLEM] = 1,
    [SPECIES_PONYTA] = 0,
    [SPECIES_RAPIDASH] = 0,
    [SPECIES_SLOWPOKE] = 0,
    [SPECIES_SLOWBRO] = 0,
    [SPECIES_MAGNEMITE] = 0,
    [SPECIES_MAGNETON] = 0,
    [SPECIES_FARFETCHD] = 1,
    [SPECIES_DODUO] = 2,
    [SPECIES_DODRIO] = 2,
    [SPECIES_SEEL] = 2,
    [SPECIES_DEWGONG] = 2,
    [SPECIES_GRIMER] = 2,
    [SPECIES_MUK] = 2,
    [SPECIES_SHELLDER] = 2,
    [SPECIES_CLOYSTER] = 2,
    [SPECIES_GASTLY] = 2,
    [SPECIES_HAUNTER] = 2,
    [SPECIES_GENGAR] = 2,
    [SPECIES_ONIX] = 2,
    [SPECIES_DROWZEE] = 2,
    [SPECIES_HYPNO] = 1,
    [SPECIES_KRABBY] = 2,
    [SPECIES_KINGLER] = 2,
    [SPECIES_VOLTORB] = 0,
    [SPECIES_ELECTRODE] = 0,
    [SPECIES_EXEGGCUTE] = 0,
    [SPECIES_EXEGGUTOR] = 1,
    [SPECIES_CUBONE] = 1,
    [SPECIES_MAROWAK] = 1,
    [SPECIES_HITMONLEE] = 2,
    [SPECIES_HITMONCHAN] = 2,
    [SPECIES_LICKITUNG] = 1,
    [SPECIES_KOFFING] = 2,
    [SPECIES_WEEZING] = 2,
    [SPECIES_RHYHORN] = 1,
    [SPECIES_RHYDON] = 1,
    [SPECIES_CHANSEY] = 0,
    [SPECIES_TANGELA] = 0,
    [SPECIES_KANGASKHAN] = 1,
    [SPECIES_HORSEA] = 0,
    [SPECIES_SEADRA] = 0,
    [SPECIES_GOLDEEN] = 0,
    [SPECIES_SEAKING] = 0,
    [SPECIES_STARYU] = 2,
    [SPECIES_STARMIE] = 2,
    [SPECIES_MR_MIME] = 0,
    [SPECIES_SCYTHER] = 1,
    [SPECIES_JYNX] = 2,
    [SPECIES_ELECTABUZZ] = 1,
    [SPECIES_MAGMAR] = 0,
    [SPECIES_PINSIR] = 2,
    [SPECIES_TAUROS] = 2,
    [SPECIES_MAGIKARP] = 0,
    [SPECIES_GYARADOS] = 0,
    [SPECIES_LAPRAS] = 2,
    [SPECIES_DITTO] = 2,
    [SPECIES_EEVEE] = 2,
    [SPECIES_VAPOREON] = 0,
    [SPECIES_JOLTEON] = 0,
    [SPECIES_FLAREON] = 0,
    [SPECIES_PORYGON] = 0,
    [SPECIES_OMANYTE] = 0,
    [SPECIES_OMASTAR] = 0,
    [SPECIES_KABUTO] = 2,
    [SPECIES_KABUTOPS] = 2,
    [SPECIES_AERODACTYL] = 0,
    [SPECIES_SNORLAX] = 1,
    [SPECIES_ARTICUNO] = 0,
    [SPECIES_ZAPDOS] = 0,
    [SPECIES_MOLTRES] = 0,
    [SPECIES_DRATINI] = 0,
    [SPECIES_DRAGONAIR] = 0,
    [SPECIES_DRAGONITE] = 2,
    [SPECIES_MEWTWO] = 2,
    [SPECIES_MEW] = 0,
    [SPECIES_CHIKORITA] = 0,
    [SPECIES_BAYLEEF] = 0,
    [SPECIES_MEGANIUM] = 0,
    [SPECIES_CYNDAQUIL] = 0,
    [SPECIES_QUILAVA] = 0,
    [SPECIES_TYPHLOSION] = 0,
    [SPECIES_TOTODILE] = 0,
    [SPECIES_CROCONAW] = 0,
    [SPECIES_FERALIGATR] = 0,
    [SPECIES_SENTRET] = 0,
    [SPECIES_FURRET] = 0,
    [SPECIES_HOOTHOOT] = 0,
    [SPECIES_NOCTOWL] = 0,
    [SPECIES_LEDYBA] = 0,
    [SPECIES_LEDIAN] = 0,
    [SPECIES_SPINARAK] = 0,
    [SPECIES_ARIADOS] = 0,
    [SPECIES_CROBAT] = 0,
    [SPECIES_CHINCHOU] = 0,
    [SPECIES_LANTURN] = 0,
    [SPECIES_PICHU] = 0,
    [SPECIES_CLEFFA] = 0,
    [SPECIES_IGGLYBUFF] = 0,
    [SPECIES_TOGEPI] = 0,
    [SPECIES_TOGETIC] = 0,
    [SPECIES_NATU] = 0,
    [SPECIES_XATU] = 0,
    [SPECIES_MAREEP] = 0,
    [SPECIES_FLAAFFY] = 0,
    [SPECIES_AMPHAROS] = 0,
    [SPECIES_BELLOSSOM] = 0,
    [SPECIES_MARILL] = 0,
    [SPECIES_AZUMARILL] = 0,
    [SPECIES_SUDOWOODO] = 0,
    [SPECIES_POLITOED] = 0,
    [SPECIES_HOPPIP] = 0,
    [SPECIES_SKIPLOOM] = 0,
    [SPECIES_JUMPLUFF] = 0,
    [SPECIES_AIPOM] = 0,
    [SPECIES_SUNKERN] = 0,
    [SPECIES_SUNFLORA] = 0,
    [SPECIES_YANMA] = 0,
    [SPECIES_WOOPER] = 0,
    [SPECIES_QUAGSIRE] = 0,
    [SPECIES_ESPEON] = 0,
    [SPECIES_UMBREON] = 0,
    [SPECIES_MURKROW] = 0,
    [SPECIES_SLOWKING] = 0,
    [SPECIES_MISDREAVUS] = 0,
    [SPECIES_UNOWN] = 0,
    [SPECIES_WOBBUFFET] = 0,
    [SPECIES_GIRAFARIG] = 0,
    [SPECIES_PINECO] = 0,
    [SPECIES_FORRETRESS] = 0,
    [SPECIES_DUNSPARCE] = 0,
    [SPECIES_GLIGAR] = 0,
    [SPECIES_STEELIX] = 0,
    [SPECIES_SNUBBULL] = 0,
    [SPECIES_GRANBULL] = 0,
    [SPECIES_QWILFISH] = 0,
    [SPECIES_SCIZOR] = 0,
    [SPECIES_SHUCKLE] = 0,
    [SPECIES_HERACROSS] = 0,
    [SPECIES_SNEASEL] = 0,
    [SPECIES_TEDDIURSA] = 0,
    [SPECIES_URSARING] = 0,
    [SPECIES_SLUGMA] = 0,
    [SPECIES_MAGCARGO] = 0,
    [SPECIES_SWINUB] = 0,
    [SPECIES_PILOSWINE] = 0,
    [SPECIES_CORSOLA] = 0,
    [SPECIES_REMORAID] = 0,
    [SPECIES_OCTILLERY] = 0,
    [SPECIES_DELIBIRD] = 0,
    [SPECIES_MANTINE] = 0,
    [SPECIES_SKARMORY] = 0,
    [SPECIES_HOUNDOUR] = 0,
    [SPECIES_HOUNDOOM] = 0,
    [SPECIES_KINGDRA] = 0,
    [SPECIES_PHANPY] = 0,
    [SPECIES_DONPHAN] = 0,
    [SPECIES_PORYGON2] = 0,
    [SPECIES_STANTLER] = 0,
    [SPECIES_SMEARGLE] = 0,
    [SPECIES_TYROGUE] = 0,
    [SPECIES_HITMONTOP] = 0,
    [SPECIES_SMOOCHUM] = 0,
    [SPECIES_ELEKID] = 0,
    [SPECIES_MAGBY] = 0,
    [SPECIES_MILTANK] = 0,
    [SPECIES_BLISSEY] = 0,
    [SPECIES_RAIKOU] = 0,
    [SPECIES_ENTEI] = 0,
    [SPECIES_SUICUNE] = 0,
    [SPECIES_LARVITAR] = 0,
    [SPECIES_PUPITAR] = 0,
    [SPECIES_TYRANITAR] = 0,
    [SPECIES_LUGIA] = 0,
    [SPECIES_HO_OH] = 0,
    [SPECIES_CELEBI] = 0,
    [SPECIES_OLD_UNOWN_B] = 0,
    [SPECIES_OLD_UNOWN_C] = 0,
    [SPECIES_OLD_UNOWN_D] = 0,
    [SPECIES_OLD_UNOWN_E] = 0,
    [SPECIES_OLD_UNOWN_F] = 0,
    [SPECIES_OLD_UNOWN_G] = 0,
    [SPECIES_OLD_UNOWN_H] = 0,
    [SPECIES_OLD_UNOWN_I] = 0,
    [SPECIES_OLD_UNOWN_J] = 0,
    [SPECIES_OLD_UNOWN_K] = 0,
    [SPECIES_OLD_UNOWN_L] = 0,
    [SPECIES_OLD_UNOWN_M] = 0,
    [SPECIES_OLD_UNOWN_N] = 0,
    [SPECIES_OLD_UNOWN_O] = 0,
    [SPECIES_OLD_UNOWN_P] = 0,
    [SPECIES_OLD_UNOWN_Q] = 0,
    [SPECIES_OLD_UNOWN_R] = 0,
    [SPECIES_OLD_UNOWN_S] = 0,
    [SPECIES_OLD_UNOWN_T] = 0,
    [SPECIES_OLD_UNOWN_U] = 0,
    [SPECIES_OLD_UNOWN_V] = 0,
    [SPECIES_OLD_UNOWN_W] = 0,
    [SPECIES_OLD_UNOWN_X] = 0,
    [SPECIES_OLD_UNOWN_Y] = 0,
    [SPECIES_OLD_UNOWN_Z] = 0,
    [SPECIES_TREECKO] = 0,
    [SPECIES_GROVYLE] = 0,
    [SPECIES_SCEPTILE] = 0,
    [SPECIES_TORCHIC] = 0,
    [SPECIES_COMBUSKEN] = 0,
    [SPECIES_BLAZIKEN] = 0,
    [SPECIES_MUDKIP] = 0,
    [SPECIES_MARSHTOMP] = 0,
    [SPECIES_SWAMPERT] = 0,
    [SPECIES_POOCHYENA] = 0,
    [SPECIES_MIGHTYENA] = 0,
    [SPECIES_ZIGZAGOON] = 0,
    [SPECIES_LINOONE] = 0,
    [SPECIES_WURMPLE] = 0,
    [SPECIES_SILCOON] = 0,
    [SPECIES_BEAUTIFLY] = 0,
    [SPECIES_CASCOON] = 0,
    [SPECIES_DUSTOX] = 0,
    [SPECIES_LOTAD] = 0,
    [SPECIES_LOMBRE] = 0,
    [SPECIES_LUDICOLO] = 0,
    [SPECIES_SEEDOT] = 0,
    [SPECIES_NUZLEAF] = 0,
    [SPECIES_SHIFTRY] = 0,
    [SPECIES_NINCADA] = 0,
    [SPECIES_NINJASK] = 0,
    [SPECIES_SHEDINJA] = 0,
    [SPECIES_TAILLOW] = 0,
    [SPECIES_SWELLOW] = 0,
    [SPECIES_SHROOMISH] = 0,
    [SPECIES_BRELOOM] = 0,
    [SPECIES_SPINDA] = 0,
    [SPECIES_WINGULL] = 0,
    [SPECIES_PELIPPER] = 0,
    [SPECIES_SURSKIT] = 0,
    [SPECIES_MASQUERAIN] = 0,
    [SPECIES_WAILMER] = 0,
    [SPECIES_WAILORD] = 0,
    [SPECIES_SKITTY] = 0,
    [SPECIES_DELCATTY] = 0,
    [SPECIES_KECLEON] = 0,
    [SPECIES_BALTOY] = 0,
    [SPECIES_CLAYDOL] = 0,
    [SPECIES_NOSEPASS] = 0,
    [SPECIES_TORKOAL] = 0,
    [SPECIES_SABLEYE] = 0,
    [SPECIES_BARBOACH] = 0,
    [SPECIES_WHISCASH] = 0,
    [SPECIES_LUVDISC] = 0,
    [SPECIES_CORPHISH] = 0,
    [SPECIES_CRAWDAUNT] = 0,
    [SPECIES_FEEBAS] = 0,
    [SPECIES_MILOTIC] = 0,
    [SPECIES_CARVANHA] = 0,
    [SPECIES_SHARPEDO] = 0,
    [SPECIES_TRAPINCH] = 0,
    [SPECIES_VIBRAVA] = 0,
    [SPECIES_FLYGON] = 0,
    [SPECIES_MAKUHITA] = 0,
    [SPECIES_HARIYAMA] = 0,
    [SPECIES_ELECTRIKE] = 0,
    [SPECIES_MANECTRIC] = 0,
    [SPECIES_NUMEL] = 0,
    [SPECIES_CAMERUPT] = 0,
    [SPECIES_SPHEAL] = 0,
    [SPECIES_SEALEO] = 0,
    [SPECIES_WALREIN] = 0,
    [SPECIES_CACNEA] = 0,
    [SPECIES_CACTURNE] = 0,
    [SPECIES_SNORUNT] = 0,
    [SPECIES_GLALIE] = 0,
    [SPECIES_LUNATONE] = 0,
    [SPECIES_SOLROCK] = 0,
    [SPECIES_AZURILL] = 0,
    [SPECIES_SPOINK] = 0,
    [SPECIES_GRUMPIG] = 0,
    [SPECIES_PLUSLE] = 0,
    [SPECIES_MINUN] = 0,
    [SPECIES_MAWILE] = 0,
    [SPECIES_MEDITITE] = 0,
    [SPECIES_MEDICHAM] = 0,
    [SPECIES_SWABLU] = 0,
    [SPECIES_ALTARIA] = 0,
    [SPECIES_WYNAUT] = 0,
    [SPECIES_DUSKULL] = 0,
    [SPECIES_DUSCLOPS] = 0,
    [SPECIES_ROSELIA] = 0,
    [SPECIES_SLAKOTH] = 0,
    [SPECIES_VIGOROTH] = 0,
    [SPECIES_SLAKING] = 0,
    [SPECIES_GULPIN] = 0,
    [SPECIES_SWALOT] = 0,
    [SPECIES_TROPIUS] = 0,
    [SPECIES_WHISMUR] = 0,
    [SPECIES_LOUDRED] = 0,
    [SPECIES_EXPLOUD] = 0,
    [SPECIES_CLAMPERL] = 0,
    [SPECIES_HUNTAIL] = 0,
    [SPECIES_GOREBYSS] = 0,
    [SPECIES_ABSOL] = 0,
    [SPECIES_SHUPPET] = 0,
    [SPECIES_BANETTE] = 0,
    [SPECIES_SEVIPER] = 0,
    [SPECIES_ZANGOOSE] = 0,
    [SPECIES_RELICANTH] = 0,
    [SPECIES_ARON] = 0,
    [SPECIES_LAIRON] = 0,
    [SPECIES_AGGRON] = 0,
    [SPECIES_CASTFORM] = 0,
    [SPECIES_VOLBEAT] = 0,
    [SPECIES_ILLUMISE] = 0,
    [SPECIES_LILEEP] = 0,
    [SPECIES_CRADILY] = 0,
    [SPECIES_ANORITH] = 0,
    [SPECIES_ARMALDO] = 0,
    [SPECIES_RALTS] = 0,
    [SPECIES_KIRLIA] = 0,
    [SPECIES_GARDEVOIR] = 0,
    [SPECIES_BAGON] = 0,
    [SPECIES_SHELGON] = 0,
    [SPECIES_SALAMENCE] = 0,
    [SPECIES_BELDUM] = 0,
    [SPECIES_METANG] = 0,
    [SPECIES_METAGROSS] = 0,
    [SPECIES_REGIROCK] = 0,
    [SPECIES_REGICE] = 0,
    [SPECIES_REGISTEEL] = 0,
    [SPECIES_KYOGRE] = 0,
    [SPECIES_GROUDON] = 0,
    [SPECIES_RAYQUAZA] = 0,
    [SPECIES_LATIAS] = 0,
    [SPECIES_LATIOS] = 0,
    [SPECIES_JIRACHI] = 0,
    [SPECIES_DEOXYS] = 0,
    [SPECIES_CHIMECHO] = 0,
    [SPECIES_EGG] = 1,
    [SPECIES_UNOWN_B] = 0,
    [SPECIES_UNOWN_C] = 0,
    [SPECIES_UNOWN_D] = 0,
    [SPECIES_UNOWN_E] = 0,
    [SPECIES_UNOWN_F] = 0,
    [SPECIES_UNOWN_G] = 0,
    [SPECIES_UNOWN_H] = 0,
    [SPECIES_UNOWN_I] = 0,
    [SPECIES_UNOWN_J] = 0,
    [SPECIES_UNOWN_K] = 0,
    [SPECIES_UNOWN_L] = 0,
    [SPECIES_UNOWN_M] = 0,
    [SPECIES_UNOWN_N] = 0,
    [SPECIES_UNOWN_O] = 0,
    [SPECIES_UNOWN_P] = 0,
    [SPECIES_UNOWN_Q] = 0,
    [SPECIES_UNOWN_R] = 0,
    [SPECIES_UNOWN_S] = 0,
    [SPECIES_UNOWN_T] = 0,
    [SPECIES_UNOWN_U] = 0,
    [SPECIES_UNOWN_V] = 0,
    [SPECIES_UNOWN_W] = 0,
    [SPECIES_UNOWN_X] = 0,
    [SPECIES_UNOWN_Y] = 0,
    [SPECIES_UNOWN_Z] = 0,
    [SPECIES_UNOWN_EMARK] = 0,
    [SPECIES_UNOWN_QMARK] = 0,
};

const struct SpritePalette gMonIconPaletteTable[] =
{
    { gMonIconPalettes[0], POKE_ICON_BASE_PAL_TAG + 0 },
    { gMonIconPalettes[1], POKE_ICON_BASE_PAL_TAG + 1 },
    { gMonIconPalettes[2], POKE_ICON_BASE_PAL_TAG + 2 },

// There are only 3 actual palettes. The following are unused
// and don't point to valid data.
    { gMonIconPalettes[3], POKE_ICON_BASE_PAL_TAG + 3 },
    { gMonIconPalettes[4], POKE_ICON_BASE_PAL_TAG + 4 },
    { gMonIconPalettes[5], POKE_ICON_BASE_PAL_TAG + 5 },
};

static const struct OamData sMonIconOamData =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x32),
    .x = 0,
    .size = SPRITE_SIZE(32x32),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
};

// fastest to slowest

static const union AnimCmd sAnim_0[] =
{
    ANIMCMD_FRAME(0, 6),
    ANIMCMD_FRAME(1, 6),
    ANIMCMD_JUMP(0),
};

static const union AnimCmd sAnim_1[] =
{
    ANIMCMD_FRAME(0, 8),
    ANIMCMD_FRAME(1, 8),
    ANIMCMD_JUMP(0),
};

static const union AnimCmd sAnim_2[] =
{
    ANIMCMD_FRAME(0, 14),
    ANIMCMD_FRAME(1, 14),
    ANIMCMD_JUMP(0),
};

static const union AnimCmd sAnim_3[] =
{
    ANIMCMD_FRAME(0, 22),
    ANIMCMD_FRAME(1, 22),
    ANIMCMD_JUMP(0),
};

static const union AnimCmd sAnim_4[] =
{
    ANIMCMD_FRAME(0, 29),
    ANIMCMD_FRAME(0, 29), // frame 0 is repeated
    ANIMCMD_JUMP(0),
};

static const union AnimCmd *const sMonIconAnims[] =
{
    sAnim_0,
    sAnim_1,
    sAnim_2,
    sAnim_3,
    sAnim_4,
};

static const union AffineAnimCmd sAffineAnim_0[] =
{
    AFFINEANIMCMD_FRAME(0, 0, 0, 10),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd sAffineAnim_1[] =
{
    AFFINEANIMCMD_FRAME(-2, -2, 0, 122),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd *const sMonIconAffineAnims[] =
{
    sAffineAnim_0,
    sAffineAnim_1,
};

static const u16 sSpriteImageSizes[3][4] =
{
    [ST_OAM_SQUARE] =
    {
        [SPRITE_SIZE(8x8)]   =  8 * 8  / 2,
        [SPRITE_SIZE(16x16)] = 16 * 16 / 2,
        [SPRITE_SIZE(32x32)] = 32 * 32 / 2,
        [SPRITE_SIZE(64x64)] = 64 * 64 / 2,
    },
    [ST_OAM_H_RECTANGLE] =
    {
        [SPRITE_SIZE(16x8)]  = 16 * 8  / 2,
        [SPRITE_SIZE(32x8)]  = 32 * 8  / 2,
        [SPRITE_SIZE(32x16)] = 32 * 16 / 2,
        [SPRITE_SIZE(64x32)] = 64 * 32 / 2,
    },
    [ST_OAM_V_RECTANGLE] =
    {
        [SPRITE_SIZE(8x16)]  =  8 * 16 / 2,
        [SPRITE_SIZE(8x32)]  =  8 * 32 / 2,
        [SPRITE_SIZE(16x32)] = 16 * 32 / 2,
        [SPRITE_SIZE(32x64)] = 32 * 64 / 2,
    },
};

u8 CreateMonIcon(u16 species, void (*callback)(struct Sprite *), s16 x, s16 y, u8 subpriority, u32 personality, bool32 handleDeoxys)
{
    u8 spriteId;
    struct MonIconSpriteTemplate iconTemplate =
    {
        .oam = &sMonIconOamData,
        .image = GetMonIconPtr(species, personality, handleDeoxys),
        .anims = sMonIconAnims,
        .affineAnims = sMonIconAffineAnims,
        .callback = callback,
        .paletteTag = POKE_ICON_BASE_PAL_TAG + gMonIconPaletteIndices[species],
    };

    if (species > NUM_SPECIES)
        iconTemplate.paletteTag = POKE_ICON_BASE_PAL_TAG;

    spriteId = CreateMonIconSprite(&iconTemplate, x, y, subpriority);

    UpdateMonIconFrame(&gSprites[spriteId]);

    return spriteId;
}

u8 CreateMonIconNoPersonality(u16 species, void (*callback)(struct Sprite *), s16 x, s16 y, u8 subpriority, bool32 handleDeoxys)
{
    u8 spriteId;
    struct MonIconSpriteTemplate iconTemplate =
    {
        .oam = &sMonIconOamData,
        .image = NULL,
        .anims = sMonIconAnims,
        .affineAnims = sMonIconAffineAnims,
        .callback = callback,
        .paletteTag = POKE_ICON_BASE_PAL_TAG + gMonIconPaletteIndices[species],
    };

    iconTemplate.image = GetMonIconTiles(species, handleDeoxys);
    spriteId = CreateMonIconSprite(&iconTemplate, x, y, subpriority);

    UpdateMonIconFrame(&gSprites[spriteId]);

    return spriteId;
}

u16 GetIconSpecies(u16 species, u32 personality)
{
    u16 result;

    if (species == SPECIES_UNOWN)
    {
        u16 letter = GetUnownLetterByPersonality(personality);
        if (letter == 0)
            letter = SPECIES_UNOWN;
        else
            letter += (SPECIES_UNOWN_B - 1);
        result = letter;
    }
    else
    {
        if (species > NUM_SPECIES)
            result = INVALID_ICON_SPECIES;
        else
            result = species;
    }

    return result;
}

u16 GetUnownLetterByPersonality(u32 personality)
{
    if (!personality)
        return 0;
    else
        return GET_UNOWN_LETTER(personality);
}

u16 GetIconSpeciesNoPersonality(u16 species)
{
    u16 value;

    if (MailSpeciesToSpecies(species, &value) == SPECIES_UNOWN)
    {
        if (value == 0)
            value += SPECIES_UNOWN;
        else
            value += (SPECIES_UNOWN_B - 1);
        return value;
    }
    else
    {
        if (species > NUM_SPECIES)
            species = INVALID_ICON_SPECIES;
        return GetIconSpecies(species, 0);
    }
}

const u8 *GetMonIconPtr(u16 species, u32 personality, bool32 handleDeoxys)
{
    return GetMonIconTiles(GetIconSpecies(species, personality), handleDeoxys);
}

void FreeAndDestroyMonIconSprite(struct Sprite *sprite)
{
    FreeAndDestroyMonIconSprite_(sprite);
}

void LoadMonIconPalettes(void)
{
    u8 i;
    for (i = 0; i < ARRAY_COUNT(gMonIconPaletteTable); i++)
        LoadSpritePalette(&gMonIconPaletteTable[i]);
}

// unused
void SafeLoadMonIconPalette(u16 species)
{
    u8 palIndex;
    if (species > NUM_SPECIES)
        species = INVALID_ICON_SPECIES;
    palIndex = gMonIconPaletteIndices[species];
    if (IndexOfSpritePaletteTag(gMonIconPaletteTable[palIndex].tag) == 0xFF)
        LoadSpritePalette(&gMonIconPaletteTable[palIndex]);
}

void LoadMonIconPalette(u16 species)
{
    u8 palIndex = gMonIconPaletteIndices[species];
    if (IndexOfSpritePaletteTag(gMonIconPaletteTable[palIndex].tag) == 0xFF)
        LoadSpritePalette(&gMonIconPaletteTable[palIndex]);
}

void FreeMonIconPalettes(void)
{
    u8 i;
    for (i = 0; i < ARRAY_COUNT(gMonIconPaletteTable); i++)
        FreeSpritePaletteByTag(gMonIconPaletteTable[i].tag);
}

// unused
void SafeFreeMonIconPalette(u16 species)
{
    u8 palIndex;
    if (species > NUM_SPECIES)
        species = INVALID_ICON_SPECIES;
    palIndex = gMonIconPaletteIndices[species];
    FreeSpritePaletteByTag(gMonIconPaletteTable[palIndex].tag);
}

void FreeMonIconPalette(u16 species)
{
    u8 palIndex;
    palIndex = gMonIconPaletteIndices[species];
    FreeSpritePaletteByTag(gMonIconPaletteTable[palIndex].tag);
}

void SpriteCB_MonIcon(struct Sprite *sprite)
{
    UpdateMonIconFrame(sprite);
}

const u8 *GetMonIconTiles(u16 species, bool32 handleDeoxys)
{
    const u8 *iconSprite = gMonIconTable[species];
    if (species == SPECIES_DEOXYS && handleDeoxys == TRUE)
    {
        iconSprite = (const u8 *)(0x400 + (u32)iconSprite); // use the specific Deoxys form icon (Speed in this case)
    }
    return iconSprite;
}

void TryLoadAllMonIconPalettesAtOffset(u16 offset)
{
    s32 i;
    if (offset <= BG_PLTT_ID(16 - ARRAY_COUNT(gMonIconPaletteTable)))
    {
        for (i = 0; i < (int)ARRAY_COUNT(gMonIconPaletteTable); i++)
        {
            LoadPalette(gMonIconPaletteTable[i].data, offset, PLTT_SIZE_4BPP);
            offset += 16;
        }
    }
}

u8 GetValidMonIconPalIndex(u16 species)
{
    if (species > NUM_SPECIES)
        species = INVALID_ICON_SPECIES;
    return gMonIconPaletteIndices[species];
}

u8 GetMonIconPaletteIndexFromSpecies(u16 species)
{
    return gMonIconPaletteIndices[species];
}

const u16 *GetValidMonIconPalettePtr(u16 species)
{
    if (species > NUM_SPECIES)
        species = INVALID_ICON_SPECIES;
    return gMonIconPaletteTable[gMonIconPaletteIndices[species]].data;
}

u8 UpdateMonIconFrame(struct Sprite *sprite)
{
    u8 result = 0;

    if (sprite->animDelayCounter == 0)
    {
        s16 frame = sprite->anims[sprite->animNum][sprite->animCmdIndex].frame.imageValue;

        switch (frame)
        {
        case -1:
            break;
        case -2:
            sprite->animCmdIndex = 0;
            break;
        default:
            RequestSpriteCopy(
                // pointer arithmetic is needed to get the correct pointer to perform the sprite copy on.
                // because sprite->images is a struct def, it has to be casted to (u8 *) before any
                // arithmetic can be performed.
                (u8 *)sprite->images + (sSpriteImageSizes[sprite->oam.shape][sprite->oam.size] * frame),
                (u8 *)(OBJ_VRAM0 + sprite->oam.tileNum * TILE_SIZE_4BPP),
                sSpriteImageSizes[sprite->oam.shape][sprite->oam.size]);
            sprite->animDelayCounter = sprite->anims[sprite->animNum][sprite->animCmdIndex].frame.duration & 0xFF;
            sprite->animCmdIndex++;
            result = sprite->animCmdIndex;
            break;
        }
    }
    else
    {
        sprite->animDelayCounter--;
    }
    return result;
}

static u8 CreateMonIconSprite(struct MonIconSpriteTemplate *iconTemplate, s16 x, s16 y, u8 subpriority)
{
    u8 spriteId;

    struct SpriteFrameImage image = { NULL, sSpriteImageSizes[iconTemplate->oam->shape][iconTemplate->oam->size] };

    struct SpriteTemplate spriteTemplate =
    {
        .tileTag = TAG_NONE,
        .paletteTag = iconTemplate->paletteTag,
        .oam = iconTemplate->oam,
        .anims = iconTemplate->anims,
        .images = &image,
        .affineAnims = iconTemplate->affineAnims,
        .callback = iconTemplate->callback,
    };

    spriteId = CreateSprite(&spriteTemplate, x, y, subpriority);
    gSprites[spriteId].animPaused = TRUE;
    gSprites[spriteId].animBeginning = FALSE;
    gSprites[spriteId].images = (const struct SpriteFrameImage *)iconTemplate->image;
    return spriteId;
}

static void FreeAndDestroyMonIconSprite_(struct Sprite *sprite)
{
    struct SpriteFrameImage image = { NULL, sSpriteImageSizes[sprite->oam.shape][sprite->oam.size] };
    sprite->images = &image;
    DestroySprite(sprite);
}

void SetPartyHPBarSprite(struct Sprite *sprite, u8 animNum)
{
    sprite->animNum = animNum;
    sprite->animDelayCounter = 0;
    sprite->animCmdIndex = 0;
}
