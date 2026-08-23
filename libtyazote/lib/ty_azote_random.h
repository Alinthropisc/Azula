#pragma once



#include <stddef.h>
#include <stdint.h>
#include "ty_azote_support.h" /* ty_azote_Status */

#ifdef __cplusplus
extern "C" {
#endif

[[nodiscard]]
ty_azote_Status ty_azote_random_bytes(void *dst, size_t n);

[[nodiscard]]
uint32_t ty_azote_random_u32(void);

[[nodiscard]]
uint64_t ty_azote_random_u64(void);

#ifdef __cplusplus
}
#endif









