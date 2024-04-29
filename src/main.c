#include <core/debug.h>
#include <core/error.h>
#include <core/library.h>
#include <deserializer.h>
#include <interpreter.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
#define PATH_SEP '\\'
#else
#define PATH_SEP '/'
#endif

#define PLUME_VERSION "0.0.1"

enum { BigEndian, LittleEndian };

int endianness(void) {
  union {
    int i;
    char b[sizeof(int)];
  } u;
  u.i = 0x01020304;
  return (u.b[0] == 0x01) ? BigEndian : LittleEndian;
}

struct Env {
  char* path;
  int32_t path_len;
  uint8_t res;
};

static inline struct Env get_std_path() {
  struct Env env;
  env.path = getenv("PLUME_PATH");
  env.path_len = env.path == NULL ? 0 : strlen(env.path);
  env.res = env.path == NULL;
  return env;
}

int main(int argc, char** argv) {
#if DEBUG
  unsigned long long start = clock_gettime_nsec_np(CLOCK_MONOTONIC);
#endif

  if (argc < 2) THROW_FMT("Usage: %s <file>\n", argv[0]);
  FILE* file = fopen(argv[1], "rb");

  Value* values = malloc(argc * sizeof(Value));
  for (int i = 0; i < argc; i++) {
    values[i] = MAKE_STRING(argv[i], strlen(argv[i]));
  }

  if (file == NULL) THROW_FMT("Could not open file: %s\n", argv[1]);

  int endianness_check = endianness();

  if (endianness_check != LittleEndian) {
    THROW("Unsupported endianness");
  }

  Deserialized des = deserialize(file);

  fclose(file);

  des.module->argc = argc;
  des.module->argv = values;
  des.module->handles = malloc(des.libraries.num_libraries * sizeof(void*));

  struct Env res = get_std_path();

  // TODO: Implement library loading in a flat manner
  //       in order to avoid `calloc` calls in the loop.
  Libraries libs = des.libraries;

  for (int32_t i = 0; i < des.libraries.num_libraries; i++) {
    Library lib = libs.libraries[i];
    char* path = lib.name;

    if (lib.is_standard && res.res != 0) {
      THROW("Standard library path not found");
    }

    char* final_path =
        malloc((lib.is_standard ? res.path_len + 1 : 0) + strlen(path) + 1);

    if (lib.is_standard && res.res == 0) {
      sprintf(final_path, "%s%c%s", res.path, PATH_SEP, path);
    } else {
      strcpy(final_path, path);
    }

    des.module->handles[i] = load_library(final_path);
    des.module->natives[i].functions =
        calloc(lib.num_functions, sizeof(Native));
  }

#if DEBUG
  DEBUG_PRINTLN("Instruction count: %zu", des.instr_count);
  unsigned long long end = clock_gettime_nsec_np(CLOCK_MONOTONIC);

  // Get time in milliseconds
  unsigned long long time = (end - start) / 1000 / 1000;
  DEBUG_PRINTLN("Deserialization took %lld ms", time);

  unsigned long long start_interp = clock_gettime_nsec_np(CLOCK_MONOTONIC);
#endif

  run_interpreter(des);

#if DEBUG
  unsigned long long end_interp = clock_gettime_nsec_np(CLOCK_MONOTONIC);

  unsigned long long interp_time = (end_interp - start_interp) / 1000 / 1000;
  DEBUG_PRINTLN("Interpretation took %lld ms", interp_time);
#endif

  return 0;
}
