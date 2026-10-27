/* The engine API behind minasim/engine.py: sets up a battle, drives the real battle code frame by
   frame, answers the controller commands through policies and reports the outcome.
   All functions are single-threaded (the game code is full of globals): one process per core. */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "global.h"
#include "main.h"
#include "pokemon.h"
#include "battle.h"
#include "battle_util.h"
#include "battle_setup.h"
#include "battle_controllers.h"
#include "battle_main.h"
#include "battle_talk.h"
#include "special_move.h"
#include "random.h"
#include "data.h"
#include "event_data.h"
#include "item.h"
#include "party_menu.h"
#include "constants/battle.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/opponents.h"
#include "constants/pokemon.h"
#include "constants/trainers.h"
#include "constants/vars.h"
#include "constants/battle_move_effects.h"
#include "constants/battle_ai.h"
#include "constants/characters.h"
#include "battle_anim.h"
#include "constants/item_effects.h"
#include "constants/party_menu.h"
#include "constants/tp_model.h"
#include "hb_api.h"

#define HB_EXPORT __attribute__((visibility("default")))

/* ---- platform objects the game expects ---- */
static struct SaveBlock1 sSb1;
static struct SaveBlock2 sSb2;
struct SaveBlock1 *gSaveBlock1Ptr = &sSb1;
struct SaveBlock2 *gSaveBlock2Ptr = &sSb2;
struct Main gMain;
void SetMainCallback2(MainCallback cb) { gMain.callback2 = cb; }

extern void CB2_InitBattle(void);
extern void HbInstallControllers(void);
extern void FreeBattleResources(void);
extern void FreeBattleSpritesData(void);
extern void FreeMonSpritesGfx(void);
extern int HbSweepBlocks(void);
extern int HbLiveBlocks(void);
extern void (*gBattleMainFunc)(void);

/* ---- per-battle state ---- */
static struct {
    const HbIn *in;
    HbOut *out;
    HbPolicyFn cb;
    int policy[2];
    uint32_t xs[2];                  /* policy PRNG per side (not the game's RNG) */
    uint16_t bag_item[HB_BAG], bag_count[HB_BAG];
    uint8_t n_bag;
    uint8_t kind;                    /* 0 wild 1 trainer */
    uint8_t plannedKind[MAX_BATTLERS_COUNT];
    uint32_t plannedArg[MAX_BATTLERS_COUNT];
    uint8_t turnKey, failed[MAX_BATTLERS_COUNT], moveAsks[MAX_BATTLERS_COUNT], keyValid;
    uint8_t abort;
    uint32_t expGained[HB_PARTY];
    uint8_t tpSnap[2][HB_PARTY];
    uint8_t startLevel[HB_PARTY];
    uint8_t decisions;               /* decisions made this turn by the player side */
    int logOn;
    uint16_t startMoves[HB_PARTY][4];
} G;
static HbLogEv sLog[HB_LOG_MAX];
static HbPolicyFn sCallback;
static struct TrainerMon sSynthParty[PARTY_SIZE];

HB_EXPORT void hb_set_callback(HbPolicyFn cb) { sCallback = cb; }
HB_EXPORT HbLogEv *hb_log(void) { return sLog; }

HB_EXPORT void hb_abi(int *o)
{
    o[0] = sizeof(HbIn); o[1] = sizeof(HbOut); o[2] = sizeof(HbState); o[3] = sizeof(HbLogEv);
    o[4] = sizeof(HbMonIn); o[5] = sizeof(HbMonOut); o[6] = sizeof(struct Pokemon); o[7] = HB_PARTY; o[8] = sizeof(struct BattlePokemon);
    o[9] = offsetof(struct BattleResources, flags); o[10] = sizeof(struct BattleResources); o[11] = sizeof(struct BattleStruct);
}

/* ---- helpers ---- */
struct Pokemon *HbParty(void) { return (gActiveBattler & 1) ? gEnemyParty : gPlayerParty; }
struct Pokemon *HbPartyOf(int side) { return side ? gEnemyParty : gPlayerParty; }
static struct Pokemon *PartyOf(int side) { return side ? gEnemyParty : gPlayerParty; }

static uint32_t Rnd(int side)
{
    uint32_t x = G.xs[side];
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return G.xs[side] = x;
}

int HbIsRom(u8 battler) { return (battler & 1) && G.policy[1] == HB_POLICY_ROM; }

static int Alive(struct Pokemon *p, int i)
{
    return GetMonData(&p[i], MON_DATA_SPECIES) != SPECIES_NONE && GetMonData(&p[i], MON_DATA_HP) != 0;
}

static u8 TpOf(int side, int partyIdx) { return gBattleStruct->tp[side][partyIdx]; }

