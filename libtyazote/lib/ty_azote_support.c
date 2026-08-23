#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "ty_azote_util.h"

#include <string.h>
#include <arpa/inet.h>

#include "state.h"
#include "utility.h"
#include "../lib/logger.h"


const char *ty_azote_status_str(ty_azote_Status status)
{
    switch (status)
    {
        case TY_AZOTE_OK:
            return "ok";
        case TY_AZOTE_ERR_RANGE:
            return "value out of range";
        case TY_AZOTE_ERR_PARSE:
            return "parse error";
        case TY_AZOTE_ERR_IO:
            return "I/O error";
        case TY_AZOTE_ERR_PERM:
            return "permission denied";
        case TY_AZOTE_ERR_NOMEM:
            return "out of memory";
        case TY_AZOTE_ERR_OVERFLOW:
            return "numeric overflow";
    }
    return "unknown error";
}

bool ty_azote_in_range(int64_t v, int64_t min, int64_t max)
{
    return v >= min && v <= max;
}

ty_azote_Status ty_azote_check_range_i(int v, int min, int max)
{
    return ty_azote_in_range(v, min, max) ? TY_AZOTE_OK : TY_AZOTE_ERR_RANGE;
}

ty_azote_StringList ty_azote_string_list_new(void)
{
    return (ty_azote_StringList){0};
}

ty_azote_Status ty_azote_string_list_push(ty_azote_StringList *list, ty_azote_StringSlice item)
{
    if (list->count == list->capacity)
    {
        size_t ncap = list->capacity ? list->capacity * 2 : 8;
        char **ni = realloc(list->items, ncap * sizeof *ni);

        if (!ni)
        {
            return TY_AZOTE_ERR_NOMEM;
        }
        list->items = ni;
        list->capacity = ncap;
    }
    char *copy = malloc(item.len + 1);

    if (!copy)
    {
        return TY_AZOTE_ERR_NOMEM;
    }
    memcpy(copy, item.data, item.len);
    copy[item.len] = '\0';
    list->items[list->count++] = copy;
    return TY_AZOTE_OK;
}

void ty_azote_string_list_free(ty_azote_StringList *list)
{
    if (!list)
    {
        return;
    }

    for (size_t i = 0; i < list->count; ++i)
    {
        free(list->items[i]);
    }
    free(list->items);
    *list = (ty_azote_StringList){0};
}

ty_azote_StringList ty_azote_split_string(const char *in, const char *delims) {
    ty_azote_StringList out = ty_azote_string_list_new();
    if (!in)
    {
        return out;
    }
    if (!delims)
    {
        delims = ", ";
    }
    const char *p = in;

    while (*p)
    {
        size_t skip = strspn(p, delims);
        p += skip;

        if (!*p)
        {
            break;
        }
        size_t tok_len = strcspn(p, delims);
        ty_azote_string_list_push(&out, (ty_azote_StringSlice){.data = p, .len = tok_len});
        p += tok_len;
    }
    return out;
}


static void wrap_one_line(FILE *out, const char *line, size_t width)
{
    size_t line_len = strlen(line);

    if (line_len <= width)
    {
        fprintf(out, "%s\n", line);
        return;
    }
    const char *cursor = line;

    while (*cursor)
    {
        size_t remaining = strlen(cursor);

        if (remaining <= width)
        {
            fprintf(out, "%s\n", cursor);
            break;
        }
        /* find the last space within [0, width] to break on a word
         * boundary; if none exists, hard-break at `width`. */
        size_t break_at = width;
        size_t last_space = SIZE_MAX;

        for (size_t i = 0; i <= width; ++i)
        {
            if (cursor[i] == ' ')
            {
                last_space = i;
            }
        }

        if (last_space != SIZE_MAX)
        {
            break_at = last_space;
        }
        fprintf(out, "%.*s\n", (int)break_at, cursor);
        cursor += break_at;
        cursor += strspn(cursor, " "); /* skip the separating space(s) */
    }
}

