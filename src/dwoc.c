
#include <stdio.h>
#include <ctype.h>


#define NOB_IMPLEMENTATION
#include "nob.h"

typedef struct {
  Nob_String_View *items;
  size_t count;
  size_t capacity;
} StringViews;

#define da_pop(da) (--(da)->count)

typedef enum {
  TOK_EOF = -1,
  TOK_UNKNOWN,
  TOK_IDENT,
  TOK_SYMBOL,
  TOK_INT,
} TokenKind;

const char *token_kind_name(TokenKind kind) {
  switch (kind) {
  case TOK_EOF:
    return "End of File";
  case TOK_UNKNOWN:
    return "Unknown";
  case TOK_IDENT:
    return "Identifier";
  case TOK_SYMBOL:
    return "Symbol";
  case TOK_INT:
    return "Int";
  default:
    return "<Unsupported-Token-Kind>";
  }
}

typedef struct {
  const char *source_path;
  size_t row;
  size_t col;
} Loc;

typedef struct {
  char *source;
  size_t source_len;
  size_t at_point;
  Loc loc;

  Nob_String_View view;
  TokenKind kind;
} Lexer;

Lexer lexer_from(const char* source_path, char *source, size_t length) {
  Lexer l = {
    .source = source,
    .source_len = length,
    .at_point = 0,
    .loc = { .source_path = source_path, .row = 1, .col = 0 },
  };
  return l;
}
bool lexer_next_token(Lexer *l) {
  l->view.data = &l->source[l->at_point];
  l->view.count = 0;

  if (*l->source == 0 || l->at_point >= l->source_len) {
    l->kind = TOK_EOF;
    return true;
  }
  while (isspace(l->source[l->at_point]) && l->at_point < l->source_len) {
    char c = l->source[l->at_point];
    l->at_point++;
    if (c == '\n') {
      l->loc.row++;
      l->loc.col = 0;
    } else {
      l->loc.col++;
    }
  }
  char *where_firstchar = l->source + l->at_point;
  l->view.data = where_firstchar;
  char firstchar = *where_firstchar;
  if (firstchar == EOF || l->at_point >= l->source_len) {
    l->kind = TOK_EOF;
    return true;
  }
  
  size_t len = 0;
  if (firstchar == '/' && *(where_firstchar+1) == '/') {
    while (l->at_point < l->source_len) {
      char c = l->source[l->at_point];
      l->at_point++;
      if (c == '\n') {
        l->loc.row++;
        l->loc.col = 0;
        break;
      }
    }
    if (l->at_point == l->source_len) {
      l->kind = TOK_EOF;
      return true;
    }
    return lexer_next_token(l);
  } else if (isalpha(firstchar) || firstchar == '_') {
    l->kind = TOK_IDENT;
    while ((isalnum(l->source[l->at_point]) || l->source[l->at_point] == '_') && l->at_point < l->source_len) {
      l->at_point++;
      len++;
    }
  } else if (isdigit(firstchar)) {
    l->kind = TOK_INT;
    while ((isdigit(l->source[l->at_point]) || l->source[l->at_point] == '_') && l->at_point < l->source_len) {
      l->at_point++;
      len++;
    }
  } else if (ispunct(firstchar)) {
    l->kind = TOK_SYMBOL;
    l->at_point++;
    len++;
  } else {
    l->kind = TOK_UNKNOWN;
    while (!isspace(l->source[l->at_point]) && l->at_point < l->source_len) {
      l->at_point++;
      len++;
    }
  }
  l->view.data = where_firstchar;
  while (l->at_point > l->source_len) {
    len--;
    l->at_point--;
  }
  l->view.count = len;
  if (len == 0 && l->at_point >= l->source_len) {
    l->kind = TOK_EOF;
    return true;
  }
  return false;
}

typedef enum {
  OT_JavaScript,
  OT_IR,
} OutputTarget;

typedef struct {
  TokenKind kind;
  Nob_String_View sv;
  int integer;
} Token;

typedef struct {
  Token *items;
  size_t count;
  size_t capacity;
} TokenList;

typedef struct {
  Nob_String_View name;
  bool immutable;
  Loc loc;
} Var;
typedef struct {
  Var *items;
  size_t count;
  size_t capacity;
} Vars;

typedef struct {
  const char *source_path;
  Lexer lex;
  Vars vars;
} Context;

