#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <threads.h>
#include <stdatomic.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__GNUC__) || defined(__clang__)
#define TY_AZOTE_CLEANUP(f) __attribute__((cleanup(f)))
#define TY_AZOTE_PRINTF(a, b) __attribute__((format(printf, (a), (b))))
#else
#define TY_AZOTE_CLEANUP(f)
#define TY_AZOTE_PRINTF(a, b)
#warning "ty_azote_tracing: compiler lacks cleanup attribute; TY_AZOTE_SPAN will not auto-exit at scope end."
#endif

#define TY_AZOTE_CONCAT_(a, b) a##b
#define TY_AZOTE_CONCAT(a, b)  TY_AZOTE_CONCAT_(a, b)

#ifndef TY_AZOTE_TARGET
#define TY_AZOTE_TARGET "ty_azote"
#endif

constexpr size_t TY_AZOTE_MAX_FIELDS = 16;
constexpr size_t TY_AZOTE_MAX_SPAN_DEPTH = 64;

typedef enum ty_azote_Level : uint8_t {
    TY_AZOTE_LEVEL_TRACE = 0,
    TY_AZOTE_LEVEL_DEBUG,
    TY_AZOTE_LEVEL_INFO,
    TY_AZOTE_LEVEL_WARN,
    TY_AZOTE_LEVEL_ERROR,
    TY_AZOTE_LEVEL_FATAL,
    TY_AZOTE_LEVEL_OFF, /* sentinel: nothing is enabled */
} ty_azote_Level;

const char *ty_azote_level_name(ty_azote_Level level);
const char *ty_azote_level_color(ty_azote_Level level); /* ANSI escape */
bool ty_azote_level_from_str(const char *s, ty_azote_Level *out);

typedef enum ty_azote_FieldType : uint8_t {
    TY_AZOTE_FIELD_I64,
    TY_AZOTE_FIELD_U64,
    TY_AZOTE_FIELD_F64,
    TY_AZOTE_FIELD_BOOL,
    TY_AZOTE_FIELD_STR,
    TY_AZOTE_FIELD_PTR,
} ty_azote_FieldType;

typedef struct ty_azote_Field {
    const char  *key;
    ty_azote_FieldType type;
    union {
        int64_t i64;
        uint64_t u64;
        double f64;
        bool boolean;
        const char *str;
        const void *ptr;
    } value;
} ty_azote_Field;

typedef struct ty_azote_FieldSet {
    const ty_azote_Field *fields;
    size_t count;
} ty_azote_FieldSet;

static inline ty_azote_Field ty_azote_field_bool(const char *k, bool v)
{
    return (ty_azote_Field){.key = k, .type = TY_AZOTE_FIELD_BOOL, .value.boolean = v};
}

static inline ty_azote_Field ty_azote_field_i64(const char *k, long long v)
{
    return (ty_azote_Field){.key = k, .type = TY_AZOTE_FIELD_I64, .value.i64 = v};
}

static inline ty_azote_Field ty_azote_field_u64(const char *k, unsigned long long v)
{
    return (ty_azote_Field){.key = k, .type = TY_AZOTE_FIELD_U64, .value.u64 = v};
}

static inline ty_azote_Field ty_azote_field_f64(const char *k, double v)
{
    return (ty_azote_Field){.key = k, .type = TY_AZOTE_FIELD_F64, .value.f64 = v};
}

static inline ty_azote_Field ty_azote_field_str(const char *k, const char *v)
{
    return (ty_azote_Field){.key = k, .type = TY_AZOTE_FIELD_STR, .value.str = v};
}

static inline ty_azote_Field ty_azote_field_ptr(const char *k, const void *v)
{
    return (ty_azote_Field){.key = k, .type = TY_AZOTE_FIELD_PTR, .value.ptr = v};
}

/* Type-safe field constructor, dispatched at compile time (like tracing's Value). */
#define ty_azote_field(k, v) _Generic((v),                    \
	bool: ty_azote_field_bool,               \
	char: ty_azote_field_i64,                \
	signed char: ty_azote_field_i64,                \
	short: ty_azote_field_i64,                \
	int: ty_azote_field_i64,                \
	long: ty_azote_field_i64,                \
	long long: ty_azote_field_i64,                \
	unsigned char: ty_azote_field_u64,                \
	unsigned short: ty_azote_field_u64,                \
	unsigned int: ty_azote_field_u64,                \
	unsigned long: ty_azote_field_u64,                \
	unsigned long long: ty_azote_field_u64,                \
	float: ty_azote_field_f64,                \
	double: ty_azote_field_f64,                \
	char *: ty_azote_field_str,                \
	const char *: ty_azote_field_str,                \
	default: ty_azote_field_ptr                 \
)((k), (v))

/* Build an anonymous ty_azote_Field[] + its count. Requires >= 1 field. */
#define TY_AZOTE_FIELDS(...)                                                    \
	(ty_azote_FieldSet){                                                     \
		.fields = (const ty_azote_Field[]){__VA_ARGS__},                 \
		.count  = sizeof((const ty_azote_Field[]){__VA_ARGS__}) /        \
		          sizeof(ty_azote_Field),                                \
	}

