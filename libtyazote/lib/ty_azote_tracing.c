#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "ty_azote_tracing.h"


struct ty_azote_StrBuf {
    char *data;
    size_t len;
    size_t cap;
};

static void sb_free(ty_azote_StrBuf *sb)
{
    free(sb->data);
    *sb = (ty_azote_StrBuf){0};
}

static void sb_reserve(ty_azote_StrBuf *sb, size_t extra)
{
    if (sb->len + extra + 1 <= sb->cap)
    {
        return;
    }
    size_t ncap = sb->cap ? sb->cap * 2 : 128;

    while (ncap < sb->len + extra + 1)
    {
        ncap *= 2;
    }
    char *nd = realloc(sb->data, ncap);

    if (!nd)
    {
        return; /* best-effort: OOM truncates output */
    }
    sb->data = nd;
    sb->cap  = ncap;
}

static void sb_appendn(ty_azote_StrBuf *sb, const char *s, size_t n)
{
    sb_reserve(sb, n);

    if (!sb->data)
    {
        return;
    }
    memcpy(sb->data + sb->len, s, n);
    sb->len += n;
    sb->data[sb->len] = '\0';
}

static void sb_append(ty_azote_StrBuf *sb, const char *s)
{
    sb_appendn(sb, s, strlen(s));
}

static void sb_appendf(ty_azote_StrBuf *sb, const char *fmt, ...) TY_AZOTE_PRINTF(2, 3);

static void sb_appendf(ty_azote_StrBuf *sb, const char *fmt, ...)
{
    va_list ap, ap2;
    va_start(ap, fmt);
    va_copy(ap2, ap);
    int need = vsnprintf(nullptr, 0, fmt, ap);
    va_end(ap);
    if (need < 0)
    {
        va_end(ap2);
        return;
    }
    sb_reserve(sb, (size_t)need);

    if (sb->data)
    {
        vsnprintf(sb->data + sb->len, (size_t)need + 1, fmt, ap2);
        sb->len += (size_t)need;
    }
    va_end(ap2);
}

static void sb_append_json_escaped(ty_azote_StrBuf *sb, const char *s)
{
    sb_append(sb, "\"");

    for (const unsigned char *p = (const unsigned char *)s; *p; ++p)
    {
        switch (*p)
        {
            case '"':
                sb_append(sb, "\\\"");
                break;
            case '\\':
                sb_append(sb, "\\\\");
                break;
            case '\n':
                sb_append(sb, "\\n");
                break;
            case '\r':
                sb_append(sb, "\\r");
                break;
            case '\t':
                sb_append(sb, "\\t");
                break;
            default:
                if (*p < 0x20)
                {
                    sb_appendf(sb, "\\u%04x", *p);
                }
                else
                {
                    sb_appendn(sb, (const char *)p, 1);
                }
        }
    }
    sb_append(sb, "\"");
}

static const char *const g_level_names[] = {
        "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL", "OFF",
};

#define RESET "\x1b[0m"
#define GRAY "\x1b[90m"
#define BLUE "\x1b[34m"
#define GREEN "\x1b[32m"
#define YELLOW "\x1b[33m"
#define RED "\x1b[31m"
#define MAGENTA "\x1b[35m"

static const char *const g_level_colors[] = {
        GRAY, BLUE, GREEN, YELLOW, RED, MAGENTA, RESET,
};

const char *ty_azote_level_name(ty_azote_Level level)
{
    if (level > TY_AZOTE_LEVEL_OFF)
    {
        return "UNKNOWN";
    }
    return g_level_names[level];
}

const char *ty_azote_level_color(ty_azote_Level level)
{
    if (level > TY_AZOTE_LEVEL_OFF)
    {
        return RESET;
    }
    return g_level_colors[level];
}

static bool ieq(const char *a, const char *b)
{
    while (*a && *b)
    {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
        {
            return false;
        }
        ++a; ++b;
    }
    return *a == *b;
}

bool ty_azote_level_from_str(const char *s, ty_azote_Level *out)
{
    for (ty_azote_Level l = TY_AZOTE_LEVEL_TRACE; l <= TY_AZOTE_LEVEL_OFF; ++l)
    {
        if (ieq(s, g_level_names[l]))
        {
            *out = l;
            return true;
        }
    }
    return false;
}

