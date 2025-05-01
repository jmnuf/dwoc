#ifndef __DWOC_UTILS_H
#define __DWOC_UTILS_H

#ifndef NOB_IMPLEMENTATION
#include "nob.h"
#endif

typedef struct {
  Nob_String_View *items;
  size_t count;
  size_t capacity;
} StringViews;

#define da_pop(da) (NOB_ASSERT((da)->count > 0 && "Attempting to pop from empty array"), --(da)->count)

#define comp_error(loc, message) fprintf(stderr, "%s:%zu:%zu: [ERROR] %s\n", loc.source_path, loc.row, loc.col, message)
#define comp_errorf(loc, fmt, ...) fprintf(stderr, "%s:%zu:%zu: [ERROR] "fmt"\n", loc.source_path, loc.row, loc.col, __VA_ARGS__)
#define comp_warn(loc, message) fprintf(stderr, "%s:%zu:%zu: [WARN] %s\n", loc.source_path, loc.row, loc.col, message)
#define comp_warnf(loc, fmt, ...) fprintf(stderr, "%s:%zu:%zu: [WARN] "fmt"\n", loc.source_path, loc.row, loc.col, __VA_ARGS__)
#define comp_note(loc, message) printf("%s:%zu:%zu: %s\n", loc.source_path, loc.row, loc.col, message)
#define comp_notef(loc, fmt, ...) printf("%s:%zu:%zu: "fmt"\n", loc.source_path, loc.row, loc.col, __VA_ARGS__)

#define sv_eq_str(sv, str) sv_eq_buf(sv, str, strlen(str))
bool sv_eq_buf(Nob_String_View sv, const char *buf, size_t buf_len);

#define sb_append_sv(sb, sv) nob_sb_append_buf(sb, sv.data, sv.count)

// Made my own todo cause abort kinda seems a bit odd in my machine sometimes
#define TODO(message) (fprintf(stderr, "%s:%d: [TODO] %s\n", __FILE__, __LINE__, message), exit(1))
#define TODOf(fmt, ...) (fprintf(stderr, "%s:%d: [TODO] "fmt"\n", __FILE__, __LINE__, __VA_ARGS__), exit(1))

#define arr_includes(arr, item, ret) arr_with_len_includes(arr, NOB_ARRAY_LEN(arr), item, ret)
#define da_includes(arr, item, ret) arr_with_len_includes((arr)->items, (arr)->count, item, ret)
#define arr_with_len_includes(arr, len, item, ret)         \
  do {                                                  \
    *ret = false;                                       \
    for (size_t i = 0; i < len; ++i) {                  \
      if (arr[i] == item) { *ret = true; break; }       \
    }                                                   \
  } while (0)

// Resize the capacity of the array to be exactly its count
#define da_compact(da)                                                               \
  do {                                                                               \
    if ((da)->count > 0 && (da)->capacity > 0 && (da)->count != (da)->capacity) {    \
      (da)->capacity = (da)->count;                                                  \
      (da)->items = NOB_REALLOC((da)->items, (da)->capacity * sizeof(*(da)->items)); \
      NOB_ASSERT((da)->items != NULL && "Buy more RAM lol");                         \
    }                                                                                \
  } while (0)

// Symbol constants
const char* EQSIGN = "=";
const char* SEMICOLON = ";";
const char* OPEN_BRACE = "{";
const char* CLOSING_BRACE = "}";


#endif // __DWOC_UTILS_H

#ifdef DWOC_UTILS_IMPLEMENTATION

bool sv_eq_buf(Nob_String_View sv, const char *buf, size_t buf_len) {
  if (sv.count != buf_len) return false;
  return strncmp(sv.data, buf, buf_len) == 0;
}

#endif // DWOC_UTILS_IMPLEMENTATION

