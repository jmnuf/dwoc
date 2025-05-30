
#ifndef __DWOC_AST_H
#define __DWOC_AST_H

#include "lexer.h"

typedef struct {
  const char *source_path;
  bool main_is_defined;
  Lexer lex;
  Vars vars;
  Fns fns;
} Context;

const char *KEYWORD_FN = "fn";
const char *KEYWORD_LET = "let";
const char *KEYWORD_IMPORT = "use";

typedef enum {
  AST_NK_EOF,
  // Atoms
  AST_NK_TOKEN, // Encapsulates identifiers, symbols and literals
  AST_NK_IMPORT, // Import name, alias, and whether it should be locally searched or in compiler

  // Molecules
  AST_NK_UNOP,
  AST_NK_BINOP,
  AST_NK_FN_PARAMS_DECL,

  // Compounds
  AST_NK_EXPR,
  AST_NK_VAR_DECL,
  AST_NK_ASSIGNMENT,
  AST_NK_FN_DECL,
  AST_NK_FN_CALL,
} AST_Node_Kind;

typedef struct AST_VarDeclAttr AST_VarDeclAttr;
typedef struct AST_NodeList AST_NodeList;
typedef struct AST_Node AST_Node;

struct AST_NodeList {
  AST_Node *items;
  size_t count;
  size_t capacity;
};


struct AST_VarDeclAttr {
  Nob_String_View name;
  AST_NodeList expr;
  bool mutable;
};

typedef struct {
  Nob_String_View name;
  Nob_String_View alias;
  bool is_local;
} AST_Import;

typedef struct {
  Nob_String_View name;
  AST_NodeList expr;
} AST_VarAssign;

typedef struct {
  Nob_String_View name;
  AST_NodeList params;
  AST_NodeList body;
} AST_FnDeclAttr;

typedef struct {
  Nob_String_View name;
  AST_NodeList params;
} AST_FnCall;

typedef union {
  Nob_String_View sv;
  int integer;
  AST_VarDeclAttr var_decl;
  AST_VarAssign var_assign;
  AST_FnDeclAttr fn_decl;
  AST_FnCall fn_call;
  AST_Import import;
  AST_NodeList expr;
  Token token;
} AST_Node_As;

struct AST_Node {
  Loc loc;
  AST_Node_Kind kind;
  AST_Node_As as;
};

char *ast_node_kind_name(AST_Node_Kind kind);

// Advances lexer consuming tokens till it produces a node or hits EOF
// On error returns false
bool ast_chomp(Lexer *l, AST_Node *node);

// Walks through all the sub-nodes of this node recursively and frees them
void ast_node_children_free(AST_Node *node);

void ast_dump_node_at_depth(Nob_String_Builder *sb, AST_Node node, int depth);
#define ast_dump_node(sb, node) ast_dump_node_at_depth(sb, node, 0)

#endif // __DWOC_AST_H

#ifdef DWOC_AST_IMPLEMENTATION

char *ast_node_kind_name(AST_Node_Kind kind) {
  switch (kind) {
  case AST_NK_EOF:
    return "End_Of_File";
    // Atoms
  case AST_NK_TOKEN:
    return "Token";
  case AST_NK_IMPORT:
    return "Import";

    // Molecules
  case AST_NK_UNOP:
    return "Unary_Operation";
  case AST_NK_BINOP:
    return "Binary_Operation";
  case AST_NK_FN_PARAMS_DECL:
    return "Function_Parameters_Declaration";

    // Compounds
  case AST_NK_EXPR:
    return "Expression";
  case AST_NK_VAR_DECL:
    return "Variable_Declaration";
  case AST_NK_ASSIGNMENT:
    return "Variable_Assignment";
  case AST_NK_FN_DECL:
    return "Function_Declaration";
  case AST_NK_FN_CALL:
    return "Function_Call";

  default:// If this is ever hit then we added a node kind that's missing
    TODOf("ast_node_kind_name: Implement missing AST Node kind (%d)", kind);
  }
}

void ast_node_list_free(AST_NodeList *list) {
  if (list->items == NULL) return;
  nob_da_foreach(AST_Node, node, list) {
    ast_node_children_free(node);
  }
  safe_da_free((*list));
}

