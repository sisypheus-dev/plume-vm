#include <assert.h>
#include <bytecode.h>
#include <callstack.h>
#include <core/error.h>
#include <deserializer.h>
#include <module.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <value.h>
#include <interpreter.h>

Value deserialize_value(GarbageCollector gc, FILE* file) {
  Value value;

  uint8_t type;
  fread(&type, sizeof(uint8_t), 1, file);

  switch (type) {
    case TYPE_INTEGER: {
      int32_t int_value;
      fread(&int_value, sizeof(int32_t), 1, file);
      value = MAKE_INTEGER(int_value);
      break;
    }
    case TYPE_FLOAT: {
      double float_value;
      fread(&float_value, sizeof(double), 1, file);
      value = MAKE_FLOAT(float_value);
      break;
    }

    case TYPE_STRING: {
      int32_t length;
      fread(&length, sizeof(int32_t), 1, file);

      char* string_value = gc_malloc(&gc, length + 1);
      fread(string_value, sizeof(char), length, file);
      string_value[length] = '\0';

      value = MAKE_STRING(gc, string_value);
      break;
    }

    default:
      THROW_FMT("Invalid value type, received %d", type);
  }

  return value;
}

Constants deserialize_constants(GarbageCollector gc, FILE* file) {
  Constants constants;

  int32_t constant_count;
  fread(&constant_count, sizeof(int32_t), 1, file);

  constants = gc_malloc(&gc, constant_count * sizeof(Value));
  for (int32_t i = 0; i < constant_count; i++) {
    constants[i] = deserialize_value(gc, file);
  }

  assert(constants != NULL);

  return constants;
}

Libraries deserialize_libraries(GarbageCollector gc, FILE* file) {
  Libraries libraries;

  int32_t library_count;
  fread(&library_count, sizeof(int32_t), 1, file);

  libraries.num_libraries = library_count;
  libraries.libraries = gc_malloc(&gc, library_count * sizeof(Library));

  for (int32_t i = 0; i < library_count; i++) {
    int32_t length;
    fread(&length, sizeof(int32_t), 1, file);

    char* library_name = gc_malloc(&gc, length + 1);
    fread(library_name, sizeof(char), length, file);
    library_name[length] = '\0';

    Library lib;

    int32_t is_std;
    fread(&is_std, sizeof(int32_t), 1, file);

    int32_t function_count;
    fread(&function_count, sizeof(int32_t), 1, file);

    lib.num_functions = function_count;
    lib.is_standard = is_std;
    lib.name = library_name;

    libraries.libraries[i] = lib;
  }

  assert(libraries.libraries != NULL);

  return libraries;
}

Deserialized deserialize(GarbageCollector gc, FILE* file) {

  Module* module = gc_malloc(&gc, sizeof(Module));

  Constants constants_ = deserialize_constants(gc, file);
  Libraries libraries = deserialize_libraries(gc, file);

  int32_t instr_count;
  fread(&instr_count, sizeof(int32_t), 1, file);

  int32_t* instrs = gc_malloc(&gc, instr_count * 4 * sizeof(int32_t));
  fread(instrs, sizeof(int32_t), instr_count * 4, file);

  module->constants = constants_;
  module->stack = stack_new(gc);
  module->callstack = 0;
  module->natives = gc_calloc(&gc, libraries.num_libraries, sizeof(Native));
  module->gc = gc;

  Deserialized deserialized;
  deserialized.module = module;
  deserialized.libraries = libraries;
  deserialized.instr_count = instr_count;
  deserialized.instrs = instrs;

  return deserialized;
}
