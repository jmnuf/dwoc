#ifndef __DWOC_UTILS_H
#define __DWOC_UTILS_H

#ifndef NOB_IMPLEMENTATION
#include "nob.h"
#endif

#define String Nob_String_View
#define SV(cstr) nob_sv_from_cstr(cstr)
#define SVl(cstr, len) ((Nob_String_View) { .data = cstr, .count = len })

typedef enum {
  OT_JavaScript,
  OT_IR,
} OutputTarget;

typedef struct {
  Nob_String_View *items;
  size_t count;
  size_t capacity;
} StringViews;

typedef struct {
  const char *source_path;
  size_t row;
  size_t col;
} Loc;

typedef struct {
  Nob_String_View name;
  Nob_String_View library;
  bool immutable;
  Loc loc;
} Var;

typedef struct {
  Var *items;
  size_t count;
  size_t capacity;
} Vars;

typedef struct {
  Nob_String_View name;
  Nob_String_View library;
  Loc loc;
} Fn;

typedef struct {
  Fn *items;
  size_t count;
  size_t capacity;
} Fns;

#define expect(cond, msg) (!(cond)) ? (HERE(msg), 1) : 1
#define expectf(cond, fmt, ...) (!(cond)) ? (HEREf(fmt, __VA_ARGS__), 1) : 1
#define carray_foreach(Type, it, arr) for (Type *it = arr; it < arr + NOB_ARRAY_LEN(arr); ++it)

#define da_pop(da) (expect((da)->count > 0, "Attempting to pop from empty array"), *((da)->items+(--(da)->count)))
#define safe_da_free(da)    \
  do {                      \
    if (da.items != NULL) { \
      nob_da_free(da);      \
      da.items = NULL;      \
      da.capacity = 0;      \
      da.count = 0;         \
    }                       \
  } while (0)

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

#define HERE(message) (fprintf(stderr, "%s:%d: [ERROR] %s\n", __FILE__, __LINE__, message), exit(1))
#define HEREf(fmt, ...) (fprintf(stderr, "%s:%d: [ERROR] "fmt"\n", __FILE__, __LINE__, __VA_ARGS__), exit(1))

#define NEVER(message) (fprintf(stderr, "%s:%d: [NEVER_IS_HERE] %s\n", __FILE__, __LINE__, message), exit(1))
#define NEVERf(fmt, ...) (fprintf(stderr, "%s:%d: [NEVER_IS_HERE] "fmt"\n", __FILE__, __LINE__, __VA_ARGS__), exit(1))

#define sb_add_indentation_level(sb, i, depth) if ((depth) > 0) for (int i = 0; i < (depth); ++i) nob_sb_append_cstr(sb, "  ")

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

#define memzero(data) memset((data), 0, sizeof(*(data)))

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