void ty_azote_wrap_print(FILE *out, const char *text, size_t width)
{
    if (!text)
    {
        return;
    }

    if (strlen(text) <= width && !strchr(text, '\n'))
    {
        fprintf(out, "%s", text);
        return;
    }
    char *copy = strdup(text);

    if (!copy)
    {
        return;
    }
    char *saveptr = nullptr;

    for (char *line = strtok_r(copy, "\n", &saveptr); line; line = strtok_r(nullptr, "\n", &saveptr))
    {
        wrap_one_line(out, line, width);
    }
    free(copy);
}

ty_azote_Status ty_azote_parse_max_targets(const char *spec, int port_count, uint64_t *out)
{
    if (!spec || !out || port_count <= 0)
    {
        return TY_AZOTE_ERR_PARSE;
    }
    char *end = nullptr;
    errno = 0;
    double v = strtod(spec, &end);

    if (end == spec || errno != 0)
    {
        return TY_AZOTE_ERR_PARSE;
    }
    uint64_t search_space = ((uint64_t)1 << 32) * (uint64_t)port_count;

    if (end[0] == '%' && end[1] == '\0')
    {
        v = v * (double)search_space / 100.0;
    }
    else if (end[0] != '\0')
    {
        return TY_AZOTE_ERR_PARSE;
    }

    if (v <= 0)
    {
        *out = 0;
    }
    else if (v >= (double)search_space)
    {
        *out = search_space;
    }
    else
    {
        *out = (uint64_t)v;
    }
    return TY_AZOTE_OK;
}

ty_azote_TimeStr ty_azote_format_duration(uint32_t total_seconds, bool estimate)
{
    ty_azote_TimeStr r = {0};
    uint32_t y = total_seconds / 31556736u;
    uint32_t d = (total_seconds % 31556736u) / 86400u;
    uint32_t h = (total_seconds % 86400u) / 3600u;
    uint32_t m = (total_seconds % 3600u) / 60u;
    uint32_t s = total_seconds % 60u;

    if (estimate)
    {
        if (y > 0)
        {
            snprintf(r.buf, sizeof r.buf, "%uy", y);
        }
        else if (d > 9)
        {
            snprintf(r.buf, sizeof r.buf, "%ud", d);
        }
        else if (d > 0)
        {
            snprintf(r.buf, sizeof r.buf, "%ud%02uh", d, h);
        }
        else if (h > 9)
        {
            snprintf(r.buf, sizeof r.buf, "%uh", h);
        }
        else if (h > 0)
        {
            snprintf(r.buf, sizeof r.buf, "%uh%02um", h, m);
        }
        else if (m > 9)
        {
            snprintf(r.buf, sizeof r.buf, "%um", m);
        }
        else if (m > 0)
        {
            snprintf(r.buf, sizeof r.buf, "%um%02us", m, s);
        }
        else
        {
            snprintf(r.buf, sizeof r.buf, "%us", s);
        }
    }
    else
    {
        if (d > 0)
        {
            snprintf(r.buf, sizeof r.buf, "%ud%u:%02u:%02u", d, h, m, s);
        }
        else if (h > 0)
        {
            snprintf(r.buf, sizeof r.buf, "%u:%02u:%02u", h, m, s);
        }
        else
        {
            snprintf(r.buf, sizeof r.buf, "%u:%02u", m, s);
        }
    }
    return r;
}