void dump_token(Nob_String_Builder *sb, Token tok) {
  switch (tok.kind) {
  case TOK_EOF:
    nob_sb_append_cstr(sb, "Token::EOF");
    break;
  case TOK_UNKNOWN:
    nob_sb_appendf(sb, "Token::Unknown('"SV_Fmt"')", SV_Arg(tok.sv));
    break;
  case TOK_SYMBOL:
    nob_sb_appendf(sb, "Token::Symbol("SV_Fmt")", SV_Arg(tok.sv));
    break;
  case TOK_IDENT:
    nob_sb_appendf(sb, "Token::Ident("SV_Fmt")", SV_Arg(tok.sv));
    break;
  case TOK_INT:
    nob_sb_appendf(sb, "Token::Int(%d)", tok.integer);
    break;
  default:
    NOB_UNREACHABLE("dump_token: TokenKind match");
    break;
  }
}

#define next_token(l, tok) move_lexer_ahead_by(l, tok, 1)
bool move_lexer_ahead_by(Lexer *l, Token *tok, int amount) {
  for (int i = 0; i < amount; ++i) {
    if (lexer_next_token(l)) {
      tok->kind = TOK_EOF;
      tok->sv.count = 0;
      return false;
    }
    tok->kind = l->kind;
    if (tok->kind == TOK_INT) {
      size_t tmp_save = nob_temp_save();
      const char* substr = nob_temp_sv_to_cstr(l->view);
      tok->integer = atoi(substr);
      nob_temp_rewind(tmp_save);
    } else {
      tok->sv = l->view;
    }
  }
  return true;
}
#define peek_token(l, tok) peek_token_ahead_by(l, tok, 1)
bool peek_token_ahead_by(Lexer l, Token *tok, int amount) {
  return move_lexer_ahead_by(&l, tok, amount);
}

#define comp_error(loc, message) fprintf(stderr, "%s:%zu:%zu: [ERROR] %s\n", loc.source_path, loc.row, loc.col, message)
#define comp_errorf(loc, fmt, ...) fprintf(stderr, "%s:%zu:%zu: [ERROR] "fmt"\n", loc.source_path, loc.row, loc.col, __VA_ARGS__)
#define comp_warn(loc, message) fprintf(stderr, "%s:%zu:%zu: [WARN] %s\n", loc.source_path, loc.row, loc.col, message)
#define comp_warnf(loc, fmt, ...) fprintf(stderr, "%s:%zu:%zu: [WARN] "fmt"\n", loc.source_path, loc.row, loc.col, __VA_ARGS__)
#define comp_note(loc, message) printf("%s:%zu:%zu: %s\n", loc.source_path, loc.row, loc.col, message)
#define comp_notef(loc, fmt, ...) printf("%s:%zu:%zu: "fmt"\n", loc.source_path, loc.row, loc.col, __VA_ARGS__)

const char* EQSIGN = "=";
const char* SEMICOLON = ";";
const char* OPEN_BRACE = "{";
const char* CLOSING_BRACE = "}";

bool sv_eq_buf(Nob_String_View sv, const char *buf, size_t buf_len) {
  if (sv.count != buf_len) return false;
  return strncmp(sv.data, buf, buf_len) == 0;
}
#define sv_eq_str(sv, str) sv_eq_buf(sv, str, strlen(str))


#define arr_includes(arr, item, ret) arr_with_len_includes(arr, NOB_ARRAY_LEN(arr), item, ret)
#define da_includes(arr, item, ret) arr_with_len_includes((arr)->items, (arr)->count, item, ret)
#define arr_with_len_includes(arr, len, item, ret)         \
  do {                                                  \
    *ret = false;                                       \
    for (size_t i = 0; i < len; ++i) {                  \
      if (arr[i] == item) { *ret = true; break; }       \
    }                                                   \
  } while (0)


bool expect_next_token_kind(Lexer *l, Token *tok, TokenKind kind) {
  if (!next_token(l, tok)) {
    return false;
  }
  if (tok->kind != kind) return false;
  return true;
}

#define expect_next_token_kind_from_arr(l, tok, kinds) expect_next_token_kind_from_sized_arr(l, tok, kinds, NOB_ARRAY_LEN(kinds))
bool expect_next_token_kind_from_sized_arr(Lexer *l, Token *tok, TokenKind *kinds, size_t kinds_count) {
  if (!next_token(l, tok)) return false;
  for (size_t i = 0; i < kinds_count; ++i) {
    TokenKind kind = kinds[i];
    if (kind == tok->kind) return true;
  }
  return false;
}

