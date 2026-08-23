#pragma once


#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>

#if defined(__NetBSD__)
#include <net/if_ether.h>
#ifndef ICMP_UNREACH_PRECEDENCE_CUTOFF
#define ICMP_UNREACH_PRECEDENCE_CUTOFF ICMP_UNREACH_PREC_CUTOFF
#endif
#else
#include <net/ethernet.h>
#endif

#include <netdb.h>
#include <net/if.h>
#include <ifaddrs.h> /* must follow net/if.h */
#include <arpa/inet.h>

#include "ty_azote_types.h" /* canonical TY_AZOTE_MAC_LEN */

#if defined(ETHER_ADDR_LEN)
static_assert(ETHER_ADDR_LEN == TY_AZOTE_MAC_LEN,"platform ETHER_ADDR_LEN disagrees with ty_azote_types.h");
#endif

#if defined(__GNUC__) || defined(__clang__)
#define TY_AZOTE_UNUSED __attribute__((unused))
#else
#define TY_AZOTE_UNUSED
#endif





