#if defined(TIME_MONOTONIC)
#define TY_AZOTE_MONO_BASE TIME_MONOTONIC
#else
#define TY_AZOTE_MONO_BASE TIME_UTC
#endif

static struct timespec ty_azote_now_wall(void)
{
    struct timespec ts = {0};
    timespec_get(&ts, TIME_UTC);
    return ts;
}

static struct timespec ty_azote_now_mono(void)
{
    struct timespec ts = {0};
    timespec_get(&ts, TY_AZOTE_MONO_BASE);
    return ts;
}

static double ts_diff_ms(struct timespec a, struct timespec b)
{
    return (double)(b.tv_sec - a.tv_sec) * 1000.0 + (double)(b.tv_nsec - a.tv_nsec) / 1e6;
}

static uint32_t ty_azote_current_thread_id(void)
{
    static atomic_uint_fast32_t next_id = 0;
    static thread_local uint32_t tls_id = 0;
    static thread_local bool tls_assigned = false;

    if (!tls_assigned)
    {
        tls_id = (uint32_t)atomic_fetch_add_explicit(&next_id, 1, memory_order_relaxed);
        tls_assigned = true;
    }
    return tls_id;
}

static atomic_uint_fast64_t g_span_id_counter = 1;

typedef struct ty_azote_SpanStack {
    ty_azote_Span *frames[TY_AZOTE_MAX_SPAN_DEPTH];
    size_t depth;
} ty_azote_SpanStack;

static thread_local ty_azote_SpanStack tls_span_stack;

uint64_t ty_azote_current_span_id(void)
{
    if (tls_span_stack.depth == 0)
    {
        return 0;
    }
    return tls_span_stack.frames[tls_span_stack.depth - 1]->id;
}

static ty_azote_Span *current_span_ptr(void)
{
    return tls_span_stack.depth ? tls_span_stack.frames[tls_span_stack.depth - 1] : nullptr;
}


typedef struct ty_azote_LayerNode {
    ty_azote_Layer layer;
    struct ty_azote_LayerNode  *next;
} ty_azote_LayerNode;

struct ty_azote_Registry {
    _Atomic(ty_azote_LayerNode *) head;
    atomic_uint_fast8_t max_level; /* fast-path shortcut, tracing's Interest */
};

ty_azote_Registry *ty_azote_registry_new(void)
{
    ty_azote_Registry *reg = malloc(sizeof *reg);

    if (!reg)
    {
        return nullptr;
    }
    atomic_init(&reg->head, nullptr);
    atomic_init(&reg->max_level, TY_AZOTE_LEVEL_OFF);
    return reg;
}

static void recompute_max_level(ty_azote_Registry *reg)
{
    ty_azote_Level min_level = TY_AZOTE_LEVEL_OFF;

    for (ty_azote_LayerNode *n = atomic_load_explicit(&reg->head, memory_order_acquire); n; n = n->next)
    {
        ty_azote_Level hint = TY_AZOTE_LEVEL_TRACE;

        if (n->layer.vtable->max_level_hint)
        {
            hint = n->layer.vtable->max_level_hint(n->layer.self);
        }

        if (hint < min_level)
        {
            min_level = hint;
        }
    }
    atomic_store_explicit(&reg->max_level, min_level, memory_order_relaxed);
}

ty_azote_Registry *ty_azote_registry_add_layer(ty_azote_Registry *reg, ty_azote_Layer layer)
{
    if (!reg)
    {
        return reg;
    }
    ty_azote_LayerNode *node = malloc(sizeof *node);

    if (!node)
    {
        return reg;
    }
    node->layer = layer;
    ty_azote_LayerNode *old_head = atomic_load_explicit(&reg->head, memory_order_relaxed);

    do {
        node->next = old_head;
    } while (!atomic_compare_exchange_weak_explicit(&reg->head, &old_head, node, memory_order_release, memory_order_relaxed));
    recompute_max_level(reg);
    return reg;
}

void ty_azote_registry_free(ty_azote_Registry *reg)
{
    if (!reg)
    {
        return;
    }
    ty_azote_LayerNode *n = atomic_load_explicit(&reg->head, memory_order_acquire);

    while (n)
    {
        ty_azote_LayerNode *next = n->next;

        if (n->layer.vtable->destroy)
        {
            n->layer.vtable->destroy(n->layer.self);
        }
        free(n);
        n = next;
    }
    free(reg);
}