bool expect_next_token_eq_sv(Lexer *l, Token *tok, TokenKind kind, Nob_String_View content) {
  if (!expect_next_token_kind(l, tok, kind)) return false;
  return nob_sv_eq(tok->sv, content);
}
bool expect_next_token_eq_buf(Lexer *l, Token *tok, TokenKind kind, const char *buf, size_t buf_len) {
  if (!expect_next_token_kind(l, tok, kind)) return false;
  return sv_eq_buf(tok->sv, buf, buf_len);
}
#define expect_next_token_eq_str(l, tok, kind, str) expect_next_token_eq_buf(l, tok, kind, str, strlen(str))

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

#define kind_in_array(kind, arr) kind_in_sized_array(kind, arr, NOB_ARRAY_LEN(arr))
bool kind_in_sized_array(TokenKind kind, TokenKind *arr, size_t arr_len) {
  for (size_t i = 0; i < arr_len; ++i) {
    if (kind == arr[i]) return true;
  }
  return false;
}

#define compile_expr_as_js(sb, ctx, local_vars) compile_expr_as_js_with_depth(sb, ctx, local_vars, 0)
bool compile_expr_as_js_with_depth(Nob_String_Builder *sb, Context *ctx, const Vars *local_vars, int depth) {
  static TokenKind base_kinds[] = {TOK_INT, TOK_IDENT};
  static TokenKind expr_kinds[] = {TOK_INT, TOK_IDENT, TOK_SYMBOL};

  Token prv, tok;
  Lexer *l = &ctx->lex;
  Nob_String_Builder expr = {0};
  if (depth >= 1) nob_sb_appendf(sb, "%*s", depth*2, "");
  if (!expect_next_token_kind_from_arr(l, &tok, base_kinds)) {
    comp_errorf(l->loc,
                "Unexpected start of expression: received %s `"SV_Fmt"` but expected either an integer or a variable name",
                token_kind_name(tok.kind),
                SV_Arg(tok.sv));
    return false;
  }
  prv = tok;
  if (tok.kind == TOK_IDENT) {
    nob_sb_appendf(&expr, SV_Fmt, SV_Arg(tok.sv));
    nob_log(NOB_INFO, "Inspecting expression token: %s `"SV_Fmt"`", token_kind_name(tok.kind), SV_Arg(tok.sv));
  } else if (tok.kind == TOK_INT) {
    nob_sb_appendf(&expr, "%d", tok.integer);
    nob_log(NOB_INFO, "Inspecting expression token: %s `%d`", token_kind_name(tok.kind), tok.integer);
  }

  bool included_in_arr;
  while (next_token(l, &tok) && tok.kind != TOK_EOF && kind_in_array(tok.kind, expr_kinds)) {
    nob_log(NOB_INFO, "Inspecting expression token: %s `"SV_Fmt"`", token_kind_name(tok.kind), SV_Arg(tok.sv));
    arr_includes(base_kinds, prv.kind, &included_in_arr);
    if (tok.kind == TOK_SYMBOL) {
      if (!included_in_arr) {
        comp_errorf(l->loc,
                    "Unexpected %s `"SV_Fmt"`: Are you missing a semicolon?",
                    token_kind_name(tok.kind),
                    SV_Arg(tok.sv));
        return false;
      }
      const char *operands = "+-/*";
      if (sv_eq_str(tok.sv, SEMICOLON)) {
        Nob_String_View sv = nob_sb_to_sv(expr);
        nob_sb_appendf(sb, SV_Fmt";\n", SV_Arg(sv));
        nob_log(NOB_INFO, "Generated expr: `"SV_Fmt";`", SV_Arg(sv));
        return true;
      }
      if (tok.sv.count != 1) {
        comp_errorf(l->loc, "Unsupported symbol `"SV_Fmt"` in expression", SV_Arg(tok.sv));
        return false;
      }
      arr_includes(operands, tok.sv.data[0], &included_in_arr);
      if (!included_in_arr) {
        comp_errorf(l->loc, "Unsupported symbol `"SV_Fmt"` in expression", SV_Arg(tok.sv));
        return false;
      }
      nob_sb_appendf(&expr, " %c ", tok.sv.data[0]);
    } else if (tok.kind == TOK_INT) {
      nob_sb_appendf(&expr, "%d", tok.integer);
    } else if (tok.kind == TOK_IDENT) {
      nob_sb_appendf(&expr, SV_Fmt, SV_Arg(tok.sv));
    } else {
      comp_errorf(l->loc, "Invalid %s `"SV_Fmt"` in expression", token_kind_name(tok.kind), SV_Arg(tok.sv));
      return false;
    }
    prv = tok;
  }
  fprintf(stderr, "%s:%d: [TODO] Handle exit of while loop when parsing expression in compile_expr_as_js_with_depth", __FILE__, __LINE__);
  nob_log(NOB_INFO, "Exited while loop with token: %s `"SV_Fmt"`", token_kind_name(tok.kind), SV_Arg(tok.sv));
  exit(1);
  return true;
}

