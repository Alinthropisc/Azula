#include <stdlib.h>
#include <string.h>
#include <threads.h>

#include "ty_azote_aes128.h"
#include "ty_azote_aes.h"
#include "../ty_azote_tracing.h"
#include "ty_azote_rijndael.h"


#define AES128_ROUNDS 10

#if defined(__x86_64__) || defined(_M_X64)
#define TY_AZOTE_AES128_HAVE_NI 1
#include <immintrin.h>
#include <cpuid.h>
#elif defined(__aarch64__)
#define TY_AZOTE_AES128_HAVE_ARM_CE 1
#ifdef __ARM_NEON
#include <arm_neon.h>
#endif
#if defined(__linux__)
#include <sys/auxv.h>
#ifndef HWCAP_AES
#define HWCAP_AES (1 << 3)
#endif
#elif defined(__APPLE__)
#include <sys/sysctl.h>
#endif
#endif

/* ======================================================================= *
 *  Backend detection (Singleton via call_once)
 * ======================================================================= */

static once_flag g_backend_once = ONCE_FLAG_INIT;
static ty_azote_Aes128Backend g_backend = TY_AZOTE_AES128_BACKEND_SOFTWARE;

static void detect_backend(void)
{
#if defined(TY_AZOTE_AES128_HAVE_NI)
    unsigned eax, ebx, ecx, edx;

	if (__get_cpuid(1, &eax, &ebx, &ecx, &edx) && (ecx & (1u << 25)))
    {
		g_backend = TY_AZOTE_AES128_BACKEND_AESNI;
	}
#elif defined(TY_AZOTE_AES128_HAVE_ARM_CE)
    #  if defined(__linux__)
	if (getauxval(AT_HWCAP) & HWCAP_AES)
    {
        g_backend = TY_AZOTE_AES128_BACKEND_ARM_CE;
    }
#  elif defined(__APPLE__)
	int v = 0; size_t sz = sizeof v;

	if (sysctlbyname("hw.optional.arm.FEAT_AES", &v, &sz, nullptr, 0) == 0 && v)
    {
        g_backend = TY_AZOTE_AES128_BACKEND_ARM_CE;
    }
#  endif
#endif
    TY_AZOTE_DEBUG("ty_azote_aes128: selected backend = %s",ty_azote_aes128_backend_name(g_backend));
}

ty_azote_Aes128Backend ty_azote_aes128_active_backend(void)
{
    call_once(&g_backend_once, detect_backend);
    return g_backend;
}

const char *ty_azote_aes128_backend_name(ty_azote_Aes128Backend b)
{
    switch (b)
    {
        case TY_AZOTE_AES128_BACKEND_AESNI:
            return "AES-NI";
        case TY_AZOTE_AES128_BACKEND_ARM_CE:
            return "ARMv8 Crypto Extensions";
        default:
            return "software";
    }
}

/* ======================================================================= *
 *  Context
 * ======================================================================= */

struct ty_azote_Aes128Ctx {
    union {
        ty_azote_AesKey sw;
#if defined(TY_AZOTE_AES128_HAVE_NI)
        __m128i ni_rk[AES128_ROUNDS + 1];
#elif defined(TY_AZOTE_AES128_HAVE_ARM_CE)
        uint8_t arm_rk[AES128_ROUNDS + 1][16];
#endif
    } u;
};

#if defined(TY_AZOTE_AES128_HAVE_NI)
[[gnu::target("sse2,aes")]]
static __m128i ni_expand_step(__m128i rk, __m128i rc)
{
	rc = _mm_shuffle_epi32(rc, _MM_SHUFFLE(3, 3, 3, 3));
	rk = _mm_xor_si128(rk, _mm_slli_si128(rk, 4));
	rk = _mm_xor_si128(rk, _mm_slli_si128(rk, 4));
	rk = _mm_xor_si128(rk, _mm_slli_si128(rk, 4));
	return _mm_xor_si128(rk, rc);
}

[[gnu::target("sse2,aes")]]
static void ni_key_schedule(const uint8_t *key, __m128i rk[static AES128_ROUNDS + 1])
{
	rk[0] = _mm_loadu_si128((const __m128i *)key);
	rk[1]  = ni_expand_step(rk[0], _mm_aeskeygenassist_si128(rk[0], 0x01));
	rk[2]  = ni_expand_step(rk[1], _mm_aeskeygenassist_si128(rk[1], 0x02));
	rk[3]  = ni_expand_step(rk[2], _mm_aeskeygenassist_si128(rk[2], 0x04));
	rk[4]  = ni_expand_step(rk[3], _mm_aeskeygenassist_si128(rk[3], 0x08));
	rk[5]  = ni_expand_step(rk[4], _mm_aeskeygenassist_si128(rk[4], 0x10));
	rk[6]  = ni_expand_step(rk[5], _mm_aeskeygenassist_si128(rk[5], 0x20));
	rk[7]  = ni_expand_step(rk[6], _mm_aeskeygenassist_si128(rk[6], 0x40));
	rk[8]  = ni_expand_step(rk[7], _mm_aeskeygenassist_si128(rk[7], 0x80));
	rk[9]  = ni_expand_step(rk[8], _mm_aeskeygenassist_si128(rk[8], 0x1B));
	rk[10] = ni_expand_step(rk[9], _mm_aeskeygenassist_si128(rk[9], 0x36));
}

[[gnu::target("sse2,aes")]]
static void ni_encrypt(const __m128i rk[static AES128_ROUNDS + 1],const uint8_t pt[16], uint8_t ct[16])
{
	__m128i b = _mm_loadu_si128((const __m128i *)pt);
	b = _mm_xor_si128(b, rk[0]);

	for (int i = 1; i < AES128_ROUNDS; ++i)
    {
        b = _mm_aesenc_si128(b, rk[i]);
    }
	b = _mm_aesenclast_si128(b, rk[AES128_ROUNDS]);
	_mm_storeu_si128((__m128i *)ct, b);
}
#endif /* HAVE_NI */

