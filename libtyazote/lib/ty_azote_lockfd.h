#pragma once

#include <stdio.h>
#include <stdbool.h>

#include "ty_azote_support.h" /* ty_azote_Status */

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__GNUC__) || defined(__clang__)
#define TY_AZOTE_CLEANUP(f) __attribute__((cleanup(f)))
#else
#define TY_AZOTE_CLEANUP(f)
#endif

[[nodiscard]]
ty_azote_Status ty_azote_lock_fd(int fd);

[[nodiscard]]
ty_azote_Status ty_azote_unlock_fd(int fd);

[[nodiscard]]
ty_azote_Status ty_azote_lock_file(FILE *f);

[[nodiscard]]
ty_azote_Status ty_azote_unlock_file(FILE *f);

/* Releases the registry's internal bookkeeping (call at process exit for
 * clean leak-checker runs; does NOT close the fds/streams themselves). */
void ty_azote_lockfd_shutdown(void);

/* RAII guard over a FILE* stream. */
typedef struct ty_azote_FileLockGuard {
    FILE *f;
    bool locked;
} ty_azote_FileLockGuard;

static inline ty_azote_FileLockGuard ty_azote_file_lock_guard_acquire(FILE *f)
{
    return (ty_azote_FileLockGuard){.f = f, .locked = ty_azote_lock_file(f) == TY_AZOTE_OK};
}

static inline void ty_azote_file_lock_guard_release(ty_azote_FileLockGuard *g)
{
    if (g->locked)
    {
        ty_azote_unlock_file(g->f);
    }
}
#define TY_AZOTE_FILE_LOCK_GUARD(name_, f_) [[maybe_unused]] TY_AZOTE_CLEANUP(ty_azote_file_lock_guard_release) ty_azote_FileLockGuard name_ = ty_azote_file_lock_guard_acquire(f_)

#ifdef __cplusplus
}
#endif






