static int ItemUsableOn(u16 item, int target, u8 battler)
{
    struct Pokemon *p = gPlayerParty;
    const u8 *fx;
    u32 hp, mx, st;

    if (item < ITEM_POTION || !ITEM_HAS_EFFECT(item) || gItemEffectTable[item - ITEM_POTION] == NULL)
        return 0;
    fx = gItemEffectTable[item - ITEM_POTION];
    if (GetMonData(&p[target], MON_DATA_SPECIES) == SPECIES_NONE)
        return 0;
    hp = GetMonData(&p[target], MON_DATA_HP);
    mx = GetMonData(&p[target], MON_DATA_MAX_HP);
    st = GetMonData(&p[target], MON_DATA_STATUS);
    if (fx[4] & ITEM4_REVIVE)
        return hp == 0;
    if (hp == 0)
        return 0;
    if ((fx[4] & ITEM4_HEAL_HP) && hp < mx)
        return 1;
    if ((fx[4] & ITEM4_HEAL_TP) && TpOf(B_SIDE_PLAYER, target) < TP_MAX)
        return 1;
    if ((fx[3] & ITEM3_STATUS_ALL) && target == gBattlerPartyIndexes[battler])
    {
        if (st & STATUS1_ANY)
            return 1;
        if ((fx[3] & ITEM3_CONFUSION) && (gBattleMons[battler].status2 & STATUS2_CONFUSION))
            return 1;
    }
    else if ((fx[3] & ITEM3_STATUS_ALL) && (st & STATUS1_ANY))
        return 1;
    if (target == gBattlerPartyIndexes[battler])
    {
        if ((fx[0] & ITEM0_X_ATTACK) && gBattleMons[battler].statStages[STAT_ATK] < MAX_STAT_STAGE) return 1;
        if ((fx[1] & ITEM1_X_DEFEND) && gBattleMons[battler].statStages[STAT_DEF] < MAX_STAT_STAGE) return 1;
        if ((fx[1] & ITEM1_X_SPEED) && gBattleMons[battler].statStages[STAT_SPEED] < MAX_STAT_STAGE) return 1;
        if ((fx[2] & ITEM2_X_SPATK) && gBattleMons[battler].statStages[STAT_SPATK] < MAX_STAT_STAGE) return 1;
        if ((fx[2] & ITEM2_X_ACCURACY) && gBattleMons[battler].statStages[STAT_ACC] < MAX_STAT_STAGE) return 1;
        if ((fx[0] & ITEM0_DIRE_HIT) && !(gBattleMons[battler].status2 & STATUS2_FOCUS_ENERGY)) return 1;
    }
    return 0;
}

static void FillSide(HbSideState *s, u8 b)
{
    int i, side = b & 1;
    struct BattlePokemon *m = &gBattleMons[b];

    memset(s, 0, sizeof(*s));
    s->species = m->species; s->level = m->level; s->party_idx = gBattlerPartyIndexes[b];
    s->tp = TpOf(side, s->party_idx); s->tp_max = TP_MAX;
    s->hp = m->hp; s->max_hp = m->maxHP; s->status = m->status1;
    for (i = 0; i < 8; i++) s->stages[i] = m->statStages[i];
    for (i = 0; i < 4; i++) { s->moves[i] = m->moves[i]; s->tp_cost[i] = m->moves[i] ? GetBattlerMoveTPCost(b, m->moves[i]) : 0; }
    s->n_moves = 4;
    s->special = GetBattlerSpecialMove(b);
    s->types[0] = m->types[0]; s->types[1] = m->types[1]; s->item = m->item;
}

static uint32_t Unusable(u8 b) { return CheckMoveLimitations(b, 0, MOVE_LIMITATIONS_ALL); }