#define compile_var_decl_as_js(sb, ctx, local_vars) compile_var_decl_as_js_with_depth(sb, ctx, local_vars, 1)
bool compile_var_decl_as_js_with_depth(Nob_String_Builder *sb, Context *ctx, Vars *local_vars, int depth) {
  Token tok;
  Lexer *l = &ctx->lex;
  if (!expect_next_token_eq_str(l, &tok, TOK_IDENT, "let")) {
    comp_error(l->loc, "Unexpected EOF: expected keyword `let` accompanied by a variable name");
    return false;
  }

  if (!expect_next_token_kind(l, &tok, TOK_IDENT)) {
    comp_errorf(l->loc, "Expected either `mut` keyword or name for variable but found %s", token_kind_name(tok.kind));
    return false;
  }

  bool immutable = true;

  if (sv_eq_str(tok.sv, "mut")) {
    immutable = false;
    if (!expect_next_token_kind(l, &tok, TOK_IDENT)) {
      comp_errorf(l->loc, "Expected variable name after `mut` keyword but found %s", token_kind_name(tok.kind));
      return false;
    }
  }

  Var *existing = find_var_by_name(local_vars, tok.sv);
  if (existing != NULL) {
    comp_errorf(l->loc, "Cannot redeclare existing variable: "SV_Fmt, SV_Arg(tok.sv));
    comp_note(existing->loc, "Originally declared here");
    return false;
  }

  Var var = {
    .name = tok.sv,
    .immutable = immutable,
    .loc = l->loc,
  };
  nob_da_append(local_vars, var);

  nob_sb_appendf(sb, "%*s", depth*2, "");

  if (!peek_token(*l, &tok)) {
    if (var.immutable) {
      comp_error(l->loc, "Unexpected End of File: Immutable variables require to be initialized");
    } else {
      comp_errorf(l->loc, "Expected semicolon for end of statement but found %s", token_kind_name(tok.kind));
    }
    return false;
  }

  if (var.immutable) {
    lexer_next_token(l);
    if (tok.kind != TOK_SYMBOL || !sv_eq_str(tok.sv, EQSIGN)) {
      comp_errorf(l->loc,
                  "Unexpected %s token `"SV_Fmt"`: Immutable variables require to be initialized",
                  token_kind_name(tok.kind),
                  SV_Arg(tok.sv));
      return false;
    }

    if (!next_token(l, &tok)) {
      comp_error(l->loc, "Unexpected end of file: missing rvalue for assignment");
      return false;
    }
    if (tok.kind != TOK_INT) {
      comp_errorf(l->loc,
                  "Invalid rvalue %s `"SV_Fmt"` for assignment: Variables can only be set to integer values for now",
                  token_kind_name(tok.kind),
                  SV_Arg(tok.sv));
      return false;
    }
    nob_sb_appendf(sb, "const "SV_Fmt" = %d;\n", SV_Arg(var.name), tok.integer);
  } else {
    if (tok.kind != TOK_SYMBOL || (!sv_eq_str(tok.sv, EQSIGN) && !sv_eq_str(tok.sv, SEMICOLON))) {
      comp_errorf(l->loc,
                  "Unexpected %s token `"SV_Fmt"`: expected end of statement with ';' or variable initialization with '='",
                  token_kind_name(tok.kind),
                  SV_Arg(tok.sv));
      return false;
    }
    if (sv_eq_str(tok.sv, EQSIGN)) {
      lexer_next_token(l);
      next_token(l, &tok);
      if (tok.kind == TOK_EOF) {
        comp_error(l->loc, "Missing rvalue for assignment: Expected a value for the variable but found end of file instead");
        return false;
      }
      if (tok.kind != TOK_INT) {
        comp_errorf(l->loc,
                    "Invalid rvalue %s `"SV_Fmt"` for assignment: Variables can only be set to integer values for now",
                    token_kind_name(tok.kind),
                    SV_Arg(tok.sv));
        return false;
      }
      nob_sb_appendf(sb, "let "SV_Fmt" = %d;\n", SV_Arg(var.name), tok.integer);
    } else {
      nob_sb_appendf(sb, "let "SV_Fmt";\n", SV_Arg(var.name));
    }
  }

  if (!expect_next_token_eq_str(l, &tok, TOK_SYMBOL, SEMICOLON)) {
    if (tok.kind = TOK_EOF) {
      comp_errorf(l->loc, "Expected semicolon for end of statement but found %s", token_kind_name(tok.kind));
    } else {
      comp_errorf(l->loc, "Expected semicolon for end of statement but found %s "SV_Fmt, token_kind_name(tok.kind), SV_Arg(tok.sv));
    }
    return false;
  }
  return true;
}

