#include <stdio.h>
#include <errno.h>

#include "ty_azote_random.h"

#if defined(__linux__)
#include <sys/random.h>
#define TY_AZOTE_HAVE_GETRANDOM 1
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
#include <stdlib.h>
#define TY_AZOTE_HAVE_ARC4RANDOM 1
#endif

static ty_azote_Status fallback_dev_urandom(void *dst, size_t n)
{
    FILE *f = fopen("/dev/urandom", "rb");

    if (!f)
    {
        return TY_AZOTE_ERR_IO;
    }
    size_t got = fread(dst, 1, n, f);
    fclose(f);
    return got == n ? TY_AZOTE_OK : TY_AZOTE_ERR_IO;
}

ty_azote_Status ty_azote_random_bytes(void *dst, size_t n)
{
    if (!dst && n)
    {
        return TY_AZOTE_ERR_PARSE;
    }
    if (n == 0)
    {
        return TY_AZOTE_OK;
    }

#if defined(TY_AZOTE_HAVE_GETRANDOM)
    uint8_t *p = dst;
	size_t left = n;

	while (left > 0)
    {
		ssize_t r = getrandom(p, left, 0);

		if (r < 0)
        {
			if (errno == EINTR) continue;
			return fallback_dev_urandom(dst, n);
		}
		p += r;
		left -= (size_t)r;
	}
	return TY_AZOTE_OK;
#elif defined(TY_AZOTE_HAVE_ARC4RANDOM)
    arc4random_buf(dst, n);
	return TY_AZOTE_OK;
#else
    return fallback_dev_urandom(dst, n);
#endif
}

uint32_t ty_azote_random_u32(void)
{
    uint32_t v = 0;
    ty_azote_random_bytes(&v, sizeof v);
    return v;
}

uint64_t ty_azote_random_u64(void)
{
    uint64_t v = 0;
    ty_azote_random_bytes(&v, sizeof v);
    return v;
}