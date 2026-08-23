#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ======================================================================= *
 *  Wire-format aliases — used only where a struct must match packet bytes
 *  1:1 (e.g. IPv4/TCP header overlays). Prefer the Value Objects below for
 *  every other purpose.
 * ======================================================================= */

typedef uint32_t ty_azote_ipv4_n_t; /* IPv4 address, network byte order */
typedef uint32_t ty_azote_ipv4_h_t; /* IPv4 address, host byte order    */
typedef uint16_t ty_azote_port_n_t; /* port, network byte order         */
typedef uint16_t ty_azote_port_h_t; /* port, host byte order            */

/* ======================================================================= *
 *  Endian helpers (Facade over compiler builtins — no <endian.h> needed,
 *  which is not part of standard C and differs across BSD/glibc/macOS)
 * ======================================================================= */

static inline uint16_t ty_azote_bswap16(uint16_t v)
{
    return (uint16_t)((v << 8) | (v >> 8));
}
static inline uint32_t ty_azote_bswap32(uint32_t v)
{
    return ((v & 0x000000FFu) << 24) | ((v & 0x0000FF00u) << 8) | ((v & 0x00FF0000u) >> 8) | ((v & 0xFF000000u) >> 24);
}
static inline uint64_t ty_azote_bswap64(uint64_t v)
{
    return ((uint64_t)ty_azote_bswap32((uint32_t)v) << 32) | ty_azote_bswap32((uint32_t)(v >> 32));
}

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define TY_AZOTE_LITTLE_ENDIAN 1
#else
#define TY_AZOTE_LITTLE_ENDIAN 0
#endif

static inline uint16_t ty_azote_hton16(uint16_t v)
{
    return TY_AZOTE_LITTLE_ENDIAN ? ty_azote_bswap16(v) : v;
}

static inline uint32_t ty_azote_hton32(uint32_t v)
{
    return TY_AZOTE_LITTLE_ENDIAN ? ty_azote_bswap32(v) : v;
}

static inline uint64_t ty_azote_hton64(uint64_t v)
{
    return TY_AZOTE_LITTLE_ENDIAN ? ty_azote_bswap64(v) : v;
}

#define ty_azote_ntoh16 ty_azote_hton16
#define ty_azote_ntoh32 ty_azote_hton32
#define ty_azote_ntoh64 ty_azote_hton64

/* Type-dispatched facade, analog of a generic to_be()/to_le() in Rust. */
#define ty_azote_hton(v) _Generic((v),uint16_t: ty_azote_hton16, uint32_t: ty_azote_hton32, uint64_t: ty_azote_hton64)(v)

/* ======================================================================= *
 *  IPv4 address (Value Object) — stored in network byte order internally,
 *  same representation used on the wire, zero-cost to memcpy into packets.
 * ======================================================================= */

typedef struct ty_azote_Ipv4Addr {
    ty_azote_ipv4_n_t addr_n;
} ty_azote_Ipv4Addr;

static inline ty_azote_Ipv4Addr ty_azote_ipv4_from_host(uint32_t host_order)
{
    return (ty_azote_Ipv4Addr){.addr_n = ty_azote_hton32(host_order)};
}

static inline ty_azote_Ipv4Addr ty_azote_ipv4_from_network(uint32_t net_order)
{
    return (ty_azote_Ipv4Addr){.addr_n = net_order};
}

static inline uint32_t ty_azote_ipv4_to_host(ty_azote_Ipv4Addr ip)
{
    return ty_azote_ntoh32(ip.addr_n);
}

static inline bool ty_azote_ipv4_eq(ty_azote_Ipv4Addr a, ty_azote_Ipv4Addr b)
{
    return a.addr_n == b.addr_n;
}

typedef struct ty_azote_Ipv4Str {
    char buf[16]; /* "255.255.255.255\0" */
} ty_azote_Ipv4Str;

