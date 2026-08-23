#include <inttype.h>

#include "ty_azote_support_fatal.h"
#include "ty_azote_tracing.h"

void ty_azote_enforce_range_or_fatal(const char *name, int v, int min, int max)
{
    if (ty_azote_check_range_i(v, min, max) != TY_AZOTE_OK)
    {
        TY_AZOTE_FATAL("argument '%s' must be between %d and %d (got %d)",name, min, max, v);
    }
}

uint64_t ty_azote_parse_max_targets_or_fatal(const char *spec, int port_count)
{
    uint64_t out;
    ty_azote_Status st = ty_azote_parse_max_targets(spec, port_count, &out);

    if (st != TY_AZOTE_OK)
    {
        TY_AZOTE_FATAL("can't parse max-targets '%s': %s", spec, ty_azote_status_str(st));
    }
    return out;
}

ty_azote_MacAddr ty_azote_mac_parse_or_fatal(const char *text)
{
    ty_azote_MacAddr mac;
    ty_azote_Status st = ty_azote_mac_parse(text, &mac);

    if (st != TY_AZOTE_OK)
    {
        TY_AZOTE_FATAL("invalid MAC address '%s': %s", text, ty_azote_status_str(st));
    }
    return mac;
}





// Example use

//#define TY_AZOTE_TARGET "ty_azote_scan"
//#include "ty_azote_tracing.h"
//#include "ty_azote_support.h"
//#include "ty_azote_support_fatal.h"
//
//int main(void)
//{
//    ty_azote_init_default(stderr, TY_AZOTE_LEVEL_INFO, true);
//    /* было: enforce_range("rate", rate, 1, 1000000) — падало через log_fatal */
//    ty_azote_enforce_range_or_fatal("rate", 500000, 1, 1'000'000);
//    uint64_t max_targets = ty_azote_parse_max_targets_or_fatal("10%", 1000);
//    TY_AZOTE_INFO_F(TY_AZOTE_FIELDS(ty_azote_field("max_targets", max_targets)),"computed scan budget");
//    TY_AZOTE_STRING_LIST_GUARD(hosts) = ty_azote_split_string("10.0.0.1, 10.0.0.2 10.0.0.3", nullptr);
//
//    for (size_t i = 0; i < hosts.count; ++i)
//    {
//        TY_AZOTE_DEBUG("target[%zu] = %s", i, hosts.items[i]);
//    }
//    /* hosts освобождается автоматически на выходе из блока */
//    ty_azote_TimeStr eta = ty_azote_format_duration(3725, true);
//    TY_AZOTE_INFO("estimated time remaining: %s", eta.buf);
//    ty_azote_MacAddr mac = ty_azote_mac_parse_or_fatal("de:ad:be:ef:00:01");
//    ty_azote_MacStr  mac_s = ty_azote_mac_to_string(mac);
//    TY_AZOTE_INFO("gateway mac: %s", mac_s.buf);
//
//    if (ty_azote_cpu_affinity_set(0) != TY_AZOTE_OK)
//    {
//        TY_AZOTE_WARN("failed to pin worker thread to core 0");
//    }
//    double t0 = ty_azote_monotonic_seconds();
//    /* ... scan work ... */
//    double dt = ty_azote_monotonic_seconds() - t0;
//    TY_AZOTE_INFO("scan finished in %.3fs", dt);
//    return 0;
//}