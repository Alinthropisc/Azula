#include "ty_azote_types.h"

ty_azote_Ipv6Str ty_azote_ipv6_to_string(ty_azote_Ipv6Addr ip)
{
    uint16_t groups[8];
    for (int i = 0; i < 8; ++i)
    {
        groups[i] = (uint16_t)((ip.octets[2 * i] << 8) | ip.octets[2 * i + 1]);
    }
    /* find the longest run of zero groups to compress as "::" (RFC 5952) */
    int best_start = -1, best_len = 0, cur_start = -1, cur_len = 0;

    for (int i = 0; i < 8; ++i)
    {
        if (groups[i] == 0)
        {
            if (cur_start < 0)
            {
                cur_start = i;
            }
            ++cur_len;

            if (cur_len > best_len)
            {
                best_len = cur_len;
                best_start = cur_start;
            }
        }
        else
        {
            cur_start = -1; cur_len = 0;
        }
    }
    if (best_len < 2)
    {
        best_start = -1; /* only compress runs of >= 2 */
    }
    ty_azote_Ipv6Str s = {0};
    char *p = s.buf;

    for (int i = 0; i < 8; )
    {
        if (i == best_start)
        {
            p += sprintf(p, "::");
            i += best_len;
            continue;
        }
        p += sprintf(p, "%x", groups[i]);
        ++i;

        if (i < 8 && i != best_start)
        {
            *p++ = ':';
        }
    }
    *p = '\0';
    return s;
}

bool ty_azote_ip_eq(ty_azote_IpAddr a, ty_azote_IpAddr b)
{
    if (a.version != b.version)
    {
        return false;
    }
    return a.version == TY_AZOTE_IP_V4 ? ty_azote_ipv4_eq(a.v4, b.v4) : ty_azote_ipv6_eq(a.v6, b.v6);
}

uint64_t ty_azote_ip_hash(ty_azote_IpAddr ip)
{
    uint64_t h = 0xcbf29ce484222325ULL; /* FNV-1a offset basis */
    const uint8_t *bytes = ip.version == TY_AZOTE_IP_V4 ? (const uint8_t *)&ip.v4.addr_n : ip.v6.octets;
    size_t len = ip.version == TY_AZOTE_IP_V4 ? sizeof ip.v4.addr_n : TY_AZOTE_IPV6_LEN;

    for (size_t i = 0; i < len; ++i)
    {
        h ^= bytes[i];
        h *= 0x100000001b3ULL;
    }
    return h;
}

ty_azote_Ipv4Str ty_azote_ip_to_string_v4(ty_azote_IpAddr ip)
{
    return ty_azote_ipv4_to_string(ip.v4);
}