#if defined(TY_AZOTE_AES128_HAVE_ARM_CE)
[[gnu::target("+crypto")]]
static void arm_key_schedule(const uint8_t *key, uint8_t rk[static AES128_ROUNDS + 1][16])
{
	const uint8_t *sbox = ty_azote_aes_sbox();
	const uint8_t *rcon = ty_azote_aes_rcon();
	memcpy(rk[0], key, 16);

	for (int i = 1; i <= AES128_ROUNDS; ++i)
    {
		rk[i][0] = (uint8_t)(rk[i-1][0] ^ sbox[rk[i-1][13]] ^ rcon[i]);
		rk[i][1] = (uint8_t)(rk[i-1][1] ^ sbox[rk[i-1][14]]);
		rk[i][2] = (uint8_t)(rk[i-1][2] ^ sbox[rk[i-1][15]]);
		rk[i][3] = (uint8_t)(rk[i-1][3] ^ sbox[rk[i-1][12]]);

		for (int j = 4; j < 16; ++j)
        {
            rk[i][j] = (uint8_t)(rk[i-1][j] ^ rk[i][j-4]);
        }
	}
}

[[gnu::target("+crypto")]]
static void arm_encrypt(const uint8_t rk[static AES128_ROUNDS + 1][16],const uint8_t pt[16], uint8_t ct[16])
{
	uint8x16_t b = vld1q_u8(pt);

	for (int i = 0; i < AES128_ROUNDS - 1; ++i)
    {
		b = vaesmcq_u8(vaeseq_u8(b, vld1q_u8(rk[i])));
	}
	b = vaeseq_u8(b, vld1q_u8(rk[AES128_ROUNDS - 1]));
	b = veorq_u8(b, vld1q_u8(rk[AES128_ROUNDS]));
	vst1q_u8(ct, b);
}
#endif /* HAVE_ARM_CE */

ty_azote_Aes128Ctx *ty_azote_aes128_new(const uint8_t key[static TY_AZOTE_AES128_KEY_LEN])
{
    ty_azote_Aes128Backend backend = ty_azote_aes128_active_backend();
    ty_azote_Aes128Ctx *ctx = calloc(1, sizeof *ctx);

    if (!ctx)
    {
        return nullptr;
    }
    switch (backend)
    {
#if defined(TY_AZOTE_AES128_HAVE_NI)
        case TY_AZOTE_AES128_BACKEND_AESNI:
		    ni_key_schedule(key, ctx->u.ni_rk);
		    return ctx;
#elif defined(TY_AZOTE_AES128_HAVE_ARM_CE)
        case TY_AZOTE_AES128_BACKEND_ARM_CE:
		    arm_key_schedule(key, ctx->u.arm_rk);
		    return ctx;
#endif
        default:
            // ty_azote_aes_key_init(&ctx->u.sw, key, TY_AZOTE_AES_128);
            ty_azote_rijndael_key_setup_enc(&ctx->u.sw, key, TY_AZOTE_AES_128);
            return ctx;

            /* ty_azote_aes128.c — было:
 *     ty_azote_aes_key_init(&ctx->u.sw, key, TY_AZOTE_AES_128);
 * ...
 *     ty_azote_aes_encrypt_block(&ctx->u.sw, pt, ct);
 *
 * стало (Strategy: тот же ty_azote_AesKey, более быстрый движок): */
// #include "ty_azote_rijndael.h"


/* внутри ty_azote_aes128_new(), default: */
//             ty_azote_rijndael_key_setup_enc(&ctx->u.sw, key, TY_AZOTE_AES_128);

/* внутри ty_azote_aes128_encrypt_block(), default: */
//             ty_azote_rijndael_encrypt_block(&ctx->u.sw, pt, ct);
    }
}

void ty_azote_aes128_free(ty_azote_Aes128Ctx *ctx)
{
    free(ctx);
}

void ty_azote_aes128_encrypt_block(const ty_azote_Aes128Ctx *ctx,const uint8_t pt[static 16], uint8_t ct[static 16])
{
    switch (ty_azote_aes128_active_backend())
    {
#if defined(TY_AZOTE_AES128_HAVE_NI)
        case TY_AZOTE_AES128_BACKEND_AESNI:
		    ni_encrypt(ctx->u.ni_rk, pt, ct);
		    return;
#elif defined(TY_AZOTE_AES128_HAVE_ARM_CE)
        case TY_AZOTE_AES128_BACKEND_ARM_CE:
		    arm_encrypt(ctx->u.arm_rk, pt, ct);
		    return;
#endif
        default:
            ty_azote_aes_encrypt_block(&ctx->u.sw, pt, ct);
            return;
    }
}

bool ty_azote_aes128_selftest(void)
{
    static const uint8_t pt[16]  = {0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff};
    static const uint8_t key[16] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
    static const uint8_t expected[16] = {0x69,0xc4,0xe0,0xd8,0x6a,0x7b,0x04,0x30,0xd8,0xcd,0xb7,0x80,0x70,0xb4,0xc5,0x5a};
    uint8_t ct[16];
    ty_azote_Aes128Ctx *ctx = ty_azote_aes128_new(key);

    if (!ctx)
    {
        return false;
    }
    ty_azote_aes128_encrypt_block(ctx, pt, ct);
    ty_azote_aes128_free(ctx);
    bool ok = memcmp(ct, expected, 16) == 0;

    if (!ok)
    {
        TY_AZOTE_ERROR("aes128 self-test FAILED against FIPS-197 test vector");
    }
    return ok;
}




