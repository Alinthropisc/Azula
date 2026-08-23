#pragma once

#include <stddef.h>
#include <stdbool.h>
#include "ty_azote_support.h" /* ty_azote_StringSlice, ty_azote_StringList */

#ifdef __cplusplus
extern "C" {
#endif

/* Finds the column index in `header` whose field exactly matches one of
 * `names` (unlike the original, this compares full field length, not just
 * a prefix — "ip" no longer spuriously matches a header field "ip_address").
 * Returns -1 if not found. */
[[nodiscard]] ptrdiff_t ty_azote_csv_find_column(const char *header,const char *const *names,size_t names_len);

/* Zero-copy view of the field at `idx` in `row`. Returns {nullptr,0} if
 * idx is out of range. The slice aliases `row` — do not outlive it. */
[[nodiscard]]
ty_azote_StringSlice ty_azote_csv_field_at(const char *row, size_t idx);

/* Owning variant, for callers that need a heap copy. */
[[nodiscard]]
char *ty_azote_csv_field_dup(const char *row, size_t idx);

/* Single-pass iterator — O(n) total instead of O(idx) per field lookup
 * when reading every field of a row sequentially. */
typedef struct ty_azote_CsvRowIter {
    const char *cursor;
    bool done;
} ty_azote_CsvRowIter;

[[nodiscard]]
ty_azote_CsvRowIter ty_azote_csv_row_iter(const char *row);

[[nodiscard]]
bool ty_azote_csv_row_next(ty_azote_CsvRowIter *it, ty_azote_StringSlice *out);

#ifdef __cplusplus
}
#endif



































































