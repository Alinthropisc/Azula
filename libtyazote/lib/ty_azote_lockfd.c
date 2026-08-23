#include <stdlib.h>
#include <threads.h>

#include "ty_azote_lockfd.h"

typedef struct {
    int fd;
    bool used;
    mtx_t *mutex;
} ty_azote_LockSlot;

static ty_azote_LockSlot *g_slots = nullptr;
static size_t g_cap = 0;
static size_t g_count = 0;
static mtx_t g_table_lock;
static once_flag g_once = ONCE_FLAG_INIT;

static void init_table(void)
{
    mtx_init(&g_table_lock, mtx_plain);
}

static mtx_t *find_or_create(int fd)
{
    call_once(&g_once, init_table);
    mtx_lock(&g_table_lock);

    for (size_t i = 0; i < g_count; ++i)
    {
        if (g_slots[i].used && g_slots[i].fd == fd)
        {
            mtx_t *m = g_slots[i].mutex;
            mtx_unlock(&g_table_lock);
            return m;
        }
    }

    if (g_count == g_cap)
    {
        size_t ncap = g_cap ? g_cap * 2 : 8;
        ty_azote_LockSlot *ns = realloc(g_slots, ncap * sizeof *ns);
        if (!ns) { mtx_unlock(&g_table_lock); return nullptr; }
        g_slots = ns;
        g_cap = ncap;
    }
    mtx_t *m = malloc(sizeof *m);

    if (!m)
    {
        mtx_unlock(&g_table_lock);
        return nullptr;
    }
    mtx_init(m, mtx_plain);
    g_slots[g_count++] = (ty_azote_LockSlot){.fd = fd, .used = true, .mutex = m};
    mtx_unlock(&g_table_lock);
    return m;
}

ty_azote_Status ty_azote_lock_fd(int fd)
{
    mtx_t *m = find_or_create(fd);

    if (!m)
    {
        return TY_AZOTE_ERR_NOMEM;
    }
    return mtx_lock(m) == thrd_success ? TY_AZOTE_OK : TY_AZOTE_ERR_IO;
}

ty_azote_Status ty_azote_unlock_fd(int fd)
{
    mtx_t *m = find_or_create(fd);

    if (!m)
    {
        return TY_AZOTE_ERR_NOMEM;
    }
    return mtx_unlock(m) == thrd_success ? TY_AZOTE_OK : TY_AZOTE_ERR_IO;
}

ty_azote_Status ty_azote_lock_file(FILE *f)
{
    return f ? ty_azote_lock_fd(fileno(f)) : TY_AZOTE_ERR_IO;
}

ty_azote_Status ty_azote_unlock_file(FILE *f)
{
    return f ? ty_azote_unlock_fd(fileno(f)) : TY_AZOTE_ERR_IO;
}

void ty_azote_lockfd_shutdown(void)
{
    if (!g_slots)
    {
        return;
    }
    mtx_lock(&g_table_lock);

    for (size_t i = 0; i < g_count; ++i)
    {
        mtx_destroy(g_slots[i].mutex);
        free(g_slots[i].mutex);
    }
    free(g_slots);
    g_slots = nullptr;
    g_cap = g_count = 0;
    mtx_unlock(&g_table_lock);
}












