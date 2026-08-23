#include <string.h>
#include <threads.h>
#include <assert.h>

#include "ty_azote_rijndael.h"


/* ======================================================================= *
 *  Portable big-endian load/store (replaces GETU32/PUTU32's MSVC branch)
 * ======================================================================= */

static inline uint32_t load_be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static inline void store_be32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

static inline uint32_t rotr32(uint32_t v, unsigned n)
{
    return (v >> n) | (v << (32 - n));
}

/* ======================================================================= *
 *  GF(2^8) multiply (same reduction polynomial 0x11B as ty_azote_aes.c)
 * ======================================================================= */

static uint8_t gf_mul(uint8_t a, uint8_t b)
{
    uint8_t p = 0;

    for (int i = 0; i < 8 && a && b; ++i)
    {
        if (b & 1)
        {
            p ^= a;
        }
        uint8_t hi = a & 0x80;
        a = (uint8_t)(a << 1);

        if (hi)
        {
            a ^= 0x1B;
        }
        b >>= 1;
    }
    return p;
}

/* ======================================================================= *
 *  T-table generation (Lazy Singleton via call_once)
 *
 *  Te0[x] = S[x] . [02,01,01,03]   Te4[x] = S[x] broadcast to all 4 bytes
 *  Td0[x] = Si[x]. [0e,09,0d,0b]   Td4[x] = Si[x] broadcast to all 4 bytes
 *  Te{1,2,3} = ROTR32(Te0, 8/16/24); Td{1,2,3} likewise (computed on the
 *  fly in the round functions below — not stored).
 * ======================================================================= */

static uint32_t g_Te0[256], g_Te4[256];
static uint32_t g_Td0[256], g_Td4[256];
static once_flag g_tables_once = ONCE_FLAG_INIT;

static void init_tables(void)
{
    const uint8_t *sbox     = ty_azote_aes_sbox();
    const uint8_t *inv_sbox = ty_azote_aes_inv_sbox();

    for (int x = 0; x < 256; ++x)
    {
        uint8_t s = sbox[x];
        g_Te4[x] = ((uint32_t)s << 24) | ((uint32_t)s << 16) | ((uint32_t)s << 8)  | s;
        uint8_t b0 = gf_mul(s, 0x02), b1 = s, b2 = s, b3 = gf_mul(s, 0x03);
        g_Te0[x] = ((uint32_t)b0 << 24) | ((uint32_t)b1 << 16) | ((uint32_t)b2 << 8)  | b3;
        uint8_t si = inv_sbox[x];
        g_Td4[x] = ((uint32_t)si << 24) | ((uint32_t)si << 16) | ((uint32_t)si << 8)  | si;
        uint8_t c0 = gf_mul(si, 0x0e), c1 = gf_mul(si, 0x09),c2 = gf_mul(si, 0x0d), c3 = gf_mul(si, 0x0b);
        g_Td0[x] = ((uint32_t)c0 << 24) | ((uint32_t)c1 << 16) | ((uint32_t)c2 << 8)  | c3;
    }
    /* Design-by-contract: cross-check against known FIPS-197 constants so
     * any future refactor of gf_mul()/S-box generation trips immediately,
     * rather than silently producing wrong ciphertext. */
    assert(g_Te0[0] == 0xc66363a5u && "Te0 table generation self-check failed");
    assert(g_Td0[0] == 0x51f4a750u && "Td0 table generation self-check failed");
}

static inline void ensure_tables(void)
{
    call_once(&g_tables_once, init_tables);
}

/* ======================================================================= *
 *  Round-function helpers (Template Method: one shape, cyclic arguments)
 * ======================================================================= */

static inline uint32_t te_round_word(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    return g_Te0[(a >> 24) & 0xff] ^ rotr32(g_Te0[(b >> 16) & 0xff], 8) ^ rotr32(g_Te0[(c >> 8) & 0xff], 16) ^ rotr32(g_Te0[d & 0xff], 24);
}

static inline uint32_t final_fwd_word(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    return (g_Te4[(a >> 24) & 0xff] & 0xff000000u) ^ (g_Te4[(b >> 16) & 0xff] & 0x00ff0000u) ^ (g_Te4[(c >> 8) & 0xff] & 0x0000ff00u) ^ (g_Te4[d & 0xff] & 0x000000ffu);
}

static inline uint32_t td_round_word(uint32_t a, uint32_t d, uint32_t c, uint32_t b)
{
    return g_Td0[(a >> 24) & 0xff] ^ rotr32(g_Td0[(d >> 16) & 0xff], 8) ^ rotr32(g_Td0[(c >> 8)  & 0xff], 16) ^ rotr32(g_Td0[b & 0xff], 24);
}

