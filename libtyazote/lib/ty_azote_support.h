#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <netinet/in.h>


#ifdef __cplusplus
extern "C" {
#endif

#if defined(__GNUC__) || defined(__clang__)
#define TY_AZOTE_CLEANUP(f) __attribute__((cleanup(f)))
#else
#define TY_AZOTE_CLEANUP(f)
#warning "[ ETA ]: ty_azote_support no cleanup attribute; RAII guards are no-ops."
#endif

typedef enum ty_azote_Status : uint8_t {
    TY_AZOTE_OK = 0,
    TY_AZOTE_ERR_RANGE,
    TY_AZOTE_ERR_PARSE,
    TY_AZOTE_ERR_IO,
    TY_AZOTE_ERR_PERM,
    TY_AZOTE_ERR_NOMEM,
    TY_AZOTE_ERR_OVERFLOW,
} ty_azote_Status;

[[nodiscard]]
const char *ty_azote_status_str(ty_azote_Status status);

static inline int32_t ty_azote_max_i32(int32_t a, int32_t b)
{
    return a > b ? a : b;
}

static inline int32_t ty_azote_min_i32(int32_t a, int32_t b)
{
    return a < b ? a : b;
}

static inline int64_t ty_azote_max_i64(int64_t a, int64_t b)
{
    return a > b ? a : b;
}

static inline int64_t ty_azote_min_i64(int64_t a, int64_t b)
{
    return a < b ? a : b;
}

static inline uint32_t ty_azote_max_u32(uint32_t a, uint32_t b)
{
    return a > b ? a : b;
}

static inline uint32_t ty_azote_min_u32(uint32_t a, uint32_t b)
{
    return a < b ? a : b;
}

static inline uint64_t ty_azote_max_u64(uint64_t a, uint64_t b)
{
    return a > b ? a : b;
}

static inline uint64_t ty_azote_min_u64(uint64_t a, uint64_t b)
{
    return a < b ? a : b;
}

static inline double ty_azote_max_f64(double a, double b)
{
    return a > b ? a : b;
}

static inline double ty_azote_min_f64(double a, double b)
{
    return a < b ? a : b;
}

#define ty_azote_max(a, b) _Generic((a) + (b),                 \
	int: ty_azote_max_i32,                   \
	long: ty_azote_max_i64,                   \
	long long: ty_azote_max_i64,                   \
	unsigned int: ty_azote_max_u32,                   \
	unsigned long: ty_azote_max_u64,                   \
	unsigned long long: ty_azote_max_u64,                   \
	double: ty_azote_max_f64                    \
)((a), (b))

#define ty_azote_min(a, b) _Generic((a) + (b),                 \
	int: ty_azote_min_i32,                   \
	long: ty_azote_min_i64,                   \
	unsigned int: ty_azote_min_u32,                   \
	long long: ty_azote_min_i64,                   \
	unsigned long: ty_azote_min_u64,                   \
	unsigned long long: ty_azote_min_u64,                   \
	double: ty_azote_min_f64                    \
)((a), (b))

[[nodiscard]]
bool ty_azote_in_range(int64_t v, int64_t min, int64_t max);

/* Non-owning view, analog of Rust's &str / C++23 string_view. */
typedef struct ty_azote_StringSlice {
    const char *data;
    size_t len;
} ty_azote_StringSlice;

static inline ty_azote_StringSlice ty_azote_str(const char *s)
{
    return (ty_azote_StringSlice){.data = s, .len = s ? __builtin_strlen(s) : 0};
}

/* Owning, growable list of heap strings. Replaces the fixed MAX_SPLITS=128
 * array from the original zmap implementation. */
typedef struct ty_azote_StringList {
    char  **items;
    size_t count;
    size_t capacity;
} ty_azote_StringList;

[[nodiscard]]
ty_azote_StringList ty_azote_string_list_new(void);

