
#ifndef __DWOC_JavaScript_H
#define __DWOC_JavaScript_H

#include "utils.h"
#include "ast.h"

#ifndef NOB_IMPLEMENTATION
#  include "nob.h"
#endif

void javascript_compilation_prologue(Nob_String_Builder *sb);

bool javascript_run_compilation(Nob_String_Builder *sb, Context *ctx);

void javascript_compilation_epilogue(Nob_String_Builder *sb, Context *ctx);

#endif // __DWOC_JavaScript_H

#ifdef DWOC_JS_IMPLEMENTATION

void javascript_compilation_prologue(Nob_String_Builder *sb) {
  nob_sb_append_cstr(sb, "\"use strict\";\n\n");
}

bool javascript_compile_expr_at_depth(Nob_String_Builder *sb, AST_NodeList *expr, int depth) {
  // nob_log(NOB_INFO, "Compiling expression...");
  sb_add_indentation_level(sb, i, depth);
  nob_da_foreach(AST_Node, node, expr) {
    switch (node->kind) {
    case AST_NK_IDENT:
      sb_append_sv(sb, node->as.sv);
      break;
    case AST_NK_INT_LITERAL:
      nob_sb_appendf(sb, "%d", node->as.integer);
      break;
    default:
      comp_errorf(node->loc, "Unsupported %s in expression", ast_node_kind_name(node->kind));
      comp_note(expr->items[0].loc, "Expression starts here");
      return false;
    }
  }
  return true;
}

bool javascript_compile_var_declaration(Nob_String_Builder *sb, AST_Node node, int depth) {
  // nob_log(NOB_INFO, "Compiling variable declaration...");
  sb_add_indentation_level(sb, i, depth);
  AST_VarDeclAttr decl = node.as.var_decl;
  if (decl.mutable) {
    nob_sb_append_cstr(sb, "let ");
  } else {
    nob_sb_append_cstr(sb, "const ");
  }
  sb_append_sv(sb, decl.name);
  if (decl.expr.count == 0) {
    if (!decl.mutable) {
      comp_error(node.loc, "Constant variables require to be set on declaration");
      return false;
    }
    nob_sb_append_cstr(sb, ";");
    return true;
  }
  nob_sb_append_cstr(sb, " = ");
  if (!javascript_compile_expr_at_depth(sb, &decl.expr, 0)) return false;
  nob_sb_append_cstr(sb, ";");
  return true;
}

bool javascript_compile_fn_declaration(Nob_String_Builder *sb, AST_Node node, int depth) {
  // nob_log(NOB_INFO, "Compiling function declaration...");
  sb_add_indentation_level(sb, i, depth);
  Vars local_vars = {0};
  NOB_UNUSED(local_vars);
  AST_FnDeclAttr decl = node.as.fn_decl;
  nob_sb_append_cstr(sb, "function ");
  sb_append_sv(sb, decl.name);
  // TODO: Actually handle parameters from decl.params
  nob_sb_append_cstr(sb, "() {\n");
  nob_da_foreach(AST_Node, node, &decl.body) {
    switch (node->kind) {
    // Atoms
    case AST_NK_IDENT:
    case AST_NK_SYMBOL:
    case AST_NK_INT_LITERAL:
      comp_warnf(node->loc, "Dangling atom %s with no operation or usage found", ast_node_kind_name(node->kind));
      sb_add_indentation_level(sb, i, depth+1);
      if (node->kind == AST_NK_INT_LITERAL) {
        nob_sb_appendf(sb, "%d", node->as.integer);
      } else {
        sb_append_sv(sb, node->as.sv);
      }
      nob_sb_append_cstr(sb, ";");
      break;

      // Molecules
    case AST_NK_UNOP:
    case AST_NK_BINOP:
      TODOf("Implement compilation of %s molecule", ast_node_kind_name(node->kind));
      break;
    case AST_NK_FN_PARAMS:
    case AST_NK_FN_PARAMS_DECL:
      NEVERf("Molecule %s should not be found in function body", ast_node_kind_name(node->kind));
      break;

      // Compounds
    case AST_NK_EXPR:
      if (!javascript_compile_expr_at_depth(sb, &node->as.expr, depth + 1)) return false;
      break;
    case AST_NK_VAR_DECL:
      // TODO: Check if local variable is being re-declared
      if (!javascript_compile_var_declaration(sb, *node, depth + 1)) return false;
      break;
    case AST_NK_FN_DECL:
      comp_error(node->loc, "Closures are not supported, yet");
      printf("    Function "SV_Fmt" should be moved outside\n", SV_Arg(node->as.fn_decl.name));
      break;
    case AST_NK_EOF:
      NEVER("End of File should never be part of function body");
      break;

    default:
      TODOf("Implement missing AST Node kind ('%s') compilation", ast_node_kind_name(node->kind));
    }
    nob_sb_append_cstr(sb, "\n");
  }
  nob_sb_append_cstr(sb, "}");
  return true;
}

bool javascript_run_compilation(Nob_String_Builder *sb, Context *ctx) {
  AST_Node node = {0};
  for (;;) {
    node.kind = AST_NK_EOF;
    if (!ast_chomp(&ctx->lex, &node)) {
      nob_log(NOB_INFO, "Errored on ast node %s", ast_node_kind_name(node.kind));
      return false;
    }
    // nob_log(NOB_INFO, "Found ast node %s", ast_node_kind_name(node.kind));
    switch (node.kind) {
    case AST_NK_EOF: return true;

    // Atoms
    case AST_NK_IDENT:
    case AST_NK_SYMBOL:
    case AST_NK_INT_LITERAL:
      comp_warnf(ctx->lex.loc, "Dangling atom %s with no operation or usage found", ast_node_kind_name(node.kind));
      break;

      // Molecules
    case AST_NK_UNOP:
    case AST_NK_BINOP:
      comp_warnf(ctx->lex.loc, "Dangling molecules %s with no operation or usage found", ast_node_kind_name(node.kind));
      break;
    case AST_NK_FN_PARAMS:
    case AST_NK_FN_PARAMS_DECL:
      comp_errorf(ctx->lex.loc, "Impossibly dangling molecules %s found", ast_node_kind_name(node.kind));
      HEREf("Impossible dangling %s found. Gotta debug lexing/parsing", ast_node_kind_name(node.kind));
      break;

      // Compounds
    case AST_NK_EXPR:
      javascript_compile_expr_at_depth(sb, &node.as.expr, 0);
      break;
    case AST_NK_VAR_DECL:
      // TODO: Check if global variable is being re-declared
      if (!javascript_compile_var_declaration(sb, node, 0)) return false;
      break;
    case AST_NK_FN_DECL:
      if (!javascript_compile_fn_declaration(sb, node, 0)) {
        // nob_log(NOB_INFO, "Function declaration failed");
        return false;
      }
      // nob_log(NOB_INFO, "Function declaration succeeded");
      break;
    default:
      TODOf("Implement missing AST Node kind ('%s') compilation", ast_node_kind_name(node.kind));
    }
    nob_sb_append_cstr(sb, "\n");
  }
  nob_log(NOB_INFO, "Wrote %zu bytes onto buffer", sb->count);
  if (node.kind != AST_NK_EOF) {
    // nob_log(NOB_INFO, "Compilation finished with node: %s", ast_node_kind_name(node.kind));
    return false;
  }
  return true;
}

void javascript_compilation_epilogue(Nob_String_Builder *sb, Context *ctx) {
  if (ctx->main_is_defined) nob_sb_append_cstr(sb, "\n\nmain();\n");
}

#endif // DWOC_JS_IMPLEMENTATION

