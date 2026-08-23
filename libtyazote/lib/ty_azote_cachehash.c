#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "ty_azote_cachehash.h"


typedef struct ty_azote_CacheNode {
    struct ty_azote_CacheNode *lru_prev, *lru_next;   /* LRU list       */
    struct ty_azote_CacheNode *bucket_next;           /* hash chain     */
    struct ty_azote_CacheNode *pool_next;             /* free-list link */
    void *key;
    size_t keylen;
    void *value;
    uint64_t hash;
    bool in_use;
} ty_azote_CacheNode;

struct ty_azote_CacheHash {
    ty_azote_CacheNode *pool;         /* contiguous array, Object Pool */
    ty_azote_CacheNode *free_list;
    ty_azote_CacheNode **buckets;
    size_t bucket_count; /* power of two */
    ty_azote_CacheNode *lru_head;     /* MRU */
    ty_azote_CacheNode *lru_tail;     /* LRU */
    size_t  max_items;
    size_t count;
    ty_azote_CacheEvictFn evict_cb;
    void *evict_ud;
};

static uint64_t fnv1a(const void *data, size_t len)
{
    const uint8_t *p = data;
    uint64_t h = 0xcbf29ce484222325ULL;

    for (size_t i = 0; i < len; ++i)
    {
        h ^= p[i];
        h *= 0x100000001b3ULL;
    }
    return h;
}

static size_t next_pow2(size_t v)
{
    size_t p = 16;

    while (p < v)
    {
        p <<= 1;
    }
    return p;
}

ty_azote_CacheHash *ty_azote_cachehash_new(size_t max_items, ty_azote_CacheEvictFn cb, void *ud)
{
    assert(max_items > 0);
    ty_azote_CacheHash *ch = calloc(1, sizeof *ch);
    ch->pool = calloc(max_items, sizeof(ty_azote_CacheNode));
    ch->bucket_count = next_pow2(max_items * 2);
    ch->buckets = calloc(ch->bucket_count, sizeof(ty_azote_CacheNode *));
    ch->max_items = max_items;
    ch->evict_cb = cb;
    ch->evict_ud = ud;
    /* Build the free-list threading through the pool. */
    for (size_t i = 0; i + 1 < max_items; ++i) ch->pool[i].pool_next = &ch->pool[i + 1];
    ch->free_list = &ch->pool[0];
    return ch;
}

void ty_azote_cachehash_set_evict_cb(ty_azote_CacheHash *ch, ty_azote_CacheEvictFn cb, void *ud)
{
    ch->evict_cb = cb;
    ch->evict_ud = ud;
}

static ty_azote_CacheNode **bucket_slot(ty_azote_CacheHash *ch, uint64_t hash)
{
    return &ch->buckets[hash & (ch->bucket_count - 1)];
}

static ty_azote_CacheNode *bucket_find(ty_azote_CacheHash *ch, const void *key, size_t keylen,uint64_t hash)
{
    for (ty_azote_CacheNode *n = *bucket_slot(ch, hash); n; n = n->bucket_next)
    {
        if (n->hash == hash && n->keylen == keylen && memcmp(n->key, key, keylen) == 0)
        {
            return n;
        }
    }
    return nullptr;
}

static void bucket_insert(ty_azote_CacheHash *ch, ty_azote_CacheNode *n)
{
    ty_azote_CacheNode **slot = bucket_slot(ch, n->hash);
    n->bucket_next = *slot;
    *slot = n;
}

static void bucket_remove(ty_azote_CacheHash *ch, ty_azote_CacheNode *n)
{
    ty_azote_CacheNode **slot = bucket_slot(ch, n->hash);

    while (*slot)
    {
        if (*slot == n)
        {
            *slot = n->bucket_next;
            return;
        }
        slot = &(*slot)->bucket_next;
    }
}

/* --- LRU list operations --- */

static void lru_unlink(ty_azote_CacheHash *ch, ty_azote_CacheNode *n)
{
    if (n->lru_prev)
    {
        n->lru_prev->lru_next = n->lru_next;
    }
    else
    {
        ch->lru_head = n->lru_next;
    }
    if (n->lru_next)
    {
        n->lru_next->lru_prev = n->lru_prev;
    }
    else
    {
        ch->lru_tail = n->lru_prev;
    }
    n->lru_prev = n->lru_next = nullptr;
}

