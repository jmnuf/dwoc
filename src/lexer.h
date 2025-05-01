#ifndef __DWOC_LEXER_H
#define __DWOC_LEXER_H

#include "utils.h"

typedef enum {
  TOK_EOF = -1,
  TOK_UNKNOWN,
  TOK_IDENT,
  TOK_SYMBOL,
  TOK_INT,
} TokenKind;

typedef struct {
  char *source;
  size_t source_len;
  size_t at_point;
  Loc loc;

  Nob_String_View view;
  TokenKind kind;
} Lexer;

typedef struct Token Token;

struct Token {
  TokenKind kind;
  Nob_String_View sv;
  int integer;
};

// Get a human readable name for the token kind
const char *token_kind_name(TokenKind kind);

// Create a lexer from a fully known source
Lexer lexer_from(const char* source_path, char *source, size_t length);

// Advance the lexer to the next token and return true if we have hit the end of the text known to the lexer
bool lexer_next_token(Lexer *l);

// Write the string representation of the passed in token
void dump_token(Nob_String_Builder *sb, Token tok);

// Move lexer ahead by one and store token data in a token
#define next_token(l, tok) move_lexer_ahead_by(l, tok, 1)
// Move lexer ahead by arbitrary amount and store last viewed token data in a token
// Returns false when TOK_EOF is hit but true otherwise
bool move_lexer_ahead_by(Lexer *l, Token *tok, int amount);

// Peek ahead in the lexer by grabbing a copy of it and moving the copy one token ahead
#define peek_token(l, tok) peek_token_ahead_by(l, tok, 1)
// Peek ahead in the lexer by grabbing a copy of it and moving the copy an arbitrary amount ahead
bool peek_token_ahead_by(Lexer l, Token *tok, int amount);

// Move lexer ahead and return whether the newly consumed token is of the expected kind
// NOTE: TOK_EOF can't be checked, this will always return false if EOF was hit
bool expect_next_token_kind(Lexer *l, Token *tok, TokenKind kind);

// Move lexer ahead once and return whether the newly consumed token is any of the expected kinds
// NOTE: TOK_EOF can't be checked, this will always return false if EOF was hit
bool expect_next_token_kind_from_sized_arr(Lexer *l, Token *tok, TokenKind *kinds, size_t kinds_count);
#define expect_next_token_kind_from_arr(l, tok, kinds) expect_next_token_kind_from_sized_arr(l, tok, kinds, NOB_ARRAY_LEN(kinds))

// Move lexer ahead once storing the last hit token
// returning whether the newly consumed token is of the expected kind and it's content is equal to the provided value
bool expect_next_token_eq_sv(Lexer *l, Token *tok, TokenKind kind, Nob_String_View content);

// Move lexer ahead once storing the last hit token
// returning whether the newly consumed token is of the expected kind and it's content is equal to the cstr value
bool expect_next_token_eq_buf(Lexer *l, Token *tok, TokenKind kind, const char *buf, size_t buf_len);
// Macro for calling expect_next_token_eq_buf with a regular null terminated cstr
#define expect_next_token_eq_str(l, tok, kind, str) expect_next_token_eq_buf(l, tok, kind, str, strlen(str))

// Check if a token kind is included inside of an array of token kinds
bool kind_in_sized_array(TokenKind kind, TokenKind *arr, size_t arr_len);
#define kind_in_array(kind, arr) kind_in_sized_array(kind, arr, NOB_ARRAY_LEN(arr))
#define kind_in_da(kind, da) kind_in_sized_array(kind, (da)->items, (da)->count)

#endif // __DWOC_LEXER_H

#ifdef DWOC_LEXER_IMPLEMENTATION

const char *token_kind_name(TokenKind kind) {
  switch (kind) {
  case TOK_EOF:
    return "End_Of_File";
  case TOK_UNKNOWN:
    return "Unknown";

  case TOK_IDENT:
    return "Identifier";
  case TOK_SYMBOL:
    return "Symbol";
  case TOK_INT:
    return "Integer_Literal";

  default:
    return "<Unsupported-Token-Kind>";
  }
}

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
  // Skip over comments. We do this recursively, if this is an issue you should write more concise comments
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
    // Identifiers
  } else if (isalpha(firstchar) || firstchar == '_') {
    l->kind = TOK_IDENT;
    while ((isalnum(l->source[l->at_point]) || l->source[l->at_point] == '_') && l->at_point < l->source_len) {
      l->at_point++;
      len++;
    }
    // Numbers
  } else if (isdigit(firstchar)) {
    l->kind = TOK_INT;
    /* while ((isdigit(l->source[l->at_point]) || l->source[l->at_point] == '_') && l->at_point < l->source_len) { */
    while ((isdigit(l->source[l->at_point])) && l->at_point < l->source_len) {
      l->at_point++;
      len++;
    }
    // Symbols
  } else if (ispunct(firstchar)) {
    l->kind = TOK_SYMBOL;
    l->at_point++;
    len++;
    // Everything else which is unknown
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

bool peek_token_ahead_by(Lexer l, Token *tok, int amount) {
  return move_lexer_ahead_by(&l, tok, amount);
}

bool expect_next_token_kind(Lexer *l, Token *tok, TokenKind kind) {
  if (!next_token(l, tok)) {
    return false;
  }
  return tok->kind == kind;
}

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

bool kind_in_sized_array(TokenKind kind, TokenKind *arr, size_t arr_len) {
  for (size_t i = 0; i < arr_len; ++i) {
    if (kind == arr[i]) return true;
  }
  return false;
}

#endif // DWOC_LEXER_IMPLEMENTATION

