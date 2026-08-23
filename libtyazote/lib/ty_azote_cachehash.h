#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ty_azote_CacheHash ty_azote_CacheHash; /* opaque */

typedef void (*ty_azote_CacheEvictFn)(void *value, void *user_data);

[[nodiscard]]
ty_azote_CacheHash *ty_azote_cachehash_new(size_t max_items, ty_azote_CacheEvictFn evict_cb, void *user_data);

void ty_azote_cachehash_free(ty_azote_CacheHash *ch);

void ty_azote_cachehash_set_evict_cb(ty_azote_CacheHash *ch,ty_azote_CacheEvictFn cb, void *user_data);

/* Look up without changing LRU order. */
[[nodiscard]]
void *ty_azote_cachehash_peek(ty_azote_CacheHash *ch,const void *key, size_t keylen);

/* Look up and move to MRU position. */
[[nodiscard]]
void *ty_azote_cachehash_get(ty_azote_CacheHash *ch,const void *key, size_t keylen);

/* Inserts/overwrites. Evicts LRU (invoking the evict callback) if full. */
void ty_azote_cachehash_put(ty_azote_CacheHash *ch,const void *key, size_t keylen, void *value);

/* If full, evicts and returns the LRU value WITHOUT invoking the callback
 * (caller takes ownership); returns nullptr if not full. */
[[nodiscard]]
void *ty_azote_cachehash_evict_if_full(ty_azote_CacheHash *ch);

/* Iterates values in MRU -> LRU order. */
void ty_azote_cachehash_iter(ty_azote_CacheHash *ch, ty_azote_CacheEvictFn cb, void *user_data);

void ty_azote_cachehash_debug_dump(ty_azote_CacheHash *ch, FILE *out);

#ifdef __cplusplus
}
#endif