static void BuildState(HbState *s, u8 b, int forced)
{
    int i, n = 0, side = b & 1, ai = 0;
    struct Pokemon *p = PartyOf(side);
    u8 foe = b ^ 1;

    memset(s, 0, sizeof(*s));
    s->side = side; s->kind = G.kind; s->forced = forced; s->turn = gBattleResults.battleTurnCounter;
    FillSide(&s->me, b); FillSide(&s->foe, foe);
    s->foe.special = 0;
    for (i = 0; i < HB_PARTY; i++)
    {
        u16 sp = GetMonData(&p[i], MON_DATA_SPECIES);
        if (sp == SPECIES_NONE) continue;
        s->party[i].species = sp; s->party[i].level = GetMonData(&p[i], MON_DATA_LEVEL);
        s->party[i].tp = TpOf(side, i); s->party[i].hp = GetMonData(&p[i], MON_DATA_HP);
        s->party[i].max_hp = GetMonData(&p[i], MON_DATA_MAX_HP); s->party[i].status = GetMonData(&p[i], MON_DATA_STATUS);
        s->party[i].alive = s->party[i].hp != 0; s->party[i].active = (i == gBattlerPartyIndexes[b]);
        s->n_party = i + 1;
    }
    if (side == 0)
    {
        for (i = 0; i < G.n_bag; i++) { s->item_id[i] = G.bag_item[i]; s->item_count[i] = G.bag_count[i]; }
        s->n_items = G.n_bag;
    }
    if (forced)
    {
        for (i = 0; i < HB_PARTY; i++)
            if (s->party[i].alive && i != gBattlerPartyIndexes[b] && n < HB_MAXLEGAL)
                s->legal[n++] = HB_CODE(HB_ACT_SWITCH, i, 0);
        s->n_legal = n;
        return;
    }
    {
        uint32_t un = Unusable(b);
        int any = 0;
        for (i = 0; i < MAX_MON_MOVES; i++)
            if (!(un & (1u << i)) && gBattleMons[b].moves[i] != MOVE_NONE) { s->legal[n++] = HB_CODE(HB_ACT_MOVE, i, 0); any++; }
        if (side == 0 && GetBattlerSpecialMove(b) != MOVE_NONE && CanBattlerAffordMoveSlot(b, SPECIAL_MOVE_POS))
            s->legal[n++] = HB_CODE(HB_ACT_MOVE, SPECIAL_MOVE_POS, 0);
        else if (!any)
            s->legal[n++] = HB_CODE(HB_ACT_MOVE, 0, 0); /* nothing affordable: the engine uses Struggle */
    }
    if (!G.failed[b])
    {
        for (i = 0; i < HB_PARTY; i++)
            if (s->party[i].alive && i != gBattlerPartyIndexes[b])
                s->legal[n++] = HB_CODE(HB_ACT_SWITCH, i, 0);
        if (side == 0)
        {
            int k, t;
            for (k = 0; k < G.n_bag; k++)
                if (G.bag_count[k] > 0)
                    for (t = 0; t < HB_PARTY && n < HB_MAXLEGAL - 8; t++)
                        if (ItemUsableOn(G.bag_item[k], t, b))
                            s->legal[n++] = HB_CODE(HB_ACT_ITEM, G.bag_item[k], t);
            if (n < HB_MAXLEGAL - 2 && GetTalkAvailability() == TALK_AVAILABLE)
            {
                s->legal[n++] = HB_CODE(HB_ACT_TALK, TALK_PROVOKE, 0);
                s->legal[n++] = HB_CODE(HB_ACT_TALK, TALK_UNNERVE, 0);
            }
        }
    }
    s->n_legal = n;
    (void)ai;
}

/* ---- greedy: maximum expected damage ---- */
static int Pct(int move, u8 atk, u8 def)
{
    int i, mult = 10;
    u8 mt = gMovesInfo[move].type;
    for (i = 0; gTypeEffectiveness[i] != TYPE_ENDTABLE; i += 3)
    {
        if (gTypeEffectiveness[i] == TYPE_FORESIGHT) break;
        if (gTypeEffectiveness[i] == mt)
        {
            if (gTypeEffectiveness[i + 1] == gBattleMons[def].types[0]) mult = mult * gTypeEffectiveness[i + 2] / 10;
            if (gTypeEffectiveness[i + 1] == gBattleMons[def].types[1] && gBattleMons[def].types[1] != gBattleMons[def].types[0])
                mult = mult * gTypeEffectiveness[i + 2] / 10;
        }
    }
    (void)atk;
    return mult;
}

static int ExpectedDamage(u8 b, u16 move)
{
    u8 def = b ^ 1;
    int dmg, mult;
    const struct MoveInfo *mi = &gMovesInfo[move];

    switch (mi->effect)
    {
    case EFFECT_LEVEL_DAMAGE: dmg = gBattleMons[b].level; break;
    case EFFECT_DRAGON_RAGE: dmg = 40; break;
    case EFFECT_SONICBOOM: dmg = 20; break;
    default:
        if (mi->power == 0) return 0;
        dmg = CalculateBaseDamage(&gBattleMons[b], &gBattleMons[def], move, gSideStatuses[GetBattlerSide(def)], 0, 0, b, def);
        if (mi->type == gBattleMons[b].types[0] || mi->type == gBattleMons[b].types[1]) dmg = dmg * 15 / 10;
        mult = Pct(move, b, def);
        dmg = dmg * mult / 10;
        break;
    }
    if (mi->accuracy) dmg = dmg * mi->accuracy / 100;
    if (dmg > gBattleMons[def].hp) dmg = gBattleMons[def].hp;
    return dmg;
}

static uint32_t PickGreedy(const HbState *s, u8 b)
{
    int i, best = -1, bestDmg = -1, bestTp = 999;
    if (s->forced) return s->legal[0];
    for (i = 0; i < s->n_legal; i++)
    {
        uint32_t c = s->legal[i];
        if ((c & 0xFF) == HB_ACT_MOVE)
        {
            int slot = (c >> 8) & 0xFF;
            u16 mv = slot == SPECIAL_MOVE_POS ? s->me.special : s->me.moves[slot];
            int d = mv ? ExpectedDamage(b, mv) : 0;
            int tp = mv ? GetBattlerMoveTPCost(b, mv) : 0;
            if (d > bestDmg || (d == bestDmg && tp < bestTp)) { best = i; bestDmg = d; bestTp = tp; }
        }
    }
    return best >= 0 ? s->legal[best] : s->legal[0];
}

