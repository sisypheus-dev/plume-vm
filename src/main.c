#include <core/debug.h>
#include <core/error.h>
#include <deserializer.h>
#include <interpreter.h>
#include <stdio.h>
#include <stdlib.h>

#define PLUME_VERSION "0.0.1"

int main(int argc, char** argv) {
  if (argc != 2) THROW_FMT("Usage: %s <file>\n", argv[0]);
  FILE* file = fopen(argv[1], "rb");

  if (file == NULL) THROW_FMT("Could not open file: %s\n", argv[1]);

  Deserialized des = deserialize(file);

  fclose(file);
  DEBUG_PRINTLN("Instruction count: %lld", des.bytecode.instruction_count);

  run_interpreter(des);

  return 0;
}
