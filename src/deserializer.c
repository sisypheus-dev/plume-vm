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

Instruction deserialize_instruction(FILE* file) {
  Instruction instr;

  uint8_t opcode;
  fread(&opcode, sizeof(uint8_t), 1, file);

  int32_t operand1 = 0, operand2 = 0, operand3 = 0;

  instr.opcode = opcode;

  switch (opcode) {
    case OP_Return:
    case OP_And:
    case OP_Or:
    case OP_TypeOf:
    case OP_ConstructorName:
    case OP_GetIndex:
    case OP_Special:
    case OP_ListLength:
    case OP_Halt:
    case OP_MakeMutable:
    case OP_Update:
    case OP_UnMut:
    case OP_Add:
    case OP_Sub:
      break;
    case OP_LoadNative: {
      fread(&operand1, sizeof(int32_t), 1, file);
      fread(&operand2, sizeof(int32_t), 1, file);
      fread(&operand3, sizeof(int32_t), 1, file);
      break;
    }
    default: {
      fread(&operand1, sizeof(int32_t), 1, file);
      switch (opcode) {
        case OP_Phi:
        case OP_MakeLambda: {
          fread(&operand2, sizeof(int32_t), 1, file);
          break;
        }
      }
      break;
    }
  }
  instr.operand1 = operand1;
  instr.operand2 = operand2;
  instr.operand3 = operand3;

  return instr;
}

Bytecode deserialize_bytecode(FILE* file) {
  Bytecode bytecode;

  int32_t instruction_count;
  fread(&instruction_count, sizeof(int32_t), 1, file);

  Instruction* instructions = malloc(instruction_count * sizeof(Instruction));
  for (size_t i = 0; i < instruction_count; i++) {
    instructions[i] = deserialize_instruction(file);
  }

  assert(instructions != NULL);

  bytecode.instructions = instructions;
  bytecode.instruction_count = instruction_count;

  return bytecode;
}

Value deserialize_value(FILE* file) {
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

      char* string_value = malloc(length + 1);
      fread(string_value, sizeof(char), length, file);
      string_value[length] = '\0';

      value = MAKE_STRING(string_value, length);
      break;
    }

    default:
      THROW_FMT("Invalid value type, received %d", type);
  }

  return value;
}

Constants deserialize_constants(FILE* file) {
  Constants constants;

  int32_t constant_count;
  fread(&constant_count, sizeof(int32_t), 1, file);

  constants = malloc(constant_count * sizeof(Value));
  for (size_t i = 0; i < constant_count; i++) {
    constants[i] = deserialize_value(file);
  }

  assert(constants != NULL);

  return constants;
}

Libraries deserialize_libraries(FILE* file) {
  Libraries libraries;

  int32_t library_count;
  fread(&library_count, sizeof(int32_t), 1, file);

  libraries.num_libraries = library_count;
  libraries.libraries = malloc(library_count * sizeof(Library));

  for (size_t i = 0; i < library_count; i++) {
    int32_t length;
    fread(&length, sizeof(int32_t), 1, file);

    char* library_name = malloc(length + 1);
    fread(library_name, sizeof(char), length, file);
    library_name[length] = '\0';

    Library lib;

    int32_t function_count;
    fread(&function_count, sizeof(int32_t), 1, file);

    lib.num_functions = function_count;
    lib.name = library_name;

    libraries.libraries[i] = lib;
  }

  assert(libraries.libraries != NULL);

  return libraries;
}

Deserialized deserialize(FILE* file) {
  Module* module = malloc(sizeof(Module));

  Constants constants = deserialize_constants(file);
  Libraries libraries = deserialize_libraries(file);

  size_t instr_count;
  fread(&instr_count, sizeof(int32_t), 1, file);

  int32_t* instrs = malloc(instr_count * 4 * sizeof(int32_t));
  fread(instrs, sizeof(int32_t), instr_count * 4, file);

  module->constants = constants;
  module->stack = stack_new();
  module->callstack = 0;
  module->natives = calloc(libraries.num_libraries, sizeof(Native));

  Deserialized deserialized;
  deserialized.module = module;
  deserialized.libraries = libraries;
  deserialized.instr_count = instr_count;
  deserialized.instrs = instrs;

  return deserialized;
}