static _Atomic(ty_azote_Registry *) g_default_registry = nullptr;

void ty_azote_set_global_default(ty_azote_Registry *reg)
{
    atomic_store_explicit(&g_default_registry, reg, memory_order_release);
}

ty_azote_Registry *ty_azote_global_default(void)
{
    return atomic_load_explicit(&g_default_registry, memory_order_acquire);
}

bool ty_azote_level_enabled(ty_azote_Level level, const char *target)
{
    (void)target;
    ty_azote_Registry *reg = ty_azote_global_default();

    if (!reg)
    {
        return false;
    }
    ty_azote_Level threshold = atomic_load_explicit(&reg->max_level, memory_order_relaxed);
    return level >= threshold;
}

static void dispatch_span_enter(ty_azote_Registry *reg, const ty_azote_Span *span)
{
    if (!reg)
    {
        return;
    }

    for (ty_azote_LayerNode *n = atomic_load_explicit(&reg->head, memory_order_acquire); n; n = n->next)
    {
        if (n->layer.vtable->enabled && !n->layer.vtable->enabled(n->layer.self, span->level, span->target))
        {
            continue;
        }

        if (n->layer.vtable->on_span_enter)
        {
            n->layer.vtable->on_span_enter(n->layer.self, span);
        }
    }
}

static void dispatch_span_exit(ty_azote_Registry *reg, const ty_azote_Span *span)
{
    if (!reg)
    {
        return;
    }

    for (ty_azote_LayerNode *n = atomic_load_explicit(&reg->head, memory_order_acquire); n; n = n->next)
    {
        if (n->layer.vtable->enabled && !n->layer.vtable->enabled(n->layer.self, span->level, span->target))
        {
            continue;
        }

        if(n->layer.vtable->on_span_exit)
        {
            n->layer.vtable->on_span_exit(n->layer.self, span);
        }
    }
}

static void dispatch_span_close(ty_azote_Registry *reg, const ty_azote_Span *span, double dur_ms)
{
    if (!reg)
    {
        return;
    }

    for (ty_azote_LayerNode *n = atomic_load_explicit(&reg->head, memory_order_acquire); n; n = n->next)
    {
        if (n->layer.vtable->enabled && !n->layer.vtable->enabled(n->layer.self, span->level, span->target))
        {
            continue;
        }

        if (n->layer.vtable->on_span_close)
        {
            n->layer.vtable->on_span_close(n->layer.self, span, dur_ms);
        }
    }
}

void ty_azote_dispatch_event(const ty_azote_Event *event)
{
    ty_azote_Registry *reg = ty_azote_global_default();

    if (!reg)
    {
        return;
    }
    ty_azote_Level threshold = atomic_load_explicit(&reg->max_level, memory_order_relaxed);

    if (event->level < threshold)
    {
        return;
    }

    for (ty_azote_LayerNode *n = atomic_load_explicit(&reg->head, memory_order_acquire); n; n = n->next)
    {
        if (n->layer.vtable->enabled && !n->layer.vtable->enabled(n->layer.self, event->level, event->target))
        {
            continue;
        }

        if (n->layer.vtable->on_event)
        {
            n->layer.vtable->on_event(n->layer.self, event);
        }
    }
}

ty_azote_ScopedSpan ty_azote_span_enter_new(ty_azote_Level level, const char *target, const char *name,const char *file, uint32_t line,const ty_azote_Field *fields, size_t field_count)
{
    ty_azote_Span *span = malloc(sizeof *span);
    *span = (ty_azote_Span) {
            .id = atomic_fetch_add_explicit(&g_span_id_counter, 1, memory_order_relaxed),
            .level = level,
            .target = target,
            .name = name,
            .file = file,
            .line = line,
            .start = ty_azote_now_mono(),
            .ref_count = 1,
            .field_count = field_count > TY_AZOTE_MAX_FIELDS ? TY_AZOTE_MAX_FIELDS : field_count,
    };

    for (size_t i = 0; i < span->field_count; ++i)
    {
        span->fields[i] = fields[i];
    }

    if (tls_span_stack.depth < TY_AZOTE_MAX_SPAN_DEPTH)
    {
        tls_span_stack.frames[tls_span_stack.depth++] = span;
    }
    else
    {
        fprintf(stderr, "ty_azote_tracing: span stack overflow, span '%s' not tracked\n", name);
    }
    dispatch_span_enter(ty_azote_global_default(), span);
    return (ty_azote_ScopedSpan){.span = span};
}

