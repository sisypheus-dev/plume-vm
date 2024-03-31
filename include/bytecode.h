#ifndef BYTECODE_H
#define BYTECODE_H

#include <stdint.h>
#include <stdlib.h>

typedef enum {
  OP_LoadLocal,
  OP_StoreLocal,
  OP_LoadConstant,
  OP_LoadGlobal,
  OP_StoreGlobal,
  OP_Return,
  OP_Compare,
  OP_And,
  OP_Or,
  OP_LoadNative,
  OP_MakeList,
  OP_ListGet,
  OP_Call,
  OP_JumpIfRel,
  OP_TypeOf,
  OP_ConstructorName,
  OP_Phi,
  OP_MakeLambda,
  OP_GetIndex,
  OP_Special,
  OP_JumpRel,
  OP_Slice,
  OP_ListLength,
} Opcode;

typedef struct {
  Opcode opcode;
  int64_t operand1;
  int64_t operand2;
  int64_t operand3;
} Instruction;

typedef struct {
  Instruction *instructions;
  int64_t instruction_count;
} Bytecode;

typedef struct {
  char *name;
  size_t num_functions;
} Library;

typedef struct {
  Library *libraries;
  size_t num_libraries;
} Libraries;

typedef enum {
  LessThan = 0,
  GreaterThan = 1,
  EqualTo = 2,
  NotEqualTo = 3,
  LessThanOrEqualTo = 4,
  GreaterThanOrEqualTo = 5,
} Comparison;

#endif  // BYTECODE_H