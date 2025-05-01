
#ifdef _MSC_VER
#    define nob_cc_flags(cmd) nob_cmd_append(cmd, "/nologo", "/utf-8", "/W2", "/we4013", "/WX")
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

const char *source_files[] = {
  "src/dwoc.c",
};
size_t source_files_count = NOB_ARRAY_LEN(source_files);

int main(int argc, char** argv) {
  NOB_GO_REBUILD_URSELF(argc, argv);
#ifdef _MSC_VER // Is there really a need to keep the .obj files that msvc leaves around?
  nob_minimal_log_level = NOB_WARNING;
  if (file_exists("nob.obj")) {
    if (!delete_file("nob.obj")) {
      nob_log(NOB_WARNING, "Failed to delete");
    }
  }
  nob_minimal_log_level = NOB_INFO;
#endif

  char *program = shift(argv, argc);

  nob_log(NOB_INFO, "Using %zu source files", source_files_count);

  bool should_run = false, force_rebuild = false;
  while(argc > 0) {
    const char *flag = shift(argv, argc);
    if (strcmp(flag, "-run") == 0) {
      should_run = true;
    } else if (strcmp(flag, "-f") == 0) {
      force_rebuild = true;
    } else {
      nob_log(NOB_ERROR, "Unknown flag: %s", flag);
      printf("Usage: %s [-run] [-f]\n", program);
      printf("    -run      -----  Run example `dwoc hello.dwo`\n");
      printf("    -f        -----  Force rebuild of dowc\n");
      return 1;
    }
  }

  Cmd cmd = {0};

  if (force_rebuild || needs_rebuild("build/dwoc.exe", source_files, source_files_count)) {
    nob_cc(&cmd);
    my_cc_output(&cmd, "./build/dwoc");
    nob_cc_flags(&cmd);
    my_cc_debug(&cmd);
    my_cc_include(&cmd, ".");
    for (size_t i = 0; i < source_files_count; ++i) {
      cmd_append(&cmd, source_files[i]);
    }
    if (!cmd_run_sync_and_reset(&cmd)) return 1;
#ifdef _MSC_VER
    nob_minimal_log_level = NOB_WARNING;
    if (file_exists("dwoc.obj")) {
      if (!delete_file("dwoc.obj")) {
        nob_log(NOB_WARNING, "Failed to delete extra dwoc.obj file");
      }
    }
    nob_minimal_log_level = NOB_INFO;
#endif


    nob_minimal_log_level = NOB_NO_LOGS;
    cmd_append(&cmd, "etags", "-o", "TAGS", "--lang=c");
    cmd_append(&cmd, "nob.h");
    for (size_t i = 0; i < source_files_count; ++i) {
      cmd_append(&cmd, source_files[i]);
    }
    cmd_run_sync_and_reset(&cmd);
    nob_minimal_log_level = NOB_INFO;
  }

  if (should_run) {
#if _WIN32
    cmd_append(&cmd, "build\\dwoc.exe", "-o", "build\\hello", "hello.dwo");
#else
    cmd_append(&cmd, "build/dwoc", "-o", "build/hello", "hello.dwo");
#endif
    cmd_run_sync_and_reset(&cmd);
  }

  return 0;
}