static void ty_azote_span_exit(ty_azote_Span *span)
{
    if (!span)
    {
        return;
    }
    ty_azote_Registry *reg = ty_azote_global_default();
    dispatch_span_exit(reg, span);

    if (tls_span_stack.depth && tls_span_stack.frames[tls_span_stack.depth - 1] == span)
    {
        --tls_span_stack.depth;
    }
    else
    {
        fprintf(stderr, "ty_azote_tracing: span exit out of order for '%s'\n", span->name);
    }

    if (atomic_fetch_sub_explicit(&span->ref_count, 1, memory_order_acq_rel) == 1)
    {
        double dur = ts_diff_ms(span->start, ty_azote_now_mono());
        dispatch_span_close(reg, span, dur);
        free(span);
    }
}

void ty_azote_scoped_span_cleanup(ty_azote_ScopedSpan *guard)
{
    ty_azote_span_exit(guard->span);
}


void ty_azote_log_emit(ty_azote_Level level, const char *target,const char *file, uint32_t line, const char *function,const ty_azote_Field *fields, size_t field_count,const char *fmt, ...)
{
    char stack_buf[1024];
    char *msg = stack_buf;
    bool  heap = false;
    va_list ap, ap2;
    va_start(ap, fmt);
    va_copy(ap2, ap);
    int need = vsnprintf(stack_buf, sizeof stack_buf, fmt, ap);
    va_end(ap);

    if (need >= (int)sizeof stack_buf)
    {
        msg = malloc((size_t)need + 1);

        if (msg)
        {
            vsnprintf(msg, (size_t)need + 1, fmt, ap2);
            heap = true;
        }
        else
        {
            msg = stack_buf; /* fallback: truncated message */
        }
    }
    va_end(ap2);

    ty_azote_Event ev = {
            .level = level,
            .target = target,
            .file = file,
            .line = line,
            .function = function,
            .timestamp = ty_azote_now_wall(),
            .span_id = ty_azote_current_span_id(),
            .message = msg,
            .fields = fields,
            .field_count = field_count,
    };
    ty_azote_dispatch_event(&ev);

    if (heap)
    {
        free(msg);
    }
}


typedef struct ty_azote_FilterDirective {
    char *target; /* nullptr => default directive */
    ty_azote_Level level;
} ty_azote_FilterDirective;

struct ty_azote_EnvFilter {
    ty_azote_FilterDirective *directives;
    size_t count;
    ty_azote_Level default_level;
};

ty_azote_EnvFilter *ty_azote_env_filter_new(const char *directives_str)
{
    ty_azote_EnvFilter *f = calloc(1, sizeof *f);
    f->default_level = TY_AZOTE_LEVEL_INFO;

    if (!directives_str || !*directives_str)
    {
        return f;
    }
    char *copy = strdup(directives_str);
    size_t cap = 4;
    f->directives = malloc(cap * sizeof *f->directives);

    for (char *tok = strtok(copy, ","); tok; tok = strtok(nullptr, ","))
    {
        while (isspace((unsigned char)*tok)) ++tok;
        char *eq = strchr(tok, '=');
        ty_azote_Level lvl;

        if (eq)
        {
            *eq = '\0';

            if (!ty_azote_level_from_str(eq + 1, &lvl))
            {
                continue;
            }

            if (f->count == cap)
            {
                cap *= 2; f->directives = realloc(f->directives, cap * sizeof *f->directives);
            }
            f->directives[f->count++] = (ty_azote_FilterDirective){.target = strdup(tok), .level = lvl};
        }
        else
        {
            if (ty_azote_level_from_str(tok, &lvl))
            {
                f->default_level = lvl;
            }
        }
    }
    free(copy);
    return f;
}

ty_azote_EnvFilter *ty_azote_env_filter_from_env(const char *var_name)
{
    const char *v = getenv(var_name);
    return ty_azote_env_filter_new(v ? v : "");
}

void ty_azote_env_filter_free(ty_azote_EnvFilter *filter)
{
    if (!filter)
    {
        return;
    }

    for (size_t i = 0; i < filter->count; ++i)
    {
        free(filter->directives[i].target);
    }
    free(filter->directives);
    free(filter);
}