typedef struct ty_azote_Event {
    ty_azote_Level level;
    const char *target;
    const char *file;
    uint32_t line;
    const char *function;
    struct timespec timestamp;
    uint64_t span_id; /* 0 == no active span */
    const char *message; /* fully expanded printf message */
    const ty_azote_Field *fields;
    size_t field_count;
} ty_azote_Event;

typedef struct ty_azote_Span {
    uint64_t id;
    ty_azote_Level level;
    const char *target;
    const char *name;
    const char *file;
    uint32_t line;
    struct timespec start;
    atomic_int ref_count;
    ty_azote_Field fields[TY_AZOTE_MAX_FIELDS];
    size_t field_count;
} ty_azote_Span;

typedef struct ty_azote_ScopedSpan {
    ty_azote_Span *span;
} ty_azote_ScopedSpan;

[[nodiscard]]
ty_azote_ScopedSpan ty_azote_span_enter_new(ty_azote_Level level, const char *target, const char *name,const char *file, uint32_t line,const ty_azote_Field *fields, size_t field_count);

void ty_azote_scoped_span_cleanup(ty_azote_ScopedSpan *guard);
uint64_t ty_azote_current_span_id(void);

/* RAII span guard: exits automatically at end of scope. */
#define TY_AZOTE_SPAN_FIELDS(name_, level_, fieldset_)                                  \
	[[maybe_unused]]                                                                       \
	ty_azote_ScopedSpan TY_AZOTE_CONCAT(_ty_azote_span_, __LINE__) TY_AZOTE_CLEANUP(ty_azote_scoped_span_cleanup) = ty_azote_span_enter_new((level_), TY_AZOTE_TARGET, (name_), __FILE__, __LINE__, (fieldset_).fields, (fieldset_).count)

#define TY_AZOTE_SPAN(name_)                                                            \
	[[maybe_unused]]                                                                       \
	ty_azote_ScopedSpan TY_AZOTE_CONCAT(_ty_azote_span_, __LINE__) TY_AZOTE_CLEANUP(ty_azote_scoped_span_cleanup) = ty_azote_span_enter_new(TY_AZOTE_LEVEL_INFO, TY_AZOTE_TARGET, (name_), __FILE__, __LINE__, nullptr, 0)

/* --------------------------------------------------------------------- *
 *  Layer (Observer) — analog of tracing_subscriber::Layer
 * --------------------------------------------------------------------- */

typedef struct ty_azote_LayerVTable {
    bool (*enabled)(void *self, ty_azote_Level level, const char *target);
    void (*on_event)(void *self, const ty_azote_Event *event);
    void (*on_span_enter)(void *self, const ty_azote_Span *span);
    void (*on_span_exit)(void *self, const ty_azote_Span *span);
    void (*on_span_close)(void *self, const ty_azote_Span *span,double duration_ms);
    /* Loosest (most verbose) level this layer could ever emit.
     * nullptr => "unknown", registry assumes TRACE (safe/conservative). */
    ty_azote_Level (*max_level_hint)(void *self);
    void (*destroy)(void *self);
} ty_azote_LayerVTable;

typedef struct ty_azote_Layer {
    void *self;
    const ty_azote_LayerVTable *vtable;
} ty_azote_Layer;

typedef struct ty_azote_Registry ty_azote_Registry;

[[nodiscard]]
ty_azote_Registry *ty_azote_registry_new(void);

void ty_azote_registry_free(ty_azote_Registry *reg);
ty_azote_Registry *ty_azote_registry_add_layer(ty_azote_Registry *reg, ty_azote_Layer layer);
void ty_azote_set_global_default(ty_azote_Registry *reg);
ty_azote_Registry *ty_azote_global_default(void);
bool ty_azote_level_enabled(ty_azote_Level level, const char *target);
void ty_azote_dispatch_event(const ty_azote_Event *event);

typedef struct ty_azote_EnvFilter ty_azote_EnvFilter;

[[nodiscard]]
ty_azote_EnvFilter *ty_azote_env_filter_new(const char *directives);

[[nodiscard]]
ty_azote_EnvFilter *ty_azote_env_filter_from_env(const char *var_name);

void ty_azote_env_filter_free(ty_azote_EnvFilter *filter);
bool ty_azote_env_filter_enabled(const ty_azote_EnvFilter *filter,ty_azote_Level level, const char *target);

typedef struct ty_azote_Writer {
    void *self;
    void (*write)(void *self, const char *data, size_t len);
    void (*flush)(void *self);
    void (*destroy)(void *self); /* may be nullptr for non-owning writers */
} ty_azote_Writer;

[[nodiscard]]
ty_azote_Writer ty_azote_stdio_writer(FILE *stream);

[[nodiscard]]
ty_azote_Writer ty_azote_file_writer_new(const char *path);

void ty_azote_writer_free(ty_azote_Writer *w);

typedef struct ty_azote_NonBlockingWriter ty_azote_NonBlockingWriter;