static uint32_t PickRandom(const HbState *s, int side)
{
    uint32_t idx[HB_MAXLEGAL];
    int i, n = 0;
    for (i = 0; i < s->n_legal; i++)
        if (s->forced || (s->legal[i] & 0xFF) == HB_ACT_MOVE) idx[n++] = s->legal[i];
    if (!n) return s->legal[0];
    return idx[Rnd(side) % n];
}

static uint32_t Pick(const HbState *s, u8 b)
{
    int side = b & 1;
    switch (G.policy[side])
    {
    case HB_POLICY_RANDOM: return PickRandom(s, side);
    case HB_POLICY_GREEDY: return PickGreedy(s, b);
    default:
        if (G.cb) return (uint32_t)G.cb(s);
        return PickGreedy(s, b);
    }
}

static void CountMove(int side, u16 move)
{
    int i;
    HbOut *o = G.out;
    for (i = 0; i < o->n_moves_used[side]; i++)
        if (o->moves_used_id[side][i] == move) { o->moves_used_n[side][i]++; return; }
    if (o->n_moves_used[side] < HB_USED)
    {
        o->moves_used_id[side][o->n_moves_used[side]] = move;
        o->moves_used_n[side][o->n_moves_used[side]++] = 1;
    }
}

static void Log(u8 b, int action, u16 move, u16 target)
{
    HbLogEv *e;
    if (!G.logOn || G.out->log_n >= HB_LOG_MAX) return;
    e = &sLog[G.out->log_n++];
    memset(e, 0, sizeof(*e));
    e->turn = gBattleResults.battleTurnCounter + 1; e->side = b & 1; e->slot = gBattlerPartyIndexes[b];
    e->action = action; e->move = move; e->target_slot = target;
}

static void NewTurnCheck(u8 b)
{
    if (!G.keyValid || G.turnKey != gBattleResults.battleTurnCounter)
    {
        G.turnKey = gBattleResults.battleTurnCounter; G.keyValid = 1;
        memset(G.failed, 0, sizeof(G.failed));
        memset(G.moveAsks, 0, sizeof(G.moveAsks));
    }
    (void)b;
}

/* ---- decisions requested by the controller commands ---- */
int HbDecideAction(u8 b, u32 *arg)
{
    HbState st;
    int side = b & 1;

    NewTurnCheck(b);
    for (;;)
    {
        uint32_t code;
        int i, ok = 0, type;
        BuildState(&st, b, 0);
        {   /* min TP */
            uint16_t t = st.me.tp;
            if (G.out->min_tp[side] > t) G.out->min_tp[side] = t;
        }
        code = Pick(&st, b);
        for (i = 0; i < st.n_legal; i++) if (st.legal[i] == code) ok = 1;
        if (!ok) { G.out->illegal++; code = st.legal[0]; }
        type = code & 0xFF;
        if (type == HB_ACT_TALK)
        {
            DoBattleTalk((code >> 8) & 0xFF);
            G.out->talk_used++;
            Log(b, HB_ACT_TALK, 0, (code >> 8) & 0xFF);
            continue; /* Talk doesn't use up the turn */
        }
        G.plannedArg[b] = 0;
        if (type == HB_ACT_MOVE)
        {
            int slot = (code >> 8) & 0xFF;
            u16 mv = slot == SPECIAL_MOVE_POS ? st.me.special : st.me.moves[slot];
            G.plannedKind[b] = B_ACTION_USE_MOVE; G.plannedArg[b] = slot;
            if (st.n_legal == 1 && Unusable(b) == ALL_MOVES_MASK) { G.out->struggles[side]++; CountMove(side, MOVE_STRUGGLE); mv = MOVE_STRUGGLE; }
            else { if (mv) CountMove(side, mv); if (slot == SPECIAL_MOVE_POS) G.out->special_used[side]++; }
            Log(b, HB_ACT_MOVE, mv, 0);
            *arg = slot;
            return B_ACTION_USE_MOVE;
        }
        if (type == HB_ACT_SWITCH)
        {
            G.plannedKind[b] = B_ACTION_SWITCH; G.plannedArg[b] = (code >> 8) & 0xFF;
            G.out->switches[side]++;
            Log(b, HB_ACT_SWITCH, 0, (code >> 8) & 0xFF);
            *arg = G.plannedArg[b];
            return B_ACTION_SWITCH;
        }
        /* item */
        G.plannedKind[b] = B_ACTION_USE_ITEM;
        G.plannedArg[b] = ((code >> 8) & 0xFFFF) | (((code >> 24) & 0xFF) << 16);
        *arg = G.plannedArg[b];
        return B_ACTION_USE_ITEM;
    }
}