bool ty_azote_env_filter_enabled(const ty_azote_EnvFilter *filter,ty_azote_Level level, const char *target)
{
    if (!filter)
    {
        return true;
    }
    size_t best_len = 0;
    ty_azote_Level effective = filter->default_level;

    for (size_t i = 0; i < filter->count; ++i)
    {
        const char *t = filter->directives[i].target;
        size_t tl = strlen(t);

        if (strncmp(target, t, tl) == 0 && tl >= best_len)
        {
            best_len  = tl;
            effective = filter->directives[i].level;
        }
    }
    return level >= effective;
}


typedef struct ty_azote_StdioWriter {
    FILE *stream;
    mtx_t mutex;
    bool  owns_stream;
} ty_azote_StdioWriter;

static void stdio_write(void *self, const char *data, size_t len)
{
    ty_azote_StdioWriter *w = self;
    mtx_lock(&w->mutex);
    fwrite(data, 1, len, w->stream);
    mtx_unlock(&w->mutex);
}

static void stdio_flush(void *self)
{
    ty_azote_StdioWriter *w = self;
    mtx_lock(&w->mutex);
    fflush(w->stream);
    mtx_unlock(&w->mutex);
}

static void stdio_destroy(void *self)
{
    ty_azote_StdioWriter *w = self;

    if (w->owns_stream)
    {
        fclose(w->stream);
    }
    mtx_destroy(&w->mutex);
    free(w);
}

static ty_azote_Writer make_stdio_writer(FILE *stream, bool owns)
{
    ty_azote_StdioWriter *w = malloc(sizeof *w);
    w->stream = stream;
    w->owns_stream = owns;
    mtx_init(&w->mutex, mtx_plain);
    return (ty_azote_Writer){.self = w, .write = stdio_write, .flush = stdio_flush, .destroy = stdio_destroy,};
}

ty_azote_Writer ty_azote_stdio_writer(FILE *stream)
{
    return make_stdio_writer(stream, false);
}

ty_azote_Writer ty_azote_file_writer_new(const char *path)
{
    FILE *f = fopen(path, "a");

    if (!f)
    {
        return (ty_azote_Writer){0}
    };
    return make_stdio_writer(f, true);
}

void ty_azote_writer_free(ty_azote_Writer *w)
{
    if (w->destroy)
    {
        w->destroy(w->self);
    }
    *w = (ty_azote_Writer){0};
}

/* ---- non-blocking decorator ------------------------------------------ */

typedef struct ty_azote_NbMessage {
    char *data;
    size_t len;
} ty_azote_NbMessage;

struct ty_azote_NonBlockingWriter {
    ty_azote_Writer inner;
    ty_azote_NbMessage *ring;
    size_t capacity, head, tail, count;
    mtx_t mutex;
    cnd_t not_empty;
    cnd_t not_full;
    atomic_bool running;
    atomic_uint_fast64_t dropped;
    bool lossy;
    thrd_t worker;
};

static int nb_worker_main(void *arg)
{
    ty_azote_NonBlockingWriter *nb = arg;

    for (;;)
    {
        mtx_lock(&nb->mutex);

        while (nb->count == 0 && atomic_load_explicit(&nb->running, memory_order_acquire))
        {
            cnd_wait(&nb->not_empty, &nb->mutex);
        }

        if (nb->count == 0 && !atomic_load_explicit(&nb->running, memory_order_acquire))
        {
            mtx_unlock(&nb->mutex);
            break;
        }
        ty_azote_NbMessage msg = nb->ring[nb->head];
        nb->head = (nb->head + 1) % nb->capacity;
        --nb->count;
        cnd_broadcast(&nb->not_full);
        mtx_unlock(&nb->mutex);
        nb->inner.write(nb->inner.self, msg.data, msg.len);
        free(msg.data);
    }

    if (nb->inner.flush)
    {
        nb->inner.flush(nb->inner.self);
    }
    return 0;
}