static inline ty_azote_Ipv4Str ty_azote_ipv4_to_string(ty_azote_Ipv4Addr ip)
{
    ty_azote_Ipv4Str s;
    uint32_t h = ty_azote_ipv4_to_host(ip);
    snprintf(s.buf, sizeof s.buf, "%u.%u.%u.%u",(h >> 24) & 0xFF, (h >> 16) & 0xFF, (h >> 8) & 0xFF, h & 0xFF);
    return s;
}

/* ======================================================================= *
 *  IPv6 address (Value Object)
 * ======================================================================= */

constexpr size_t TY_AZOTE_IPV6_LEN = 16;

typedef struct ty_azote_Ipv6Addr {
    uint8_t octets[TY_AZOTE_IPV6_LEN]; /* network byte order, as on the wire */
} ty_azote_Ipv6Addr;

static inline bool ty_azote_ipv6_eq(ty_azote_Ipv6Addr a, ty_azote_Ipv6Addr b)
{
    return memcmp(a.octets, b.octets, TY_AZOTE_IPV6_LEN) == 0;
}

typedef struct ty_azote_Ipv6Str {
    char buf[46];
} ty_azote_Ipv6Str;

ty_azote_Ipv6Str ty_azote_ipv6_to_string(ty_azote_Ipv6Addr ip); /* impl in .c: compresses "::" runs */

/* ======================================================================= *
 *  Tagged union — analog of Rust's std::net::IpAddr (Composite)
 * ======================================================================= */

typedef enum ty_azote_IpVersion : uint8_t {
    TY_AZOTE_IP_V4 = 4,
    TY_AZOTE_IP_V6 = 6,
} ty_azote_IpVersion;

typedef struct ty_azote_IpAddr {
    ty_azote_IpVersion version;
    union {
        ty_azote_Ipv4Addr v4;
        ty_azote_Ipv6Addr v6;
    };
} ty_azote_IpAddr;

static inline ty_azote_IpAddr ty_azote_ip_from_v4(ty_azote_Ipv4Addr v4)
{
    return (ty_azote_IpAddr){.version = TY_AZOTE_IP_V4, .v4 = v4};
}

static inline ty_azote_IpAddr ty_azote_ip_from_v6(ty_azote_Ipv6Addr v6)
{
    return (ty_azote_IpAddr){.version = TY_AZOTE_IP_V6, .v6 = v6};
}

bool ty_azote_ip_eq(ty_azote_IpAddr a, ty_azote_IpAddr b);
uint64_t ty_azote_ip_hash(ty_azote_IpAddr ip); /* FNV-1a, for hash-set based dedup */
ty_azote_Ipv4Str ty_azote_ip_to_string_v4(ty_azote_IpAddr ip); /* asserts version == V4 */

/* ======================================================================= *
 *  Port (Value Object)
 * ======================================================================= */

typedef struct ty_azote_Port {
    ty_azote_port_n_t port_n;
} ty_azote_Port;

static inline ty_azote_Port ty_azote_port_from_host(uint16_t host_order)
{
    return (ty_azote_Port){.port_n = ty_azote_hton16(host_order)};
}

static inline uint16_t ty_azote_port_to_host(ty_azote_Port p)
{
    return ty_azote_ntoh16(p.port_n);
}

/* ======================================================================= *
 *  MAC address (Value Object) — canonical definition; other modules
 *  (e.g. ty_azote_util) should include this header rather than redefine it.
 * ======================================================================= */

constexpr size_t TY_AZOTE_MAC_LEN = 6;

typedef struct ty_azote_MacAddr {
    uint8_t octets[TY_AZOTE_MAC_LEN];
} ty_azote_MacAddr;

static inline bool ty_azote_mac_eq(ty_azote_MacAddr a, ty_azote_MacAddr b)
{
    return memcmp(a.octets, b.octets, TY_AZOTE_MAC_LEN) == 0;
}

#ifdef __cplusplus
}
#endif
