int HbApplyItem(u8 b, u32 arg, u16 *itemOut)
{
    u16 item = arg & 0xFFFF;
    u8 target = arg >> 16;
    int k, i;
    for (k = 0; k < G.n_bag; k++) if (G.bag_item[k] == item && G.bag_count[k] > 0) break;
    if (k == G.n_bag) return 0;
    gBattlerInMenuId = b;
    gActiveBattler = b;
    if (PokemonUseItemEffects(&gPlayerParty[target], item, target, 0, FALSE))
        return 0; /* no effect */
    G.bag_count[k]--;
    for (i = 0; i < G.out->n_items_used; i++)
        if (G.out->items_used_id[i] == item) { G.out->items_used_n[i]++; break; }
    if (i == G.out->n_items_used && i < 8) { G.out->items_used_id[i] = item; G.out->items_used_n[i] = 1; G.out->n_items_used++; }
    Log(b, HB_ACT_ITEM, item, target);
    *itemOut = item;
    return 1;
}

int HbDecideSwitch(u8 b, int partyAction)
{
    HbState st;
    struct Pokemon *p = PartyOf(b & 1);
    int slot, i;

    if (partyAction == PARTY_ACTION_CHOOSE_MON && G.plannedKind[b] == B_ACTION_SWITCH)
        slot = G.plannedArg[b];
    else
    {
        uint32_t code;
        BuildState(&st, b, 1);
        if (st.n_legal == 0) return PARTY_SIZE;
        code = Pick(&st, b);
        for (i = 0; i < st.n_legal; i++) if (st.legal[i] == code) break;
        if (i == st.n_legal) { G.out->illegal++; code = st.legal[0]; }
        slot = (code >> 8) & 0xFF;
        G.plannedKind[b] = 0;
    }
    if (slot >= PARTY_SIZE || !Alive(p, slot))
        for (slot = 0; slot < PARTY_SIZE; slot++) if (Alive(p, slot) && slot != gBattlerPartyIndexes[b]) break;
    return slot;
}

void HbExpConsumed(u8 monId, u32 amount) { if (monId < HB_PARTY) G.expGained[monId] += amount; }

void HbNoteFailed(u8 b, int what)
{
    if (what == 0)
    {
        if (++G.moveAsks[b] > 4) G.abort = 2; /* the policy keeps choosing a move the engine refuses */
    }
    else
        G.failed[b] = 1; /* a refused switch/item: only moves are offered for the rest of the turn */
}

/* ---- battle set-up ---- */
static void MakeMon(struct Pokemon *mon, const HbMonIn *m)
{
    int i;
    u8 fixedIV = USE_RANDOM_IVS, same = 1, any = 0;
    u32 one = 1;

    for (i = 0; i < 6; i++)
    {
        if (m->ivs[i] != 0xFF) any = 1;
        if (m->ivs[i] != m->ivs[0]) same = 0;
    }
    if (any && same) fixedIV = m->ivs[0];
    if (m->nature != 0xFF)
        CreateMonWithNature(mon, m->species, m->level, fixedIV, m->nature);
    else
        CreateMon(mon, m->species, m->level, fixedIV, FALSE, 0, OT_ID_PLAYER_ID, 0);
    if (any && !same)
        for (i = 0; i < 6; i++)
            if (m->ivs[i] != 0xFF) { u8 v = m->ivs[i]; SetMonData(mon, MON_DATA_HP_IV + i, &v); }
    if (m->has_exp) { u32 e = m->exp; SetMonData(mon, MON_DATA_EXP, &e); }
    if (m->has_moves)
        for (i = 0; i < 4; i++) { u16 mv = m->moves[i]; SetMonData(mon, MON_DATA_MOVE1 + i, &mv); }
    if (m->item) { u16 it = m->item; SetMonData(mon, MON_DATA_HELD_ITEM, &it); }
    if (m->awakened) SetMonData(mon, MON_DATA_SPECIAL_AWAKENED, &one);
    if (m->special_progress) { u8 v = m->special_progress; SetMonData(mon, MON_DATA_SPECIAL_PROGRESS, &v); }
    if (m->has_friendship) { u8 v = m->friendship; SetMonData(mon, MON_DATA_FRIENDSHIP, &v); }
    CalculateMonStats(mon);
    if (m->has_hp) { u32 hp = m->hp; SetMonData(mon, MON_DATA_HP, &hp); }
    if (m->status) { u32 st = m->status; SetMonData(mon, MON_DATA_STATUS, &st); }
    { u8 tp = TP_MAX - (m->tp_spent > TP_MAX ? TP_MAX : m->tp_spent); SetMonData(mon, MON_DATA_SAVED_TP, &tp); }
}

