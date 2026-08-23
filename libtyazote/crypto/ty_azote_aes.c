#include <string.h>
#include <threads.h>

#include "ty_azote_aes.h"

/* ======================================================================= *
 *  GF(2^8) arithmetic (Rijndael field, reduction polynomial 0x11B)
 * ======================================================================= */

static uint8_t gf_xtime(uint8_t a)
{
    uint8_t hi = a & 0x80;
    uint8_t r  = (uint8_t)(a << 1);
    return hi ? (uint8_t)(r ^ 0x1B) : r;
}

/* Add to ty_azote_aes.c */
const uint8_t *ty_azote_aes_sbox(void)
{
    ensure_tables();
    return g_sbox;
}

const uint8_t *ty_azote_aes_rcon(void)
{
    ensure_tables();
    return g_rcon;
}

static uint8_t gf_mul(uint8_t a, uint8_t b)
{
    uint8_t p = 0;

    for (int i = 0; i < 8 && a && b; ++i)
    {
        if (b & 1)
        {
            p ^= a;
        }
        a = gf_xtime(a);
        b >>= 1;
    }
    return p;
}

/* ======================================================================= *
 *  S-box generation (Lazy Singleton via call_once) — avoids hand-transcribed
 *  256-entry lookup tables, computed once per process from first principles.
 * ======================================================================= */

static uint8_t g_sbox[256];
static uint8_t g_inv_sbox[256];
static uint8_t g_rcon[15]; /* rcon[1..14] used; rcon[0] unused */
static once_flag g_tables_once = ONCE_FLAG_INIT;

static uint8_t gf_inverse(uint8_t a)
{
    if (a == 0)
    {
        return 0; /* by convention in AES S-box construction */
    }
    for (uint8_t x = 1;; ++x)
    {
        if (gf_mul(a, x) == 1)
        {
            return x;
        }
        if (x == 0xFF)
        {
            break;
        }
    }
    return 0;
}

static uint8_t rotl8(uint8_t v, int shift)
{
    return (uint8_t)((v << shift) | (v >> (8 - shift)));
}

static void init_tables(void)
{
    for (int a = 0; a < 256; ++a)
    {
        uint8_t inv = gf_inverse((uint8_t)a);
        /* AES affine transform: s = inv ^ rotl(inv,1) ^ rotl(inv,2) ^
         * rotl(inv,3) ^ rotl(inv,4) ^ 0x63 */
        uint8_t s = (uint8_t)(inv ^ rotl8(inv, 1) ^ rotl8(inv, 2) ^ rotl8(inv, 3) ^ rotl8(inv, 4) ^ 0x63);
        g_sbox[a] = s;
        g_inv_sbox[s] = (uint8_t)a;
    }
    g_rcon[1] = 0x01;

    for (int i = 2; i <= 14; ++i)
    {
        g_rcon[i] = gf_xtime(g_rcon[i - 1]);
    }
}

static void ensure_tables(void)
{
    call_once(&g_tables_once, init_tables);
}

/* ======================================================================= *
 *  Key schedule (FIPS-197 KeyExpansion)
 * ======================================================================= */

static uint32_t sub_word(uint32_t w)
{
    return ((uint32_t)g_sbox[(w >> 24) & 0xFF] << 24) | ((uint32_t)g_sbox[(w >> 16) & 0xFF] << 16) | ((uint32_t)g_sbox[(w >> 8)  & 0xFF] << 8) | ((uint32_t)g_sbox[w & 0xFF]);
}

static uint32_t rot_word(uint32_t w)
{
    return (w << 8) | (w >> 24);
}

ty_azote_AesStatus ty_azote_aes_key_init(ty_azote_AesKey *key, const uint8_t *key_bytes,ty_azote_AesKeyBits bits)
{
    ensure_tables();

    int nk;

    switch (bits)
    {
        case TY_AZOTE_AES_128:
            nk = 4;
            break;
        case TY_AZOTE_AES_192:
            nk = 6;
            break;
        case TY_AZOTE_AES_256:
            nk = 8;
            break;
        default:
            return TY_AZOTE_AES_ERR_BAD_KEY_SIZE;
    }
    int nr = nk + 6;
    key->rounds = nr;
    uint32_t *w = key->round_keys;

    for (int i = 0; i < nk; ++i)
    {
        w[i] = ((uint32_t)key_bytes[4 * i] << 24) | ((uint32_t)key_bytes[4 * i + 1] << 16) | ((uint32_t)key_bytes[4 * i + 2] << 8) | key_bytes[4 * i + 3];
    }
    int total_words = 4 * (nr + 1);

    for (int i = nk; i < total_words; ++i)
    {
        uint32_t temp = w[i - 1];

        if (i % nk == 0)
        {
            temp = sub_word(rot_word(temp)) ^ ((uint32_t)g_rcon[i / nk] << 24);
        }
        else if (nk > 6 && i % nk == 4)
        {
            temp = sub_word(temp);
        }
        w[i] = w[i - nk] ^ temp;
    }
    return TY_AZOTE_AES_OK;
}

