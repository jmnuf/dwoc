
#ifdef _MSC_VER
#  define nob_cc_flags(cmd) nob_cmd_append(cmd, "/nologo", "/W4", "/wd4146", "/wd4244", "/wd4245", "/wd4456", "/wd4457", "/wd4819", "/we4013", "/we4706", "/D_CRT_SECURE_NO_WARNINGS")
#  define my_cc_debug(cmd) nob_cmd_append(cmd, "/DEBUG:FULL")
#  define my_cc_output(cmd, output) nob_cmd_append(cmd, nob_temp_sprintf("/Fe:%s.exe", output))
#  define my_cc_include(cmd, include) nob_cmd_append(cmd, nob_temp_sprintf("/I%s", include))
#else
#  define my_cc_debug(cmd) nob_cmd_append(cmd, "-ggdb")
#  define my_cc_output(cmd, output) nob_cmd_append(cmd, "-o", output)
#  define my_cc_include(cmd, include) nob_cmd_append(cmd, nob_temp_sprintf("-I%s", include))
#endif

#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

#define cstr_eq(a, b) (strcmp(a, b) == 0)

const char *source_files[] = {
  "src/dwoc.c",
  "src/javascript.h",
  "src/utils.h",
  "src/lexer.h",
  "src/ast.h",
};
size_t source_files_count = NOB_ARRAY_LEN(source_files);

bool generate_etags(Cmd *cmd) {
  cmd_append(cmd, "etags", "-o", "TAGS", "--lang=c");
  cmd_append(cmd, "nob.h");
  for (size_t i = 0; i < source_files_count; ++i) {
    cmd_append(cmd, source_files[i]);
  }
  return cmd_run_sync_and_reset(cmd);
}

bool generate_etags_silent(Cmd *cmd) {
  Nob_Log_Level base_min_level = nob_minimal_log_level;
  nob_minimal_log_level = NOB_NO_LOGS;
  bool result = generate_etags(cmd);
  nob_minimal_log_level = base_min_level;
  return result;
}

void usage(const char *program) {
  printf("Usage: %s [FLAGS]\n", program);
  printf("    -run <(ir|js)>   -----  Run example `dwoc hello.dwo`\n");
  printf("    -etags           -----  Generate TAGS file with etags for project\n");
  printf("    -f               -----  Force rebuild of dowc\n");
}

int main(int argc, char** argv) {
  NOB_GO_REBUILD_URSELF(argc, argv);

  const char *program = shift(argv, argc);
  Cmd cmd = {0};

  nob_log(NOB_INFO, "Project has %zu source files registered (nob files aren't counted)", source_files_count);

  bool should_run = false, force_rebuild = false, create_etags_on_rebuild = false;
  char *target = "ir";
  while(argc > 0) {
    const char *flag = shift(argv, argc);
    if (cstr_eq(flag, "-run")) {
      should_run = true;
      if (argc == 0) {
        nob_log(NOB_ERROR, "Missing run target for run flag");
        usage(program);
        return 1;
      }
      char *rt = shift(argv, argc);
      if (cstr_eq(rt, "ir")) {
        target = "ir";
      } else if (cstr_eq(rt, "js")) {
        target = "js";
      } else {
        nob_log(NOB_ERROR, "Invalid run target for run flag");
        usage(program);
      }
    } else if (cstr_eq(flag, "-f")) {
      force_rebuild = true;
    } else if (cstr_eq(flag, "-etags")) {
      if (generate_etags(&cmd)) {
        nob_log(NOB_INFO, "Generated TAGS file succesfully");
      } else {
        nob_log(NOB_ERROR, "Failed to generate TAGS file");
      }
      create_etags_on_rebuild = true;
      // I am likely to literally run any of these at random cause I sometimes am dumb
    } else if (cstr_eq(flag, "-h") || cstr_eq(flag, "--help") || cstr_eq(flag, "/?")) {
      usage(program);
      return 0;
    } else if (cstr_eq(flag, "-netags")) {
      create_etags_on_rebuild = false;
    } else {
      nob_log(NOB_ERROR, "Unknown flag: %s", flag);
      usage(program);
      return 1;
    }
  }


  if (!mkdir_if_not_exists("build")) return 1;
  if (force_rebuild || needs_rebuild("build/dwoc.exe", source_files, source_files_count)) {
    nob_cc(&cmd);
    my_cc_output(&cmd, "./build/dwoc");
    nob_cc_flags(&cmd);
    my_cc_debug(&cmd);
    my_cc_include(&cmd, ".");
    for (size_t i = 0; i < source_files_count; ++i) {
      Nob_String_View ssv = nob_sv_from_cstr(source_files[i]);
      if (sv_end_with(ssv, ".c")) cmd_append(&cmd, source_files[i]);
    }
    if (!cmd_run_sync_and_reset(&cmd)) return 1;

    if (create_etags_on_rebuild) generate_etags_silent(&cmd);
  }

  if (should_run) {
#if _WIN32
    cmd_append(&cmd, "build\\dwoc.exe", "-o", "build\\hello", "hello.dwo");
#else
    cmd_append(&cmd, "build/dwoc", "-o", "build/hello", "hello.dwo");
    #endif
    cmd_append(&cmd, "-t", target);
    cmd_run_sync_and_reset(&cmd);
  }

  return 0;
}
