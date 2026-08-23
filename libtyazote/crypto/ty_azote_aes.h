#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

constexpr size_t TY_AZOTE_AES_BLOCK_LEN = 16;
constexpr int TY_AZOTE_AES_MAX_ROUNDS = 14;

typedef enum ty_azote_AesKeyBits : uint16_t {
    TY_AZOTE_AES_128 = 128,
    TY_AZOTE_AES_192 = 192,
    TY_AZOTE_AES_256 = 256,
} ty_azote_AesKeyBits;

typedef struct ty_azote_AesKey {
    uint32_t round_keys[4 * (TY_AZOTE_AES_MAX_ROUNDS + 1)];
    int rounds;
} ty_azote_AesKey;

typedef enum ty_azote_AesStatus : uint8_t {
    TY_AZOTE_AES_OK = 0,
    TY_AZOTE_AES_ERR_BAD_KEY_SIZE,
} ty_azote_AesStatus;

/* Factory: derives round keys for the given key size (128/192/256 bits). */
[[nodiscard]]
ty_azote_AesStatus ty_azote_aes_key_init(ty_azote_AesKey *key, const uint8_t *key_bytes, ty_azote_AesKeyBits bits);

void ty_azote_aes_encrypt_block(const ty_azote_AesKey *key,const uint8_t in[static TY_AZOTE_AES_BLOCK_LEN],uint8_t out[static TY_AZOTE_AES_BLOCK_LEN]);
void ty_azote_aes_decrypt_block(const ty_azote_AesKey *key,const uint8_t in[static TY_AZOTE_AES_BLOCK_LEN],uint8_t out[static TY_AZOTE_AES_BLOCK_LEN]);

/* Add to ty_azote_aes.h public API — lets ty_azote_aes128's ARM-CE key
 * schedule reuse the already-generated S-box instead of duplicating it. */
[[nodiscard]]
const uint8_t *ty_azote_aes_sbox(void);   /* ensures init, 256 bytes  */

[[nodiscard]]
const uint8_t *ty_azote_aes_rcon(void);   /* ensures init, [1..14]    */

#ifdef __cplusplus
}
#endif
































