#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__GNUC__) || defined(__clang__)
#define TY_AZOTE_CLEANUP(f) __attribute__((cleanup(f)))
#else
#define TY_AZOTE_CLEANUP(f)
#endif

constexpr size_t TY_AZOTE_AES128_KEY_LEN   = 16;
constexpr size_t TY_AZOTE_AES128_BLOCK_LEN = 16;

typedef struct ty_azote_Aes128Ctx ty_azote_Aes128Ctx; /* opaque */

typedef enum ty_azote_Aes128Backend : uint8_t {
    TY_AZOTE_AES128_BACKEND_SOFTWARE,
    TY_AZOTE_AES128_BACKEND_AESNI,
    TY_AZOTE_AES128_BACKEND_ARM_CE,
} ty_azote_Aes128Backend;

[[nodiscard]]
ty_azote_Aes128Ctx *ty_azote_aes128_new(const uint8_t key[static TY_AZOTE_AES128_KEY_LEN]);

void ty_azote_aes128_free(ty_azote_Aes128Ctx *ctx);

void ty_azote_aes128_encrypt_block(const ty_azote_Aes128Ctx *ctx,const uint8_t pt[static TY_AZOTE_AES128_BLOCK_LEN],uint8_t ct[static TY_AZOTE_AES128_BLOCK_LEN]);

[[nodiscard]]
ty_azote_Aes128Backend ty_azote_aes128_active_backend(void);

[[nodiscard]]
const char *ty_azote_aes128_backend_name(ty_azote_Aes128Backend backend);

/* FIPS-197 appendix C test vector. Pure — never aborts the process. */
[[nodiscard]]
bool ty_azote_aes128_selftest(void);

static inline void ty_azote_aes128_cleanup_(ty_azote_Aes128Ctx **c)
{
    if (*c)
	{
		ty_azote_aes128_free(*c);
	}
}
#define TY_AZOTE_AES128_GUARD(name_, key_) [[maybe_unused]] TY_AZOTE_CLEANUP(ty_azote_aes128_cleanup_) ty_azote_Aes128Ctx *name_ = ty_azote_aes128_new(key_)

#ifdef __cplusplus
}
#endif






























