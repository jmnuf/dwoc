
#ifndef __DWOC_AST_H
#define __DWOC_AST_H

#include "lexer.h"

typedef struct {
  const char *source_path;
  bool main_is_defined;
  Lexer lex;
  Vars vars;
} Context;

const char *KEYWORD_FN = "fn";
const char *KEYWORD_LET = "let";

typedef enum {
  AST_NK_EOF,
  // Atoms
  AST_NK_TOKEN, // Encapsulates identifiers, symbols and literals

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


void ast_dump_node_list(Nob_String_Builder *sb, AST_NodeList *nodes) {
  nob_da_foreach(AST_Node, n, nodes) {
    ast_dump_node(sb, *n);
    size_t index = n - nodes->items;
    if (index < nodes->count - 1) nob_sb_append_cstr(sb, ", ");
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
    nob_da_foreach(AST_Node, n, &node.as.expr) {
      ast_dump_node(sb, *n);
      size_t index = n - node.as.expr.items;
      if (index < node.as.expr.count - 1) nob_sb_append_cstr(sb, ", ");
    }
    nob_sb_append_cstr(sb, "\n");
    sb_add_indentation_level(sb, i, depth);
    nob_sb_append_cstr(sb, ")");
    return;

  case AST_NK_VAR_DECL:
    if (node.as.var_decl.mutable) {
      nob_sb_append_cstr(sb, "Node::VarDecl<mutable>(\n");
    } else {
      nob_sb_append_cstr(sb, "Node::VarDecl<immutable>(\n");
    }
    sb_add_indentation_level(sb, i, depth+1);
    nob_sb_appendf(sb, "Token::Ident('"SV_Fmt"'),\n", SV_Arg(node.as.var_decl.name));
    nob_da_foreach(AST_Node, n, &node.as.var_decl.expr) {
      ast_dump_node_at_depth(sb, *n, depth+1);
      size_t index = n - node.as.var_decl.expr.items;
      if (index < node.as.expr.count - 1) nob_sb_append_cstr(sb, ", ");
    }
    nob_sb_append_cstr(sb, "\n");
    sb_add_indentation_level(sb, i, depth);
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
      ast_dump_node_at_depth(sb, *n, depth+1);
      size_t index = n - node.as.fn_decl.body.items;
      if (index < node.as.expr.count - 1) nob_sb_append_cstr(sb, ";\n");
    }
    
    sb_add_indentation_level(sb, i, depth);
    nob_sb_append_cstr(sb, "}");
    return;

  case AST_NK_FN_CALL:
    nob_sb_append_cstr(sb, "Node::FnCall(");
    dump_token(sb, (Token) { .kind = TOK_IDENT, .sv = node.as.fn_call.name });
    nob_sb_append_cstr(sb, ", [");
    if(node.as.fn_call.params.count > 0) {
      nob_da_foreach(AST_Node, n, &node.as.fn_call.params) {
        ast_dump_node(sb, *n);
        size_t index = n - node.as.fn_call.params.items;
        if (index + 1 < node.as.fn_call.params.count - 1) nob_sb_append_cstr(sb, ", ");
      }
    }
    nob_sb_append_cstr(sb, "])");
    return;
  }
  nob_log(NOB_WARNING, "Fell through switch statement of node kinds %s(%d)", ast_node_kind_name(node.kind), node.kind);
  HERE("ast_dump_node_at_depth: Unknown node kind");
  // nob_sb_append_cstr(sb, "<UNKNOWN_NODE_KIND>");
}

bool ast_create_expr(Lexer *l, AST_NodeList *expr) {
  Token tok;
  static TokenKind allowed_expr_tokens[] = { TOK_IDENT, TOK_INT };
  static size_t allowed_expr_count = NOB_ARRAY_LEN(allowed_expr_tokens);
  
  // TODO: Parse an actual expression
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
                  "Unexpected token %s(`"SV_Fmt"`): Symbol `+` or `;` was expected",
                  token_kind_name(tok.kind), SV_Arg(tok.sv));
      return false;
    }
    if (sv_eq_str(tok.sv, SEMICOLON)) {
      break;
    }
    if (sv_eq_str(tok.sv, "+")) {
      lexer_next_token(l);
      AST_Node operand = {
        .loc = l->loc,
        .kind = AST_NK_TOKEN,
      };
      operand.as.token = tok;
      nob_da_append(expr, operand);
      continue;
    }
    comp_errorf(peeker.loc,
                "Unexpected token %s(`"SV_Fmt"`): Symbol `+` or `;` was expected",
                token_kind_name(tok.kind), SV_Arg(tok.sv));
    return false;
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
      if (!expect_next_token_kind_from_arr(l, &tok, ((TokenKind[]){TOK_SYMBOL, TOK_IDENT}))) {
        if (tok.kind == TOK_EOF) {
          comp_error(l->loc, "Unexpected End of File unfinished function call");
        } else {
          comp_errorf(l->loc, "Unexpected token expected closing parenthesis `)` or an identifier but got %s `"SV_Fmt"`", token_kind_name(tok.kind), SV_Arg(tok.sv));
        }
        return false;
      }
      if (tok.kind == TOK_SYMBOL) {
        if (sv_eq_str(tok.sv, ")")) {
          comp_errorf(l->loc, "Unexpected token expected closing parenthesis `)` but got %s `"SV_Fmt"`", token_kind_name(tok.kind), SV_Arg(tok.sv));
          return false;
        }
      } else if (tok.kind == TOK_IDENT) {
        AST_NodeList params = {0};
        AST_Node ident_node = {
          .loc = l->loc,
          .kind = AST_NK_TOKEN,
        };
        ident_node.as.token = tok;
        nob_da_append(&params, ident_node);
        node.as.fn_call.params = params;

        if (!expect_next_token_eq_str(l, &tok, TOK_SYMBOL, ")")) {
          if (tok.kind == TOK_EOF) {
            comp_error(l->loc, "Unexpected End of File unfinished function call");
          } else {
            comp_errorf(l->loc, "Unexpected token expected closing parenthesis `)` but got %s `"SV_Fmt"`", token_kind_name(tok.kind), SV_Arg(tok.sv));
          }
          printf("    NOTE: Only one or zero parameters are allowed for now\n");
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
  nob_log(NOB_ERROR, "Don't know how to parse %s `"SV_Fmt"`", token_kind_name(tok.kind), SV_Arg(tok.sv));
  return false;
}

#endif // DWOC_AST_IMPLEMENTATION

