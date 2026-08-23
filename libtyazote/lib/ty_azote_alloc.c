#include <stdlib.h>
#include <string.h>
#include <stdalign.h>

#include "ty_azote_alloc.h"


static void *sys_alloc(void *self, size_t size, size_t align)
{
    (void)self;

    if (align <= alignof(max_align_t))
    {
        return malloc(size);
    }
    void *p = nullptr;

    if (posix_memalign(&p, align, size) != 0)
    {
        return nullptr;
    }
    return p;
}

static void *sys_realloc(void *self, void *ptr, size_t old_size, size_t new_size)
{
    (void)self;
    (void)old_size;
    return realloc(ptr, new_size);
}

static void sys_free(void *self, void *ptr, size_t size)
{
    (void)self;
    (void)size;
    free(ptr);
}

static const ty_azote_Allocator g_system_allocator = {
        .self = nullptr,
        .alloc = sys_alloc,
        .realloc = sys_realloc,
        .free = sys_free,
};

const ty_azote_Allocator *ty_azote_system_allocator(void)
{
    return &g_system_allocator;
}

static const ty_azote_Allocator *g_default_allocator = &g_system_allocator;

void ty_azote_set_default_allocator(const ty_azote_Allocator *alloc)
{
    g_default_allocator = alloc ? alloc : &g_system_allocator;
}

const ty_azote_Allocator *ty_azote_default_allocator(void)
{
    return g_default_allocator;
}


void *ty_azote_try_malloc(size_t size)
{
    return g_default_allocator->alloc(g_default_allocator->self, size, alignof(max_align_t));
}

void *ty_azote_try_zalloc(size_t size)
{
    void *p = ty_azote_try_malloc(size);

    if (p)
    {
        memset(p, 0, size);
    }
    return p;
}

void *ty_azote_try_calloc(size_t count, size_t size)
{
    if (count != 0 && size > SIZE_MAX / count)
    {
        return nullptr; /* overflow guard */
    }
    return ty_azote_try_zalloc(count * size);
}

void *ty_azote_try_realloc(void *ptr, size_t new_size)
{
    return g_default_allocator->realloc(g_default_allocator->self, ptr, 0, new_size);
}

void ty_azote_free(void *ptr)
{
    g_default_allocator->free(g_default_allocator->self, ptr, 0);
}

struct ty_azote_ArenaBlock {
    ty_azote_ArenaBlock *next;
    size_t capacity;
    size_t used;
    alignas(max_align_t) unsigned char data[];
};

static ty_azote_ArenaBlock *arena_block_new(size_t capacity)
{
    ty_azote_ArenaBlock *b = malloc(sizeof *b + capacity);

    if (!b)
    {
        return nullptr;
    }
    b->next = nullptr;
    b->capacity = capacity;
    b->used = 0;
    return b;
}

ty_azote_Arena ty_azote_arena_new(size_t block_size)
{
    return (ty_azote_Arena){.head = nullptr, .block_size = block_size ? block_size : 64 * 1024};
}

static size_t align_up(size_t v, size_t align)
{
    return (v + (align - 1)) & ~(align - 1);
}

void *ty_azote_arena_alloc(ty_azote_Arena *arena, size_t size, size_t align)
{
    if (align == 0)
    {
        align = alignof(max_align_t);
    }

    if (arena->head)
    {
        size_t start = align_up(arena->head->used, align);

        if (start + size <= arena->head->capacity)
        {
            void *p = arena->head->data + start;
            arena->head->used = start + size;
            return p;
        }
    }
    size_t cap = arena->block_size;

    if (size + align > cap)
    {
        cap = size + align; /* oversized allocation gets its own block */
    }
    ty_azote_ArenaBlock *nb = arena_block_new(cap);

    if (!nb)
    {
        return nullptr;
    }
    nb->next = arena->head;
    arena->head = nb;
    size_t start = align_up(0, align);
    void *p = nb->data + start;
    nb->used = start + size;
    return p;
}

void ty_azote_arena_reset(ty_azote_Arena *arena)
{
    for (ty_azote_ArenaBlock *b = arena->head; b; b = b->next)
    {
        b->used = 0;
    }
}

void ty_azote_arena_free(ty_azote_Arena *arena)
{
    ty_azote_ArenaBlock *b = arena->head;

    while (b)
    {
        ty_azote_ArenaBlock *next = b->next;
        free(b);
        b = next;
    }
    arena->head = nullptr;
}

static void *arena_alloc_shim(void *self, size_t size, size_t align)
{
    return ty_azote_arena_alloc((ty_azote_Arena *)self, size, align);
}

static void *arena_realloc_shim(void *self, void *ptr, size_t old_size, size_t new_size)
{
    if (new_size <= old_size)
    {
        return ptr;
    }
    void *np = ty_azote_arena_alloc((ty_azote_Arena *)self, new_size, alignof(max_align_t));

    if (np && ptr)
    {
        memcpy(np, ptr, old_size);
    }
    return np;
}
static void arena_free_shim(void *self, void *ptr, size_t size)
{
    (void)self;
    (void)ptr;
    (void)size; /* arenas free in bulk only */
}

ty_azote_Allocator ty_azote_arena_as_allocator(ty_azote_Arena *arena)
{
    return (ty_azote_Allocator) {
            .self = arena,
            .alloc = arena_alloc_shim,
            .realloc = arena_realloc_shim,
            .free = arena_free_shim,
    };
}