static void nb_write(void *self, const char *data, size_t len)
{
    ty_azote_NonBlockingWriter *nb = self;
    char *copy = malloc(len);

    if (!copy)
    {
        return;
    }
    memcpy(copy, data, len);
    mtx_lock(&nb->mutex);

    if (nb->count == nb->capacity)
    {
        if (nb->lossy)
        {
            mtx_unlock(&nb->mutex);
            atomic_fetch_add_explicit(&nb->dropped, 1, memory_order_relaxed);
            free(copy);
            return;
        }

        while (nb->count == nb->capacity)
        {
            cnd_wait(&nb->not_full, &nb->mutex);
        }
    }
    nb->ring[nb->tail] = (ty_azote_NbMessage){.data = copy, .len = len};
    nb->tail = (nb->tail + 1) % nb->capacity;
    ++nb->count;
    cnd_signal(&nb->not_empty);
    mtx_unlock(&nb->mutex);
}

static void nb_flush(void *self)
{
    ty_azote_NonBlockingWriter *nb = self;
    mtx_lock(&nb->mutex);
    while (nb->count > 0) cnd_wait(&nb->not_full, &nb->mutex);
    mtx_unlock(&nb->mutex);

    if (nb->inner.flush)
    {
        nb->inner.flush(nb->inner.self);
    }
}

ty_azote_Writer ty_azote_non_blocking_new(ty_azote_Writer inner, size_t capacity, bool lossy,ty_azote_NonBlockingWriter **out_handle)
{
    ty_azote_NonBlockingWriter *nb = malloc(sizeof *nb);
    *nb = (ty_azote_NonBlockingWriter){
            .inner = inner,
            .ring  = calloc(capacity, sizeof(ty_azote_NbMessage)),
            .capacity = capacity,
            .lossy = lossy,
            .running = true,
    };
    mtx_init(&nb->mutex, mtx_plain);
    cnd_init(&nb->not_empty);
    cnd_init(&nb->not_full);
    thrd_create(&nb->worker, nb_worker_main, nb);

    if (out_handle)
    {
        *out_handle = nb;
    }
    return (ty_azote_Writer){.self = nb, .write = nb_write, .flush = nb_flush, .destroy = nullptr};
}

void ty_azote_non_blocking_shutdown(ty_azote_NonBlockingWriter *handle) {
    if (!handle)
    {
        return;
    }
    mtx_lock(&handle->mutex);
    atomic_store_explicit(&handle->running, false, memory_order_release);
    cnd_broadcast(&handle->not_empty);
    mtx_unlock(&handle->mutex);
    thrd_join(handle->worker, nullptr);

    if (handle->inner.destroy)
    {
        handle->inner.destroy(handle->inner.self);
    }
    mtx_destroy(&handle->mutex);
    cnd_destroy(&handle->not_empty);
    cnd_destroy(&handle->not_full);
    free(handle->ring);
    free(handle);
}

uint64_t ty_azote_non_blocking_dropped(const ty_azote_NonBlockingWriter *handle)
{
    return atomic_load_explicit(&handle->dropped, memory_order_relaxed);
}

static void append_field_compact(ty_azote_StrBuf *sb, const ty_azote_Field *f)
{
    sb_appendf(sb, " %s=", f->key);

    switch (f->type)
    {
        case TY_AZOTE_FIELD_I64:
            sb_appendf(sb, "%lld", (long long)f->value.i64);
            break;
        case TY_AZOTE_FIELD_U64:
            sb_appendf(sb, "%llu", (unsigned long long)f->value.u64);
            break;
        case TY_AZOTE_FIELD_F64:
            sb_appendf(sb, "%g", f->value.f64);
            break;
        case TY_AZOTE_FIELD_BOOL:
            sb_append(sb, f->value.boolean ? "true" : "false");
            break;
        case TY_AZOTE_FIELD_STR:
            sb_appendf(sb, "\"%s\"", f->value.str);
            break;
        case TY_AZOTE_FIELD_PTR:
            sb_appendf(sb, "%p", f->value.ptr);
            break;
    }
}

static void append_field_json(ty_azote_StrBuf *sb, const ty_azote_Field *f, bool first)
{
    if (!first)
    {
        sb_append(sb, ",");
    }
    sb_append_json_escaped(sb, f->key);
    sb_append(sb, ":");

    switch (f->type)
    {
        case TY_AZOTE_FIELD_I64:
            sb_appendf(sb, "%lld", (long long)f->value.i64);
            break;
        case TY_AZOTE_FIELD_U64:
            sb_appendf(sb, "%llu", (unsigned long long)f->value.u64);
            break;
        case TY_AZOTE_FIELD_F64:
            sb_appendf(sb, "%g", f->value.f64);
            break;
        case TY_AZOTE_FIELD_BOOL:
            sb_append(sb, f->value.boolean ? "true" : "false");
            break;
        case TY_AZOTE_FIELD_STR:
            sb_append_json_escaped(sb, f->value.str);
            break;
        case TY_AZOTE_FIELD_PTR:
            sb_appendf(sb, "\"%p\"", f->value.ptr);
            break;
    }
}

