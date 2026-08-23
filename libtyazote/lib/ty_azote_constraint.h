#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "ty_azote_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ty_azote_AddrState : uint8_t {
    TY_AZOTE_ADDR_DISALLOWED = 0,
    TY_AZOTE_ADDR_ALLOWED    = 1,
} ty_azote_AddrState;

typedef struct ty_azote_Constraint ty_azote_Constraint; /* opaque */

/* Builder phase: root state defaults to `default_state`. */
[[nodiscard]]
ty_azote_Constraint *ty_azote_constraint_new(ty_azote_AddrState default_state);

/* Overrides the state of the CIDR block ip_host/prefix_len (host byte order).
 * May be called repeatedly; later calls override overlapping earlier ones. */
void ty_azote_constraint_set(ty_azote_Constraint *c, uint32_t ip_host,int prefix_len, ty_azote_AddrState state);

/* Finalizes the trie: computes per-subtree allowed-address counts.
 * Must be called once, after all `set()` calls, before any query below. */
void ty_azote_constraint_compile(ty_azote_Constraint *c);
void ty_azote_constraint_free(ty_azote_Constraint *c);

/* --- Queries (valid only after compile()) --- */

[[nodiscard]]
ty_azote_AddrState ty_azote_constraint_state(const ty_azote_Constraint *c,uint32_t ip_host);

[[nodiscard]]
uint64_t ty_azote_constraint_count(const ty_azote_Constraint *c,ty_azote_AddrState state);

/* nth (0-based) ALLOWED address in ascending order, host byte order.
 * `index` must be < count(ALLOWED). */
[[nodiscard]]
uint32_t ty_azote_constraint_index_to_ip(const ty_azote_Constraint *c,uint64_t index);

/* Rank of `ip_host` among ALLOWED addresses. Returns false if ip is not
 * itself ALLOWED (out param left untouched). */
[[nodiscard]]
bool ty_azote_constraint_ip_to_index(const ty_azote_Constraint *c,uint32_t ip_host, uint64_t *out_index);

/* Visitor (Composite traversal) over maximal uniform CIDR blocks matching
 * `state`, in ascending address order — replaces zmap's parallel bl_ll_t
 * bookkeeping linked lists with a single source of truth (the trie itself). */
typedef void (*ty_azote_ConstraintVisitFn)(uint32_t net_host, int prefix_len, void *user_data);

void ty_azote_constraint_visit(const ty_azote_Constraint *c, ty_azote_AddrState state,ty_azote_ConstraintVisitFn cb, void *user_data);

#ifdef __cplusplus
}
#endif


