ty_azote_QuantityStr ty_azote_format_quantity(uint64_t n)
{
    ty_azote_QuantityStr r = {0};

    if (n < 1000)
    {
        snprintf(r.buf, sizeof r.buf, "%llu ", (unsigned long long)n);
    }
    else if (n < 1'000'000)
    {
        int figs = (n < 10'000) ? 2 : (n < 100'000) ? 1 : 0;
        snprintf(r.buf, sizeof r.buf, "%0.*f K", figs, (double)n / 1000.0);
    }
    else if (n < 1'000'000'000)
    {
        int figs = (n < 10'000'000) ? 2 : (n < 100'000'000) ? 1 : 0;
        snprintf(r.buf, sizeof r.buf, "%0.*f M", figs, (double)n / 1'000'000.0);
    }
    else
    {
        int figs = (n < 10'000'000'000ull) ? 2 : (n < 100'000'000'000ull) ? 1 : 0;
        snprintf(r.buf, sizeof r.buf, "%0.*f G", figs, (double)n / 1'000'000'000.0);
    }
    return r;
}


ty_azote_Status ty_azote_mac_parse(const char *text, ty_azote_MacAddr *out)
{
    if (!text || !out)
    {
        return TY_AZOTE_ERR_PARSE;
    }
    unsigned b[TY_AZOTE_MAC_LEN];
    int consumed = 0;
    int matched = sscanf(text, "%2x:%2x:%2x:%2x:%2x:%2x%n",&b[0], &b[1], &b[2], &b[3], &b[4], &b[5], &consumed);

    if (matched != TY_AZOTE_MAC_LEN || text[consumed] != '\0')
    {
        return TY_AZOTE_ERR_PARSE;
    }

    for (size_t i = 0; i < TY_AZOTE_MAC_LEN; ++i)
    {
        out->octets[i] = (uint8_t)b[i];
    }
    return TY_AZOTE_OK;
}

ty_azote_MacStr ty_azote_mac_to_string(ty_azote_MacAddr mac)
{
    ty_azote_MacStr r;
    snprintf(r.buf, sizeof r.buf, "%02x:%02x:%02x:%02x:%02x:%02x",mac.octets[0], mac.octets[1], mac.octets[2],mac.octets[3], mac.octets[4], mac.octets[5]);
    return r;
}


bool ty_azote_file_exists(const char *path)
{
    struct stat st;
    return path && stat(path, &st) == 0;
}

bool ty_azote_file_readable(const char *path)
{
    return path && access(path, R_OK) == 0;
}

ty_azote_Status ty_azote_drop_privileges(const char *user)
{
    if (geteuid() != 0)
    {
        return TY_AZOTE_OK; /* not root: nothing to do */
    }
    const char *target = user ? user : "nobody";
    struct passwd pwbuf, *pw = nullptr;
    char strbuf[4096];

    if (getpwnam_r(target, &pwbuf, strbuf, sizeof strbuf, &pw) != 0 || !pw)
    {
        return TY_AZOTE_ERR_PERM;
    }

    if (setgid(pw->pw_gid) != 0)
    {
        return TY_AZOTE_ERR_PERM;
    }

    if (setuid(pw->pw_uid) != 0)
    {
        return TY_AZOTE_ERR_PERM;
    }
    return TY_AZOTE_OK;
}


#if defined(__APPLE__)

#include <pthread.h>
#include <mach/thread_act.h>
#include <mach/thread_policy.h>

ty_azote_Status ty_azote_cpu_affinity_set(uint32_t core)
{
	mach_port_t tid = pthread_mach_thread_np(pthread_self());
	struct thread_affinity_policy policy = {
            .affinity_tag = (integer_t)core,
    };
	kern_return_t ret = thread_policy_set(tid, THREAD_AFFINITY_POLICY,(thread_policy_t)&policy,THREAD_AFFINITY_POLICY_COUNT);
	return ret == KERN_SUCCESS ? TY_AZOTE_OK : TY_AZOTE_ERR_IO;
}

#elif defined(__DragonFly__)

#include <sys/usched.h>
#include <unistd.h>

ty_azote_Status ty_azote_cpu_affinity_set(uint32_t core)
{
	if (usched_set(getpid(), USCHED_SET_CPU, &core, sizeof core) != 0)
    {
        return TY_AZOTE_ERR_IO;
    }
	return TY_AZOTE_OK;
}

#elif defined(__NetBSD__)

#include <pthread.h>
#include <sched.h>

ty_azote_Status ty_azote_cpu_affinity_set(uint32_t core)
{
	cpuset_t *set = cpuset_create();
	if (!set)
    {
         return TY_AZOTE_ERR_NOMEM;
    }
	cpuset_zero(set);
	cpuset_set((cpuid_t)core, set);
	int rc = pthread_setaffinity_np(pthread_self(), cpuset_size(set), set);
	cpuset_destroy(set);
	return rc == 0 ? TY_AZOTE_OK : TY_AZOTE_ERR_IO;
}

#else /* Linux / FreeBSD */

#include <pthread.h>
#include <sched.h>

#if defined(__FreeBSD__)
#include <sys/param.h>
#include <sys/cpuset.h>
#include <pthread_np.h>
#define cpu_set_t cpuset_t
#endif

ty_azote_Status ty_azote_cpu_affinity_set(uint32_t core)
{
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET((int)core, &set);
    int rc = pthread_setaffinity_np(pthread_self(), sizeof set, &set);
    return rc == 0 ? TY_AZOTE_OK : TY_AZOTE_ERR_IO;
}

#endif

double ty_azote_wall_clock_seconds(void)
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

double ty_azote_monotonic_seconds(void)
{
    struct timespec ts;
#if defined(TIME_MONOTONIC)
    timespec_get(&ts, TIME_MONOTONIC);
#elif defined(_POSIX_TIMERS) && defined(_POSIX_MONOTONIC_CLOCK)
    clock_gettime(CLOCK_MONOTONIC, &ts);
#else
    timespec_get(&ts, TIME_UTC);
#endif
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}


in_addr_t string_to_ip_address(char *t)
{
    in_addr_t r = inet_addr(t);
    if (r == INADDR_NONE) {
        log_fatal("send", "invalid ip address: `%s'", t);
    }
    return r;
}

void add_to_array(char *to_add)
{
    if (zconf.number_source_ips >= 256) {
        // log fatal here
        log_fatal("parse", "over 256 source IP addresses provided");
    }
    log_debug("SEND", "ipaddress: %s\n", to_add);
    zconf.source_ip_addresses[zconf.number_source_ips] =
            string_to_ip_address(to_add);
    zconf.number_source_ips++;
}

void parse_source_ip_addresses(char given_string[])
{
    char *dash = strchr(given_string, '-');
    char *comma = strchr(given_string, ',');
    if (dash && comma) {
        *comma = '\0';
        parse_source_ip_addresses(given_string);
        parse_source_ip_addresses(comma + 1);
    } else if (comma) {
        while (comma) {
            *comma = '\0';
            add_to_array(given_string);
            given_string = comma + 1;
            comma = strchr(given_string, ',');
            if (!comma) {
                add_to_array(given_string);
            }
        }
    } else if (dash) {
        *dash = '\0';
        log_debug("SEND", "address: %s\n", given_string);
        log_debug("SEND", "address: %s\n", dash + 1);
        in_addr_t start = ntohl(string_to_ip_address(given_string));
        in_addr_t end = ntohl(string_to_ip_address(dash + 1)) + 1;
        while (start != end) {
            struct in_addr temp;
            temp.s_addr = htonl(start);
            add_to_array(strdup(inet_ntoa(temp)));
            start++;
        }
    } else {
        add_to_array(given_string);
    }
}

// Not all platforms have strlcpy, so we provide our own using strncpy
size_t cross_platform_strlcpy(char *dst, const char *src, size_t siz)
{
    if (siz == 0) // Handle zero size as strlcpy does
        return strlen(src);

    strncpy(dst, src, siz - 1); // Copy at most size - 1 characters
    dst[siz - 1] = '\0';	    // Ensure null-termination

    return strlen(src); // Return the length of src
}