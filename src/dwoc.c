
#include <stdio.h>
#include <ctype.h>

// External
#define NOB_IMPLEMENTATION
#include "nob.h"

// Lexing and Parsing
// If you don't understand why this is needed, I love your ignorant bliss to the chaos I have built and the atrosity that is header files that are used in multiple areas. I'll never get the time I fought against mf MSVC to end up here.
#define DWOC_UTILS_IMPLEMENTATION
#include "utils.h"
#undef DWOC_UTILS_IMPLEMENTATION

#define DWOC_LEXER_IMPLEMENTATION
#include "lexer.h"
#undef DWOC_LEXER_IMPLEMENTATION

#define DWOC_AST_IMPLEMENTATION
#include "ast.h"
#undef DWOC_AST_IMPLEMENTATION

#define DWOC_JS_IMPLEMENTATION
#include "javascript.h"

bool var_exist(const Vars *vars, Nob_String_View sv) {
  nob_da_foreach(Var, v, vars) {
    if (nob_sv_eq(v->name, sv)) return true;
  }
  return false;
}

Var *find_var_by_name(const Vars *vars, Nob_String_View name) {
  nob_da_foreach(Var, v, vars) {
    if (nob_sv_eq(v->name, name)) return v;
  }
  return NULL;
}

void usage(const char* program) {
  printf("Usage: %s [OPTIONS] <input.dwo>\n", program);
  printf("  -o <output-name>    ----  Specify output file name\n");
  printf("  -t <js|ir>          ----  Specify output target\n");
}

int main(int argc, char **argv) {
  char *program = nob_shift(argv, argc);
  // printf("Hello, dwo!\n");
  if (argc < 1) {
    nob_log(NOB_ERROR, "No input file was provided\n");
    usage(program);
    return 1;
  }
  char *input_path = NULL;
  char *output_name = "out";
  OutputTarget output_target = OT_JavaScript;
  while (argc > 0) {
    char *flag = nob_shift(argv, argc);
    if (strcmp(flag, "-o") == 0) {
      if (argc == 0) {
        nob_log(NOB_ERROR, "Missing output name");
        usage(program);
        return 1;
      }
      output_name = nob_shift(argv, argc);
      continue;
    }
    if (strcmp(flag, "-t") == 0) {
      if (argc == 0) {
        nob_log(NOB_ERROR, "Missing output target");
        usage(program);
        return 1;
      }
      char *target_name = nob_shift(argv, argc);
      if (strcmp(target_name, "js") == 0) {
        output_target = OT_JavaScript;
      } else if (strcmp(target_name, "ir") == 0) {
        output_target = OT_IR;
      } else {
        nob_log(NOB_ERROR, "Unknown target %s supported targets are only JavaScript (js) and Intermediate Representation (ir)", target_name);
        usage(program);
        return 1;
      }
      continue;
    }
    if (flag[0] == '-') {
      nob_log(NOB_ERROR, "Unknown flag %s", flag);
      usage(program);
      return 1;
    }
    input_path = flag;
  }
  Nob_String_Builder output_path_sb = {0};
  nob_sb_append_cstr(&output_path_sb, output_name);

  Nob_String_Builder sb = {0};
  if (!nob_read_entire_file(input_path, &sb)) return 1;
  // printf("Read %zu bytes from file %s\n", sb.count, input_path);
  Nob_String_Builder out = {0};

  Context ctx = {
    .source_path = input_path,
    .lex = lexer_from(input_path, sb.items, sb.count),
  };
  Token t;

  if (output_target == OT_IR) {
    if (!nob_sv_end_with(nob_sb_to_sv(output_path_sb), ".ir")) {
      nob_sb_append_cstr(&output_path_sb, ".ir");
    }
    AST_Node node = {0}, main = {0};
    bool errored = true;
    while (ast_chomp(&ctx.lex, &node)) {
      AST_Node_Kind nk = node.kind;
      ast_dump_node(&out, node);
      if (nk == AST_NK_FN_DECL) {
        if (sv_eq_str(node.as.fn_decl.name, "main")) {
          main = node;
          ctx.main_is_defined = true;
        }
      }
      memset(&node, 0, sizeof(node));
      nob_da_append(&out, '\n');
      if (nk == AST_NK_EOF) {
        errored = false;
        break;
      }
    }
    if (errored) return 1;
  } else if (output_target == OT_JavaScript) {
    if (!nob_sv_end_with(nob_sb_to_sv(output_path_sb), ".js")) {
      nob_sb_append_cstr(&output_path_sb, ".js");
    }
    javascript_compilation_prologue(&out);
    if (!javascript_run_compilation(&out, &ctx)) {
      nob_log(NOB_INFO, "Wrote onto buffer %zu bytes", out.count);
      // Nob_String_View out_sv = nob_sb_to_sv(out);
      // nob_log(NOB_INFO, SV_Fmt, SV_Arg(out_sv));
      return 1;
    }
    javascript_compilation_epilogue(&out, &ctx);
  } else {
    nob_log(NOB_ERROR, "Unsupported target");
    return 1;
  }

  Nob_String_View given_output_path = nob_sv_from_parts(output_path_sb.items, output_path_sb.count);
  StringViews path_parts = {0};

  bool arr_includes = false;
  arr_with_len_includes(output_path_sb.items, output_path_sb.count, '/', &arr_includes);
  char slash = 0;
  if (arr_includes) {
    slash = '/';
  } else {
    arr_with_len_includes(output_path_sb.items, output_path_sb.count, '\\', &arr_includes);
    if (arr_includes) {
      slash = '\\';
    }
  }
  if (slash != 0) {
    while (given_output_path.count) {
      Nob_String_View chopped = nob_sv_chop_by_delim(&given_output_path, slash);
      if (chopped.count == 0 || sv_eq_str(chopped, ".")) continue;
      if (sv_eq_str(chopped, "..")) {
        da_pop(&path_parts);
        continue;
      }
      nob_da_append(&path_parts, chopped);
    }
  } else {
    nob_da_append(&path_parts, given_output_path);
  }


  output_path_sb.count = 0;
  nob_minimal_log_level = NOB_WARNING;
  for (size_t i = 0; i < path_parts.count; ++i) {
    Nob_String_View it = path_parts.items[i];
    const char *path = nob_temp_sv_to_cstr(it);
    /* nob_log(NOB_INFO, "Path parth: `%s`", path); */
    if (i + 1 < path_parts.count && strlen(path) > 0 && strcmp(path, ".") != 0 && strcmp(path, "..") != 0 && !nob_mkdir_if_not_exists(path)) return 1;
    if (i > 0) {
#ifdef _WIN32
      nob_sb_appendf(&output_path_sb, "\\%s", path);
#else
      nob_sb_appendf(&output_path_sb, "/%s", path);
#endif
    } else {
      nob_sb_append_cstr(&output_path_sb, path);
    }
  }
  nob_minimal_log_level = NOB_INFO;

  nob_sb_append_null(&output_path_sb);
  const char *output_path = output_path_sb.items;
  // nob_log(NOB_INFO, "Crafted output path: `%s`", output_path);

  if (!nob_write_entire_file(output_path, out.items, out.count)) return 1;
  nob_log(NOB_INFO, "Succesfully compiled: %s", output_path);

  return 0;
}