static void lru_push_front(ty_azote_CacheHash *ch, ty_azote_CacheNode *n)
{
    n->lru_prev = nullptr;
    n->lru_next = ch->lru_head;

    if (ch->lru_head)
    {
        ch->lru_head->lru_prev = n;
    }
    ch->lru_head = n;

    if (!ch->lru_tail)
    {
        ch->lru_tail = n;
    }
}

static void touch(ty_azote_CacheHash *ch, ty_azote_CacheNode *n)
{
    if (ch->lru_head == n)
    {
        return;
    }
    lru_unlink(ch, n);
    lru_push_front(ch, n);
}

/* --- Public API --- */

void *ty_azote_cachehash_peek(ty_azote_CacheHash *ch, const void *key, size_t keylen)
{
    ty_azote_CacheNode *n = bucket_find(ch, key, keylen, fnv1a(key, keylen));
    return n ? n->value : nullptr;
}

void *ty_azote_cachehash_get(ty_azote_CacheHash *ch, const void *key, size_t keylen)
{
    ty_azote_CacheNode *n = bucket_find(ch, key, keylen, fnv1a(key, keylen));

    if (!n)
    {
        return nullptr;
    }
    touch(ch, n);
    return n->value;
}

static void evict_node(ty_azote_CacheHash *ch, ty_azote_CacheNode *n)
{
    bucket_remove(ch, n);
    lru_unlink(ch, n);
    free(n->key);
    n->key = nullptr;
    n->keylen = 0;
    n->in_use = false;
    n->pool_next = ch->free_list;
    ch->free_list = n;
    --ch->count;
}

void *ty_azote_cachehash_evict_if_full(ty_azote_CacheHash *ch)
{
    if (ch->count < ch->max_items)
    {
        return nullptr;
    }
    ty_azote_CacheNode *lru = ch->lru_tail;
    assert(lru);
    void *value = lru->value;
    evict_node(ch, lru);
    return value;
}

void ty_azote_cachehash_put(ty_azote_CacheHash *ch, const void *key, size_t keylen, void *value)
{
    uint64_t hash = fnv1a(key, keylen);
    /* Overwrite in place if the key already exists. */
    ty_azote_CacheNode *existing = bucket_find(ch, key, keylen, hash);

    if (existing)
    {
        existing->value = value;
        touch(ch, existing);
        return;
    }

    if (ch->count == ch->max_items)
    {
        ty_azote_CacheNode *lru = ch->lru_tail;
        void *evicted_value = lru->value;
        evict_node(ch, lru);

        if (ch->evict_cb)
        {
            ch->evict_cb(evicted_value, ch->evict_ud);
        }
    }
    ty_azote_CacheNode *n = ch->free_list;
    assert(n);
    ch->free_list = n->pool_next;
    n->pool_next = nullptr;
    n->key = malloc(keylen);
    memcpy(n->key, key, keylen);
    n->keylen = keylen;
    n->value = value;
    n->hash = hash;
    n->in_use = true;
    bucket_insert(ch, n);
    lru_push_front(ch, n);
    ++ch->count;
}

void ty_azote_cachehash_iter(ty_azote_CacheHash *ch, ty_azote_CacheEvictFn cb, void *ud)
{
    for (ty_azote_CacheNode *n = ch->lru_head; n; n = n->lru_next) cb(n->value, ud);
}

void ty_azote_cachehash_debug_dump(ty_azote_CacheHash *ch, FILE *out)
{
    fprintf(out, "ty_azote_cachehash: %zu/%zu items\n", ch->count, ch->max_items);
    size_t i = 0;

    for (ty_azote_CacheNode *n = ch->lru_head; n; n = n->lru_next, ++i)
    {
        fprintf(out, "  [%zu MRU-order] key=%.*s value=%p\n",i, (int)n->keylen, (const char *)n->key, n->value);
    }
}

void ty_azote_cachehash_free(ty_azote_CacheHash *ch)
{
    if (!ch)
    {
        return;
    }
    for (ty_azote_CacheNode *n = ch->lru_head; n; n = n->lru_next)
    {
        free(n->key);
    }
    free(ch->pool);
    free(ch->buckets);
    free(ch);
}