static void SynthTrainer(const HbIn *in)
{
    struct Trainer *t = (struct Trainer *)&gTrainers[TRAINER_NONE];
    int i, j;

    memset(sSynthParty, 0, sizeof(sSynthParty));
    memset(t, 0, sizeof(*t));
    t->trainerName[0] = EOS;
    t->trainerClass = in->trainer_class;
    t->aiFlags = in->ai_flags;
    for (i = 0; i < in->n_foe_items && i < MAX_TRAINER_ITEMS; i++) t->items[i] = in->foe_items[i];
    t->partySize = in->n_foe;
    t->party = sSynthParty;
    for (i = 0; i < in->n_foe; i++)
    {
        const HbMonIn *m = &in->foe[i];
        u32 iv = 0;
        for (j = 0; j < 6; j++) iv |= (u32)((m->ivs[j] == 0xFF ? 0 : m->ivs[j]) & 31) << (5 * j);
        sSynthParty[i].species = m->species; sSynthParty[i].lvl = m->level; sSynthParty[i].heldItem = m->item;
        sSynthParty[i].iv = iv;
        if (m->has_moves) for (j = 0; j < 4; j++) sSynthParty[i].moves[j] = m->moves[j];
        sSynthParty[i].nature = m->nature != 0xFF ? m->nature : TRAINER_MON_NATURE_DEFAULT;
    }
}

static void FillMonOut(HbMonOut *o, struct Pokemon *p, int side, int i)
{
    int k;
    memset(o, 0, sizeof(*o));
    o->species = GetMonData(&p[i], MON_DATA_SPECIES);
    if (o->species == SPECIES_NONE) return;
    o->level = GetMonData(&p[i], MON_DATA_LEVEL);
    o->hp = GetMonData(&p[i], MON_DATA_HP); o->max_hp = GetMonData(&p[i], MON_DATA_MAX_HP);
    o->alive = o->hp != 0; o->status = GetMonData(&p[i], MON_DATA_STATUS);
    o->exp = GetMonData(&p[i], MON_DATA_EXP);
    o->tp = G.tpSnap[side][i]; o->tp_max = TP_MAX; o->tp_spent = TP_MAX - o->tp;
    o->item = GetMonData(&p[i], MON_DATA_HELD_ITEM);
    o->special_awake = GetMonData(&p[i], MON_DATA_SPECIAL_AWAKENED);
    o->special_progress = GetMonData(&p[i], MON_DATA_SPECIAL_PROGRESS); o->friendship = GetMonData(&p[i], MON_DATA_FRIENDSHIP);
    for (k = 0; k < 4; k++) o->moves[k] = GetMonData(&p[i], MON_DATA_MOVE1 + k);
    if (side == 0) o->exp_gained = G.expGained[i];
}

/* ---- per-frame log observer ---- */
static struct { int open; u8 atk; u16 move; u16 turn; u8 act; u16 hpBefore; HbLogEv *ev; uint8_t flags, crit; u8 key; } O;
static void ObsClose(void)
{
    if (!O.open || !O.ev) { O.open = 0; return; }
    {
        u8 def = O.atk ^ 1;
        int after = gBattleMons[def].hp;
        O.ev->damage = O.hpBefore > after ? O.hpBefore - after : 0;
        O.ev->hp_after = after;
        O.ev->miss = (O.flags & MOVE_RESULT_MISSED) != 0;
        O.ev->eff = (O.flags & MOVE_RESULT_SUPER_EFFECTIVE) ? 2 : (O.flags & MOVE_RESULT_NOT_VERY_EFFECTIVE) ? 0 : (O.flags & MOVE_RESULT_DOESNT_AFFECT_FOE) ? 3 : 1;
        O.ev->crit = O.crit; O.ev->ko = after == 0 && O.hpBefore > 0;
    }
    O.open = 0;
}
static void ObsFrame(void)
{
    if (!G.logOn || !gBattleStruct) return;
    if (O.open)
    {
        O.flags |= gMoveResultFlags;
        if (gCritMultiplier > 1) O.crit = 1;
    }
    /* a move starts when the engine is executing a move script */
    if (gCurrentMove != MOVE_NONE && (gCurrentMove != O.move || gBattlerAttacker != O.atk || gCurrentTurnActionNumber != O.key || gBattleResults.battleTurnCounter != O.turn))
    {
        /* only for battle script executions of a move (not selection) */
        if (gBattleMainFunc && gBattleControllerExecFlags == 0 && gChosenActionByBattler[gBattlerAttacker] == B_ACTION_USE_MOVE
            && gBattleCommunication[0] != 0xFF)
        {
            ObsClose();
            O.move = gCurrentMove; O.atk = gBattlerAttacker; O.key = gCurrentTurnActionNumber; O.turn = gBattleResults.battleTurnCounter;
            O.flags = 0; O.crit = 0;
            if (G.out->log_n < HB_LOG_MAX && gBattleMons[gBattlerAttacker].hp)
            {
                O.ev = &sLog[G.out->log_n++];
                memset(O.ev, 0, sizeof(*O.ev));
                O.ev->turn = O.turn + 1; O.ev->side = O.atk & 1; O.ev->slot = gBattlerPartyIndexes[O.atk];
                O.ev->action = 5; O.ev->move = O.move; /* 5 = move executed (with result) */
                O.hpBefore = gBattleMons[O.atk ^ 1].hp;
                O.open = 1;
            }
        }
    }
}