static void fmt_timestamp(ty_azote_StrBuf *sb, struct timespec ts)
{
    struct tm tmv;
    time_t sec = ts.tv_sec;
    localtime_r(&sec, &tmv);
    char buf[32];
    strftime(buf, sizeof buf, "%H:%M:%S", &tmv);
    sb_appendf(sb, "%s.%03ld", buf, ts.tv_nsec / 1000000);
}

static void fmt_compact(ty_azote_StrBuf *out, const ty_azote_Event *ev, bool ansi)
{
    if (ansi)
    {
        sb_append(out, ty_azote_level_color(ev->level));
    }
    fmt_timestamp(out, ev->timestamp);
    sb_appendf(out, " %-5s ", ty_azote_level_name(ev->level));

    if (ansi)
    {
        sb_append(out, RESET);
    }
    sb_appendf(out, "%s: %s", ev->target, ev->message);

    for (size_t i = 0; i < ev->field_count; ++i)
    {
        append_field_compact(out, &ev->fields[i]);
    }
    sb_append(out, "\n");
}

static void fmt_full(ty_azote_StrBuf *out, const ty_azote_Event *ev, bool ansi)
{
    if (ansi)
    {
        sb_append(out, ty_azote_level_color(ev->level));
    }
    fmt_timestamp(out, ev->timestamp);
    sb_appendf(out, " %-5s ", ty_azote_level_name(ev->level));

    if (ansi)
    {
        sb_append(out, RESET);
    }
    sb_appendf(out, "thread=%u %s (%s:%u)", ty_azote_current_thread_id(), ev->target, ev->file, ev->line);

    if (ev->span_id)
    {
        sb_appendf(out, " span=%llu", (unsigned long long)ev->span_id);
    }
    sb_appendf(out, ": %s", ev->message);

    for (size_t i = 0; i < ev->field_count; ++i)
    {
        append_field_compact(out, &ev->fields[i]);
    }
    sb_append(out, "\n");
}

static void fmt_json(ty_azote_StrBuf *out, const ty_azote_Event *ev, bool ansi)
{
    (void)ansi;
    sb_append(out, "{\"ts\":\"");
    fmt_timestamp(out, ev->timestamp);
    sb_appendf(out, "\",\"level\":\"%s\",\"target\":", ty_azote_level_name(ev->level));
    sb_append_json_escaped(out, ev->target);
    sb_appendf(out, ",\"file\":\"%s\",\"line\":%u,\"message\":", ev->file, ev->line);
    sb_append_json_escaped(out, ev->message);

    if (ev->span_id)
    {
        sb_appendf(out, ",\"span_id\":%llu", (unsigned long long)ev->span_id);
    }
    sb_append(out, ",\"fields\":{");

    for (size_t i = 0; i < ev->field_count; ++i)
    {
        append_field_json(out, &ev->fields[i], i == 0);
    }
    sb_append(out, "}}\n");
}


typedef struct ty_azote_FmtLayerState {
    ty_azote_FmtLayerConfig cfg;
    ty_azote_FormatFn fmt_fn;
} ty_azote_FmtLayerState;

static bool fmt_layer_enabled(void *self, ty_azote_Level level, const char *target)
{
    ty_azote_FmtLayerState *st = self;

    if (st->cfg.env_filter)
    {
        return ty_azote_env_filter_enabled(st->cfg.env_filter, level, target);
    }
    return level >= st->cfg.level_filter;
}

static void fmt_layer_on_event(void *self, const ty_azote_Event *ev)
{
    ty_azote_FmtLayerState *st = self;
    ty_azote_StrBuf sb = {0};
    st->fmt_fn(&sb, ev, st->cfg.ansi_color);

    if (sb.data)
    {
        st->cfg.writer.write(st->cfg.writer.self, sb.data, sb.len);
    }
    sb_free(&sb);
}