[[nodiscard]]
ty_azote_Writer ty_azote_non_blocking_new(ty_azote_Writer inner, size_t capacity, bool lossy,ty_azote_NonBlockingWriter **out_handle);
void ty_azote_non_blocking_shutdown(ty_azote_NonBlockingWriter *handle);
uint64_t ty_azote_non_blocking_dropped(const ty_azote_NonBlockingWriter *handle);

typedef enum ty_azote_FmtStyle {
    TY_AZOTE_FMT_COMPACT,
    TY_AZOTE_FMT_FULL,
    TY_AZOTE_FMT_JSON,
} ty_azote_FmtStyle;

typedef struct ty_azote_StrBuf ty_azote_StrBuf; /* opaque, see .c */
typedef void (*ty_azote_FormatFn)(ty_azote_StrBuf *out, const ty_azote_Event *ev, bool ansi);

typedef struct ty_azote_FmtLayerConfig {
    ty_azote_Writer writer;
    ty_azote_FmtStyle style;
    ty_azote_FormatFn custom_formatter; /* overrides `style` if non-null */
    bool ansi_color;
    bool show_file_line;
    bool show_thread_id;
    bool show_span_events;
    ty_azote_Level level_filter;     /* static per-layer threshold        */
    ty_azote_EnvFilter *env_filter;       /* optional, owned by the layer      */
} ty_azote_FmtLayerConfig;

[[nodiscard]]
ty_azote_Layer ty_azote_fmt_layer_new(ty_azote_FmtLayerConfig config);

void ty_azote_log_emit(ty_azote_Level level, const char *target,const char *file, uint32_t line, const char *function,const ty_azote_Field *fields, size_t field_count,const char *fmt, ...) TY_AZOTE_PRINTF(8, 9);

/* Quick-start facade: registry + fmt-layer(compact, stdio) + global default. */
ty_azote_Registry *ty_azote_init_default(FILE *stream, ty_azote_Level level, bool ansi_color);
void ty_azote_shutdown(void);

#define TY_AZOTE_LOG(level_, ...)                                                 \
	do {                                                                       \
		if (ty_azote_level_enabled((level_), TY_AZOTE_TARGET)) {          \
			ty_azote_log_emit((level_), TY_AZOTE_TARGET, __FILE__,    \
			                   __LINE__, __func__, nullptr, 0,        \
			                   __VA_ARGS__);                          \
		}                                                                  \
	} while (0)

#define TY_AZOTE_LOG_FIELDS(level_, fieldset_, ...)                               \
	do {                                                                       \
		if (ty_azote_level_enabled((level_), TY_AZOTE_TARGET)) {          \
			ty_azote_FieldSet _ty_fs = (fieldset_);                   \
			ty_azote_log_emit((level_), TY_AZOTE_TARGET, __FILE__, __LINE__, __func__, _ty_fs.fields, _ty_fs.count, __VA_ARGS__);            \
		}                                                                  \
	} while (0)

#define TY_AZOTE_TRACE(...) TY_AZOTE_LOG(TY_AZOTE_LEVEL_TRACE, __VA_ARGS__)
#define TY_AZOTE_DEBUG(...) TY_AZOTE_LOG(TY_AZOTE_LEVEL_DEBUG, __VA_ARGS__)
#define TY_AZOTE_INFO(...) TY_AZOTE_LOG(TY_AZOTE_LEVEL_INFO,  __VA_ARGS__)
#define TY_AZOTE_WARN(...) TY_AZOTE_LOG(TY_AZOTE_LEVEL_WARN,  __VA_ARGS__)
#define TY_AZOTE_ERROR(...) TY_AZOTE_LOG(TY_AZOTE_LEVEL_ERROR, __VA_ARGS__)

#define TY_AZOTE_TRACE_F(fs, ...) TY_AZOTE_LOG_FIELDS(TY_AZOTE_LEVEL_TRACE, fs, __VA_ARGS__)
#define TY_AZOTE_DEBUG_F(fs, ...) TY_AZOTE_LOG_FIELDS(TY_AZOTE_LEVEL_DEBUG, fs, __VA_ARGS__)
#define TY_AZOTE_INFO_F(fs, ...) TY_AZOTE_LOG_FIELDS(TY_AZOTE_LEVEL_INFO,  fs, __VA_ARGS__)
#define TY_AZOTE_WARN_F(fs, ...) TY_AZOTE_LOG_FIELDS(TY_AZOTE_LEVEL_WARN,  fs, __VA_ARGS__)
#define TY_AZOTE_ERROR_F(fs, ...) TY_AZOTE_LOG_FIELDS(TY_AZOTE_LEVEL_ERROR, fs, __VA_ARGS__)

[[noreturn]]
static inline void ty_azote_fatal_impl(void)
{
    ty_azote_shutdown();
    abort();
}

#define TY_AZOTE_FATAL(...)                                                       \
	do {                                                                       \
		ty_azote_log_emit(TY_AZOTE_LEVEL_FATAL, TY_AZOTE_TARGET, __FILE__, \
		                   __LINE__, __func__, nullptr, 0, __VA_ARGS__);   \
		ty_azote_fatal_impl();                                             \
	} while (0)

#ifdef __cplusplus
}
#endif

































