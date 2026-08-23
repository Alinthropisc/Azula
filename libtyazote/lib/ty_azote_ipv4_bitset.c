#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>

#include "ty_azote_ipv4_bitset.h"

constexpr size_t TY_AZOTE_BITSET_PAGE_BITS = 65536;
constexpr size_t TY_AZOTE_BITSET_PAGE_BYTES = TY_AZOTE_BITSET_PAGE_BITS / 8;
constexpr size_t TY_AZOTE_BITSET_NUM_PAGES = 65536;

struct ty_azote_Ipv4Bitset {
    uint8_t *pages[TY_AZOTE_BITSET_NUM_PAGES]; /* lazily allocated */
};

ty_azote_Ipv4Bitset *ty_azote_ipv4_bitset_new(void)
{
    return calloc(1, sizeof(ty_azote_Ipv4Bitset));
}

void ty_azote_ipv4_bitset_free(ty_azote_Ipv4Bitset *bs)
{
    if (!bs)
    {
        return;
    }

    for (size_t i = 0; i < TY_AZOTE_BITSET_NUM_PAGES; ++i)
    {
        free(bs->pages[i]);
    }
    free(bs);
}

static inline void split_addr(uint32_t host_order, uint16_t *page, uint16_t *bit)
{
    *page = (uint16_t)(host_order >> 16);
    *bit  = (uint16_t)(host_order & 0xFFFF);
}

bool ty_azote_ipv4_bitset_test(const ty_azote_Ipv4Bitset *bs, ty_azote_Ipv4Addr ip)
{
    uint16_t page, bit;
    split_addr(ty_azote_ipv4_to_host(ip), &page, &bit);
    const uint8_t *p = bs->pages[page];
    return p && (p[bit >> 3] & (uint8_t)(1u << (bit & 7))) != 0;
}

void ty_azote_ipv4_bitset_set(const ty_azote_Ipv4Bitset *bs, ty_azote_Ipv4Addr ip)
{
    uint16_t page, bit;
    split_addr(ty_azote_ipv4_to_host(ip), &page, &bit);
    uint8_t **slot = &((ty_azote_Ipv4Bitset *)bs)->pages[page];

    if (!*slot)
    {
        *slot = calloc(1, TY_AZOTE_BITSET_PAGE_BYTES);
    }
    (*slot)[bit >> 3] |= (uint8_t)(1u << (bit & 7));
}

ty_azote_Status ty_azote_ipv4_bitset_load_file(ty_azote_Ipv4Bitset *bs, const char *path,uint32_t *out_count)
{
    if (!bs || !path)
    {
        return TY_AZOTE_ERR_PARSE;
    }
    FILE *fp = fopen(path, "r");

    if (!fp)
    {
        return TY_AZOTE_ERR_IO;
    }
    char line[1024];
    uint32_t count = 0;

    while (fgets(line, sizeof line, fp))
    {
        char *comment = strchr(line, '#');

        if (comment)
        {
            *comment = '\0';
        }

        char token[1024];
        if (sscanf(line, "%1023s", token) != 1)
        {
            continue; /* blank line */
        }
        struct in_addr addr;

        if (inet_aton(token, &addr) != 1)
        {
            fclose(fp);
            return TY_AZOTE_ERR_PARSE;
        }
        ty_azote_ipv4_bitset_set(bs, ty_azote_ipv4_from_network(addr.s_addr));
        ++count;
    }
    fclose(fp);

    if (out_count)
    {
        *out_count = count;
    }
    return TY_AZOTE_OK;
}