bool compile_fn_body_as_js(Nob_String_Builder *sb, Context *ctx) {
  Token tok;
  Lexer *l = &ctx->lex;
  if (!expect_next_token_eq_str(l, &tok, TOK_SYMBOL, "{")) {
    comp_errorf(l->loc, "Expected '{' for declaring function body but found %s", token_kind_name(tok.kind));
    return false;
  }
  nob_sb_append_cstr(sb, "{\n");

  Nob_String_View closing_brace = nob_sv_from_parts("}", 1);
  Nob_String_View var_decl = nob_sv_from_cstr("let");
  Vars local_vars = {0};

  while (peek_token(*l, &tok) && !nob_sv_eq(tok.sv, closing_brace)) {
    nob_log(NOB_INFO, "Expr/Stmt starts with %s "SV_Fmt, token_kind_name(tok.kind), SV_Arg(tok.sv));
    if (nob_sv_eq(tok.sv, var_decl)) {
      if (!compile_var_decl_as_js(sb, ctx, &local_vars)) {
        return false;
      }
      continue;
    }
    if (tok.kind == TOK_IDENT) {
      lexer_next_token(l);
      Var *var = find_var_by_name(&local_vars, tok.sv);
      if (var == NULL) var = find_var_by_name(&ctx->vars, tok.sv);
      if (var == NULL) {
        comp_errorf(l->loc, "Unknown identifier: "SV_Fmt", no variable exists with such name", SV_Arg(tok.sv));
        return false;
      }
      if (!expect_next_token_kind(l, &tok, TOK_SYMBOL)) {
        if (tok.kind == TOK_EOF) {
          comp_error(l->loc, "Unexpected End of File created hanging statement");
        } else {
          comp_errorf(l->loc, "Unexpected token expected ';' or '=' but got %s `"SV_Fmt"`", token_kind_name(tok.kind), SV_Arg(tok.sv));
        }
        return false;
      }
      nob_sb_appendf(sb, "  "SV_Fmt, SV_Arg(var->name));
      if (sv_eq_str(tok.sv, SEMICOLON)) {
        nob_sb_append_cstr(sb, SEMICOLON);
        continue;
      }
      if (!sv_eq_str(tok.sv, "=")) {
        comp_errorf(l->loc, "Unexpected token expected ';' or '=' but got `"SV_Fmt"`", SV_Arg(tok.sv));
        return false;
      }
      if (!peek_token(*l, &tok)) {
        comp_error(l->loc, "Unexpected end of file: missing rvalue for assignment");
        return false;
      }
      nob_sb_appendf(sb, " = ");
      Loc loc = l->loc;
      if (!compile_expr_as_js(sb, ctx, &local_vars)) {
        comp_notef(loc, "Invalid rvalue for variable assignment for variable `"SV_Fmt"`", SV_Arg(var->name));
        return false;
      }
      continue;
    }
    next_token(l, &tok);
  }
  nob_log(NOB_INFO, "Created %zu variables in function", local_vars.count);
  // If we exitted the loop without hitting a closing brace it means that we hit the end of the file
  if (!nob_sv_eq(tok.sv, closing_brace)) {
    comp_error(l->loc, "Expected '}' to close the function body but found end of file instead");
    return false;
  }
  lexer_next_token(l);
  nob_sb_append_cstr(sb, "}\n");
  return true;
}

bool compile_fn_as_js(Nob_String_Builder *sb, Context *ctx, Token tok) {
  Lexer *l = &ctx->lex;
  if (!expect_next_token_kind(l, &tok, TOK_IDENT)) {
    comp_errorf(l->loc, "Expected identifier for function name but found %s", token_kind_name(tok.kind));
    return false;
  }

  nob_sb_appendf(sb, "function "SV_Fmt, SV_Arg(tok.sv));

  if (!expect_next_token_eq_str(l, &tok, TOK_SYMBOL, "(")) {
    comp_error(l->loc, "Unexpected end of file");
    return false;
  }
  
  // TODO: Add function parameters
  
  if (!expect_next_token_eq_str(l, &tok, TOK_SYMBOL, ")")) {
    comp_error(l->loc, "Unexpected end of file");
    return false;
  }
  nob_sb_append_cstr(sb, "() ");
  return compile_fn_body_as_js(sb, ctx);
}