void ast_node_children_free(AST_Node *node) {
  switch (node->kind) {
  case AST_NK_EOF:
  case AST_NK_TOKEN:
    return;

  case AST_NK_EXPR:
    ast_node_list_free(&node->as.expr);
    return;

  case AST_NK_VAR_DECL:
    ast_node_list_free(&node->as.var_decl.expr);
    return;
  case AST_NK_ASSIGNMENT:
    ast_node_list_free(&node->as.var_assign.expr);
    return;

  case AST_NK_FN_CALL:
    ast_node_list_free(&node->as.fn_call.params);
    return;

  case AST_NK_FN_DECL:
    ast_node_list_free(&node->as.fn_decl.params);
    ast_node_list_free(&node->as.fn_decl.body);
    return;

  default:
    TODOf("ast_node_children_free: Free node %s", ast_node_kind_name(node->kind));
  }
}

void ast_dump_node_list(Nob_String_Builder *sb, AST_NodeList *nodes) {
  nob_da_foreach(AST_Node, n, nodes) {
    size_t index = n - nodes->items;
    if (index > 0) nob_sb_append_cstr(sb, ", ");
    ast_dump_node(sb, *n);
  }
}

void ast_dump_node_at_depth(Nob_String_Builder *sb, AST_Node node, int depth) {
  sb_add_indentation_level(sb, i, depth);
  switch (node.kind) {
  case AST_NK_EOF: return;
    // Atoms
  case AST_NK_TOKEN:
    dump_token(sb, node.as.token);
    return;
  case AST_NK_IMPORT:
    if (node.as.import.alias.count == 0) {
      nob_sb_append_cstr(sb, "Node::Import(");
      sb_append_sv(sb, node.as.import.name);
      nob_sb_append_cstr(sb, ")");
    }
    return;

  // Molecules
  case AST_NK_UNOP:
    TODO("Dumping of AST_Node AST_NK_UNOP");
    return;
  case AST_NK_BINOP:
    TODO("Dumping of AST_Node AST_NK_BINOP");
    return;
  case AST_NK_FN_PARAMS_DECL:
    TODO("Dumping of AST_Node AST_NK_FN_PARAMS_DECL");
    return;

  // Compounds
  case AST_NK_EXPR:
    nob_sb_append_cstr(sb, "Node::Expr(");
    ast_dump_node_list(sb, &node.as.expr);
    nob_sb_append_cstr(sb, "\n");
    sb_add_indentation_level(sb, i, depth);
    nob_sb_append_cstr(sb, ")");
    return;

  case AST_NK_VAR_DECL:
    if (node.as.var_decl.mutable) {
      nob_sb_append_cstr(sb, "Node::VarDecl<mutable>(");
    } else {
      nob_sb_append_cstr(sb, "Node::VarDecl<immutable>(");
    }
    nob_sb_appendf(sb, "Token::Ident('"SV_Fmt"'), ", SV_Arg(node.as.var_decl.name));
    ast_dump_node_list(sb, &node.as.var_decl.expr);
    nob_sb_append_cstr(sb, "");
    nob_sb_append_cstr(sb, ")");
    return;

  case AST_NK_ASSIGNMENT:
    nob_sb_append_cstr(sb, "Node::VarAssign(");
    dump_token(sb, (Token) { .kind = TOK_IDENT, .sv = node.as.var_assign.name });
    nob_sb_append_cstr(sb, ", ");
    ast_dump_node_list(sb, &node.as.var_assign.expr);
    nob_sb_append_cstr(sb, ")");
    return;

  case AST_NK_FN_DECL:
    nob_sb_appendf(sb, "Node::FnDecl(Token::Ident('"SV_Fmt"'), []) {\n", SV_Arg(node.as.fn_decl.name));
    nob_da_foreach(AST_Node, n, &node.as.fn_decl.body) {
      size_t index = n - node.as.fn_decl.body.items;
      if (index > 0) nob_sb_append_cstr(sb, ";\n");
      ast_dump_node_at_depth(sb, *n, depth+1);
    }
    
    sb_add_indentation_level(sb, i, depth);
    nob_sb_append_cstr(sb, "}");
    return;

  case AST_NK_FN_CALL:
    nob_sb_append_cstr(sb, "Node::FnCall(");
    dump_token(sb, (Token) { .kind = TOK_IDENT, .sv = node.as.fn_call.name });
    nob_sb_append_cstr(sb, ", [");
    if(node.as.fn_call.params.count > 0) {
      ast_dump_node_list(sb, &node.as.fn_call.params);
    }
    nob_sb_append_cstr(sb, "])");
    return;
  }
  nob_log(NOB_ERROR, "Fell through switch statement of node kinds with kind: %s(%d)", ast_node_kind_name(node.kind), node.kind);
  HERE("ast_dump_node_at_depth: Unsupported node kind");
}