static void fmt_layer_on_span_enter(void *self, const ty_azote_Span *span)
{
    ty_azote_FmtLayerState *st = self;

    if (!st->cfg.show_span_events)
    {
        return;
    }
    ty_azote_Event ev = {
            .level = span->level, .target = span->target, .file = span->file,
            .line = span->line, .function = "<span>", .timestamp = ty_azote_now_wall(),
            .span_id = span->id, .message = "enter span",
            .fields = span->fields, .field_count = span->field_count,
    };
    fmt_layer_on_event(self, &ev);
}

static void fmt_layer_on_span_exit(void *self, const ty_azote_Span *span)
{
    ty_azote_FmtLayerState *st = self;

    if (!st->cfg.show_span_events)
    {
        return;
    }
    ty_azote_Event ev = {
            .level = span->level, .target = span->target, .file = span->file,
            .line = span->line, .function = "<span>", .timestamp = ty_azote_now_wall(),
            .span_id = span->id, .message = "exit span",
    };
    fmt_layer_on_event(self, &ev);
}

static void fmt_layer_on_span_close(void *self, const ty_azote_Span *span, double dur_ms)
{
    ty_azote_FmtLayerState *st = self;

    if (!st->cfg.show_span_events)
    {
        return;
    }
    char msg[128];
    snprintf(msg, sizeof msg, "close span '%s' (%.3f ms)", span->name, dur_ms);
    ty_azote_Event ev = {
            .level = span->level, .target = span->target, .file = span->file,
            .line = span->line, .function = "<span>", .timestamp = ty_azote_now_wall(),
            .span_id = span->id, .message = msg,
    };
    fmt_layer_on_event(self, &ev);
}

static ty_azote_Level fmt_layer_max_level_hint(void *self)
{
    ty_azote_FmtLayerState *st = self;
    return st->cfg.env_filter ? TY_AZOTE_LEVEL_TRACE : st->cfg.level_filter;
}

static void fmt_layer_destroy(void *self)
{
    ty_azote_FmtLayerState *st = self;

    if (st->cfg.env_filter)
    {
        ty_azote_env_filter_free(st->cfg.env_filter);
    }

    if (st->cfg.writer.destroy)
    {
        st->cfg.writer.destroy(st->cfg.writer.self);
    }
    free(st);
}

static const ty_azote_LayerVTable g_fmt_layer_vtable = {
        .enabled = fmt_layer_enabled,
        .on_event = fmt_layer_on_event,
        .on_span_enter = fmt_layer_on_span_enter,
        .on_span_exit = fmt_layer_on_span_exit,
        .on_span_close = fmt_layer_on_span_close,
        .max_level_hint = fmt_layer_max_level_hint,
        .destroy = fmt_layer_destroy,
};

ty_azote_Layer ty_azote_fmt_layer_new(ty_azote_FmtLayerConfig config)
{
    ty_azote_FmtLayerState *st = malloc(sizeof *st);
    st->cfg = config;

    if (config.custom_formatter)
    {
        st->fmt_fn = config.custom_formatter;
    }
    else
    {
        switch (config.style)
        {
            case TY_AZOTE_FMT_FULL:
                st->fmt_fn = fmt_full;
                break;
            case TY_AZOTE_FMT_JSON:
                st->fmt_fn = fmt_json;
                break;
            default:
                st->fmt_fn = fmt_compact;
                break;
        }
    }
    return (ty_azote_Layer){.self = st, .vtable = &g_fmt_layer_vtable};
}


ty_azote_Registry *ty_azote_init_default(FILE *stream, ty_azote_Level level, bool ansi_color)
{
    ty_azote_Registry *reg = ty_azote_registry_new();
    ty_azote_Layer layer = ty_azote_fmt_layer_new((ty_azote_FmtLayerConfig){
            .writer = ty_azote_stdio_writer(stream),
            .style = TY_AZOTE_FMT_COMPACT,
            .ansi_color = ansi_color,
            .level_filter = level,
    });
    ty_azote_registry_add_layer(reg, layer);
    ty_azote_set_global_default(reg);
    atexit(ty_azote_shutdown);
    return reg;
}

void ty_azote_shutdown(void)
{
    ty_azote_Registry *reg = ty_azote_global_default();

    if (!reg)
    {
        return;
    }
    ty_azote_set_global_default(nullptr);
    ty_azote_registry_free(reg);
}