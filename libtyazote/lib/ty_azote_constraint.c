#include <stdlib.h>
#include <assert.h>

#include "ty_azote_constraint.h"


typedef struct ty_azote_TrieNode {
    struct ty_azote_TrieNode *child[2]; /* nullptr => leaf, uniform `state`   */
    ty_azote_AddrState state;    /* meaningful only when leaf          */
    uint64_t count;    /* valid only after compile()         */
} ty_azote_TrieNode;

struct ty_azote_Constraint {
    ty_azote_TrieNode *root;
    bool compiled;
};

static ty_azote_TrieNode *node_new(ty_azote_AddrState state)
{
    ty_azote_TrieNode *n = calloc(1, sizeof *n);
    n->state = state;
    return n;
}

ty_azote_Constraint *ty_azote_constraint_new(ty_azote_AddrState default_state)
{
    ty_azote_Constraint *c = malloc(sizeof *c);
    c->root = node_new(default_state);
    c->compiled = false;
    return c;
}

static void node_free(ty_azote_TrieNode *n)
{
    if (!n)
    {
        return;
    }
    node_free(n->child[0]);
    node_free(n->child[1]);
    free(n);
}

void ty_azote_constraint_free(ty_azote_Constraint *c)
{
    if (!c)
    {
        return;
    }
    node_free(c->root);
    free(c);
}

static inline int bit_at(uint32_t ip, int depth)
{ /* depth 0 = MSB */
    return (ip >> (31 - depth)) & 1;
}

/* Splits a leaf into two children carrying its old (uniform) state,
 * maintaining the "0 or 2 children" invariant. No-op if already internal. */
static void node_split(ty_azote_TrieNode *n)
{
    if (n->child[0])
    {
        return; /* already internal */
    }
    n->child[0] = node_new(n->state);
    n->child[1] = node_new(n->state);
}

static void node_set(ty_azote_TrieNode *n, uint32_t ip, int prefix_len, int depth,ty_azote_AddrState state)
{
    if (depth == prefix_len)
    {
        /* Whole subtree becomes uniform `state`; discard any children. */
        node_free(n->child[0]);
        node_free(n->child[1]);
        n->child[0] = n->child[1] = nullptr;
        n->state = state;
        return;
    }
    node_split(n);
    int bit = bit_at(ip, depth);
    node_set(n->child[bit], ip, prefix_len, depth + 1, state);
}

void ty_azote_constraint_set(ty_azote_Constraint *c, uint32_t ip_host,int prefix_len, ty_azote_AddrState state)
{
    assert(prefix_len >= 0 && prefix_len <= 32);
    node_set(c->root, ip_host, prefix_len, 0, state);
    c->compiled = false;
}

static uint64_t node_compile(ty_azote_TrieNode *n, int depth)
{
    if (!n->child[0])
    { /* leaf */
        n->count = (n->state == TY_AZOTE_ADDR_ALLOWED) ? ((uint64_t)1 << (32 - depth)) : 0;
        return n->count;
    }
    uint64_t left  = node_compile(n->child[0], depth + 1);
    uint64_t right = node_compile(n->child[1], depth + 1);
    n->count = left + right;
    return n->count;
}

void ty_azote_constraint_compile(ty_azote_Constraint *c)
{
    node_compile(c->root, 0);
    c->compiled = true;
}

ty_azote_AddrState ty_azote_constraint_state(const ty_azote_Constraint *c, uint32_t ip)
{
    const ty_azote_TrieNode *n = c->root;
    int depth = 0;

    while (n->child[0])
    {
        n = n->child[bit_at(ip, depth)];
        ++depth;
    }
    return n->state;
}

uint64_t ty_azote_constraint_count(const ty_azote_Constraint *c, ty_azote_AddrState state)
{
    assert(c->compiled);
    uint64_t allowed = c->root->count;
    return state == TY_AZOTE_ADDR_ALLOWED ? allowed : (((uint64_t)1 << 32) - allowed);
}

uint32_t ty_azote_constraint_index_to_ip(const ty_azote_Constraint *c, uint64_t index)
{
    assert(c->compiled);
    const ty_azote_TrieNode *n = c->root;
    uint32_t base = 0;
    int depth = 0;

    while (n->child[0])
    {
        uint64_t left_count = n->child[0]->count;

        if (index < left_count)
        {
            n = n->child[0];
        }
        else
        {
            index -= left_count;
            base |= (uint32_t)1 << (31 - depth);
            n = n->child[1];
        }
        ++depth;
    }
    /* n is a leaf spanning 2^(32-depth) allowed addresses; index is the
     * offset within it. */
    return base | (uint32_t)index;
}

bool ty_azote_constraint_ip_to_index(const ty_azote_Constraint *c, uint32_t ip,uint64_t *out_index)
{
    assert(c->compiled);
    const ty_azote_TrieNode *n = c->root;
    uint64_t index = 0;
    int depth = 0;

    while (n->child[0])
    {
        int bit = bit_at(ip, depth);

        if (bit == 1)
        {
            index += n->child[0]->count;
        }
        n = n->child[bit];
        ++depth;
    }
    if (n->state != TY_AZOTE_ADDR_ALLOWED)
    {
        return false;
    }
    uint32_t mask = (depth == 32) ? 0 : (((uint32_t)1 << (32 - depth)) - 1);
    index += (ip & mask);
    *out_index = index;
    return true;
}

static void node_visit(const ty_azote_TrieNode *n, uint32_t base, int depth,ty_azote_AddrState state, ty_azote_ConstraintVisitFn cb, void *ud)
{
    if (!n->child[0])
    {
        if (n->state == state)
        {
            cb(base, depth, ud);
        }
        return;
    }
    node_visit(n->child[0], base, depth + 1, state, cb, ud);
    node_visit(n->child[1], base | ((uint32_t)1 << (31 - depth)), depth + 1, state, cb, ud);
}

void ty_azote_constraint_visit(const ty_azote_Constraint *c, ty_azote_AddrState state,ty_azote_ConstraintVisitFn cb, void *user_data)
{
    node_visit(c->root, 0, 0, state, cb, user_data);
}