bool compile_tok_to_js(Nob_String_Builder *sb, Context *ctx, Token tok) {
  Lexer *l = &ctx->lex;
  switch (tok.kind) {
  case TOK_UNKNOWN:
    nob_log(NOB_WARNING, "Cannot translate unknown token to JavaScript");
    return true;
  case TOK_SYMBOL:
    nob_log(NOB_WARNING, "Translating symbol token to JavaScript is not implemented yet: "SV_Fmt, SV_Arg(tok.sv));
    /* nob_sb_append_buf(sb, tok.sv.data, tok.sv.count); */
    return true;
  case TOK_IDENT:
    if (sv_eq_buf(tok.sv, "fn", 2)) {
      return compile_fn_as_js(sb, ctx, tok);
    }

    if (nob_sv_eq(tok.sv, nob_sv_from_parts("let", 3))) {
      if (!next_token(l, &tok)) {
        nob_log(NOB_ERROR, "Expected identifier for variable name or keyword 'mut' but found end of file");
        return false;
      }
      if (tok.kind != TOK_IDENT) {
        nob_log(NOB_ERROR, "Expected identifier for variable name or keyword 'mut' but found %s", token_kind_name(tok.kind));
        return false;
      }
      if (nob_sv_eq(tok.sv, nob_sv_from_parts("mut", 3))) {
        if (!next_token(l, &tok)) {
          nob_log(NOB_ERROR, "Expected identifier for variable name but found end of file");
          return false;
        }
        if (tok.kind != TOK_IDENT) {
          nob_log(NOB_ERROR, "Expected identifier for variable name but found %s", token_kind_name(tok.kind));
          return false;
        }
        nob_sb_appendf(sb, "let "SV_Fmt, SV_Arg(tok.sv));
      } else {
        nob_sb_appendf(sb, "const "SV_Fmt, SV_Arg(tok.sv));
      }
      return true;
    }
    nob_log(NOB_ERROR, "Unknown identifier hit: "SV_Fmt, SV_Arg(tok.sv));
    return false;
  case TOK_INT:
    nob_sb_append_buf(sb, tok.sv.data, tok.sv.count);
    return true;
  default:
    NOB_UNREACHABLE("compile_tok_to_js: TokenKind match");
    return false;
  }
}

void usage(const char* program) {
  printf("Usage: %s [OPTIONS] <input.dwo>\n", program);
  printf("  -o <output-name>    ----  Specify output file name\n");
  printf("  -t <js|ir>    ----  Specify output target\n");
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
    while (next_token(&ctx.lex, &t)) {
      dump_token(&out, t);
      nob_da_append(&out, '\n');
    }
  } else if (output_target == OT_JavaScript) {
    if (!nob_sv_end_with(nob_sb_to_sv(output_path_sb), ".js")) {
      nob_sb_append_cstr(&output_path_sb, ".js");
    }
    nob_sb_append_cstr(&out, "\"use strict\";\n\n");
    while (next_token(&ctx.lex, &t)) {
      if (!compile_tok_to_js(&out, &ctx, t)) {
        nob_log(NOB_INFO, "Wrote onto buffer %zu bytes", out.count);
        // Nob_String_View out_sv = nob_sb_to_sv(out);
        // nob_log(NOB_INFO, SV_Fmt, SV_Arg(out_sv));
        return 1;
      }
    }
    nob_sb_append_cstr(&out, "\nmain();\n");
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
  for (size_t i = 0; i < path_parts.count; ++i) {
    Nob_String_View it = path_parts.items[i];
    const char *path = nob_temp_sv_to_cstr(it);
    nob_log(NOB_INFO, "Path parth: `%s`", path);
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

  nob_sb_append_null(&output_path_sb);
  const char *output_path = output_path_sb.items;
  // nob_log(NOB_INFO, "Crafted output path: `%s`", output_path);

  if (!nob_write_entire_file(output_path, out.items, out.count)) return 1;
  nob_log(NOB_INFO, "Succesfully compiled: %s", output_path);

  return 0;
}


