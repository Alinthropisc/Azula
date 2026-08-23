#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "ty_azote_types.h"

#include "ty_azote_support.h" /* ty_azote_Status */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ty_azote_Ipv4Bitset ty_azote_Ipv4Bitset; /* opaque */

[[nodiscard]]
ty_azote_Ipv4Bitset *ty_azote_ipv4_bitset_new(void);

void ty_azote_ipv4_bitset_free(ty_azote_Ipv4Bitset *bs);

[[nodiscard]]
bool ty_azote_ipv4_bitset_test(const ty_azote_Ipv4Bitset *bs, ty_azote_Ipv4Addr ip);

void ty_azote_ipv4_bitset_set(const ty_azote_Ipv4Bitset *bs, ty_azote_Ipv4Addr ip);

/* Loads newline-separated dotted-quad IPv4 addresses (optional '#'
 * comments), setting each one. Returns TY_AZOTE_ERR_PARSE on the first
 * malformed line (no log_fatal — caller decides how to react). */
[[nodiscard]]
ty_azote_Status ty_azote_ipv4_bitset_load_file(ty_azote_Ipv4Bitset *bs, const char *path, uint32_t *out_count);

#ifdef __cplusplus
}
#endif




