bool ast_create_expr(Lexer *l, AST_NodeList *expr) {
  Token tok;
  static TokenKind allowed_expr_tokens[] = { TOK_IDENT, TOK_INT };
  static size_t allowed_expr_count = NOB_ARRAY_LEN(allowed_expr_tokens);
  
  for (;;) {
    AST_Node value = {0};
    if (!expect_next_token_kind_from_sized_arr(l, &tok, allowed_expr_tokens, allowed_expr_count)) {
      if (tok.kind == TOK_EOF) {
        comp_error(l->loc, "Unexpected end of file: Missing rvalue for variable initialization");
      } else {
        comp_errorf(l->loc, "Invalid token %s(`"SV_Fmt"`) found in expression", token_kind_name(tok.kind), SV_Arg(tok.sv));
      }
      return false;
    }
    value.loc = l->loc;
    if (tok.kind == TOK_INT || tok.kind == TOK_IDENT) {
      value.kind = AST_NK_TOKEN;
      value.as.token = tok;
    } else {
      comp_errorf(l->loc, "Unsupported token %s(`"SV_Fmt"`) found in expression", token_kind_name(tok.kind), SV_Arg(tok.sv));
      TODO("Add missing allowed expression tokens");
      // return false;
    }
    nob_da_append(expr, value);

    Lexer peeker = *l;
    if (!peek_token(peeker, &tok)) {
      comp_error(peeker.loc, "Unexpected end of file: Missing semicolon?");
      return false;
    }
    if (tok.kind != TOK_SYMBOL) {
      comp_errorf(peeker.loc,
                  "Unexpected %s(`"SV_Fmt"`): Expected math operand or a expression finisher was expected",
                  token_kind_name(tok.kind), SV_Arg(tok.sv));
      return false;
    }
    if (sv_eq_str(tok.sv, "+") || sv_eq_str(tok.sv, "-")) {
      lexer_next_token(l);
      AST_Node operand = {
        .loc = l->loc,
        .kind = AST_NK_TOKEN,
      };
      operand.as.token = tok;
      nob_da_append(expr, operand);
      continue;
    }
    break;
  }

  return true;
}