static inline uint32_t final_inv_word(uint32_t a, uint32_t d, uint32_t c, uint32_t b)
{
    return (g_Td4[(a >> 24) & 0xff] & 0xff000000u) ^ (g_Td4[(d >> 16) & 0xff] & 0x00ff0000u) ^ (g_Td4[(c >> 8)  & 0xff] & 0x0000ff00u) ^ (g_Td4[b & 0xff] & 0x000000ffu);
}

/* InvMixColumns applied to a single round-key word, expressed via the
 * classic identity Td0[S[b]] == InvMixColumn(b) — avoids needing a
 * separate step to compute the inverse S-box byte explicitly. */
static inline uint32_t inv_mix_column_word(uint32_t w)
{
    uint8_t b0 = (uint8_t)(w >> 24), b1 = (uint8_t)(w >> 16),b2 = (uint8_t)(w >> 8),  b3 = (uint8_t)w;
    uint8_t y0 = (uint8_t)g_Te4[b0]; /* == sbox[b0] (Te4 broadcasts sbox) */
    uint8_t y1 = (uint8_t)g_Te4[b1];
    uint8_t y2 = (uint8_t)g_Te4[b2];
    uint8_t y3 = (uint8_t)g_Te4[b3];
    return g_Td0[y0] ^ rotr32(g_Td0[y1], 8) ^
    rotr32(g_Td0[y2], 16) ^ rotr32(g_Td0[y3], 24);
}

/* ======================================================================= *
 *  Key setup
 * ======================================================================= */

ty_azote_AesStatus ty_azote_rijndael_key_setup_enc(ty_azote_AesKey *key, const uint8_t *cipher_key, ty_azote_AesKeyBits bits)
{
    ensure_tables();
    /* The FIPS-197 key-expansion arithmetic is backend-independent — the
     * same round-key words feed the naive per-byte cipher in
     * ty_azote_aes.c, AES-NI/ARM-CE in ty_azote_aes128.c, and the T-table
     * cipher here. No need to duplicate that logic (DRY). */
    return ty_azote_aes_key_init(key, cipher_key, bits);
}

ty_azote_AesStatus ty_azote_rijndael_key_setup_dec(ty_azote_AesKey *key, const uint8_t *cipher_key, ty_azote_AesKeyBits bits)
{
    ensure_tables();
    ty_azote_AesStatus st = ty_azote_aes_key_init(key, cipher_key, bits);

    if (st != TY_AZOTE_AES_OK)
    {
        return st;
    }
    int nr = key->rounds;
    uint32_t *rk = key->round_keys;

    /* The T-table decrypt routine uses the "equivalent inverse cipher"
     * structure (FIPS-197 5.3.5), which requires: (1) reversing the
     * round-key order, and (2) applying InvMixColumns to every round key
     * except the first and last. */
    for (int i = 0, j = 4 * nr; i < j; i += 4, j -= 4)
    {
        for (int k = 0; k < 4; ++k)
        {
            uint32_t tmp = rk[i + k];
            rk[i + k] = rk[j + k];
            rk[j + k] = tmp;
        }
    }
    for (int i = 1; i < nr; ++i)
    {
        uint32_t *w = rk + 4 * i;

        for (int k = 0; k < 4; ++k)
        {
            w[k] = inv_mix_column_word(w[k]);
        }
    }
    return TY_AZOTE_AES_OK;
}

/* ======================================================================= *
 *  Encrypt / decrypt (compact "Nr-1 full rounds" loop — original's
 *  non-FULL_UNROLL branch; modern compilers unroll this just as
 *  effectively at -O2/-O3 while keeping the source auditable)
 * ======================================================================= */

void ty_azote_rijndael_encrypt_block(const ty_azote_AesKey *key,const uint8_t in[static 16], uint8_t out[static 16])
{
    const uint32_t *rk = key->round_keys;
    int r = key->rounds >> 1;
    uint32_t s0 = load_be32(in)      ^ rk[0];
    uint32_t s1 = load_be32(in + 4)  ^ rk[1];
    uint32_t s2 = load_be32(in + 8)  ^ rk[2];
    uint32_t s3 = load_be32(in + 12) ^ rk[3];
    uint32_t t0, t1, t2, t3;

    for (;;)
    {
        t0 = te_round_word(s0, s1, s2, s3) ^ rk[4];
        t1 = te_round_word(s1, s2, s3, s0) ^ rk[5];
        t2 = te_round_word(s2, s3, s0, s1) ^ rk[6];
        t3 = te_round_word(s3, s0, s1, s2) ^ rk[7];
        rk += 8

        if (--r == 0)
        {
            break;
        }
        s0 = te_round_word(t0, t1, t2, t3) ^ rk[0];
        s1 = te_round_word(t1, t2, t3, t0) ^ rk[1];
        s2 = te_round_word(t2, t3, t0, t1) ^ rk[2];
        s3 = te_round_word(t3, t0, t1, t2) ^ rk[3];
    }
    store_be32(out,      final_fwd_word(t0, t1, t2, t3) ^ rk[0]);
    store_be32(out + 4,  final_fwd_word(t1, t2, t3, t0) ^ rk[1]);
    store_be32(out + 8,  final_fwd_word(t2, t3, t0, t1) ^ rk[2]);
    store_be32(out + 12, final_fwd_word(t3, t0, t1, t2) ^ rk[3]);
}