/* ---- the battle ---- */
HB_EXPORT int hb_battle(const HbIn *in, HbOut *out)
{
    int i, f = 0, turnCap = in->turn_cap ? in->turn_cap : 200;
    int lastTurn = 0, turns = 0;
    uint8_t lastCounter = 0;

    memset(out, 0, sizeof(*out));
    memset(&G, 0, sizeof(G));
    memset(&O, 0, sizeof(O));
    G.in = in; G.out = out; G.cb = sCallback;
    G.policy[0] = in->player_policy; G.policy[1] = in->foe_policy;
    G.xs[0] = in->seed * 2654435761u + 0x9E3779B9u; G.xs[1] = in->seed * 40503u + 0x7F4A7C15u;
    if (!G.xs[0]) G.xs[0] = 1;
    if (!G.xs[1]) G.xs[1] = 2;
    G.logOn = in->log;
    G.n_bag = in->n_bag > HB_BAG ? HB_BAG : in->n_bag;
    memcpy(G.bag_item, in->bag_item, sizeof(G.bag_item)); memcpy(G.bag_count, in->bag_count, sizeof(G.bag_count));
    out->min_tp[0] = out->min_tp[1] = 255;
    G.kind = (in->use_trainer || in->foe_is_trainer) ? 1 : 0;

    memset(&sSb1, 0, sizeof(sSb1)); memset(&sSb2, 0, sizeof(sSb2));
    memset(&gMain, 0, sizeof(gMain));
    memset(&gBattleResults, 0, sizeof(gBattleResults));
    sSb2.optionsBattleStyle = OPTIONS_BATTLE_STYLE_SET; /* no free "switch before the foe's next mon" prompt */
    VarSet(VAR_CHARISMA, in->charisma);

    gRngValue = in->seed;
    ZeroPlayerPartyMons(); ZeroEnemyPartyMons();
    if (in->n_player < 1 || in->n_player > HB_PARTY || (!in->use_trainer && (in->n_foe < 1 || in->n_foe > HB_PARTY)))
    {
        out->error = 3; out->winner = 2;
        return 2;
    }
    for (i = 0; i < in->n_player; i++) MakeMon(&gPlayerParty[i], &in->player[i]);
    gBattleTypeFlags = 0;
    if (in->use_trainer)
    {
        if (gTrainers[in->trainer].doubleBattle) { out->error = 3; out->winner = 2; return 2; }
        gTrainerBattleOpponent_A = in->trainer;
        gBattleTypeFlags = BATTLE_TYPE_TRAINER;
    }
    else if (in->foe_is_trainer)
    {
        SynthTrainer(in);
        gTrainerBattleOpponent_A = TRAINER_NONE;
        gBattleTypeFlags = BATTLE_TYPE_TRAINER;
    }
    else
    {
        for (i = 0; i < in->n_foe; i++) MakeMon(&gEnemyParty[i], &in->foe[i]);
    }
    CalculatePlayerPartyCount(); CalculateEnemyPartyCount();
    for (i = 0; i < in->n_player; i++) { G.startLevel[i] = gPlayerParty[i].level; }

    gMain.inBattle = 0; gMain.callback1 = NULL; gMain.callback2 = CB2_InitBattle;
    HbInstallControllers();
    out->n_player = in->n_player;

    while (f < 30000)
    {
        /* menus in battle scripts: A answers "yes" (use the next mon / stop trying to learn); the "learn this move?"
           box gets B (no), so a level-up over four known moves keeps them */
        gMain.newKeys = (gBattlescriptCurrInstr && *gBattlescriptCurrInstr == B_SCR_OP_YESNOBOXLEARNMOVE) ? B_BUTTON : A_BUTTON;
        if (gMain.callback1) gMain.callback1();
        if (!gMain.callback2) break;
        gMain.callback2();
        f++;
        if (gBattleStruct)
            for (i = 0; i < HB_PARTY; i++) { G.tpSnap[0][i] = gBattleStruct->tp[0][i]; G.tpSnap[1][i] = gBattleStruct->tp[1][i]; }
        if (G.logOn) ObsFrame();
        if (gBattleStruct)
        {
            uint8_t c = gBattleResults.battleTurnCounter;
            if (c != lastCounter) { turns += (uint8_t)(c - lastCounter); lastCounter = c; }
            if (turns >= turnCap) { out->error = out->error ? out->error : 0; out->winner = 2; break; }
        }
        if (G.abort) { out->error = G.abort; break; }
    }
    if (f >= 30000 && !out->error) { out->error = 1; if (getenv("HB_ABORT")) abort(); }
    ObsClose();
    (void)lastTurn;

    out->frames = f; out->turns = turns + 1;
    out->outcome = gBattleOutcome;
    if (out->winner != 2 && !out->error)
        out->winner = gBattleOutcome == B_OUTCOME_WON ? 0 : gBattleOutcome == B_OUTCOME_LOST ? 1 : 2;
    else
        out->winner = 2;
    if (!out->error && !(gBattleOutcome == B_OUTCOME_WON || gBattleOutcome == B_OUTCOME_LOST)) out->winner = 2;
    out->kind = G.kind;
    CalculateEnemyPartyCount();
    for (i = 0; i < HB_PARTY; i++)
    {
        FillMonOut(&out->player[i], gPlayerParty, 0, i);
        FillMonOut(&out->foe[i], gEnemyParty, 1, i);
    }
    out->n_foe = 0;
    for (i = 0; i < HB_PARTY; i++) if (out->foe[i].species) out->n_foe = i + 1;
    /* leaks: whatever the game left allocated */
    /* the game frees these when the fade to the overworld finishes (and not at all on aborted battles) */
    FreeBattleResources(); FreeBattleSpritesData(); FreeMonSpritesGfx();
    out->leaked_blocks = HbLiveBlocks();
    if (getenv("HB_LEAKS")) { extern int hb_live_sizes(unsigned *, int); unsigned sz[64]; int n = hb_live_sizes(sz, 64), k; for (k = 0; k < n; k++) fprintf(stderr, "leak %u\n", sz[k]); }
    HbSweepBlocks();
    out->rng_end = gRngValue;
    return out->winner;
}

