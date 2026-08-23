#include "ty_azote_alloc.h"
#include "ty_azote_tracing.h"

void *ty_azote_malloc(size_t size)
{
    void *p = ty_azote_try_malloc(size);

    if (!p)
    {
        TY_AZOTE_FATAL("out of memory: malloc(%zu) failed", size);
    }
    return p;
}

void *ty_azote_zalloc(size_t size)
{
    void *p = ty_azote_try_zalloc(size);

    if (!p)
    {
        TY_AZOTE_FATAL("out of memory: zalloc(%zu) failed", size);
    }
    return p;
}

void *ty_azote_calloc(size_t count, size_t size)
{
    void *p = ty_azote_try_calloc(count, size);

    if (!p)
    {
        TY_AZOTE_FATAL("out of memory: calloc(%zu, %zu) failed", count, size);
    }
    return p;
}

void *ty_azote_realloc(void *ptr, size_t new_size)
{
    void *p = ty_azote_try_realloc(ptr, new_size);

    if (!p && new_size != 0)
    {
        TY_AZOTE_FATAL("out of memory: realloc(%zu) failed", new_size);
    }
    return p;
}