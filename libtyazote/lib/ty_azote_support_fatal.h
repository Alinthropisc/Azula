#pragma once



#include "ty_azote_support.h"

#ifdef __cplusplus
extern "C" {
#endif

void ty_azote_enforce_range_or_fatal(const char *name, int v, int min, int max);
uint64_t ty_azote_parse_max_targets_or_fatal(const char *spec, int port_count);
ty_azote_MacAddr ty_azote_mac_parse_or_fatal(const char *text);

#ifdef __cplusplus
}
#endif
