/* ---- oracle entry points: single functions compared against the ROM (oracle.py) ---- */
HB_EXPORT unsigned hb_rand_seq(unsigned seed, int n) { unsigned v = 0; gRngValue = seed; while (n--) v = Random(); return v; }
HB_EXPORT void hb_createmon_dump(unsigned seed, int sp, int lvl, int fixedIV, unsigned char *out)
{
    gRngValue = seed; memset(&sSb2, 0, sizeof(sSb2)); ZeroPlayerPartyMons();
    CreateMon(&gPlayerParty[0], sp, lvl, fixedIV, FALSE, 0, OT_ID_PLAYER_ID, 0);
    memcpy(out, &gPlayerParty[0], sizeof(struct Pokemon));
}

/* BattlePokemon built from a fresh mon, with stat stages and status set: the input of the damage oracle.
   out must hold sizeof(struct BattlePokemon) bytes (hb_abi o[8]). */
HB_EXPORT void hb_battlemon(unsigned seed, int species, int level, int fixedIV, const unsigned short *moves, int item,
                            const signed char *stages, unsigned status1, unsigned char *out)
{
    struct BattlePokemon b;
    struct Pokemon *p = &gPlayerParty[0];
    int i;

    gRngValue = seed; memset(&sSb2, 0, sizeof(sSb2)); ZeroPlayerPartyMons();
    CreateMon(p, species, level, fixedIV, FALSE, 0, OT_ID_PLAYER_ID, 0);
    for (i = 0; i < 4; i++) { u16 mv = moves[i]; SetMonData(p, MON_DATA_MOVE1 + i, &mv); }
    { u16 it = item; SetMonData(p, MON_DATA_HELD_ITEM, &it); }
    memset(&b, 0, sizeof(b));
    b.species = species; b.attack = GetMonData(p, MON_DATA_ATK); b.defense = GetMonData(p, MON_DATA_DEF);
    b.speed = GetMonData(p, MON_DATA_SPEED); b.spAttack = GetMonData(p, MON_DATA_SPATK); b.spDefense = GetMonData(p, MON_DATA_SPDEF);
    for (i = 0; i < 4; i++) b.moves[i] = moves[i];
    for (i = 0; i < 8; i++) b.statStages[i] = stages[i];
    b.types[0] = gSpeciesInfo[species].types[0]; b.types[1] = gSpeciesInfo[species].types[1];
    b.level = level; b.hp = b.maxHP = GetMonData(p, MON_DATA_MAX_HP); b.item = item; b.status1 = status1;
    memcpy(out, &b, sizeof(b));
}

HB_EXPORT int hb_damage(const unsigned char *atk, const unsigned char *def, int move, int sideStatus)
{
    static struct BattleStruct sBs;
    static struct BattleResources sBr;
    if (gMovesInfo[move].power == 0) return -1; /* status moves never reach the damage calc */
    static struct ResourceFlags sFl;
    memset(&sBs, 0, sizeof(sBs)); gBattleStruct = &sBs;
    memset(&sBr, 0, sizeof(sBr)); memset(&sFl, 0, sizeof(sFl)); sBr.flags = &sFl; gBattleResources = &sBr;
    memset(gBattleMons, 0, sizeof(struct BattlePokemon) * 2);
    memcpy(&gBattleMons[0], atk, sizeof(struct BattlePokemon)); memcpy(&gBattleMons[1], def, sizeof(struct BattlePokemon));
    gBattleTypeFlags = 0; gBattleWeather = 0; gCritMultiplier = 1; gBattleMovePower = 0;
    {
        int d = CalculateBaseDamage(&gBattleMons[0], &gBattleMons[1], move, sideStatus, 0, 0, 0, 1);
        gBattleStruct = NULL; gBattleResources = NULL;
        return d;
    }
}
