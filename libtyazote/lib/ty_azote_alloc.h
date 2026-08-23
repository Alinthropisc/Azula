#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__GNUC__) || defined(__clang__)
#define TY_AZOTE_CLEANUP(f)  __attribute__((cleanup(f)))
#define TY_AZOTE_MALLOC_LIKE __attribute__((malloc, warn_unused_result))
#else
#define TY_AZOTE_CLEANUP(f)
#define TY_AZOTE_MALLOC_LIKE
#endif


typedef struct ty_azote_Allocator {
    void *self;
    void *(*alloc)(void *self, size_t size, size_t align);
    void *(*realloc)(void *self, void *ptr, size_t old_size, size_t new_size);
    void (*free)(void *self, void *ptr, size_t size);
} ty_azote_Allocator;

/* Default libc-backed allocator (Singleton, stateless). */
[[nodiscard]]
const ty_azote_Allocator *ty_azote_system_allocator(void);

/* Global default used by facades below. Overridable, e.g. in tests. */
void ty_azote_set_default_allocator(const ty_azote_Allocator *alloc);

[[nodiscard]]
const ty_azote_Allocator *ty_azote_default_allocator(void);

[[nodiscard]]
TY_AZOTE_MALLOC_LIKE void *ty_azote_try_malloc(size_t size);

[[nodiscard]]
TY_AZOTE_MALLOC_LIKE void *ty_azote_try_zalloc(size_t size);

[[nodiscard]]
TY_AZOTE_MALLOC_LIKE void *ty_azote_try_calloc(size_t count, size_t size);

[[nodiscard]]
void *ty_azote_try_realloc(void *ptr, size_t new_size);

void ty_azote_free(void *ptr);


[[nodiscard]]
TY_AZOTE_MALLOC_LIKE void *ty_azote_malloc(size_t size);

[[nodiscard]]
TY_AZOTE_MALLOC_LIKE void *ty_azote_zalloc(size_t size);

[[nodiscard]]
TY_AZOTE_MALLOC_LIKE void *ty_azote_calloc(size_t count, size_t size);

[[nodiscard]]
void *ty_azote_realloc(void *ptr, size_t new_size);

/* ======================================================================= *
 *  Arena / bump allocator (Object Pool) — for per-probe scratch memory.
 *  Individual ty_azote_arena_alloc() calls are O(1) and never freed
 *  individually; ty_azote_arena_reset() reclaims everything at once.
 *  Not thread-safe by design — give each worker thread its own arena.
 * ======================================================================= */

typedef struct ty_azote_ArenaBlock ty_azote_ArenaBlock;

typedef struct ty_azote_Arena {
    ty_azote_ArenaBlock *head;
    size_t block_size;
} ty_azote_Arena;

[[nodiscard]]
ty_azote_Arena ty_azote_arena_new(size_t block_size);

[[nodiscard]]
void *ty_azote_arena_alloc(ty_azote_Arena *arena, size_t size, size_t align);

void ty_azote_arena_reset(ty_azote_Arena *arena); /* keeps blocks, resets bump pointer */
void ty_azote_arena_free(ty_azote_Arena *arena);  /* releases all blocks               */

/* Adapter: exposes an Arena through the generic ty_azote_Allocator interface. */
[[nodiscard]]
ty_azote_Allocator ty_azote_arena_as_allocator(ty_azote_Arena *arena);

static inline void ty_azote_arena_cleanup_(ty_azote_Arena *a)
{
    ty_azote_arena_free(a);
}

#define TY_AZOTE_ARENA_GUARD(name_, block_size_) [[maybe_unused]] TY_AZOTE_CLEANUP(ty_azote_arena_cleanup_) ty_azote_Arena name_ = ty_azote_arena_new(block_size_)

#ifdef __cplusplus
}
#endif
































