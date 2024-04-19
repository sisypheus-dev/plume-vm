#include <core/debug.h>
#include <core/error.h>
#include <core/library.h>
#include <deserializer.h>
#include <interpreter.h>
#include <stdio.h>
#include <stdlib.h>

#define PLUME_VERSION "0.0.1"

int main(int argc, char** argv) {
  if (argc < 2) THROW_FMT("Usage: %s <file>\n", argv[0]);
  FILE* file = fopen(argv[1], "rb");

  ValueList l = {.length = argc};
  l.values = malloc(argc * sizeof(Value));
  for (int i = 0; i < argc; i++) {
    l.values[i] = MAKE_STRING(argv[i]);
  }

  if (file == NULL) THROW_FMT("Could not open file: %s\n", argv[1]);

  Deserialized des = deserialize(file);

  fclose(file);

  des.module->args = l;
  des.module->handles = malloc(des.libraries.num_libraries * sizeof(void*));

  Libraries libs = des.libraries;

  for (int i = 0; i < des.libraries.num_libraries; i++) {
    Library lib = libs.libraries[i];
    char* path = lib.name;
    des.module->handles[i] = load_library(path);
    des.module->natives[i].functions =
        calloc(lib.num_functions, sizeof(Native));
  }

  DEBUG_PRINTLN("Instruction count: %lld", des.instr_count);

  run_interpreter(des);

  return 0;
}