ty_azote_Status ty_azote_string_list_push(ty_azote_StringList *list, ty_azote_StringSlice item);
void ty_azote_string_list_free(ty_azote_StringList *list);

/* RAII guard: auto-frees the list at scope exit. */
static inline void ty_azote_string_list_cleanup(ty_azote_StringList *l)
{
    ty_azote_string_list_free(l);
}
#define TY_AZOTE_STRING_LIST_GUARD(name_)                                       \
	[[maybe_unused]]                                                               \
    TY_AZOTE_CLEANUP(ty_azote_string_list_cleanup) ty_azote_StringList name_ = ty_azote_string_list_new()

/* Split `in` on any byte from `delims` (default ", " if nullptr). */
[[nodiscard]]
ty_azote_StringList ty_azote_split_string(const char *in, const char *delims);

/* Word-wrap `text` to `width` columns, writing to `out`. Correctly preserves
 * pre-existing newlines (fixes the printf/fprintf mismatch bug present in
 * the original zmap fprintw()). */
void ty_azote_wrap_print(FILE *out, const char *text, size_t width);

/* ======================================================================= *
 *  Numeric parsing
 * ======================================================================= */

/* Parses a count or a "NN%" percentage of the (2^32 * port_count) address
 * space, as used by zmap's --max-targets. */
ty_azote_Status ty_azote_parse_max_targets(const char *spec, int port_count, uint64_t *out);
ty_azote_Status ty_azote_check_range_i(int v, int min, int max);

/* ======================================================================= *
 *  Pretty-printing (Facade + Value Object)
 * ======================================================================= */

typedef struct ty_azote_TimeStr {
    char buf[32];
} ty_azote_TimeStr;

typedef struct ty_azote_QuantityStr {
    char buf[16];
} ty_azote_QuantityStr;

/* estimate=true  -> compact single-unit form ("3h", "12m", "45s") for ETAs.
 * estimate=false -> clock form ("1:23:45") for elapsed time. */
[[nodiscard]]
ty_azote_TimeStr ty_azote_format_duration(uint32_t seconds, bool estimate);

/* "123 ", "4.5 K", "1.2 M" style human counts. */
[[nodiscard]]
ty_azote_QuantityStr ty_azote_format_quantity(uint64_t n);


constexpr size_t TY_AZOTE_MAC_LEN = 6;

typedef struct ty_azote_MacAddr {
    uint8_t octets[TY_AZOTE_MAC_LEN];
} ty_azote_MacAddr;

typedef struct ty_azote_MacStr {
    char buf[18]; /* "xx:xx:xx:xx:xx:xx\0" */
} ty_azote_MacStr;

[[nodiscard]]
ty_azote_Status ty_azote_mac_parse(const char *text, ty_azote_MacAddr *out);

[[nodiscard]]
ty_azote_MacStr ty_azote_mac_to_string(ty_azote_MacAddr mac);

[[nodiscard]]
bool ty_azote_file_exists(const char *path);

[[nodiscard]]
bool ty_azote_file_readable(const char *path);


/* Drops root privileges to `user` (defaults to "nobody" if user == nullptr).
 * No-op (returns TY_AZOTE_OK) if not running as root. */
ty_azote_Status ty_azote_drop_privileges(const char *user);

/* Pins the calling thread to a single CPU core. Implemented per-OS
 * (Linux/FreeBSD/NetBSD/DragonFly/macOS) behind one uniform signature. */
ty_azote_Status ty_azote_cpu_affinity_set(uint32_t core);

[[nodiscard]]
double ty_azote_wall_clock_seconds(void);      /* CLOCK_REALTIME  */

[[nodiscard]]
double ty_azote_monotonic_seconds(void);       /* CLOCK_MONOTONIC */


void parse_source_ip_addresses(char given_string[]);
in_addr_t string_to_ip_address(char *t);

size_t cross_platform_strlcpy(char *dst, const char *src, size_t siz);



#ifdef __cplusplus
}
#endif
