bool ast_create_var_decl(Lexer *l, AST_Node *decl) {
  Token tok;
  decl->kind = AST_NK_VAR_DECL;
  if (!expect_next_token_eq_str(l, &tok, TOK_IDENT, "let")) {
    comp_error(l->loc, "Unexpected EOF: expected keyword `let` accompanied by a variable name");
    return false;
  }
  Loc decl_start_loc = l->loc;
  
  if (!expect_next_token_kind(l, &tok, TOK_IDENT)) {
    comp_errorf(l->loc, "Expected name for variable but found %s `"SV_Fmt"`", token_kind_name(tok.kind), SV_Arg(tok.sv));
    comp_note(decl_start_loc, "Variable declaration starts here");
    return false;
  }

  decl->loc = l->loc;
  decl->as.var_decl.name = tok.sv;

  if (!expect_next_token_eq_str(l, &tok, TOK_SYMBOL, ":")) {
    if (tok.kind == TOK_EOF) {
      comp_error(l->loc, "Unexpected end of file: expected mutable variable initialization with ':=' or immutable with '::'");
    } else {
      comp_errorf(l->loc,
                  "Unexpected %s token `"SV_Fmt"`: expected variable initialization with ':=' or immutable with '::'",
                  token_kind_name(tok.kind),
                  SV_Arg(tok.sv));
    }
    comp_note(decl_start_loc, "Variable declaration starts here");
    return false;
  }

  if (!expect_next_token_kind(l, &tok, TOK_SYMBOL) || (!sv_eq_str(tok.sv, "=") && !sv_eq_str(tok.sv, ":"))) {
    if (tok.kind == TOK_EOF) {
      comp_error(l->loc, "Unexpected end of file: expected mutable variable initialization with ':=' or immutable with '::'");
    } else {
      comp_errorf(l->loc,
                  "Unexpected %s token `"SV_Fmt"`: expected variable initialization with ':=' or immutable with '::'",
                  token_kind_name(tok.kind),
                  SV_Arg(tok.sv));
    }
    comp_note(decl_start_loc, "Variable declaration starts here");
    return false;
  }
  decl->as.var_decl.mutable = sv_eq_str(tok.sv, "=");

  AST_NodeList expr = {
    .items = nob_temp_alloc(sizeof(AST_Node)),
    .capacity = 1,
  };
  if (!ast_create_expr(l, &expr)) {
    comp_note(decl_start_loc, "Variable declaration starts here");
    return false;
  }

  decl->as.var_decl.expr = expr;

  if (!expect_next_token_eq_str(l, &tok, TOK_SYMBOL, SEMICOLON)) {
    if (tok.kind == TOK_EOF) {
      comp_errorf(l->loc, "Expected semicolon for end of statement but found %s", token_kind_name(tok.kind));
    } else {
      comp_errorf(l->loc, "Expected semicolon for end of statement but found %s "SV_Fmt, token_kind_name(tok.kind), SV_Arg(tok.sv));
    }
    return false;
  }
  return true;
}

bool ast_create_assignment(Lexer *l, AST_Node *node, Nob_String_View name) {
  Token tok = {0};
  node->kind = AST_NK_ASSIGNMENT;
  node->as.var_assign.name = name;
  if (!peek_token(*l, &tok)) {
    comp_error(l->loc, "Unexpected end of file: missing rvalue for assignment");
    return false;
  }
  Loc loc = l->loc;
  AST_NodeList expr = {0};
  if (!ast_create_expr(l, &expr)) {
    comp_notef(loc, "Invalid rvalue for variable assignment for variable `"SV_Fmt"`", SV_Arg(name));
    return false;
  }
  node->as.var_assign.expr = expr;
  if (!expect_next_token_eq_str(l, &tok, TOK_SYMBOL, SEMICOLON)) {
    if (tok.kind == TOK_EOF) {
      comp_errorf(l->loc, "Expected semicolon for end of statement but found %s", token_kind_name(tok.kind));
    } else {
      comp_errorf(l->loc, "Expected semicolon for end of statement but found %s "SV_Fmt, token_kind_name(tok.kind), SV_Arg(tok.sv));
    }
    return false;
  }
  return true;
}