void ty_azote_rijndael_decrypt_block(const ty_azote_AesKey *key,const uint8_t in[static 16], uint8_t out[static 16])
{
    const uint32_t *rk = key->round_keys;
    int r = key->rounds >> 1;
    uint32_t s0 = load_be32(in)      ^ rk[0];
    uint32_t s1 = load_be32(in + 4)  ^ rk[1];
    uint32_t s2 = load_be32(in + 8)  ^ rk[2];
    uint32_t s3 = load_be32(in + 12) ^ rk[3];
    uint32_t t0, t1, t2, t3;

    for (;;)
    {
        t0 = td_round_word(s0, s3, s2, s1) ^ rk[4];
        t1 = td_round_word(s1, s0, s3, s2) ^ rk[5];
        t2 = td_round_word(s2, s1, s0, s3) ^ rk[6];
        t3 = td_round_word(s3, s2, s1, s0) ^ rk[7];
        rk += 8;

        if (--r == 0)
        {
            break;
        }
        s0 = td_round_word(t0, t3, t2, t1) ^ rk[0];
        s1 = td_round_word(t1, t0, t3, t2) ^ rk[1];
        s2 = td_round_word(t2, t1, t0, t3) ^ rk[2];
        s3 = td_round_word(t3, t2, t1, t0) ^ rk[3];
    }
    store_be32(out, final_inv_word(t0, t3, t2, t1) ^ rk[0]);
    store_be32(out + 4, final_inv_word(t1, t0, t3, t2) ^ rk[1]);
    store_be32(out + 8, final_inv_word(t2, t1, t0, t3) ^ rk[2]);
    store_be32(out + 12, final_inv_word(t3, t2, t1, t0) ^ rk[3]);
}

/* ======================================================================= *
 *  Self-test — FIPS-197 Appendix B (AES-128) and Appendix C.2/C.3
 *  (AES-192/256). The original zmap only ever exercised the 128-bit path;
 *  covering all three key sizes here catches key-schedule regressions
 *  that a 128-only test would miss.
 * ======================================================================= */

typedef struct {
    ty_azote_AesKeyBits bits;
    uint8_t key[32];
    uint8_t pt[16];
    uint8_t ct[16];
} SelftestVector;

static const SelftestVector g_vectors[] = {
        {
                .bits = TY_AZOTE_AES_128,
                .key = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f},
                .pt  = {0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff},
                .ct  = {0x69,0xc4,0xe0,0xd8,0x6a,0x7b,0x04,0x30,0xd8,0xcd,0xb7,0x80,0x70,0xb4,0xc5,0x5a},
        },
        {
                .bits = TY_AZOTE_AES_192,
                .key = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17},
                .pt  = {0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff},
                .ct  = {0xdd,0xa9,0x7c,0xa4,0x86,0x4c,0xdf,0xe0,0x6e,0xaf,0x70,0xa0,0xec,0x0d,0x71,0x91},
        },
        {
                .bits = TY_AZOTE_AES_256,
                .key = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f},
                .pt  = {0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff},
                .ct  = {0x8e,0xa2,0xb7,0xca,0x51,0x67,0x45,0xbf,0xea,0xfc,0x49,0x90,0x4b,0x49,0x60,0x89},
        },
};

bool ty_azote_rijndael_selftest(void)
{
    for (size_t i = 0; i < sizeof g_vectors / sizeof g_vectors[0]; ++i)
    {
        const SelftestVector *v = &g_vectors[i];
        ty_azote_AesKey enc_key, dec_key;

        if (ty_azote_rijndael_key_setup_enc(&enc_key, v->key, v->bits) != TY_AZOTE_AES_OK)
        {
            return false;
        }
        if (ty_azote_rijndael_key_setup_dec(&dec_key, v->key, v->bits) != TY_AZOTE_AES_OK)
        {
            return false;
        }
        uint8_t ct[16], pt[16];
        ty_azote_rijndael_encrypt_block(&enc_key, v->pt, ct);

        if (memcmp(ct, v->ct, 16) != 0)
        {
            return false;
        }
        ty_azote_rijndael_decrypt_block(&dec_key, ct, pt);

        if (memcmp(pt, v->pt, 16) != 0)
        {
            return false;
        }

    }
    return true;
}