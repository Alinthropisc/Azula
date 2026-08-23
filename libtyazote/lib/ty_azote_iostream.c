#include <string.h>
#include <stdlib.h>

#include "ty_azote_csv.h"

ptrdiff_t ty_azote_csv_find_column(const char *header, const char *const *names,size_t names_len)
{
    ptrdiff_t idx = 0;
    const char *cursor = header;

    while (cursor)
    {
        const char *comma = strchr(cursor, ',');
        size_t field_len = comma ? (size_t)(comma - cursor) : strlen(cursor);

        for (size_t i = 0; i < names_len; ++i)
        {
            size_t name_len = strlen(names[i]);

            if (field_len == name_len && strncmp(cursor, names[i], name_len) == 0)
            {
                return idx;
            }
        }
        cursor = comma ? comma + 1 : nullptr;
        ++idx;
    }
    return -1;
}

ty_azote_StringSlice ty_azote_csv_field_at(const char *row, size_t idx)
{
    const char *cursor = row;

    for (size_t i = 0; i < idx; ++i)
    {
        cursor = strchr(cursor, ',');
        if (!cursor)
        {
            return (ty_azote_StringSlice){0}
        };
        ++cursor;
    }
    const char *comma = strchr(cursor, ',');
    size_t len = comma ? (size_t)(comma - cursor) : strlen(cursor);
    return (ty_azote_StringSlice){.data = cursor, .len = len};
}

char *ty_azote_csv_field_dup(const char *row, size_t idx)
{
    ty_azote_StringSlice s = ty_azote_csv_field_at(row, idx);

    if (!s.data)
    {
        return nullptr;
    }
    char *out = malloc(s.len + 1);

    if (!out)
    {
        return nullptr;
    }
    memcpy(out, s.data, s.len);
    out[s.len] = '\0';
    return out;
}

ty_azote_CsvRowIter ty_azote_csv_row_iter(const char *row)
{
    return (ty_azote_CsvRowIter){.cursor = row, .done = (row == nullptr)};
}

bool ty_azote_csv_row_next(ty_azote_CsvRowIter *it, ty_azote_StringSlice *out)
{
    if (it->done)
    {
        return false;
    }
    const char *comma = strchr(it->cursor, ',');
    size_t len = comma ? (size_t)(comma - it->cursor) : strlen(it->cursor);
    *out = (ty_azote_StringSlice){.data = it->cursor, .len = len};

    if (comma)
    {
        it->cursor = comma + 1;
    }
    else
    {
        it->done = true;
    }
    return true;
}