bool ast_create_fn_body(Lexer *l, AST_Node *fn_node) {
  Token tok;
  if (!expect_next_token_eq_str(l, &tok, TOK_SYMBOL, "{")) {
    comp_errorf(l->loc, "Expected '{' for declaring function body but found %s", token_kind_name(tok.kind));
    return false;
  }

  AST_NodeList *body = &fn_node->as.fn_decl.body;
  while (peek_token(*l, &tok) && !sv_eq_str(tok.sv, "}")) {
    if (sv_eq_str(tok.sv, KEYWORD_LET)) {
      AST_Node node = { .loc = l->loc };
      if (!ast_create_var_decl(l, &node)) {
        return false;
      }
      nob_da_append(body, node);
      continue;
    }
    if (tok.kind == TOK_IDENT) {
      next_token(l, &tok);
      AST_Node node = {
        .loc = l->loc,
        .kind = AST_NK_EOF,
      };
      Nob_String_View name = tok.sv;
      if (!expect_next_token_kind(l, &tok, TOK_SYMBOL)) {
        if (tok.kind == TOK_EOF) {
          comp_error(l->loc, "Unexpected End of File created hanging statement");
        } else {
          comp_errorf(l->loc, "Unexpected token expected ';' or '()' but got %s `"SV_Fmt"`", token_kind_name(tok.kind), SV_Arg(tok.sv));
        }
        return false;
      }
      if (sv_eq_str(tok.sv, EQSIGN)) {
        if (!ast_create_assignment(l, &node, name)) {
          return false;
        }
        nob_da_append(body, node);
        continue;
      }
      node.kind = AST_NK_FN_CALL;
      node.as.fn_call.name = name;
      if (!sv_eq_str(tok.sv, "(")) {
        comp_errorf(l->loc, "Unexpected token expected ';' or '()' but got `"SV_Fmt"`", SV_Arg(tok.sv));
        return false;
      }
      Lexer peeker = *l;
      if (!expect_next_token_kind_from_arr(&peeker, &tok, ((TokenKind[]){TOK_SYMBOL, TOK_IDENT, TOK_INT}))) {
        if (tok.kind == TOK_EOF) {
          comp_error(l->loc, "Unexpected End of File unfinished function call");
        } else {
          comp_errorf(l->loc, "Unexpected token expected closing parenthesis `)` or an function argument but got %s `"SV_Fmt"`", token_kind_name(tok.kind), SV_Arg(tok.sv));
        }
        return false;
      }
      if (tok.kind == TOK_SYMBOL) {
        lexer_next_token(l);
        if (!sv_eq_str(tok.sv, ")")) {
          comp_errorf(l->loc, "Unexpected token expected closing parenthesis `)` but got %s `"SV_Fmt"`", token_kind_name(tok.kind), SV_Arg(tok.sv));
          return false;
        }
      } else if (tok.kind == TOK_IDENT || tok.kind == TOK_INT) {
        AST_NodeList params = {0};
        AST_NodeList p_expr = {0};
        AST_Node p_node = {
          .loc = l->loc,
          .kind = AST_NK_TOKEN,
        };
        p_node.as.token = tok;
        if (!ast_create_expr(l, &p_expr)) {
          comp_error(l->loc, "Failed to parse function call argument");
          return false;
        }
        if (p_expr.count == 1) {
          p_node = p_expr.items[0];
        } else {
          p_node.kind = AST_NK_EXPR;
          p_node.loc = peeker.loc;
          p_node.as.expr = p_expr;
        }
        nob_da_append(&params, p_node);

        Token peeked = {0};
        if (!next_token(&peeker, &peeked)) {
          comp_error(peeker.loc, "Unexpected End of File unfinished function call");
          comp_note(l->loc, "After first parameter expected closing parenthesis `)` or more arguments");
          return false;
        }
        if (peeked.kind == TOK_SYMBOL && sv_eq_str(peeked.sv, ",")) {
          lexer_next_token(l);
          while (true) {
            p_expr.count = 0;
            lexer_next_token(&peeker);
            if (!ast_create_expr(l, &p_expr)) {
              comp_error(l->loc, "Failed to parse function call argument");
              return false;
            }
            if (p_expr.count == 1) {
              p_node = p_expr.items[0];
            } else {
              p_node.kind = AST_NK_EXPR;
              p_node.loc = peeker.loc;
              p_node.as.expr = p_expr;
            }
            nob_da_append(&params, p_node);
            peeker = *l;
            if (!next_token(&peeker, &peeked)) {
              comp_error(l->loc, "Unexpected End of File unfinished function call");
              return false;
            }
            if (!sv_eq_str(peeked.sv, ",")) {
              break;
            }
            lexer_next_token(l);
          }
        }
        node.as.fn_call.params = params;

        if (!expect_next_token_eq_str(l, &tok, TOK_SYMBOL, ")")) {
          if (tok.kind == TOK_EOF) {
            comp_error(l->loc, "Unexpected End of File unfinished function call");
          } else {
            comp_errorf(l->loc, "Unexpected token expected closing parenthesis `)` but got %s `"SV_Fmt"`", token_kind_name(tok.kind), SV_Arg(tok.sv));
          }
          return false;
        }
      }
      if (!expect_next_token_eq_str(l, &tok, TOK_SYMBOL, SEMICOLON)) {
        if (tok.kind == TOK_EOF) {
          comp_errorf(l->loc, "Expected semicolon for end of statement but found %s", token_kind_name(tok.kind));
        } else {
          comp_errorf(l->loc, "Expected semicolon for end of statement but found %s "SV_Fmt, token_kind_name(tok.kind), SV_Arg(tok.sv));
        }
        return false;
      }
      nob_da_append(body, node);
      continue;
    }
    next_token(l, &tok);
  }
  if (!sv_eq_str(tok.sv, "}")) {
    comp_error(l->loc, "Expected '}' to close the function body but found end of file instead");
    return false;
  }
  lexer_next_token(l);
  da_compact(&fn_node->as.fn_decl.body);
  return true;
}

