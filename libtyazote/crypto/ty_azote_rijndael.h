#pragma once

#include <stdbool.h>

#include "ty_azote_aes.h" /* ty_azote_AesKey, ty_azote_AesKeyBits, ty_azote_AesStatus */


#ifdef __cplusplus
extern "C" {
#endif

[[nodiscard]]
ty_azote_AesStatus ty_azote_rijndael_key_setup_enc(ty_azote_AesKey *key, const uint8_t *cipher_key, ty_azote_AesKeyBits bits);

[[nodiscard]]
ty_azote_AesStatus ty_azote_rijndael_key_setup_dec(ty_azote_AesKey *key, const uint8_t *cipher_key, ty_azote_AesKeyBits bits);

void ty_azote_rijndael_encrypt_block(const ty_azote_AesKey *key,const uint8_t in[static TY_AZOTE_AES_BLOCK_LEN],uint8_t out[static TY_AZOTE_AES_BLOCK_LEN]);

void ty_azote_rijndael_decrypt_block(const ty_azote_AesKey *key,const uint8_t in[static TY_AZOTE_AES_BLOCK_LEN],uint8_t out[static TY_AZOTE_AES_BLOCK_LEN]);

/* FIPS-197 test vectors for AES-128/192/256 (Appendix B/C). Pure — never
 * aborts the process. */
[[nodiscard]]
bool ty_azote_rijndael_selftest(void);

#ifdef __cplusplus
}
#endif





