/* ======================================================================= *
 *  State helpers: state[r][c] = byte[r + 4c]  (FIPS-197 column-major layout)
 * ======================================================================= */

typedef uint8_t State[4][4];

static void bytes_to_state(const uint8_t in[16], State s)
{
    for (int c = 0; c < 4; ++c)
    {
        for (int r = 0; r < 4; ++r)
        {
            s[r][c] = in[r + 4 * c];
        }
    }
}

static void state_to_bytes(const State s, uint8_t out[16])
{
    for (int c = 0; c < 4; ++c)
    {
        for (int r = 0; r < 4; ++r)
        {
            out[r + 4 * c] = s[r][c];
        }
    }
}

static void add_round_key(State s, const uint32_t *rk_words)
{
    for (int c = 0; c < 4; ++c)
    {
        uint32_t w = rk_words[c];
        s[0][c] ^= (uint8_t)(w >> 24);
        s[1][c] ^= (uint8_t)(w >> 16);
        s[2][c] ^= (uint8_t)(w >> 8);
        s[3][c] ^= (uint8_t)(w);
    }
}

static void sub_bytes(State s)
{
    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 4; ++c)
        {
            s[r][c] = g_sbox[s[r][c]];
        }
    }
}
static void inv_sub_bytes(State s)
{
    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 4; ++c)
        {
            s[r][c] = g_inv_sbox[s[r][c]];
        }
    }
}

static void shift_rows(State s)
{
    for (int r = 1; r < 4; ++r)
    {
        uint8_t tmp[4];

        for (int c = 0; c < 4; ++c)
        {
            tmp[c] = s[r][(c + r) % 4];
        }
        memcpy(s[r], tmp, 4);
    }
}
static void inv_shift_rows(State s)
{
    for (int r = 1; r < 4; ++r)
    {
        uint8_t tmp[4];

        for (int c = 0; c < 4; ++c)
        {
            tmp[c] = s[r][(c - r + 4) % 4];
        }
        memcpy(s[r], tmp, 4);
    }
}

static void mix_columns(State s)
{
    for (int c = 0; c < 4; ++c)
    {
        uint8_t s0 = s[0][c], s1 = s[1][c], s2 = s[2][c], s3 = s[3][c];
        s[0][c] = (uint8_t)(gf_xtime(s0) ^ (gf_xtime(s1) ^ s1) ^ s2 ^ s3);
        s[1][c] = (uint8_t)(s0 ^ gf_xtime(s1) ^ (gf_xtime(s2) ^ s2) ^ s3);
        s[2][c] = (uint8_t)(s0 ^ s1 ^ gf_xtime(s2) ^ (gf_xtime(s3) ^ s3));
        s[3][c] = (uint8_t)((gf_xtime(s0) ^ s0) ^ s1 ^ s2 ^ gf_xtime(s3));
    }
}

static void inv_mix_columns(State s)
{
    for (int c = 0; c < 4; ++c)
    {
        uint8_t s0 = s[0][c], s1 = s[1][c], s2 = s[2][c], s3 = s[3][c];
        s[0][c] = (uint8_t)(gf_mul(s0, 14) ^ gf_mul(s1, 11) ^ gf_mul(s2, 13) ^ gf_mul(s3, 9));
        s[1][c] = (uint8_t)(gf_mul(s0, 9)  ^ gf_mul(s1, 14) ^ gf_mul(s2, 11) ^ gf_mul(s3, 13));
        s[2][c] = (uint8_t)(gf_mul(s0, 13) ^ gf_mul(s1, 9)  ^ gf_mul(s2, 14) ^ gf_mul(s3, 11));
        s[3][c] = (uint8_t)(gf_mul(s0, 11) ^ gf_mul(s1, 13) ^ gf_mul(s2, 9)  ^ gf_mul(s3, 14));
    }
}

/* ======================================================================= *
 *  Public API
 * ======================================================================= */

void ty_azote_aes_encrypt_block(const ty_azote_AesKey *key, const uint8_t in[16], uint8_t out[16])
{
    State s;
    bytes_to_state(in, s);
    add_round_key(s, &key->round_keys[0]);

    for (int round = 1; round < key->rounds; ++round)
    {
        sub_bytes(s);
        shift_rows(s);
        mix_columns(s);
        add_round_key(s, &key->round_keys[4 * round]);
    }
    sub_bytes(s);
    shift_rows(s);
    add_round_key(s, &key->round_keys[4 * key->rounds]);
    state_to_bytes(s, out);
}

void ty_azote_aes_decrypt_block(const ty_azote_AesKey *key, const uint8_t in[16], uint8_t out[16])
{
    State s;
    bytes_to_state(in, s);
    add_round_key(s, &key->round_keys[4 * key->rounds]);

    for (int round = key->rounds - 1; round >= 1; --round)
    {
        inv_shift_rows(s);
        inv_sub_bytes(s);
        add_round_key(s, &key->round_keys[4 * round]);
        inv_mix_columns(s);
    }
    inv_shift_rows(s);
    inv_sub_bytes(s);
    add_round_key(s, &key->round_keys[0]);
    state_to_bytes(s, out);
}