bool ast_chomp(Lexer *l, AST_Node *node) {
  Token tok;
  // EOF when not expecting anything isn't an error
  if (!next_token(l, &tok)) {
    node->kind = AST_NK_EOF;
    return true;
  }
  if (sv_eq_str(tok.sv, KEYWORD_FN)) {
    node->loc = l->loc;
    if (!expect_next_token_kind(l, &tok, TOK_IDENT)) {
      node->kind = AST_NK_EOF;
      comp_errorf(l->loc, "Expected identifier for function name but found %s", token_kind_name(tok.kind));
      return false;
    }
    node->loc = l->loc;
    node->kind = AST_NK_FN_DECL;
    node->as.fn_decl.name = tok.sv;
    if (!expect_next_token_eq_str(l, &tok, TOK_SYMBOL, "(")) {
      comp_error(l->loc, "Unexpected end of file: Was expecting the continuation to a function declaration but got EOF");
      return false;
    }
    // TODO: Add function parameters
    AST_NodeList params = {0};
    node->as.fn_decl.params = params;
  
    if (!expect_next_token_eq_str(l, &tok, TOK_SYMBOL, ")")) {
      comp_error(l->loc, "Unexpected end of file: Was expecting the closing of the function parameters declaration but got EOF");
      return false;
    }

    AST_NodeList body = {0};
    node->as.fn_decl.body = body;
    if (!ast_create_fn_body(l, node)) {
      return false;
    }
    return true;
  }
  if (sv_eq_str(tok.sv, KEYWORD_IMPORT)) {
    AST_Import import = {0};
    Loc init_loc = l->loc;
    Loc last_colon = l->loc;
    TokenKind prv_kind = TOK_SYMBOL;
    Nob_String_Builder sb = {0};
    nob_sb_to_sv(sb);
    while (true) {
      if (!next_token(l, &tok)) {
        comp_error(l->loc, "Unexpected end of file: use statement must end with ;");
        if (l->loc.row != init_loc.row) comp_note(init_loc, "Import statement started here");
        return false;
      }
      if (tok.kind == TOK_SYMBOL) {
        if (sv_eq_str(tok.sv, SEMICOLON)) {
          break;
        }
        if (sv_eq_str(tok.sv, ":")) {
          last_colon = l->loc;
          if (prv_kind != TOK_IDENT) {
            comp_error(l->loc, "Invalid way to declare an import. Imports are declared with the following syntax `use core:io;`");
            return false;
          }
          prv_kind = TOK_SYMBOL;
          nob_da_append(&sb, ':');
          continue;
        }
      }
      if (tok.kind == TOK_IDENT) {
        prv_kind = TOK_IDENT;
        sb_append_sv(&sb, tok.sv);
        continue;
      }
      comp_errorf(l->loc, "Unexpected token %s `"SV_Fmt"` in import statement", token_kind_name(tok.kind), SV_Arg(tok.sv));
      comp_note(init_loc, "Import statement started here");
      return false;
    }
    Nob_String_View name = nob_sb_to_sv(sb);
    if (nob_sv_end_with(name, ":")) {
      comp_errorf(last_colon, "Import name cannot end with a ':' did you miss to type something?");
      return false;
    }
    import.name = name;
    node->kind = AST_NK_IMPORT;
    node->as.import = import;
    return true;
  }
  nob_log(NOB_ERROR, "Don't know how to parse %s `"SV_Fmt"`", token_kind_name(tok.kind), SV_Arg(tok.sv));
  return false;
}

#endif // DWOC_AST_IMPLEMENTATION

