#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "ty_azote_types.h"
#include "ty_azote_constraint.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ty_azote_BlStatus : uint8_t {
    TY_AZOTE_BL_OK = 0,
    TY_AZOTE_BL_ERR_FILE_OPEN,
    TY_AZOTE_BL_ERR_PARSE_ENTRY,
    TY_AZOTE_BL_ERR_NO_ADDRESSES,
} ty_azote_BlStatus;

typedef struct ty_azote_BlocklistConfig {
    const char *allowlist_file;      /* nullable */
    const char *blocklist_file;      /* nullable */
    const char **allowlist_entries;   /* nullable, array of CIDR/host strings */
    size_t allowlist_entries_len;
    const char **blocklist_entries;   /* nullable */
    size_t blocklist_entries_len;
    bool ignore_invalid_hosts;
} ty_azote_BlocklistConfig;

typedef struct ty_azote_Blocklist ty_azote_Blocklist; /* opaque */

[[nodiscard]]
ty_azote_BlStatus ty_azote_blocklist_init(ty_azote_Blocklist **out, const ty_azote_BlocklistConfig *cfg);

void ty_azote_blocklist_free(ty_azote_Blocklist *bl);

[[nodiscard]]
bool ty_azote_blocklist_is_allowed(const ty_azote_Blocklist *bl,ty_azote_Ipv4Addr ip);

[[nodiscard]]
uint64_t ty_azote_blocklist_count_allowed(const ty_azote_Blocklist *bl);

[[nodiscard]]
uint64_t ty_azote_blocklist_count_disallowed(const ty_azote_Blocklist *bl);

/* nth allowed address, for cyclic/sharded scan ordering (zmap's use case). */
[[nodiscard]]
ty_azote_Ipv4Addr ty_azote_blocklist_index_to_ip(const ty_azote_Blocklist *bl,uint64_t index);

/* Rank of `ip` among allowed addresses; false if ip is not allowed. */
[[nodiscard]]
bool ty_azote_blocklist_ip_to_index(const ty_azote_Blocklist *bl,ty_azote_Ipv4Addr ip, uint64_t *out_index);

/* Visitor over the resolved CIDR blocks, for logging/debug reporting. */
void ty_azote_blocklist_visit_blocked(const ty_azote_Blocklist *bl,ty_azote_ConstraintVisitFn cb, void *user_data);
void ty_azote_blocklist_visit_allowed(const ty_azote_Blocklist *bl,ty_azote_ConstraintVisitFn cb, void *user_data);

#ifdef __cplusplus
}
#endif

































