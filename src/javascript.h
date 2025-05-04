
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
  // TODO: Embed some runtime stuff
  // possibly also needs options passed in to know what runtime things need to be added.
  nob_sb_append_cstr(sb, "\"use strict\";\n\n");
}

bool javascript_compile_expr_at_depth(Nob_String_Builder *sb, AST_NodeList *expr, int depth) {
  // nob_log(NOB_INFO, "Compiling expression...");
  sb_add_indentation_level(sb, i, depth);
  nob_da_foreach(AST_Node, node, expr) {
    switch (node->kind) {
    case AST_NK_TOKEN:
      sb_append_sv(sb, node->as.token.sv);
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
    case AST_NK_TOKEN:
      comp_warnf(node->loc, "Dangling atom %s with no operation or usage found", token_kind_name(node->as.token.kind));
      sb_add_indentation_level(sb, i, depth+1);
      sb_append_sv(sb, node->as.token.sv);
      nob_sb_append_cstr(sb, ";");
      break;

      // Molecules
    case AST_NK_UNOP:
    case AST_NK_BINOP:
      TODOf("Implement compilation of %s molecule", ast_node_kind_name(node->kind));
      break;
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
    case AST_NK_ASSIGNMENT:
      sb_add_indentation_level(sb, i, depth+1);
      sb_append_sv(sb, node->as.var_assign.name);
      nob_sb_append_cstr(sb, " = ");
      if (!javascript_compile_expr_at_depth(sb, &node->as.var_assign.expr, 0)) return false;
      nob_sb_append_cstr(sb, ";");
      break;
    case AST_NK_FN_CALL:
      sb_add_indentation_level(sb, i, depth+1);
      sb_append_sv(sb, node->as.fn_call.name);
      nob_sb_append_cstr(sb, "(");
      if (node->as.fn_call.params.count > 0) {
        AST_NodeList *fn_params = &node->as.fn_call.params;
        if (fn_params->count > 0) {
          nob_da_foreach(AST_Node, param, fn_params) {
            size_t index = param - node->as.fn_call.params.items;
            if (index > 0) nob_sb_append_cstr(sb, ", ");
            switch (param->kind) {
            case AST_NK_TOKEN:
              sb_append_sv(sb, param->as.token.sv);
              break;
            case AST_NK_EXPR:
              if (!javascript_compile_expr_at_depth(sb, &param->as.expr, 0)) return false;
              break;
            default:
              comp_errorf(param->loc, "Unsupported %s in expression", ast_node_kind_name(param->kind));
              comp_note(fn_params->items[0].loc, "Expression starts here");
              return false;
            }
          }
        }
      }
      nob_sb_append_cstr(sb, ");");
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

void javascript_import_core_io(Nob_String_Builder *sb, Context *ctx) {
  static char *variable_names[] = {"stdin", "stdout", "stderr", "stdwarn"};
  static char *fn_names[] = {"print", "println", "putchar", "flush"};
  Nob_String_View library = SVl("core:io", 7);
  size_t tmp_sp = nob_temp_save();

  // TODO: These variables should be defined before we reach compilation stage for the sake of type checking
  carray_foreach(char*, it, variable_names) {
    char *name = *it;
    Var io_var = {
      .name = SV(name),
      .immutable = true,
      .library = library,
    };
    nob_da_append(&ctx->vars, io_var);
  }

  carray_foreach(char*, it, fn_names) {
    char *name = *it;
    Fn io_fn = {
      .name = SV(name),
      .library = library,
    };
    nob_da_append(&ctx->fns, io_fn);
  }

  // Define base FDs (standard input/output/error)
  nob_sb_append_cstr(sb,
  "(function(){\n"
  "const utf8Decoder = new TextDecoder();\n"
  "const utf8Encoder = new TextEncoder();\n"
  "const buffers = [];\n"
  "const stdin = buffers.push(null)-1, stdout=buffers.push('')-1, stderr=buffers.push('')-1, stdwarn=buffers.push('')-1;\n"
  "globalThis.stdin=stdin;globalThis.stdout=stdout;globalThis.stderr=stderr;globalThis.stdwarn=stdwarn;\n");

  nob_sb_append_cstr(sb,
  "const print = (...args) => {\n"
  "  for (const arg of args) {\n"
  "    if (arg == '\\n') { console.log(buffers[stdout]); buffers[stdout] = ''; continue; }\n"
  "    if (typeof arg === 'string' && arg.includes('\\n')) {\n"
  "      const idx = arg.lastIndexOf('\\n');\n"
  "      const content = buffers[stdout] + arg.substring(0, idx);\n"
  "      console.log(content);\n"
  "      buffers[stdout] = arg.substring(idx+1);\n"
  "      continue;\n"
  "    }\n"
  "    \n"
  "    buffers[stdout] += `${arg}`;\n"
  "  }\n"
  "};globalThis.print = print;\n");
  
  nob_sb_append_cstr(sb,
  "const println = (...args) => {\n"
  "  for (const arg of args) {\n"
  "    if (arg == '\\n') { console.log(buffers[stdout]); buffers[stdout] = ''; continue; }\n"
  "    if (typeof arg === 'string' && arg.includes('\\n')) {\n"
  "      const idx = arg.lastIndexOf('\\n');\n"
  "      const content = buffers[stdout] + arg.substring(0, idx);\n"
  "      console.log(content);\n"
  "      buffers[stdout] = arg.substring(idx+1);\n"
  "      continue;\n"
  "    }\n"
  "    \n"
  "    buffers[stdout] += `${arg}`;\n"
  "  }\n"
  "  console.log(buffers[stdout]);\n"
  "  buffers[stdout] = '';\n"
  "};globalThis.println = println;\n");
  
  nob_sb_append_cstr(sb,
  "const putchar = (...chars) => {\n"
  "  if (chars.length == 0) { return; };\n"
  "  if (chars.length == 1 && chars[0] === 10) { console.log(buffers[stdout]); buffers[stdout] = ''; return; }\n"
  "  if (chars.length == 1) { buffers[stdout] += utf8Decoder.decode(new Uint8Array(chars)); return; }\n"
  "  const subbuf = [];\n"
  "  for (const ch of chars) {\n"
  "    if (ch == 10) { console.log(buffers[stdout] + (subbuf.length == 0 ? '' : utf8Decoder.decode(new Uint8Array(subbuf)))); buffers[stdout] = ''; subbuf.length = 0; continue;  }\n"
  "    subbuf.push(ch);\n"
  "  }\n"
  "  if (subbuf.length == 0) return;\n"
  "  buffers[stdout] += utf8Decoder.decode(new Uint8Array(subbuf));\n"
  "};globalThis.putchar = putchar;\n");

  nob_sb_append_cstr(sb,
  "const flush = () => {\n"
  "  if (buffers[stdout]) console.log(buffers[stdout]);\n"
  "  buffers[stdout] = '';\n"
  "};globalThis.flush = flush;\n");

  nob_sb_append_cstr(sb, "})();\n");

  nob_temp_rewind(tmp_sp);
}

bool javascript_run_compilation(Nob_String_Builder *sb, Context *ctx) {
  AST_Node node = {0};
  while (true) {
    node.kind = AST_NK_EOF;
    if (!ast_chomp(&ctx->lex, &node)) {
      nob_log(NOB_INFO, "Errored on ast node %s", ast_node_kind_name(node.kind));
      return false;
    }
    switch (node.kind) {
    case AST_NK_EOF: return true;

    // Atoms
    case AST_NK_TOKEN:
      comp_warnf(ctx->lex.loc, "Dangling atom %s at top level", ast_node_kind_name(node.kind));
      break;
    case AST_NK_IMPORT:
      if (sv_eq_str(node.as.import.name, "core:io")) {
        javascript_import_core_io(sb, ctx);
        break;
      }
      TODO("Implement imports in javascript declaration");
      break;

      // Molecules
    case AST_NK_UNOP:
    case AST_NK_BINOP:
      comp_warnf(ctx->lex.loc, "Dangling molecules %s at top level", ast_node_kind_name(node.kind));
      break;
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
        return false;
      }
      if (sv_eq_str(node.as.fn_decl.name, "main")) {
        ctx->main_is_defined = true;
      }
      break;
    default:
      TODOf("Implement missing AST Node kind ('%s') compilation", ast_node_kind_name(node.kind));
    }
    nob_sb_append_cstr(sb, "\n");
  }
}

void javascript_compilation_epilogue(Nob_String_Builder *sb, Context *ctx) {
  if (ctx->main_is_defined) nob_sb_append_cstr(sb, "\n{ const r = main(); flush(); if (typeof r === 'number') if (r != 0) { throw new Error(`Program exited with non-zero exit code: ${r}`); } }\n");
}

#endif // DWOC_JS_IMPLEMENTATION

