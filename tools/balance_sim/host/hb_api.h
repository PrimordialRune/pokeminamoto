/* ABI between libminasim.so and minasim/engine.py (ctypes mirrors these structs; hb_abi() lets
   Python verify sizes). Plain C, no game headers. Append fields at the end only. */
#ifndef HB_API_H
#define HB_API_H
#include <stdint.h>

#define HB_PARTY 6
#define HB_MAXLEGAL 128
#define HB_BAG 16
#define HB_USED 32
#define HB_LOG_MAX 2048

enum { HB_POLICY_CALLBACK = 0, HB_POLICY_RANDOM = 1, HB_POLICY_GREEDY = 2, HB_POLICY_ROM = 3 };
enum { HB_ACT_MOVE = 1, HB_ACT_SWITCH = 2, HB_ACT_ITEM = 3, HB_ACT_TALK = 4 };
/* action code: type | arg << 8 | arg2 << 24 ; move arg 0-3 slot, 4 = the Special; switch arg = party index;
   item arg = item id, arg2 = party index of the target; talk arg 0 provoke, 1 unnerve */
#define HB_CODE(t, a, a2) ((uint32_t)(t) | ((uint32_t)(a) << 8) | ((uint32_t)(a2) << 24))

typedef struct {
    uint16_t species; uint8_t level; uint8_t nature;          /* nature 0xFF: random */
    uint8_t ivs[6];                                            /* each 0xFF: random (wild) */
    uint16_t moves[4]; uint16_t item;
    uint8_t has_moves, tp_spent, awakened, has_hp;
    uint16_t hp; uint16_t pad0;
    uint32_t exp; uint8_t has_exp; uint8_t special_progress, friendship, has_friendship;
    uint32_t status;                                           /* raw STATUS1 */
} HbMonIn;

typedef struct {
    uint32_t seed;
    uint16_t trainer;        /* trainers.party id used when use_trainer */
    uint8_t n_player, n_foe;
    uint8_t player_policy, foe_policy;
    uint8_t log, charisma;
    uint16_t turn_cap; uint8_t use_trainer; uint8_t foe_is_trainer;
    uint8_t trainer_class; uint8_t n_foe_items; uint16_t foe_items[4];
    uint32_t ai_flags;
    HbMonIn player[HB_PARTY], foe[HB_PARTY];
    uint16_t bag_item[HB_BAG], bag_count[HB_BAG];
    uint8_t n_bag; uint8_t arena; uint8_t pad[2];   /* arena: ARENA_* (include/arena_ecology.h) */
} HbIn;

typedef struct {
    uint16_t species; uint8_t level; uint8_t tp; uint8_t tp_max; uint8_t alive; uint8_t special_awake; uint8_t tp_spent;
    uint16_t hp, max_hp; uint32_t status; uint32_t exp, exp_gained; uint16_t moves[4]; uint16_t item; uint8_t special_progress, friendship;
} HbMonOut;

typedef struct {
    uint16_t turn; uint8_t side, slot, action, crit, miss, eff; uint8_t ko, pad; uint16_t move, damage, hp_after, target_slot;
} HbLogEv;

typedef struct {
    int8_t winner;             /* 0 player, 1 foe, 2 draw / turn cap / error */
    uint8_t outcome; uint16_t turns; uint32_t frames;
    uint8_t n_player, n_foe, kind, error;   /* error: 0 ok, 1 frame cap, 2 policy loop, 3 bad input */
    HbMonOut player[HB_PARTY], foe[HB_PARTY];
    uint16_t moves_used_id[2][HB_USED], moves_used_n[2][HB_USED]; uint8_t n_moves_used[2], pad0[2];
    uint16_t items_used_id[8], items_used_n[8]; uint8_t n_items_used, pad1;
    uint16_t talk_used, special_used[2], struggles[2], min_tp[2], illegal, switches[2];
    uint16_t log_n; int32_t leaked_blocks; uint32_t rng_end;
} HbOut;

typedef struct {
    uint16_t species; uint8_t level; uint8_t tp, tp_max, party_idx, n_moves, pad0;
    uint16_t hp, max_hp; uint32_t status; int8_t stages[8]; uint16_t moves[4]; uint8_t tp_cost[4];
    uint16_t special; uint8_t types[2]; uint16_t item; uint16_t pad1;
} HbSideState;

typedef struct {
    uint8_t side, kind, forced, turn;     /* kind 0 wild, 1 trainer; forced: choose the replacement after a faint */
    HbSideState me, foe;
    struct { uint16_t species; uint8_t level, tp; uint16_t hp, max_hp; uint32_t status; uint8_t alive, active, pad[2]; } party[HB_PARTY];
    uint8_t n_party; uint8_t n_legal; uint8_t n_items; uint8_t pad2;
    uint16_t item_id[HB_BAG], item_count[HB_BAG];
    uint32_t legal[HB_MAXLEGAL];
} HbState;

typedef int32_t (*HbPolicyFn)(const HbState *st);
#endif
